/*******************************************************************************
* --------------------------------------------
*(c) 2001 University of South Florida, Tampa
* Use, or copying without permission prohibited.
* PERMISSION TO USE
* In transmitting this software, permission to use for research and
* educational purposes is hereby granted.  This software may be copied for
* archival and backup purposes only.  This software may not be transmitted
* to a third party without prior permission of the copyright holder. This
* permission may be granted only by Mike Heath or Prof. Sudeep Sarkar of
* University of South Florida (sarkar@csee.usf.edu). Acknowledgment as
* appropriate is respectfully requested.
*
*  Heath, M., Sarkar, S., Sanocki, T., and Bowyer, K. Comparison of edge
*    detectors: a methodology and initial study, Computer Vision and Image
*    Understanding 69 (1), 38-54, January 1998.
*  Heath, M., Sarkar, S., Sanocki, T. and Bowyer, K.W. A Robust Visual
*    Method for Assessing the Relative Performance of Edge Detection
*    Algorithms, IEEE Transactions on Pattern Analysis and Machine
*    Intelligence 19 (12),  1338-1359, December 1997.
*  ------------------------------------------------------
*
* PROGRAM: canny_edge
* PURPOSE: This program implements a "Canny" edge detector. The processing
* steps are as follows:
*
*   1) Convolve the image with a separable gaussian filter.
*   2) Take the dx and dy the first derivatives using [-1,0,1] and [1,0,-1]'.
*   3) Compute the magnitude: sqrt(dx*dx+dy*dy).
*   4) Perform non-maximal suppression.
*   5) Perform hysteresis.
*
* The user must input three parameters. These are as follows:
*
*   sigma = The standard deviation of the gaussian smoothing filter.
*   tlow  = Specifies the low value to use in hysteresis. This is a
*           fraction (0-1) of the computed high threshold edge strength value.
*   thigh = Specifies the high value to use in hysteresis. This fraction (0-1)
*           specifies the percentage point in a histogram of the gradient of
*           the magnitude. Magnitude values of zero are not counted in the
*           histogram.
*
* NAME: Mike Heath
*       Computer Vision Laboratory
*       University of South Floeida
*       heath@csee.usf.edu
*
* DATE: 2/15/96
*
* Modified: 5/17/96 - To write out a floating point RAW headerless file of
*                     the edge gradient "up the edge" where the angle is
*                     defined in radians counterclockwise from the x direction.
*                     (Mike Heath)
*******************************************************************************/

#define VERBOSE 0
#define BOOSTBLURFACTOR 90.0

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "markers.h"
#include "Timer.h"

int read_pgm_image(char *infilename, unsigned char **image, int *rows,
                   int *cols);
int write_pgm_image(char *outfilename, unsigned char *image, int rows,
                    int cols, char *comment, int maxval);

void canny(unsigned char *image, int rows, int cols, float sigma,
           float tlow, float thigh, unsigned char **edge, char *fname);
short int* gaussian_smooth(unsigned char *image, int rows, int cols, float sigma);
void make_gaussian_kernel(float sigma, float **kernel, int *windowsize);
void derrivative_x_y(short int *smoothedim, int rows, int cols,
        short int **delta_x, short int **delta_y);
void magnitude_x_y(short int *delta_x, short int *delta_y, int rows, int cols,
                   short int *magnitude);
void apply_hysteresis(short int *mag, unsigned char *nms, int rows, int cols,
                      float tlow, float thigh, unsigned char *edge);
void radian_direction(short int *delta_x, short int *delta_y, int rows,
                      int cols, float **dir_radians, int xdirtag, int ydirtag);
double angle_radians(double x, double y);

void non_max_supp(short *mag, short *gradx, short *grady, int nrows,
                  int ncols, unsigned char *result);

Timer gaussianTime;

