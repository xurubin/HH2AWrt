/********************************************************************************
 * Program to package the kernel and filesystems for the BT HomeHub V1.x for	*
 * re-builds of the standard firmware, with the original Thomson bootloader.	*
 * These partitions are statically defined in the kernel in the standard way.	*
 * We are not checking partition sizes, but assuming that the provided files	*
 * correspond to the settings in the kernel's .config file.			*
 * The version string in the header is different for each released firmware	*
 * version, but it does not seem to matter what is there.			*
 * This program requires lzma version 427 (This is the one Thomson used to	*
 * produce their firmwares, other versions may or may not work)			*
 * To use:									*
 *	./v1_most_flash jffs2.img vmlinux.stripped 0xB0000 \			*
 *	 rootfs.squashfs-lzma output.img > flash-map.txt			*
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

#define FLASH_START 0xBF400000
#define FLASH_SIZE 0x0800000
#define ERASE_BLOCK 0x10000
#define BOOTLOADER_SIZE 0x40000
#define LZMA_CMD "lzma427 -d21 e"
#define LZMA_TMP_FILE "./lzma.tmp"


static unsigned char thomson_header[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*	0x00, 0x06, 0x02, 0x02, 0x06, 0x00 */	/* 6226 */
	0x00, 0x06, 0x02, 0x06, 0x11, 0x00 	/* 626H */
};
#define THOM_HEADER_SIZE (sizeof(thomson_header))
#define THOM_HEADER_TOTAL (THOM_HEADER_SIZE + 14) /* for 2x int_big and 0xB6,LINU */


void report_partition(char name[16], unsigned int flash_offset, unsigned int size);

