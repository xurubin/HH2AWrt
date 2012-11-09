/* bitsnbobs.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <my_file_functions.h>
#include <bitsnbobs.h>


/* #define IGNORE_LINE "%*[^\n]" */
/* eg.	while(fscanf(listptr,"%60s"IGNORE_LINE,relpath)!=EOF) */


/* Function to write out a u32 in big endian format */
void write_int_big(FILE *out_fptr, unsigned int value)
	{
	putc(value >>24, out_fptr);
	putc(value >>16, out_fptr);
	putc(value >>8, out_fptr);
	putc(value & 0xFF, out_fptr);
}


/* Function to calculate number of bytes of padding required */
unsigned int calc_padding(unsigned int orig_size, unsigned int block_size)
	{
	unsigned int remainder, padding;
	remainder = orig_size % block_size;
	if (remainder == 0)  {
		padding = 0;
	}
	else  {
		padding = block_size - remainder;
	}
	return(padding);
}

/*
static int fill(void *buf, unsigned int size)
{
	return fread(buf, 1, size, stdin);
}

static int flush( void *buf, unsigned int size)
{
	return fwrite(buf, 1, size, stdout);
}

static void test_buf_to_buf(void)
{
	size_t in_size;
	int ret;
	in_size = fread(in, 1, sizeof(in), stdin);
	fprintf(stderr, "something\n");
}

error:
	fputs(argv[0], stderr);
	fputs(": ", stderr);
	fputs(msg, stderr);
	return 1;
}
*/

/* Function with behaviour like `mkdir -p'	Copied from:
http://niallohiggins.com/2009/01/08/mkpath-mkdir-p-alike-in-c-for-unix/  */
/*
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>

int  mkpath(const char *s, mode_t mode)
	{
        char *q, *r = NULL, *path = NULL, *up = NULL;
        int rv;
 
        rv = -1;
        if (strcmp(s, ".") == 0 || strcmp(s, "/") == 0)
                return (0);
        if ((path = strdup(s)) == NULL)
                exit(1);
        if ((q = strdup(s)) == NULL)
                exit(1);
        if ((r = dirname(q)) == NULL)
                goto out;
        if ((up = strdup(r)) == NULL)
                exit(1);
        if ((mkpath(up, mode) == -1) && (errno != EEXIST))
                goto out;
        if ((mkdir(path, mode) == -1) && (errno != EEXIST))
                rv = -1;
        else
                rv = 0;
out:
        if (up != NULL)
                free(up);
        free(q);
        free(path);
        return (rv);
}
*/
