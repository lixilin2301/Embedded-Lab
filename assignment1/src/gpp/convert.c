#include "convert.h"

/**
 * Convert src to bytes, write them to dest. Returns a pointer to the new end (dest+4).
 * src should be a 32 bit integer.
 * dest should be allocated beforehand, large enough!
 */
char* conv_int_byte (int src, char *dest) {
	*dest++ = (unsigned char)(src & 0xFF);
	*dest++ = (unsigned char)((src >> 8) & 0xFF);
	*dest++ = (unsigned char)((src >> 16) & 0xFF);
	*dest++ = (unsigned char)((src >> 24) & 0xFF);
	return dest;
}

/**
 * Convert 4 bytes from src to an int, write it to dest. 
 * Returns a pointer to the new end of src (src+4).
 * src should be a 32 bit integer.
 * dest should be allocated beforehand, large enough!
 */
char* conv_byte_int (char *src, int *dest) {
	*dest = (unsigned char)(*src++);
	*dest += ((unsigned char)(*src++) << 8);
	*dest += ((unsigned char)(*src++) << 16);
	*dest += ((unsigned char)(*src++) << 24);
	return src;	
}

/**
 * Converts n ints from the src array to a byte buffer in dest.
 * Returns a pointer to the new end of the dest buffer.
 * The ints should be 32 bits each!
 */
char* conv_ints_bytes (int *src, char *dest, size_t n) {
	size_t i;
	
	for (i = 0; i < n; ++i) {
		dest = conv_int_byte(*src++, dest);
	}
	return dest;
}

/**
 * Converts the src buffer to n integers in the dest array.
 * Returns a pointer to the new end of the src buffer.
 * The ints should be 32 bits!
 */
char* conv_bytes_ints (char *src, int *dest, size_t n) {
	size_t i;
	
	for (i = 0; i < n; ++i) {
		src = conv_byte_int(src, dest++);
	}
	return src;
}

