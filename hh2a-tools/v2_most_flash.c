/********************************************************************************
 * Program to package the kernel and filesystems for the BT HomeHub V2.0A for	*
 * re-builds of the standard firmware, with the original Thomson bootloader.	*
 * These partitions are probed by the kernel at boot time,			*
 * the code which does this is in drivers/mtd/bthub.c				*
 * The version string in the headers is different for each released firmware	*
 * version, but it does not seem to matter what is there.			*
 * This program requires lzma version 427 (This is the one Thomson used to	*
 * produce their firmwares, other versions may or may not work)			*
 * To use:									*
 *	./v2_most_flash jffs2.img vmlinux.stripped modfs.squashfs-lzma \	*
 *	 rootfs.squashfs-lzma extfs.squashfs.lzma output.img > flash-map.txt	*
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

#define FLASH_START 0xBE000000
#define FLASH_SIZE 0x1000000
#define ERASE_BLOCK 0x20000
#define BOOTLOADER_SIZE 0x20000
#define LZMA_CMD "lzma427 -d21 e"
#define LZMA_TMP_FILE "./lzma.tmp"


static unsigned char thomson_header[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x08, 0x01, 0x11, 0x13, 0x00	/* 81HJ */
};
#define THOM_HEADER_SIZE (sizeof(thomson_header))
#define THOM_HEADER_TOTAL (THOM_HEADER_SIZE + 14) /* for 2x int_big and 0xB6,LINU */

static unsigned char exte_header[] = {
	0x00, 0x00, 0x00, 0x00, 0x02, 0x00,
	0x00, 0x08, 0x01, 0x11, 0x13, 0x00,	/* 81HJ */
	0x00, 0x00, 0x20, 0x00,
	0x45, 0x58, 0x54, 0x45
};
#define EXTE_HEADER_SIZE (sizeof(exte_header))


void report_partition(char name[16], unsigned int flash_offset, unsigned int size);

