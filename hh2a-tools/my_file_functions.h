/* my_file_functions.h */
/*
To add:-
	a) buf_to_file(char *output_file, *buf, unsigned int size)	+ offsets?
	b) buf_to_fptr(FILE *out_fptr, *buf, unsigned int size)
	c) file_to_buf(char *input_file, *buf, unsigned int wanted_size)
	d) fptr_to_buf(FILE *in_fptr, *buf, unsigned int wanted_size)
	e) fptr_to_fptr
*/

#define IO_BLK_SIZE (1 << 16)
#define PAD_BLK_SIZE (1 << 16)

#define MY_SIZE_ERROR 0xFFFFFFF0
#define MY_READ_ERROR 0xFFFFFFF1
#define MY_WRITE_ERROR 0xFFFFFFF2


/* Function to get the size of a file */
unsigned int get_file_size(char *filename);


/* Function to open a file */
FILE * openfile(char *file, char *mode);


/* Function to write out n bytes of value byte as padding */
unsigned int do_padding(FILE *fptr, unsigned int number, unsigned char byte);


/* Function to append a file to an already open outputfile */
unsigned int cat_file(FILE *out_file_ptr, char *input_file);


/* Function to append a portion of a file to an already open outputfile */
unsigned int cat_part_file(FILE *out_file_ptr, char *input_file, unsigned int offset, unsigned int wanted_size);


