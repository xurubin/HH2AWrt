/********************************************************************************
 * Program to package the kernel and rootfs produced by the openwrt buildkit	*
 * for the BT HomeHub v2.0A. We output a bogus redboot partition table to	*
 * cheat openwrt into working with the standard Thomson bootloader.		*
 * I've done the bare minimum to get past 'parse_redboot_partitions()'		*
 *						in drivers/mtd/redboot.c	*
 * The Redboot bootloader itself will definitely bork if it ever sees this.	*
 * The version string in the header is different for each released firmware	*
 * version, but it does not seem to matter what is there.			*
 * This program requires lzma version 427 (This is the one Thomson used to	*
 * produce their firmwares, other versions may or may not work)			*
 * 	To use: ./redtable vmlinux.elf root.squashfs openwrt.img		*
 * The kernel MUST be in the ELF format to satisfy the Thomson bootloader.	*
 ********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <my_file_functions.h>
#include <bitsnbobs.h>

#define FLASH_START 0xBE000000
#define FLASH_SIZE 0x1000000
#define ERASE_BLOCK 0x20000
#define BOOTLOADER_SIZE 0x20000
#define LZMA_CMD "lzma427 -d21 e"
#define LZMA_TMP_FILE "./lzma.tmp"

static unsigned char thomson_header[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x08, 0x01, 0x11, 0x13, 0x00 	/* 81HJ */
};
#define THOM_HEADER_SIZE (sizeof(thomson_header))
#define THOM_HEADER_TOTAL (THOM_HEADER_SIZE + 14) /* for 2x int_big and 0xB6,LINU */

void red_part(FILE *out_fptr, char name[16], unsigned int flash_offset, unsigned int size);

int main(int argc, char *argv[])
	{
	FILE *fopen(), *out_fptr;
	const char cmd[] = LZMA_CMD;
	char *cmd_line, *lzma_file = LZMA_TMP_FILE;
	char *kernel_file, *root_fs_file, *output_file;

	unsigned int fis_directory_start;
	unsigned int jffs2_start, jffs2_size, lzma_size;
	unsigned int kernel_start, kernel_size, kernel_padding;
	unsigned int root_fs_start, root_fs_size, root_fs_padding;
	unsigned int unused_start, unused_size, final_image_size;

	if (argc != 4)  {
		fprintf(stderr, "Usage: $0 <vmlinux.elf> <root.squashfs> <output_flash_image>\n");
		exit(1);
		}
	kernel_file = argv[1];
	root_fs_file = argv[2];
	output_file = argv[3];

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
			|   FIS directory	|			|
   fis_directory_start	|-----------------------|   			|
			|    			|			|
			|			|			|
			|			|   			|
			|			|			|
			|    Space reserved	|			|
			|	  for		|   			|
			|    jffs2 filesys	|			|
			|     			|			|
			|     ( All 0xFF)	|   			|
			|			|		Included in
			|			| 		output file
			|			|			|
			|     			|			|
			|			|			|
	jffs2_start	|-----------------------|			|
			|			|			|
			|     root filesys	|			|
			|			|			|
			|	 		|			|
			|			| 			|
	rootfs_start	|-----------------------|			|
			|			|			|
			|	 kernel		|			|
	kernel_start	|-----------------------|   <-------------------
	FLASH_START	|____bootloader/CFE_____|
*/

	/* All unpadded sizes */
	lzma_size = get_file_size(LZMA_TMP_FILE);
	kernel_size = lzma_size + THOM_HEADER_TOTAL;
	root_fs_size = get_file_size(root_fs_file);
/*
	printf("kernel size:            %08X\n", kernel_size);
	printf("rootfs size:            %08X\n", root_fs_size);
*/
	/* Calculate required padding for each block */
	kernel_padding = calc_padding(kernel_size, ERASE_BLOCK);
	root_fs_padding = calc_padding(root_fs_size, ERASE_BLOCK);
/*
	printf("kernel padding:         %08X\n", kernel_padding);
	printf("rootfs padding:         %08X\n", root_fs_padding);
*/
	/* All_start values are relative to flash start */
	kernel_start = BOOTLOADER_SIZE;
	root_fs_start = kernel_start + kernel_size + kernel_padding;
	jffs2_start = root_fs_start + root_fs_size + root_fs_padding;
	fis_directory_start = FLASH_SIZE - ERASE_BLOCK;
	unused_start = fis_directory_start + (0x100 * 5) + 2; /* 5= num of partitions */
/*
	printf("kernel start:           %08X\n", kernel_start);
	printf("rootfs start:           %08X\n", root_fs_start);
	printf("jffs2 start:            %08X\n", jffs2_start);
	printf("unused_start:           %08X\n\n", unused_start);
*/
	jffs2_size = fis_directory_start - jffs2_start;
	unused_size = FLASH_SIZE - unused_start;
	final_image_size = unused_start - kernel_start;


	/* OK now we've got all our ducks in a row, write the whole lot out */
	out_fptr = openfile(output_file,"wb");
	/* Write out compressed kernel to outfile */
	fwrite(thomson_header, THOM_HEADER_SIZE, 1, out_fptr);	/* Should be 12 bytes */
	write_int_big(out_fptr, 0xFFFFFFFF);
	fprintf(out_fptr, "%cLINU\n", 0xB6);		/* terminating zero? No? -- Good! */
	write_int_big(out_fptr, lzma_size);
	cat_file(out_fptr, lzma_file);
	do_padding(out_fptr, kernel_padding, 0xFF);

	/* Write out root_fs to outfile */
	cat_file(out_fptr, root_fs_file);
	do_padding(out_fptr, root_fs_padding, 0xFF);

	/* Write out padding equal to jffs2_size */
	do_padding(out_fptr, jffs2_size, 0xFF);

	/* Write out bogus redboot partition table and report the flash map to stdout*/
	printf("   partition   file offset    flash offset    jtag address        size\n");

	red_part(out_fptr,"RedBoot", 0, BOOTLOADER_SIZE);
	red_part(out_fptr,"FIS directory", fis_directory_start, 0xF000);
	red_part(out_fptr,"kernel_fs", kernel_start, kernel_size + kernel_padding);
	red_part(out_fptr,"root_fs", root_fs_start, root_fs_size + root_fs_padding);
	red_part(out_fptr,"rootfs_data", jffs2_start, jffs2_size);
	do_padding(out_fptr, 2, 0xFF);
	fclose(out_fptr);

	if(remove(lzma_file) != 0)
		fprintf(stderr, "Error deleting %s\n", lzma_file);

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

/* Function to write out one redboot partition table entry */
void red_part(FILE *out_fptr, char name[16], unsigned int flash_offset, unsigned int size)
	{
	unsigned int file_offset, flash_address, jtag_address, pad;
	/* Report what we're doing to stdout */
	file_offset = flash_offset - BOOTLOADER_SIZE;
	flash_address = FLASH_START + flash_offset;
	jtag_address = flash_address & 0x1FFFFFFF;
	if (flash_offset >= BOOTLOADER_SIZE )
		printf("%12s \t%08X\t%08X\t%08X\t%08X\n", name, file_offset ,flash_offset, jtag_address, size);
	else
		printf("%12s \t--------\t%08X\t%08X\t%08X\n", name, flash_offset, jtag_address, size);

	pad=16-strlen(name);
	fprintf(out_fptr,"%s", name);
	do_padding(out_fptr, pad, 0x00);
	write_int_big(out_fptr, flash_address);
	write_int_big(out_fptr, 0);
	write_int_big(out_fptr, size);
	write_int_big(out_fptr, 0);
	do_padding(out_fptr, 224, 0x00);
}

