/********************************************************************************
 * Program to package a kernel for the BT HomeHub versions 1.x, and 2.0A	*
 * for use with the standard Thomson bootloader.				*
 * The version string in the header is different for each released firmware	*
 * version, but it does not seem to matter what is there.			*
 * This program requires lzma version 427 (This is the one Thomson used to	*
 * produce their firmwares, other versions may or may not work)			*
 * To use:									*
 * 	./thomson_kernel vmlinux.elf vmlinux.thom				*
 *										*
 * The kernel MUST be in the ELF format to satisfy the Thomson bootloader.	*
 ********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <parse_num.h>
#include <my_file_functions.h>
#include <bitsnbobs.h>

#define LZMA_CMD "lzma427 -d21 e"
#define LZMA_TMP_FILE "./lzma.tmp"


static unsigned char thomson_header[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*	0x00, 0x06, 0x02, 0x02, 0x06, 0x00 */	/* 6226 */
	0x00, 0x06, 0x02, 0x06, 0x11, 0x00 	/* 626H */
/*	0x00, 0x08, 0x01, 0x11, 0x13, 0x00 */	/* 81HJ */
};
#define THOM_HEADER_SIZE (sizeof(thomson_header))
#define THOM_HEADER_TOTAL (THOM_HEADER_SIZE + 14) /* for 2x int_big and 0xB6,LINU */

int main(int argc, char *argv[])
	{
	FILE *fopen(), *out_fptr;
	const char cmd[] = LZMA_CMD;
	char *cmd_line, *lzma_file = LZMA_TMP_FILE;
	char *kernel_file, *output_file;

	unsigned int partition_size[1], lzma_size, kernel_size, kernel_padding;

	if (argc != 4)  {
		fprintf(stderr, "Usage: $0 <inputfile> <outputfile> <partition-size>\n");
		exit(1);
		}
	kernel_file = argv[1];
	output_file = argv[2];
	if (!parse_num(argv[3], partition_size))  {
		printf("Malformatted number\n");
		exit(1);
		}

	/* Call lzma and compress kernel_file to temp file */
	cmd_line = calloc(strlen(cmd) + strlen(kernel_file) + strlen(lzma_file) + 20,sizeof(char));
	sprintf(cmd_line,"%s %s %s\n",cmd, kernel_file, lzma_file);
/*	printf("About to execute %s",cmd_line); */
	system(cmd_line);
	free(cmd_line);

	/* unpadded size */
	lzma_size = get_file_size(LZMA_TMP_FILE);
	kernel_size = lzma_size + THOM_HEADER_TOTAL;

	/* Calculate required padding */
	if (kernel_size > *partition_size)  {
		fprintf(stderr, "Error: kernel (0x%08X) is too big for partition (0x%08X)\n", \
							kernel_size, *partition_size);
		exit(1);
		}
		else {
		kernel_padding = *partition_size - kernel_size;
		}
	printf("\nlzma'd kernel: 0x%08X\tpadding: 0x%08X\n", kernel_size, kernel_padding);

	/* OK now we've got all our ducks in a row, write the whole lot out */
	out_fptr = openfile(output_file,"wb");
	/* Write out compressed kernel to outfile */
	fwrite(thomson_header, THOM_HEADER_SIZE, 1, out_fptr);	/* Should be 12 bytes */
	write_int_big(out_fptr, 0xFFFFFFFF);
	fprintf(out_fptr, "%cLINU\n", 0xB6);		/* terminating zero? No? -- Good! */
	write_int_big(out_fptr, lzma_size);
	cat_file(out_fptr, lzma_file);
	do_padding(out_fptr, kernel_padding, 0xFF);
	fclose(out_fptr);

	if(remove(lzma_file) != 0)
		fprintf(stderr, "Error deleting %s\n", lzma_file);

	if(kernel_size + kernel_padding != get_file_size(output_file)) {
		fprintf(stderr, "\n\n\tOops output file size not correct -- disk error?\n\n");
		return -1;
	}
	return 0;
}

