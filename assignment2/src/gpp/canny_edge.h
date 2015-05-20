#ifndef CANNY_EDGE_H
#define CANNY_EDGE_H

void canny(unsigned char *image, int rows, int cols, float sigma,
           float tlow, float thigh, unsigned char **edge, char *fname);
int read_pgm_image(char *infilename, unsigned char **image, int *rows,
                   int *cols);
int write_pgm_image(char *outfilename, unsigned char *image, int rows,
                    int cols, char *comment, int maxval);

#endif /* CANNY_EDGE_H */