int main(int argc, char *argv[])
{
    char *infilename = NULL;  /* Name of the input image */
    char *dirfilename = NULL; /* Name of the output gradient direction image */
    char outfilename[128];    /* Name of the output "edge" image */
    char composedfname[128];  /* Name of the output "direction" image */
    unsigned char *image;     /* The input image */
    unsigned char *edge;      /* The output edge image */
    int rows, cols;           /* The dimensions of the image. */
    float sigma=2.5,              /* Standard deviation of the gaussian kernel. */
          tlow=0.5,               /* Fraction of the high threshold in hysteresis. */
          thigh=0.5;              /* High hysteresis threshold control. The actual
			        threshold is the (100 * thigh) percentage point
			        in the histogram of the magnitude of the
			        gradient image that passes non-maximal
			        suppression. */

    /****************************************************************************
    * Get the command line arguments.
    ****************************************************************************/
    if(argc < 2)
    {
        fprintf(stderr,"\n<USAGE> %s image sigma tlow thigh [writedirim]\n",argv[0]);
        fprintf(stderr,"\n      image:      An image to process. Must be in ");
        fprintf(stderr,"PGM format.\n");
        exit(1);
    }

    infilename = argv[1];

    Timer totalTime;
    initTimer(&totalTime, "Total Time");
    initTimer(&gaussianTime, "Gaussian Time");


    /****************************************************************************
    * Read in the image. This read function allocates memory for the image.
    ****************************************************************************/
    if(VERBOSE) printf("Reading the image %s.\n", infilename);
    if(read_pgm_image(infilename, &image, &rows, &cols) == 0)
    {
        fprintf(stderr, "Error reading the input image, %s.\n", infilename);
        exit(1);
    }

    /****************************************************************************
    * Perform the edge detection. All of the work takes place here.
    ****************************************************************************/
    if(VERBOSE) printf("Starting Canny edge detection.\n");
    if(dirfilename != NULL)
    {
        sprintf(composedfname, "%s_s_%3.2f_l_%3.2f_h_%3.2f.fim", infilename,
                sigma, tlow, thigh);
        dirfilename = composedfname;
    }

    startTimer(&totalTime);
    MCPROF_START();
    canny(image, rows, cols, sigma, tlow, thigh, &edge, dirfilename);
    MCPROF_STOP();
    stopTimer(&totalTime);
    printTimer(&totalTime);


    /****************************************************************************
    * Write out the edge image to a file.
    ****************************************************************************/
    sprintf(outfilename, "%s_s_%3.2f_l_%3.2f_h_%3.2f.pgm", infilename,
            sigma, tlow, thigh);
    if(VERBOSE) printf("Writing the edge iname in the file %s.\n", outfilename);
    if(write_pgm_image(outfilename, edge, rows, cols, "", 255) == 0)
    {
        fprintf(stderr, "Error writing the edge image, %s.\n", outfilename);
        exit(1);
    }

    free(image);
    free(edge);
    return 0;
}