int main(int argc, char *argv[])
	{
	FILE *fopen(), *out_fptr;
	const char cmd[] = LZMA_CMD;
	char *cmd_line, *lzma_file = LZMA_TMP_FILE;
	char *jffs2_file, *kernel_file, *root_fs_file, *output_file;

	unsigned int partition_size[1];
	unsigned int jffs2_start, jffs2_size, lzma_size;
	unsigned int kernel_start, kernel_size, kernel_padding;
	unsigned int root_fs_start, root_fs_size, root_fs_padding;
	unsigned int unused_start, unused_size, final_image_size;

	if (argc != 6)  {
		 fprintf(stderr, "Usage: $0 <jffs2.file> <vmlinux.elf> <kernel.part.size> <rootfs.file> <out.file>\n");
		exit(1);
		}
	jffs2_file = argv[1];
	kernel_file = argv[2];
	if (!parse_num(argv[3], partition_size))  {
		 fprintf(stderr, "Malformatted number\n");
		exit(1);
		}
	root_fs_file = argv[4];
	output_file = argv[5];

	/* Call lzma and compress kernel_file to temp file */
	cmd_line = calloc(strlen(cmd) + strlen(kernel_file) + strlen(lzma_file) + 20,sizeof(char));
	sprintf(cmd_line,"%s %s %s\n",cmd, kernel_file, lzma_file);
/*	printf("About to execute %s",cmd_line); */
	system(cmd_line);
	free(cmd_line);

/*
	flash_end	 _______________________
			|	unused		|
	unused_start	|-----------------------|   <-------------------
			|			|			|
			|     			|			|
			|			|			|
			|    			|			|
			|			|			|
			|     root filesys	|			|
			|			|		Included in
			|	 		|		output file
			|			| 			|
	rootfs_start	|-----------------------|			|
			|			|			|
			|	 kernel		|			|
	kernel_start	|-----------------------|			|
			|    jffs2 filesys	|			|
			|			|			|
	jffs2_start	|-----------------------|   <-------------------
	FLASH_START	|____bootloader/CFE_____|
*/

	/* All unpadded sizes */
	jffs2_size = get_file_size(jffs2_file);
	lzma_size = get_file_size(LZMA_TMP_FILE);
	kernel_size = lzma_size + THOM_HEADER_TOTAL;
	root_fs_size = get_file_size(root_fs_file);
/*
	printf("jffs2 size:             %08X\n", jffs2_size);
	printf("kernel size:            %08X\n", kernel_size);
	printf("rootfs size:            %08X\n", root_fs_size);
*/

	/* Calculate required padding for each block */
	/* No padding for jffs2, we're assuming it is correctly a whole no. of eraseblocks */
	if (kernel_size > *partition_size)  {
		fprintf(stderr, "Error: kernel (0x%08X) is too big for partition (0x%08X)\n", \
							kernel_size, *partition_size);
		exit(1);
		}
		else {
		kernel_padding = *partition_size - kernel_size;
		}
	root_fs_padding = calc_padding(root_fs_size, ERASE_BLOCK);
/*
	printf("kernel padding:         %08X\n", kernel_padding);
	printf("rootfs padding:         %08X\n", root_fs_padding);
*/
	jffs2_start = BOOTLOADER_SIZE; /* Doing this makes all_start values relative to flash start */
	kernel_start = jffs2_start + jffs2_size;
	root_fs_start = kernel_start + kernel_size + kernel_padding;
	unused_start = root_fs_start + root_fs_size + root_fs_padding;
/*
	printf("kernel start:           %08X\n", kernel_start);
	printf("rootfs start:           %08X\n", root_fs_start);
	printf("unused_start:           %08X\n\n", unused_start);
*/
	unused_size = FLASH_SIZE - unused_start;
	final_image_size = unused_start - jffs2_start;


	/* OK now we've got all our ducks in a row, write the whole lot out */
	out_fptr = openfile(output_file,"wb");
	/* Write out jffs2 to outfile */
	cat_file(out_fptr, jffs2_file);

	/* Write out compressed kernel to outfile */
	fwrite(thomson_header, THOM_HEADER_SIZE, 1, out_fptr);	/* Should be 12 bytes */
	write_int_big(out_fptr, 0xFFFFFFFF);
	fprintf(out_fptr, "%cLINU\n", 0xB6);		/* terminating zero? No? -- Good! */
	write_int_big(out_fptr, lzma_size);
	cat_file(out_fptr, lzma_file);
	do_padding(out_fptr, kernel_padding, 0xFF);

	/* Write out root_fs to outfile */
	cat_file(out_fptr, root_fs_file);
	/* root_fs_file is last out, so it's not essential to pad. */
	do_padding(out_fptr, root_fs_padding, 0xFF);
	fclose(out_fptr);

	if(remove(lzma_file) != 0)
		fprintf(stderr, "Error deleting %s\n", lzma_file);

	/* Report the flash map to stdout */
	printf("   partition   file offset    flash offset    jtag address        size\n");
	report_partition("CFE", 0, BOOTLOADER_SIZE);
	report_partition("jffs2", jffs2_start, jffs2_size);
	report_partition("kernel", kernel_start, kernel_size + kernel_padding);
	report_partition("root_fs", root_fs_start, root_fs_size + root_fs_padding);
	report_partition("unused", unused_start, unused_size);
	printf("\nFinal size of image: 0x%08X\n", final_image_size);
	fflush(stdout);
	cmd_line = calloc(strlen(output_file) + strlen(output_file) + 30,sizeof(char));
	sprintf(cmd_line,"ls -l %s; md5sum %s; date\n", output_file, output_file);
/*	printf("About to execute %s",cmd_line); */
	system(cmd_line);
	free(cmd_line);
	if(final_image_size > (FLASH_SIZE - BOOTLOADER_SIZE)) {
		fprintf(stderr, "\n\n\tOutput file is OVER SIZE, will not fit in flash!\n\n");
		return -1;
	}
	if(final_image_size != get_file_size(output_file)) {
		fprintf(stderr, "\n\n\tOops output file size not correct -- disk error?\n\n");
		return -1;
	}
	return 0;
}

/* Function to print out one section for the flash map */
void report_partition(char name[16], unsigned int flash_offset, unsigned int size)
	{
	unsigned int file_offset, flash_address, jtag_address;
	file_offset = flash_offset - BOOTLOADER_SIZE;
	flash_address = FLASH_START + flash_offset;
	jtag_address = flash_address & 0x1FFFFFFF;
	if (flash_offset >= BOOTLOADER_SIZE )
		printf("%12s \t%08X\t%08X\t%08X\t%08X\n", name, file_offset ,flash_offset, jtag_address, size);
	else
		printf("%12s \t--------\t%08X\t%08X\t%08X\n", name, flash_offset, jtag_address, size);

}

