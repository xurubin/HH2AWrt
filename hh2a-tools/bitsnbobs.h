/* bitsnbobs.h */



/* Function to write out a u32 in big endian format */
void write_int_big(FILE *fptr, unsigned int value);


/* Function to calculate number of bytes of padding required */
unsigned int calc_padding(unsigned int orig_size, unsigned int block_size);



/* Function with behaviour like `mkdir -p' */
/*int  mkpath(const char *s, mode_t mode); */