/*******************************************************************************
* PROCEDURE: canny
* PURPOSE: To perform canny edge detection.
* NAME: Mike Heath
* DATE: 2/15/96
*******************************************************************************/
void canny(unsigned char *image, int rows, int cols, float sigma,
           float tlow, float thigh, unsigned char **edge, char *fname)
{
    FILE *fpdir=NULL;          /* File to write the gradient image to.     */
    unsigned char *nms;        /* Points that are local maximal magnitude. */
    short int *smoothedim,     /* The image after gaussian smoothing.      */
          *delta_x,        /* The first devivative image, x-direction. */
          *delta_y,        /* The first derivative image, y-direction. */
          *magnitude;      /* The magnitude of the gadient image.      */
    float *dir_radians=NULL;   /* Gradient direction image.                */

	Timer derivative, radian, magnitudeTimer, nonmax, hysteresis;
	initTimer(&derivative, "derivative");
	initTimer(&radian, "radian");
	initTimer(&magnitudeTimer, "magnitude");
	initTimer(&nonmax, "nonmax");
	initTimer(&hysteresis, "hysteresis");

    /****************************************************************************
    * Perform gaussian smoothing on the image using the input standard
    * deviation.
    ****************************************************************************/
    if(VERBOSE) printf("Smoothing the image using a gaussian kernel.\n");
    
	startTimer(&gaussianTime);
    smoothedim = gaussian_smooth(image, rows, cols, sigma);
    stopTimer(&gaussianTime);
    printTimer(&gaussianTime);


    /****************************************************************************
    * Compute the first derivative in the x and y directions.
    ****************************************************************************/
    if(VERBOSE) printf("Computing the X and Y first derivatives.\n");
	startTimer(&derivative);
    derrivative_x_y(smoothedim, rows, cols, &delta_x, &delta_y);
	stopTimer(&derivative);
	printTimer(&derivative);

    /****************************************************************************
    * This option to write out the direction of the edge gradient was added
    * to make the information available for computing an edge quality figure
    * of merit.
    ****************************************************************************/
    if(fname != NULL)
    {
        /*************************************************************************
        * Compute the direction up the gradient, in radians that are
        * specified counteclockwise from the positive x-axis.
        *************************************************************************/
		startTimer(&radian);
        radian_direction(delta_x, delta_y, rows, cols, &dir_radians, -1, -1);
		stopTimer(&radian);
		printTimer(&radian);

        /*************************************************************************
        * Write the gradient direction image out to a file.
        *************************************************************************/
        if((fpdir = fopen(fname, "wb")) == NULL)
        {
            fprintf(stderr, "Error opening the file %s for writing.\n", fname);
            exit(1);
        }
        fwrite(dir_radians, sizeof(float), rows*cols, fpdir);
        fclose(fpdir);
        free(dir_radians);
    }

    /****************************************************************************
    * Compute the magnitude of the gradient.
    ****************************************************************************/
    /****************************************************************************
    * Allocate an image to store the magnitude of the gradient.
    ****************************************************************************/
    if((magnitude = (short *) malloc(rows*cols* sizeof(short))) == NULL)
    {
        fprintf(stderr, "Error allocating the magnitude image.\n");
        exit(1);
    }

    if(VERBOSE) printf("Computing the magnitude of the gradient.\n");
	startTimer(&magnitudeTimer);
    magnitude_x_y(delta_x, delta_y, rows, cols, magnitude);
	stopTimer(&magnitudeTimer);
	printTimer(&magnitudeTimer);

    /****************************************************************************
    * Perform non-maximal suppression.
    ****************************************************************************/
    if(VERBOSE) printf("Doing the non-maximal suppression.\n");
    if((nms = (unsigned char *) malloc(rows*cols*sizeof(unsigned char)))==NULL)
    {
        fprintf(stderr, "Error allocating the nms image.\n");
        exit(1);
    }
	startTimer(&nonmax);
    non_max_supp(magnitude, delta_x, delta_y, rows, cols, nms);
	stopTimer(&nonmax);
	printTimer(&nonmax);

    /****************************************************************************
    * Use hysteresis to mark the edge pixels.
    ****************************************************************************/
    if(VERBOSE) printf("Doing hysteresis thresholding.\n");
    if( (*edge=(unsigned char *)malloc(rows*cols*sizeof(unsigned char))) == NULL )
    {
        fprintf(stderr, "Error allocating the edge image.\n");
        exit(1);
    }
	startTimer(&hysteresis);
    apply_hysteresis(magnitude, nms, rows, cols, tlow, thigh, *edge);
	stopTimer(&hysteresis);
	printTimer(&hysteresis);

    /****************************************************************************
    * Free all of the memory that we allocated except for the edge image that
    * is still being used to store out result.
    ****************************************************************************/
    free(smoothedim);
    free(delta_x);
    free(delta_y);
    free(magnitude);
    free(nms);
}

/*******************************************************************************
* Procedure: radian_direction
* Purpose: To compute a direction of the gradient image from component dx and
* dy images. Because not all derriviatives are computed in the same way, this
* code allows for dx or dy to have been calculated in different ways.
*
* FOR X:  xdirtag = -1  for  [-1 0  1]
*         xdirtag =  1  for  [ 1 0 -1]
*
* FOR Y:  ydirtag = -1  for  [-1 0  1]'
*         ydirtag =  1  for  [ 1 0 -1]'
*
* The resulting angle is in radians measured counterclockwise from the
* xdirection. The angle points "up the gradient".
*******************************************************************************/
void radian_direction(short int *delta_x, short int *delta_y, int rows,
                      int cols, float **dir_radians, int xdirtag, int ydirtag)
{
    int r, c, pos;
    float *dirim=NULL;
    double dx, dy;

    /****************************************************************************
    * Allocate an image to store the direction of the gradient.
    ****************************************************************************/
    if((dirim = (float *) malloc(rows*cols* sizeof(float))) == NULL)
    {
        fprintf(stderr, "Error allocating the gradient direction image.\n");
        exit(1);
    }
    *dir_radians = dirim;

    for(r=0,pos=0; r<rows; r++)
    {
        for(c=0; c<cols; c++,pos++)
        {
            dx = (double)delta_x[pos];
            dy = (double)delta_y[pos];

            if(xdirtag == 1) dx = -dx;
            if(ydirtag == -1) dy = -dy;

            dirim[pos] = (float)angle_radians(dx, dy);
        }
    }
}