int main(int argc, char *argv[])
	{
	FILE *fopen(), *out_fptr;
	const char cmd[] = LZMA_CMD;
	char *cmd_line, *lzma_file = LZMA_TMP_FILE;
	char *jffs2_file, *kernel_file, *root_fs_file, *output_file;
	char *mod_fs_file, *ext_fs_file;

	unsigned int jffs2_start, jffs2_size, lzma_size;
	unsigned int kernel_start, kernel_size, kernel_padding;
	unsigned int mod_fs_start, mod_fs_size, mod_fs_padding;
	unsigned int root_fs_start, root_fs_size, root_fs_padding;
	unsigned int core_signature_start, core_signature_size;
	unsigned int core_signature_padding, core_signature_offset;
	unsigned int exte_header_start, exte_header_size, exte_header_padding;
	unsigned int ext_fs_start, ext_fs_size, ext_fs_padding;
	unsigned int second_sig_start, second_sig_size, second_sig_padding;
	unsigned int unused_start, unused_size, final_image_size;

	if (argc != 7)  {
		 fprintf(stderr, "Usage: $0 <jffs2.file> <vmlinux.elf> <mod.fs.file> <rootfs.file> <extfs.file> <out.file>\n");
		exit(1);
		}
	jffs2_file = argv[1];
	kernel_file = argv[2];
	mod_fs_file = argv[3];
	root_fs_file = argv[4];
	ext_fs_file = argv[5];
	output_file = argv[6];

	/* Call lzma and compress kernel_file to temp file */
	cmd_line = calloc(strlen(cmd) + strlen(kernel_file) + strlen(lzma_file) + 20,sizeof(char));
	sprintf(cmd_line,"%s %s %s\n",cmd, kernel_file, lzma_file);
/*	printf("About to execute %s",cmd_line); */
	if (system(cmd_line)) exit(1);	/* Call to lzma failed! */
	free(cmd_line);
/*
	flash_end	 _______________________
			|	unused		|
	unused_start	|-----------------------|   <-------------------
			|   second_signature	|			|
     second_sig_start	|-----------------------|			|
			|	ext_fs		|			|
	ext_fs_start	|-----------------------|			|
			|     exte_header	|			|
    exte_header_start	|-----------------------|			|
			|    core_signature	|			|
   core_signature_start	|-----------------------| <--			|
			|			|   |			|
			|			|   |			|
			|			|   |			|
			|			|   ^			|
			|     root filesys	|   |		Included in 
			|			|   |		output file
			|    			|   ^			|
			|			|   |<<--		|
	rootfs_start	|-----------------------|   |	 |		|
			|			|   ^	 core		|
			|	 modfs		|   |	 signature	|
			|			|   |	 offset		|
	mod_fs_start	|-----------------------|   ^	 pointer	|
			|			|   |			|
			|	 kernel		|   |			|
	kernel_start	|-----------------------| >--			|
			|    jffs2 filesys	|			|
			|			|			|
	jffs2_start	|-----------------------|   <-------------------
	FLASH_START	|____bootloader/CFE_____|
*/

	/* All unpadded sizes */
	jffs2_size = get_file_size(jffs2_file);
	lzma_size = get_file_size(LZMA_TMP_FILE);
	kernel_size = lzma_size + THOM_HEADER_TOTAL;
	mod_fs_size = get_file_size(mod_fs_file);
	root_fs_size = get_file_size(root_fs_file);
	core_signature_size = 0x28;
	exte_header_size = EXTE_HEADER_SIZE;
	ext_fs_size = get_file_size(ext_fs_file);
	second_sig_size = 0x28;
/*
	printf("jffs2 size:             %08X\n", jffs2_size);
	printf("kernel size:            %08X\n", kernel_size);
	printf("modfs size:             %08X\n", mod_fs_size);
	printf("rootfs size:            %08X\n", root_fs_size);
	printf("core_signature_size:    %08X\n", core_signature_size);
	printf("exte_header_size:       %08X\n", exte_header_size);
	printf("ext_fs_size:            %08X\n", ext_fs_size);
	printf("second_sig_size:        %08X\n\n", second_sig_size);
*/
	/* Calculate required padding for each block */
	/* No padding for jffs2, we're assuming it is correctly a whole no. of eraseblocks */
	kernel_padding = calc_padding(kernel_size, 0x1000);
	mod_fs_padding = calc_padding(kernel_size + kernel_padding + mod_fs_size, 0x10000);
	root_fs_padding = calc_padding(root_fs_size, 0x10000);
	core_signature_padding = ERASE_BLOCK - core_signature_size;
	exte_header_padding = 0x1000 - exte_header_size;
	ext_fs_padding = calc_padding(ext_fs_size, 0x1000);
	second_sig_padding = calc_padding(exte_header_size + exte_header_padding + ext_fs_size + ext_fs_padding + second_sig_size, ERASE_BLOCK);
/*
	printf("kernel padding:         %08X\n", kernel_padding);
	printf("modfs padding:          %08X\n", mod_fs_padding);
	printf("rootfs padding:         %08X\n", root_fs_padding);
	printf("core_signature_padding: %08X\n", core_signature_padding);
	printf("exte_header_padding:    %08X\n", exte_header_padding);
	printf("ext_fs_padding:         %08X\n", ext_fs_padding);
	printf("second_sig_padding:     %08X\n\n", second_sig_padding);
*/
	jffs2_start = BOOTLOADER_SIZE; /* Doing this makes all_start values relative to flash start */
	kernel_start = jffs2_start + jffs2_size;
	mod_fs_start = kernel_start + kernel_size + kernel_padding;
	root_fs_start = mod_fs_start + mod_fs_size + mod_fs_padding;
	core_signature_start = root_fs_start + root_fs_size + root_fs_padding;
	exte_header_start = core_signature_start + core_signature_size + core_signature_padding;
	ext_fs_start = exte_header_start + exte_header_size + exte_header_padding;
	second_sig_start = ext_fs_start + ext_fs_size + ext_fs_padding;
	unused_start = second_sig_start + second_sig_size + second_sig_padding;
/*
	printf("kernel start:           %08X\n", kernel_start);
	printf("modfs start:            %08X\n", mod_fs_start);
	printf("rootfs start:           %08X\n", root_fs_start);
	printf("core_signature_start:   %08X\n", core_signature_start);
	printf("exte_header_start:      %08X\n", exte_header_start);
	printf("ext_fs_start:           %08X\n", ext_fs_start);
	printf("second_sig_start:       %08X\n", second_sig_start);
	printf("unused_start:           %08X\n\n", unused_start);
*/
	core_signature_offset = core_signature_start - kernel_start;
	printf("core_signature_offset:  %08X\n\n", core_signature_offset);
	unused_size = FLASH_SIZE - unused_start;
	final_image_size = unused_start - jffs2_start;


	/* OK now we've got all our ducks in a row, write the whole lot out */
	out_fptr = openfile(output_file,"wb");
	/* Write out jffs2 to outfile */
	cat_file(out_fptr, jffs2_file);

	/* Write out compressed kernel to outfile */
	fwrite(thomson_header, THOM_HEADER_SIZE, 1, out_fptr);	/* Should be 12 bytes */
	write_int_big(out_fptr, core_signature_offset);		/* Only in V2A */
	fprintf(out_fptr, "%cLINU\n", 0xB6);		/* terminating zero? No? -- Good! */
	write_int_big(out_fptr, lzma_size);
	cat_file(out_fptr, lzma_file);
	do_padding(out_fptr, kernel_padding, 0xFF);

	/* Write out mod_fs to outfile */
	cat_file(out_fptr, mod_fs_file);
	do_padding(out_fptr, mod_fs_padding, 0xFF);

	/* Write out root_fs to outfile */
	cat_file(out_fptr, root_fs_file);
	do_padding(out_fptr, root_fs_padding, 0xFF);

	/* Write out core_signature to outfile */
	write_int_big(out_fptr, 0x04);
	write_int_big(out_fptr, 0x24);
	/* This line below looks like an md5sum, find out what of, and do it right! */
	fprintf (out_fptr, "c46518416240b253150bed68284cce21");
	do_padding(out_fptr, core_signature_padding, 0xFF);

	/* Write out exte_header to outfile */
	fwrite(exte_header, EXTE_HEADER_SIZE, 1, out_fptr);
	do_padding(out_fptr, exte_header_padding, 0xFF);

	/* Write out ext_fs to outfile */
	cat_file(out_fptr, ext_fs_file);
	do_padding(out_fptr, ext_fs_padding, 0x00);

	/* Write out second_signature to outfile */
	write_int_big(out_fptr, 0x04);
	write_int_big(out_fptr, 0x24);
	fprintf (out_fptr, "c1aa29ed0e5891e81316e69cbeea37bd");
	do_padding(out_fptr, second_sig_padding, 0xFF);
	fclose(out_fptr);

	if(remove(lzma_file) != 0)
		fprintf(stderr, "Error deleting %s\n", lzma_file);

	/* Report the flash map to stdout */
	printf("   partition   file offset    flash offset    jtag address        size\n");
	report_partition("CFE", 0, BOOTLOADER_SIZE);
	report_partition("jffs2", jffs2_start, jffs2_size);
	report_partition("kernel", kernel_start, kernel_size + kernel_padding);
	report_partition("mod_fs", mod_fs_start, mod_fs_size + mod_fs_padding);
	report_partition("root_fs", root_fs_start, root_fs_size + root_fs_padding);
	report_partition("core_signature", core_signature_start, core_signature_size + core_signature_padding);
	report_partition("exte_header", exte_header_start, exte_header_size + exte_header_padding);
	report_partition("ext_fs", ext_fs_start, ext_fs_size + ext_fs_padding);
	report_partition("second_sig", second_sig_start, second_sig_size + second_sig_padding);
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

