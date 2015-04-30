#include <stdlib.h>

/**
 * Convert src to bytes, write them to dest. Returns a pointer to the new end (dest+4).
 * src should be a 32 bit integer.
 * dest should be allocated beforehand, large enough!
 */
char* conv_int_byte (int src, char *dest);

/**
 * Convert 4 bytes from src to an int, write it to dest. Returns a pointer to the new end of src (src+4).
 * src should be a 32 bit integer.
 * dest should be allocated beforehand, large enough!
 */
char* conv_byte_int (char *src, int *dest);


char* conv_ints_bytes (int *src, char *dest, size_t n);
char* conv_bytes_ints (char *src, int *dest, size_t n);