/*******************************************************************************
* FUNCTION: angle_radians
* PURPOSE: This procedure computes the angle of a vector with components x and
* y. It returns this angle in radians with the answer being in the range
* 0 <= angle <2*PI.
*******************************************************************************/
double angle_radians(double x, double y)
{
    double xu, yu, ang;

    xu = fabs(x);
    yu = fabs(y);

    if((xu == 0) && (yu == 0)) return(0);

    ang = atan(yu/xu);

    if(x >= 0)
    {
        if(y >= 0) return(ang);
        else return(2*M_PI - ang);
    }
    else
    {
        if(y >= 0) return(M_PI - ang);
        else return(M_PI + ang);
    }
}

/*******************************************************************************
* PROCEDURE: magnitude_x_y
* PURPOSE: Compute the magnitude of the gradient. This is the square root of
* the sum of the squared derivative values.
* NAME: Mike Heath
* DATE: 2/15/96
*******************************************************************************/
void magnitude_x_y(short int *delta_x, short int *delta_y, int rows, int cols,
                   short int *magnitude)
{
    int r, c, pos, sq1, sq2;

    for(r=0,pos=0; r<rows; r++)
    {
        for(c=0; c<cols; c++,pos++)
        {
            sq1 = (int)delta_x[pos] * (int)delta_x[pos];
            sq2 = (int)delta_y[pos] * (int)delta_y[pos];
            magnitude[pos] = (short)(0.5 + sqrt((float)sq1 + (float)sq2));
        }
    }
}


/*******************************************************************************
* PROCEDURE: derrivative_x_y
* PURPOSE: Compute the first derivative of the image in both the x any y
* directions. The differential filters that are used are:
*
*                                          -1
*         dx =  -1 0 +1     and       dy =  0
*                                          +1
*
* NAME: Mike Heath
* DATE: 2/15/96
*******************************************************************************/
void derrivative_x_y(short int *smoothedim, int rows, int cols,
        short int **delta_x, short int **delta_y)
{
   int r, c, pos;
   /****************************************************************************
   * Allocate images to store the derivatives.
   ****************************************************************************/
   if(((*delta_x) = (short *) malloc(rows*cols* sizeof(short))) == NULL){
      fprintf(stderr, "Error allocating the delta_x image.\n");
      exit(1);
   }
   if(((*delta_y) = (short *) malloc(rows*cols* sizeof(short))) == NULL){
      fprintf(stderr, "Error allocating the delta_x image.\n");
      exit(1);
   }


   for(r=0;r<rows;r++){
      pos = r * cols;
      (*delta_x)[pos] = smoothedim[pos+1] - smoothedim[pos];
      pos++;
      for(c=1;c<(cols-1);c++,pos++){
         (*delta_x)[pos] = smoothedim[pos+1] - smoothedim[pos-1];
      }
      (*delta_x)[pos] = smoothedim[pos] - smoothedim[pos-1];
   }

   for(c=0;c<cols;c++){
      pos = c;
      (*delta_y)[pos] = smoothedim[pos+cols] - smoothedim[pos];
      pos += cols;
      for(r=1;r<(rows-1);r++,pos+=cols){
         (*delta_y)[pos] = smoothedim[pos+cols] - smoothedim[pos-cols];
      }
      (*delta_y)[pos] = smoothedim[pos] - smoothedim[pos-cols];
   }
}

