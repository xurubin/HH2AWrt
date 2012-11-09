/* my_file_functions.c */
/*
To add:-
	a) buf_to_file(char *output_file, *buf, unsigned int size)	+ offsets?
	b) buf_to_fptr(FILE *out_fptr, *buf, unsigned int size)
	c) file_to_buf(char *input_file, *buf, unsigned int wanted_size)
	d) fptr_to_buf(FILE *in_fptr, *buf, unsigned int wanted_size)
	e) fptr_to_fptr
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <my_file_functions.h>


/* Function to get the size of a file */
unsigned int get_file_size(char *filename)
	{
	struct stat st;
	if(stat(filename, &st)) {
		fprintf(stderr, "stat: cannot stat %s\n", filename);
		exit(1);
		}
	return(st.st_size);
}


/* Function to open a file */
FILE * openfile(char *file, char *mode)
	{
	FILE *fptr;
	if ((fptr = fopen(file, mode)) == NULL)  {
		fprintf(stderr, "Can't open %s for mode %s\n",file, mode);
		exit(1);
		}
	return(fptr);
}


/* Function to write out n bytes of value byte as padding */
unsigned int do_padding(FILE *out_fptr, unsigned int number, unsigned char byte)
	{
	unsigned int no_blocks, remnant, count;
	void *buf = malloc(PAD_BLK_SIZE);
	buf = memset(buf, byte, PAD_BLK_SIZE);
	no_blocks = number / PAD_BLK_SIZE;
	remnant = number % PAD_BLK_SIZE;

	for (count=0; count != no_blocks; count++)  {
		if ( fwrite(buf, PAD_BLK_SIZE, 1, out_fptr) != 1 ) { goto write_error; }
		}
	if (remnant) {
		if ( fwrite(buf, remnant, 1, out_fptr) != 1 ) { goto write_error; }
		}
	free(buf);
	return 0;
write_error:
	free(buf);
	fprintf(stderr, "do_padding: disk write error: block_count %d\n", count);
	exit(1);
	/* return MY_WRITE_ERROR; */
}


/* Function to append a file to an already open outputfile */
unsigned int cat_file(FILE *out_fptr, char *input_file)
	{
	return(cat_part_file(out_fptr, input_file, 0, 0));
	}


/* Function to append a portion of a file to an already open outputfile */
unsigned int cat_part_file(FILE *out_fptr, char *input_file, unsigned int offset, unsigned int wanted_size)
	{
	FILE *in_fptr;
	unsigned int total_size, size_to_be_copied, no_blocks, remnant, count;
	char *buf = (char*)malloc(IO_BLK_SIZE);

	total_size = get_file_size(input_file);

	if ( offset + wanted_size > total_size) {
		fprintf(stderr, "cat_part_file: Tried to copy more than available\n");
		exit(1);
		/* return MY_SIZE_ERROR; */
		}

	if ( wanted_size == 0) {
		size_to_be_copied = total_size - offset; /*  gimmie all of it  */
		}
		else  {
		size_to_be_copied = wanted_size;
	}

	no_blocks = size_to_be_copied / IO_BLK_SIZE;
	remnant = size_to_be_copied % IO_BLK_SIZE;

	in_fptr = openfile(input_file, "rb");
	fseek(in_fptr, offset, SEEK_SET);

	for (count=0; count != no_blocks; count++)  {
		if ( fread(buf, IO_BLK_SIZE, 1, in_fptr) != 1 ) { goto read_error; }
		if ( fwrite(buf, IO_BLK_SIZE, 1, out_fptr) != 1 ) { goto write_error; }
		}

	if (remnant) {
		if ( fread(buf, remnant, 1, in_fptr) != 1 ) { goto read_error; }
		if ( fwrite(buf, remnant, 1, out_fptr) != 1 ) { goto write_error; }
		}

	fclose(in_fptr);
	free(buf);
	return(size_to_be_copied);

read_error:
	fclose(in_fptr);
	free(buf);
	fprintf(stderr, "cat_part_file: disk read error: %s\n", input_file);
	exit(1);
	/* return MY_READ_ERROR; */

write_error:
	fclose(in_fptr);
	free(buf);
	fprintf(stderr, "cat_part_file: disk write error while copying %s\n", input_file);
	exit(1);
	/* return MY_WRITE_ERROR; */
	}

