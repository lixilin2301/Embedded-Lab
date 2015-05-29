#ifndef NEON_H
#define NEON_H

void make_gaussian_kernel(float sigma, float **kernel, int *windowsize);
short int* gaussian_smooth_neon(unsigned char *image, int rows, int cols, float sigma);

#endif /* NEON_H */

