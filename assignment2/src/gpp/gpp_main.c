/*  ----------------------------------- OS Specific Headers           */
#include <stdio.h>
#include <stdlib.h>

/*  ----------------------------------- DSP/BIOS Link                 */
#include <dsplink.h>

/*  ----------------------------------- Application Header            */
#include <pool_notify.h>

#include "Timer.h"

#include "canny_edge.h"

#define infilename "pics/tiger.pgm"

/** ============================================================================
 *  @func   main
 *
 *  @desc   Entry point for the application
 *
 *  @modif  None
 *  ============================================================================
 */
int main (int argc, char ** argv)
{
    Char8 * dspExecutable    = NULL ;
    Char8 * strBufferSize    = NULL ;
    char composedfname[128];  /* Name of the output "direction" image */
    char outfilename[128];    /* Name of the output "edge" image */
    char *dirfilename = NULL; /* Name of the output gradient direction image */
    
    int rows, cols;           /* The dimensions of the image. */
    unsigned char *image;     /* The input image */
    unsigned char *edge;      /* The output edge image */
    
    float sigma=2.5,              /* Standard deviation of the gaussian kernel. */
      tlow=0.5,               /* Fraction of the high threshold in hysteresis. */
      thigh=0.5;              /* High hysteresis threshold control. The actual
                threshold is the (100 * thigh) percentage point
                in the histogram of the magnitude of the
                gradient image that passes non-maximal
                suppression. */


    Timer totalTime;
    initTimer(&totalTime, "Total Time");
    
    if (argc != 3) {
        printf ("Usage : %s <absolute path of DSP executable> "
           "<Buffer Size> <number of transfers>\n",
           argv [0]) ;
    }
    else {
        dspExecutable    = argv [1] ;
        strBufferSize    = argv [2] ;

        pool_notify_Main (dspExecutable,
                          strBufferSize) ;
    }
    
    /****************************************************************************
    * Read in the image. This read function allocates memory for the image.
    ****************************************************************************/
    #ifdef VERBOSE
        printf("Reading the image %s.\n", infilename);
    #endif
    if(read_pgm_image(infilename, &image, &rows, &cols) == 0)
    {
        fprintf(stderr, "Error reading the input image, %s.\n", infilename);
        return 1;
    }
    #ifdef VERBOSE
        printf("Starting Canny edge detection.\n");
    #endif
    
    if(dirfilename != NULL)
    {
        sprintf(composedfname, "%s_s_%3.2f_l_%3.2f_h_%3.2f.fim", infilename,
                sigma, tlow, thigh);
        dirfilename = composedfname;
    }
    
    
    /****************************************************************************
    * Perform the edge detection. All of the work takes place here.
    ****************************************************************************/

    startTimer(&totalTime);
    canny(image, rows, cols, sigma, tlow, thigh, &edge, dirfilename);
    stopTimer(&totalTime);
    printTimer(&totalTime);
    
    /****************************************************************************
    * Write out the edge image to a file.
    ****************************************************************************/
    sprintf(outfilename, "%s_s_%3.2f_l_%3.2f_h_%3.2f.pgm", infilename,
            sigma, tlow, thigh);
    #ifdef VERBOSE
        printf("Writing the edge iname in the file %s.\n", outfilename);
    #endif
    if(write_pgm_image(outfilename, edge, rows, cols, "", 255) == 0)
    {
        fprintf(stderr, "Error writing the edge image, %s.\n", outfilename);
        return 2;
    }

    free(edge);
    free(image);

    return 0 ;
}