/*******************************************************************************
* PROCEDURE: gaussian_smooth
* PURPOSE: Blur an image with a gaussian filter.
* NAME: Mike Heath
* DATE: 2/15/96
*******************************************************************************/
short int* gaussian_smooth(unsigned char *image, int rows, int cols, float sigma)
{
    int r, c, rr, cc,     /* Counter variables. */
        windowsize,        /* Dimension of the gaussian kernel. */
        center;            /* Half of the windowsize. */
    float *tempim,        /* Buffer for separable filter gaussian smoothing. */
          *kernel,        /* A one dimensional gaussian kernel. */
          dot,            /* Dot product summing variable. */
          sum;            /* Sum of the kernel weights variable. */
    int i;
    /****************************************************************************
    * Create a 1-dimensional gaussian smoothing kernel.
    ****************************************************************************/
    if(VERBOSE) printf("   Computing the gaussian smoothing kernel.\n");
    make_gaussian_kernel(sigma, &kernel, &windowsize);
    for(i=0;i<16;i++){
      printf("%d: %f, ",i,kernel[i]);
      printf("%lu, ",(unsigned short int)(kernel[i]*(1<<16)));
      printf("%lu, \n",(unsigned short int)(kernel[i]*(1<<17)));
    }
    center = windowsize / 2;


    /****************************************************************************
    * Allocate a temporary buffer image and the smoothed image.
    ****************************************************************************/
    if((tempim = (float *) malloc(rows*cols* sizeof(float))) == NULL)
    {
        fprintf(stderr, "Error allocating the buffer image.\n");
        exit(1);
    }
    short int* smoothedim;
    if(((smoothedim) = (short int *) malloc(rows*cols*sizeof(short int))) == NULL)
    {
        fprintf(stderr, "Error allocating the smoothed image.\n");
        exit(1);
    }

    MCPROF_ZONE_ENTER(1);
    /****************************************************************************
    * Blur in the x - direction.
    ****************************************************************************/
    if(VERBOSE) printf("   Bluring the image in the X-direction.\n");
    for(r=0; r<rows; r++)
    {
        for(c=0; c<cols; c++)
        {
            dot = 0.0;
            sum = 0.0;
            for(cc=(-center); cc<=center; cc++)
            {
                if(((c+cc) >= 0) && ((c+cc) < cols))
                {
                    dot += (float)image[r*cols+(c+cc)] * kernel[center+cc];
                    sum += kernel[center+cc];
                }
            }
            tempim[r*cols+c] = dot/sum;
        }
    }
    MCPROF_ZONE_EXIT(1);

    MCPROF_ZONE_ENTER(2);
    /****************************************************************************
    * Blur in the y - direction.
    ****************************************************************************/
    if(VERBOSE) printf("   Bluring the image in the Y-direction.\n");
    for(c=0; c<cols; c++)
    {
        for(r=0; r<rows; r++)
        {
            sum = 0.0;
            dot = 0.0;
            for(rr=(-center); rr<=center; rr++)
            {
                if(((r+rr) >= 0) && ((r+rr) < rows))
                {
                    dot += tempim[(r+rr)*cols+c] * kernel[center+rr];
                    sum += kernel[center+rr];
                }
            }
            smoothedim[r*cols+c] = (short int)(dot*BOOSTBLURFACTOR/sum + 0.5);
        }
    }
    MCPROF_ZONE_EXIT(2);

    free(tempim);
    free(kernel);
    return smoothedim;
}

/*******************************************************************************
* PROCEDURE: make_gaussian_kernel
* PURPOSE: Create a one dimensional gaussian kernel.
* NAME: Mike Heath
* DATE: 2/15/96
*******************************************************************************/
void make_gaussian_kernel(float sigma, float **kernel, int *windowsize)
{
    int i, center;
    float x, fx, sum=0.0;

    *windowsize = 1 + 2 * ceil(2.5 * sigma);
    center = (*windowsize) / 2;

    if((*kernel = (float *) malloc((*windowsize)* sizeof(float))) == NULL)
    {
        fprintf(stderr, "Error callocing the gaussian kernel array.\n");
        exit(1);
    }

    for(i=0; i<(*windowsize); i++)
    {
        x = (float)(i - center);
        fx = pow(2.71828, -0.5*x*x/(sigma*sigma)) / (sigma * sqrt(6.2831853));
        (*kernel)[i] = fx;
        sum += fx;
    }

    for(i=0; i<(*windowsize); i++) (*kernel)[i] /= sum;

    if(VERBOSE)
    {
        printf("The filter coefficients are:\n");
        for(i=0; i<(*windowsize); i++)
            printf("kernel[%d] = %f\n", i, (*kernel)[i]);
    }
}

