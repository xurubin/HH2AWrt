/************************************************
 * Program to swap around the bytes in a file	*
 *   (convert between big and little endian)	*
 ************************************************/

#include <stdio.h>
#include <stdlib.h>
/*#include <sys/stat.h>*/
#include <my_file_functions.h>

unsigned int get_file_size(char *filename);
FILE * openfile(char *file, char *mode);

int main(int argc, char *argv[])
	{
	FILE *fopen(), *in_fptr, *out_fptr;
	char *input_file, *output_file, buf[4];

	unsigned int i, size;

	if (argc != 3)  {
		fprintf(stderr, "Usage: $0 <inputfile> <outputfile>\n");
		exit(1);
		}
	input_file = argv[1];
	output_file = argv[2];
	out_fptr = openfile(output_file,"wb");
	in_fptr = openfile(input_file,"rb");
	size = get_file_size(input_file);
	for ( i = 0; i != (size >> 2); i++)
		{
		buf[0] = fgetc(in_fptr);
		buf[1] = fgetc(in_fptr);
		buf[2] = fgetc(in_fptr);
		buf[3] = fgetc(in_fptr);
		fputc(buf[3],out_fptr);
		fputc(buf[2],out_fptr);
		fputc(buf[1],out_fptr);
		fputc(buf[0],out_fptr);
		}
	fclose(out_fptr);
	fclose(in_fptr);
	return 0;
}

/* Function to get the size of a file */
/*unsigned int get_file_size(char *filename)
	{
	struct stat st;
	if(stat(filename, &st)) {
		fprintf(stderr, "stat: cannot stat %s\n", filename);
		exit(1);
		}
	return(st.st_size);
}
*/

/* Function to open a file */
/*FILE * openfile(char *file, char *mode)
	{
	FILE *fptr;
	if ((fptr = fopen(file, mode)) == NULL)  {
		fprintf(stderr, "Can't open %s for mode %s\n",file, mode);
		exit(1);
		}
	return(fptr);
}
*/

