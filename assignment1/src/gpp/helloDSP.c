/** ============================================================================
 *  @file   helloDSP.c
 *
 *  @path
 *
 *  @desc   This is an application which sends messages to the DSP
 *          processor and receives them back using DSP/BIOS LINK.
 *          The message contents received are verified against the data
 *          sent to DSP.
 *
*/
/*  ----------------------------------- DSP/BIOS Link                   */
#include <dsplink.h>

/*  ----------------------------------- DSP/BIOS LINK API               */
#include <proc.h>
#include <msgq.h>
#include <pool.h>

/*  ----------------------------------- Application Header              */
#include <helloDSP.h>
#include <system_os.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "timer.h"
#include <stdint.h>


#if defined (__cplusplus)
extern "C"
{
#endif /* defined (__cplusplus) */


////////////
Timer totalTime;

Timer dsp_only;
////////////

    /* Number of arguments specified to the DSP application. */
#define NUM_ARGS 1

    /* Argument size passed to the control message queue */
#define ARG_SIZE 256

    /* ID of the POOL used by helloDSP. */
#define SAMPLE_POOL_ID  0

    /*  Number of BUF pools in the entire memory pool */
#define NUMMSGPOOLS     4

    /* Number of messages in each BUF pool. */
#define NUMMSGINPOOL0   1
#define NUMMSGINPOOL1   2
#define NUMMSGINPOOL2   2
#define NUMMSGINPOOL3   4

#define MAT_SIZE 128
#define SIZE (MAT_SIZE/2)

//#define DEBUG

    /* Control message data structure. */
    /* Must contain a reserved space for the header */
    typedef struct ControlMsg
    {
        MSGQ_MsgHeader header;
        Uint16 command;
        Char8 arg1[ARG_SIZE];
        int mat1[SIZE][SIZE];
        int mat2[SIZE][SIZE];
   
    } ControlMsg;

	int mat1[MAT_SIZE][MAT_SIZE], mat2[MAT_SIZE][MAT_SIZE], prod[MAT_SIZE][MAT_SIZE], prod_ver[MAT_SIZE][MAT_SIZE];
     	
    /* Messaging buffer used by the application.
     * Note: This buffer must be aligned according to the alignment expected
     * by the device/platform. */
#define APP_BUFFER_SIZE DSPLINK_ALIGN (sizeof (ControlMsg), DSPLINK_BUF_ALIGN)

    /* Definitions required for the sample Message queue.
     * Using a Zero-copy based transport on the shared memory physical link. */
#if defined ZCPY_LINK
#define SAMPLEMQT_CTRLMSG_SIZE  ZCPYMQT_CTRLMSG_SIZE
    STATIC ZCPYMQT_Attrs mqtAttrs;
#endif

    /* Message sizes managed by the pool */
    STATIC Uint32 SampleBufSizes[NUMMSGPOOLS] =
    {
        APP_BUFFER_SIZE,
        SAMPLEMQT_CTRLMSG_SIZE,
        DSPLINK_ALIGN (sizeof(MSGQ_AsyncLocateMsg), DSPLINK_BUF_ALIGN),
        DSPLINK_ALIGN (sizeof(MSGQ_AsyncErrorMsg), DSPLINK_BUF_ALIGN)
    };

    /* Number of messages in each pool */
    STATIC Uint32 SampleNumBuffers[NUMMSGPOOLS] =
    {
        NUMMSGINPOOL0,
        NUMMSGINPOOL1,
        NUMMSGINPOOL2,
        NUMMSGINPOOL3
    };

    /* Definition of attributes for the pool based on physical link used by the transport */
#if defined ZCPY_LINK
    STATIC SMAPOOL_Attrs SamplePoolAttrs =
    {
        NUMMSGPOOLS,
        SampleBufSizes,
        SampleNumBuffers,
        TRUE   /* If allocating a buffer smaller than the POOL size, set this to FALSE */
    };
#endif

    /* Name of the first MSGQ on the GPP and on the DSP. */
    STATIC Char8 SampleGppMsgqName[DSP_MAX_STRLEN] = "GPPMSGQ1";
    STATIC Char8 SampleDspMsgqName[DSP_MAX_STRLEN] = "DSPMSGQ";

    /* Local GPP's and DSP's MSGQ Objects. */
    STATIC MSGQ_Queue SampleGppMsgq = (Uint32) MSGQ_INVALIDMSGQ;
    STATIC MSGQ_Queue SampleDspMsgq = (Uint32) MSGQ_INVALIDMSGQ;

    /* Place holder for the MSGQ name created on DSP */
    Char8 dspMsgqName[DSP_MAX_STRLEN];

    /* Extern declaration to the default DSP/BIOS LINK configuration structure. */
    extern LINKCFG_Object LINKCFG_config;

#if defined (VERIFY_DATA)
    /** ============================================================================
     *  @func   helloDSP_VerifyData
     *
     *  @desc   This function verifies the data-integrity of given message.
     *  ============================================================================
     */
    STATIC NORMAL_API DSP_STATUS helloDSP_VerifyData(IN MSGQ_Msg msg, IN Uint16 sequenceNumber);
#endif


    /** ============================================================================
     *  @func   helloDSP_Create
     *
     *  @desc   This function allocates and initializes resources used by
     *          this application.
     *
     *  @modif  helloDSP_InpBufs , helloDSP_OutBufs
     *  ============================================================================
     */
    NORMAL_API DSP_STATUS helloDSP_Create(IN Char8* dspExecutable, IN Char8* strNumIterations, IN Uint8 processorId)
    {
        DSP_STATUS status = DSP_SOK;
        Uint32 numArgs = NUM_ARGS;
        MSGQ_LocateAttrs syncLocateAttrs;
        Char8* args[NUM_ARGS];

		/////////////////////////////////////////////////////
		initTimer(&totalTime, "Total Time");
		initTimer(&dsp_only, "DSP Time");
		/////////////////////////////////////////////////////
		
        SYSTEM_0Print("Entered helloDSP_Create ()\n");

        /* Create and initialize the proc object. */
        status = PROC_setup(NULL);

        /* Attach the Dsp with which the transfers have to be done. */
        if (DSP_SUCCEEDED(status))
        {
            status = PROC_attach(processorId, NULL);
            if (DSP_FAILED(status))
            {
                SYSTEM_1Print("PROC_attach () failed. Status = [0x%x]\n", status);
            }
        }

        /* Open the pool. */
        if (DSP_SUCCEEDED(status))
        {
            status = POOL_open(POOL_makePoolId(processorId, SAMPLE_POOL_ID), &SamplePoolAttrs);
            if (DSP_FAILED(status))
            {
                SYSTEM_1Print("POOL_open () failed. Status = [0x%x]\n", status);
            }
        }
        else
        {
            SYSTEM_1Print("PROC_setup () failed. Status = [0x%x]\n", status);
        }

        /* Open the GPP's message queue */
        if (DSP_SUCCEEDED(status))
        {
            status = MSGQ_open(SampleGppMsgqName, &SampleGppMsgq, NULL);
            if (DSP_FAILED(status))
            {
                SYSTEM_1Print("MSGQ_open () failed. Status = [0x%x]\n", status);
            }
        }

        /* Set the message queue that will receive any async. errors */
        if (DSP_SUCCEEDED(status))
        {
            status = MSGQ_setErrorHandler(SampleGppMsgq, POOL_makePoolId(processorId, SAMPLE_POOL_ID));
            if (DSP_FAILED(status))
            {
                SYSTEM_1Print("MSGQ_setErrorHandler () failed. Status = [0x%x]\n", status);
            }
        }

        /* Load the executable on the DSP. */
        if (DSP_SUCCEEDED(status))
        {
            args [0] = strNumIterations;
            {
                status = PROC_load(processorId, dspExecutable, numArgs, args);
            }
            if (DSP_FAILED(status))
            {
                SYSTEM_1Print("PROC_load () failed. Status = [0x%x]\n", status);
            }
        }
	
		 SYSTEM_0Print("  Executable loaded onto DSP! \n");
		 
        /* Start execution on DSP. */
        if (DSP_SUCCEEDED(status))
        {
            status = PROC_start(processorId);
            if (DSP_FAILED(status))
            {
                SYSTEM_1Print("PROC_start () failed. Status = [0x%x]\n", status);
            }
        }

        /* Open the remote transport. */
        if (DSP_SUCCEEDED(status))
        {
            mqtAttrs.poolId = POOL_makePoolId(processorId, SAMPLE_POOL_ID);
            status = MSGQ_transportOpen(processorId, &mqtAttrs);
            if (DSP_FAILED(status))
            {
                SYSTEM_1Print("MSGQ_transportOpen () failed. Status = [0x%x]\n", status);
            }
        }

        /* Locate the DSP's message queue */
        /* At this point the DSP must open a message queue named "DSPMSGQ" */
        if (DSP_SUCCEEDED(status))
        {
            syncLocateAttrs.timeout = WAIT_FOREVER;
            status = DSP_ENOTFOUND;
            SYSTEM_2Sprint(dspMsgqName, "%s%d", (Uint32) SampleDspMsgqName, processorId);
            while ((status == DSP_ENOTFOUND) || (status == DSP_ENOTREADY))
            {
                status = MSGQ_locate(dspMsgqName, &SampleDspMsgq, &syncLocateAttrs);
                if ((status == DSP_ENOTFOUND) || (status == DSP_ENOTREADY))
                {
                    SYSTEM_Sleep(100000);
                }
                else if (DSP_FAILED(status))
                {
                    SYSTEM_1Print("MSGQ_locate () failed. Status = [0x%x]\n", status);
                }
            }
        }
        
		SYSTEM_0Print("  DSP opened a message queue named \"DSPMSGQ\" \n");
        SYSTEM_0Print("Leaving helloDSP_Create ()\n\n");
        return status;
    }


	NORMAL_API DSP_STATUS helloDSP_Recieve(ControlMsg *msg)
	{
		DSP_STATUS status = DSP_SOK;
		/* Receive the message. */
		status = MSGQ_get(SampleGppMsgq, WAIT_FOREVER, (MsgqMsg *) &msg);
		if (DSP_FAILED(status))
		{
			SYSTEM_1Print("MSGQ_get () failed. Status = [0x%x]\n", status);
		}
#if defined (VERIFY_DATA)
		/* Verify correctness of data received. */
		if (DSP_SUCCEEDED(status))
		{
			status = helloDSP_VerifyData(msg, sequenceNumber);
			if (DSP_FAILED(status))
			{
				MSGQ_free((MsgqMsg) msg);
			}
		}
#endif
		return status;
	}

	NORMAL_API void print_matrix(int *mat, int size)
	{
		int k, j;
		for (k = 0; k < size; k++)
		{
			printf("\n");
			for (j = 0; j < size; j++)
			{
				printf("\t%d ", mat[k * size + j]);
			}
		}
		printf("\n");
	}
	
	NORMAL_API int helloDSP_VerifyCalculations(void)
	{
		int l, j, k, success;
		
		startTimer(&totalTime); // START TIMER
				
		for (l = 0;l < MAT_SIZE; l++)
		{
			for (j = 0; j < MAT_SIZE; j++)
			{
				prod_ver[l][j]=0;
				for(k=0; k<MAT_SIZE;k++)
					prod_ver[l][j] = prod_ver[l][j]+mat1[l][k] * mat2[k][j];
			}
		}
		
		stopTimer(&totalTime);
		printf("\n Serial calculation time is: \n");
		printTimer(&totalTime);
		
		printf("Verification: \n");
		success = 1;
		for (k = 0; (k < MAT_SIZE) && success; k++)
		{
			for (j = 0; j < MAT_SIZE; j++)
			{
				if (prod_ver[k][j] != prod[k][j])
				{
				   success = 0;
				   break;
				}
			}
		}
		return success;
	}

    /** ============================================================================
     *  @func   helloDSP_Execute
     *
     *  @desc   This function implements the execute phase for this application.
     *
     *  @modif  None
     *  ============================================================================
     */
    NORMAL_API DSP_STATUS helloDSP_Execute(IN Uint32 numIterations, Uint8 processorId)
    {
        DSP_STATUS status = DSP_SOK;
        Uint16 sequenceNumber = 0;
        Uint16 msgId = 0;
        Uint32 i, j, k, l;
        ControlMsg *msg = NULL;
        

        SYSTEM_0Print("Entered helloDSP_Execute ()\n");

#if defined (PROFILE)
        SYSTEM_GetStartTime();
#endif

        for (i = 1 ; ((numIterations == 0) || (i <= (numIterations + 1))) && (DSP_SUCCEEDED (status)); i++)
        {
			status = helloDSP_Recieve(msg);

            if (msg->command == 0x01)
                SYSTEM_1Print("Message received: %s\n", (Uint32) msg->arg1);
            else if (msg->command == 0x02)
                SYSTEM_1Print("Message received: %s\n", (Uint32) msg->arg1);

            /* If the message received is the final one, free it. */
            
            ///////// Final means end of story ///////
            if ((numIterations != 0) && (i == (numIterations + 1)))
            {
				
				stopTimer(&totalTime);
				printf("\n Parallel calculation time includind communication overhead is: \n");
				printTimer(&totalTime);
				
				stopTimer(&dsp_only);
				printf("\n Parallel calculation time includind communication overhead is: \n");
				printTimer(&dsp_only);
				
				//stich the stuff together
				
				for (l = 0;l < SIZE; l++)   // <-- this is half of the product caluclations
				{
					for (j = 0; j < SIZE; j++)
					{		
						prod[l][j] = msg->mat1[l][j];
						prod[l][j+SIZE] = msg->mat2[l][j]; 				
					}
				}
				
			    ////// Verification  /////////////////////
				//do the multiplication locally to check with the returned values later (can be moved down when the matricies are generated)
				#define verification
				#ifdef verification	
				
				printf("%s\n", (helloDSP_VerifyCalculations() ? "Succes!" : "Failure!"));
				
				#endif
				
								
				//////////////// printing /////////////////////
				#ifdef DEBUG
				printf("\nMessage back from DSP: msg->mat1 \n");
				matrix_print(msg->mat1, SIZE);
					
				printf("Message back from DSP: msg->mat2 \n");
				matrix_print(msg->mat2, SIZE);
				
				printf("Calculated product: \n");
				matrix_print(prod, MAT_SIZE);
				
				printf("Correct product: \n");
				matrix_print(prod_ver, MAT_SIZE);
				#endif
				
				// adding matricies locally! for debug purposes!
				//#define mat_add
				#ifdef mat_add
				for (l = 0;l < MAT_SIZE; l++)
				{
					for (j = 0; j < MAT_SIZE; j++)
					{
							prod[l][j] = mat1[l][j] + mat2[l][j];
					}
				}
				
				success = 1;
				for (k = 0; (k < SIZE) && success; k++)
				{
					for (j = 0; j < SIZE; j++)
					{
						if (msg->mat1[k][j] != prod[k][j])
						{  
						   printf("Error \n");
						   success = 0;
						   break;
						}
						else if ((j==SIZE-1) && (k==SIZE-1))  
						   printf("\n Correct! \n\n");
					}
				}
				#endif
				
				
                MSGQ_free((MsgqMsg) msg);
            }
            else
            {
                /* Send the same message received in earlier MSGQ_get () call. */
                if (DSP_SUCCEEDED(status))
                {
					//Sending the matrices to the DSP
					if (i == 1) //Sending first quarter
					{
						msgId = MSGQ_getMsgId(msg);
						MSGQ_setMsgId(msg, msgId);
					
						startTimer(&totalTime); // START TIMER
						
						memcpy(msg->mat1, mat1, SIZE*SIZE*sizeof(int));
						memcpy(msg->mat2, mat2, SIZE*SIZE*sizeof(int));
						
						status = MSGQ_put(SampleDspMsgq, (MsgqMsg) msg);
						if (DSP_FAILED(status))
						{
							MSGQ_free((MsgqMsg) msg);
							SYSTEM_1Print("MSGQ_put () failed. Status = [0x%x]\n", status);
						}
					}
					else if (i == 2) //Sending second quarter
					{
						msgId = MSGQ_getMsgId(msg);
						MSGQ_setMsgId(msg, msgId);

						memcpy(msg->mat1, (&mat1[0][0] + SIZE*SIZE), SIZE*SIZE*sizeof(int));
						memcpy(msg->mat2, (&mat2[0][0] + SIZE*SIZE), SIZE*SIZE*sizeof(int));

						status = MSGQ_put(SampleDspMsgq, (MsgqMsg) msg);
						if (DSP_FAILED(status))
						{
							MSGQ_free((MsgqMsg) msg);
							SYSTEM_1Print("MSGQ_put () failed. Status = [0x%x]\n", status);
						}
					}	
				    else if (i == 3) //Sending third quarter
					{
						msgId = MSGQ_getMsgId(msg);
						MSGQ_setMsgId(msg, msgId);

						memcpy(msg->mat1, (&mat1[0][0] + 2*SIZE*SIZE), SIZE*SIZE*sizeof(int));
						memcpy(msg->mat2, (&mat2[0][0] + 2*SIZE*SIZE), SIZE*SIZE*sizeof(int));

						status = MSGQ_put(SampleDspMsgq, (MsgqMsg) msg);
						if (DSP_FAILED(status))
						{
							MSGQ_free((MsgqMsg) msg);
							SYSTEM_1Print("MSGQ_put () failed. Status = [0x%x]\n", status);
						}
					}	
					else if (i == 4) //Sending fourth quarter
					{
						msgId = MSGQ_getMsgId(msg);
						MSGQ_setMsgId(msg, msgId);

						memcpy(msg->mat1, (&mat1[0][0] + 3*SIZE*SIZE), SIZE*SIZE*sizeof(int));
						memcpy(msg->mat2, (&mat2[0][0] + 3*SIZE*SIZE), SIZE*SIZE*sizeof(int));

						status = MSGQ_put(SampleDspMsgq, (MsgqMsg) msg);
						if (DSP_FAILED(status))
						{
							MSGQ_free((MsgqMsg) msg);
							SYSTEM_1Print("MSGQ_put () failed. Status = [0x%x]\n", status);
						}
						
						startTimer(&dsp_only); // START TIMER for DSP
						
						/*
						 * We can start calculating the half here!
						 */
						 
						 
						//Do the multiplication here!
						for (l = SIZE;l < MAT_SIZE; l++)   // <-- this is half of the product caluclations
							{
								for (j = 0; j < MAT_SIZE; j++)
								{		
									prod[l][j]=0;
									for(k=0;k<MAT_SIZE;k++)
										prod[l][j] = prod[l][j]+mat1[l][k] * mat2[k][j];				
								}
							}
							
							

					}			 
                    
                }

                sequenceNumber++;
                /* Make sure that the sequenceNumber stays within the permitted
                 * range for applications. */
                if (sequenceNumber == MSGQ_INTERNALIDSSTART)
                {
                    sequenceNumber = 0;
                }

#if !defined (PROFILE)
                if (DSP_SUCCEEDED(status) && ((i % 100) == 0))
                {
                    SYSTEM_1Print("Transferred %ld messages\n", i);
                }
#endif
            }
        }

#if defined (PROFILE)
        if (DSP_SUCCEEDED(status))
        {
            SYSTEM_GetEndTime();
            SYSTEM_GetProfileInfo(numIterations);
        }
#endif

        SYSTEM_0Print("Leaving helloDSP_Execute ()\n");

        return status;
    }


    /** ============================================================================
     *  @func   helloDSP_Delete
     *
     *  @desc   This function releases resources allocated earlier by call to
     *          helloDSP_Create ().
     *          During cleanup, the allocated resources are being freed
     *          unconditionally. Actual applications may require stricter check
     *          against return values for robustness.
     *
     *  @modif  None
     *  ============================================================================
     */
    NORMAL_API Void helloDSP_Delete(Uint8 processorId)
    {
        DSP_STATUS status = DSP_SOK;
        DSP_STATUS tmpStatus = DSP_SOK;

        SYSTEM_0Print("Entered helloDSP_Delete ()\n");

        /* Release the remote message queue */
        status = MSGQ_release(SampleDspMsgq);
        if (DSP_FAILED(status))
        {
            SYSTEM_1Print("MSGQ_release () failed. Status = [0x%x]\n", status);
        }

        /* Close the remote transport */
        tmpStatus = MSGQ_transportClose(processorId);
        if (DSP_SUCCEEDED(status) && DSP_FAILED(tmpStatus))
        {
            status = tmpStatus;
            SYSTEM_1Print("MSGQ_transportClose () failed. Status = [0x%x]\n", status);
        }

        /* Stop execution on DSP. */
        tmpStatus = PROC_stop(processorId);
        if (DSP_SUCCEEDED(status) && DSP_FAILED(tmpStatus))
        {
            status = tmpStatus;
            SYSTEM_1Print("PROC_stop () failed. Status = [0x%x]\n", status);
        }

        /* Reset the error handler before deleting the MSGQ that receives */
        /* the error messages.                                            */
        tmpStatus = MSGQ_setErrorHandler(MSGQ_INVALIDMSGQ, MSGQ_INVALIDMSGQ);

        if (DSP_SUCCEEDED(status) && DSP_FAILED(tmpStatus))
        {
            status = tmpStatus;
            SYSTEM_1Print("MSGQ_setErrorHandler () failed. Status = [0x%x]\n", status);
        }

        /* Close the GPP's message queue */
        tmpStatus = MSGQ_close(SampleGppMsgq);
        if (DSP_SUCCEEDED(status) && DSP_FAILED(tmpStatus))
        {
            status = tmpStatus;
            SYSTEM_1Print("MSGQ_close () failed. Status = [0x%x]\n", status);
        }

        /* Close the pool */
        tmpStatus = POOL_close(POOL_makePoolId(processorId, SAMPLE_POOL_ID));
        if (DSP_SUCCEEDED(status) && DSP_FAILED(tmpStatus))
        {
            status = tmpStatus;
            SYSTEM_1Print("POOL_close () failed. Status = [0x%x]\n", status);
        }

        /* Detach from the processor */
        tmpStatus = PROC_detach(processorId);
        if (DSP_SUCCEEDED(status) && DSP_FAILED(tmpStatus))
        {
            status = tmpStatus;
            SYSTEM_1Print("PROC_detach () failed. Status = [0x%x]\n", status);
        }

        /* Destroy the PROC object. */
        tmpStatus = PROC_destroy();
        if (DSP_SUCCEEDED(status) && DSP_FAILED(tmpStatus))
        {
            status = tmpStatus;
            SYSTEM_1Print("PROC_destroy () failed. Status = [0x%x]\n", status);
        }

        SYSTEM_0Print("Leaving helloDSP_Delete ()\n");
    }


    /** ============================================================================
     *  @func   helloDSP_Main
     *
     *  @desc   Entry point for the application
     *
     *  @modif  None
     *  ============================================================================
     */
     
    NORMAL_API Void helloDSP_Main(IN Char8* dspExecutable, IN Char8* strNumIterations, IN Char8* strProcessorId)
    {
        DSP_STATUS status = DSP_SOK;
        Uint32 numIterations = 0;
        Uint8 processorId = 0;
        
        ///////////////////////////////////////////////////////////////
        //GENERATING MATRICES
		int i, j;
		for (i = 0; i < MAT_SIZE; i++)
		{
			for (j = 0; j < MAT_SIZE; j++)
			{
				#ifdef DEBUG
				mat1[i][j] = i*MAT_SIZE+j;
				#else
				mat1[i][j] = i+j*2;
				#endif
			}
		}
		for( i = 0; i < MAT_SIZE; i++ )
		{
			for (j = 0; j < MAT_SIZE; j++)
			{
				#ifdef DEBUG
				mat2[i][j] = i*MAT_SIZE+j + MAT_SIZE*MAT_SIZE;
				//mat2[i][j] = (i == j) ; //identity matrix
				#else
				mat2[i][j] = i+j*3;
				#endif
			}
		}
	

	  SYSTEM_0Print ("========== Initial matricies ==========\n");
		//printing last ten items for visual verification
		#ifdef DEBUG
		for (i = 0;i < MAT_SIZE; i++)
			{
				printf("\n");
				for (j = 0; j < MAT_SIZE; j++)
				{
					printf("%d ", mat1[i][j]);
				}
			}
		
		for (i = 0;i < MAT_SIZE; i++)
			{
				printf("\n");
				for (j = 0; j < MAT_SIZE; j++)
				{
					printf(" %d ", mat2[i][j]);
				}
			}
			printf("\n");
		#endif
		//////////////////////////////////////////////////

        SYSTEM_0Print ("========== Matrix Multiplication ==========\n");

        if ((dspExecutable != NULL) && (strNumIterations != NULL))
        {
            numIterations = SYSTEM_Atoi(strNumIterations);

            if (numIterations > 0xFFFF)
            {
                status = DSP_EINVALIDARG;
                SYSTEM_1Print("ERROR! Invalid arguments specified for helloDSP application.\n Max iterations = %d\n", 0xFFFF);
            }
            else
            {
                processorId = SYSTEM_Atoi(strProcessorId);

                if (processorId >= MAX_DSPS)
                {
                    SYSTEM_1Print("== Error: Invalid processor id %d specified ==\n", processorId);
                    status = DSP_EFAIL;
                }
                /* Specify the dsp executable file name for message creation phase. */
                if (DSP_SUCCEEDED(status))
                {
                    status = helloDSP_Create(dspExecutable, strNumIterations, processorId);

                    /* Execute the message execute phase. */
                    if (DSP_SUCCEEDED(status))
                    {
                        status = helloDSP_Execute(numIterations, processorId);
                    }

                    /* Perform cleanup operation. */
                    helloDSP_Delete(processorId);
                }
            }
        }
        else
        {
            status = DSP_EINVALIDARG;
            SYSTEM_0Print("ERROR! Invalid arguments specified for helloDSP application\n");
        }
        SYSTEM_0Print ("====================================================\n");
    }

#if defined (VERIFY_DATA)
    /** ============================================================================
     *  @func   helloDSP_VerifyData
     *
     *  @desc   This function verifies the data-integrity of given buffer.
     *
     *  @modif  None
     *  ============================================================================
     */
    STATIC NORMAL_API DSP_STATUS helloDSP_VerifyData(IN MSGQ_Msg msg, IN Uint16 sequenceNumber)
    {
        DSP_STATUS status = DSP_SOK;
        Uint16 msgId;

        /* Verify the message */
        msgId = MSGQ_getMsgId(msg.header);
        if (msgId != sequenceNumber)
        {
            status = DSP_EFAIL;
            SYSTEM_0Print("ERROR! Data integrity check failed\n");
        }

        return status;
    }
#endif /* if defined (VERIFY_DATA) */


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
