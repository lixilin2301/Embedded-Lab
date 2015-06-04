#include <stdlib.h>
#include <stdio.h>
#include "pgm_io.h"

#define abs(n) (n < 0 ? -n : n)

int main (int argc, char **argv) {
	unsigned char *image1, *image2, *diff;
	int rows1, rows2,
		cols1, cols2,
		x, y;

	if (argc < 4) {
		printf("Usage: %s <image1> <image2> <output>\n", argv[0]);
		return 1;
	}

	if (read_pgm_image(argv[1], &image1, &rows1, &cols1) == 0) {
		printf("Unable to open image %s\n", argv[1]);
		return 2;
	}
	if (read_pgm_image(argv[2], &image2, &rows2, &cols2) == 0) {
		printf("Unable to open image %s\n", argv[2]);
		return 3;
	}

	if (rows1 != rows2 || cols1 != cols2) {
		printf("Images are of different sizes!\n");
		return 4;
	}

	diff = malloc(rows1 * cols1 * sizeof(unsigned char));

	for (y = 0; y < rows1; ++y) {
		for (x = 0; x < cols1; ++x) {
			diff[y * rows1 + x] = abs(image1[y * rows1 + x] - image2[y * rows1 + x]);
		}
	}

	if (write_pgm_image(argv[3], diff, rows1, cols1, "", 255) == 0) {
		printf("Unable to write image %s\n", argv[3]);
		return 5;
	}

	free(image1);
	free(image2);
	free(diff);
	return 0;
}