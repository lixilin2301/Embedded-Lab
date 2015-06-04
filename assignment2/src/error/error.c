#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "pgm_io.h"


int main (int argc, char **argv) {
	unsigned char *original, *image1, *image2, *diff;
	int rows, rows1, rows2,
		cols, cols1, cols2,
		x, y;
	float mse1 = 0, mse2 = 0;
	float temp1, temp2;

	if (argc < 5) {
		printf("Usage: %s <original> <image1> <image2> <output>\n", argv[0]);
		printf("\n<original> is the filename of the original image\n");
		printf("\n<image1> is the filename of the \"correct\" image\n");
		printf("\n<image2> is the filename of the second image\n");
		printf("\n<output> is the filename where a difference map will be written to\n");
		return 1;
	}

	if (read_pgm_image(argv[1], &original, &rows, &cols) == 0) {
		printf("Unable to open image %s\n", argv[1]);
		return 2;
	}
	if (read_pgm_image(argv[2], &image1, &rows1, &cols1) == 0) {
		printf("Unable to open image %s\n", argv[2]);
		return 2;
	}
	if (read_pgm_image(argv[3], &image2, &rows2, &cols2) == 0) {
		printf("Unable to open image %s\n", argv[3]);
		return 3;
	}

	if (rows1 != rows2 || cols1 != cols2) {
		printf("Images are of different sizes!\n");
		return 4;
	}

	diff = malloc(rows1 * cols1 * sizeof(unsigned char));

	for (y = 0; y < rows1; ++y) {
		for (x = 0; x < cols1; ++x) {
			diff[y * rows1 + x] = 255 - abs(image1[y * rows1 + x] - image2[y * rows1 + x]);

			temp1 = (original[y * rows1 + x] - image1[y * rows1 + x]);
			temp2 = (original[y * rows1 + x] - image2[y * rows1 + x]);
			mse1 += (temp1 * temp1);
			mse2 += (temp2 * temp2);
		}
	}
	mse1 /= (rows1 * cols1);
	mse2 /= (rows1 * cols1);

	if (write_pgm_image(argv[4], diff, rows1, cols1, "", 255) == 0) {
		printf("Unable to write image %s\n", argv[4]);
		return 5;
	}

	printf("mean squared error 1: %f\n", mse1);
	printf("mean squared error 2: %f\n", mse2);
	printf("mean squared error difference: %f\n", fabs(mse2 - mse1));
	printf("error rate: %f\n", fabs(mse2 - mse1) / mse1);

	free(original);
	free(image1);
	free(image2);
	free(diff);
	return 0;
}