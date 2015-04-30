#include<stdio.h>
#include "Timer.h"

void matMult(int *mat1, int *mat2, int *prod, unsigned int size);

int main(int argc, char **argv)
{
	unsigned int size;
    Timer totalTime;
	int *mat1, *mat2, *prod;
	int i, j;
    initTimer(&totalTime, "Total Time");


	if (argc != 2)
	{
		printf("Usage: %s matrix_size\n", argv[0]);
		return 1;
	}

	if (sscanf(argv[1], "%u", &size) != 1)
	{
		printf("Parameter is not a valid integer\n");
		return 2;
	}
	mat1 = malloc(size * size * sizeof(int));
	mat2 = malloc(size * size * sizeof(int));
	prod = malloc(size * size * sizeof(int));

	
	for (i = 0;i < size; i++)
	{
		for (j = 0; j < size; j++)
		{
			mat1[size * i + j] = i+j*2;
		}
	}
	
	for(i = 0; i < size; i++)
	{
		for (j = 0; j < size; j++)
		{
			mat2[size * i + j] = i+j*3;
		}
	}

    startTimer(&totalTime);
	matMult(mat1,mat2,prod,size);
    stopTimer(&totalTime);
    printTimer(&totalTime);	
	/*
	for (i = 0;i < SIZE; i++)
	{
		printf("\n");
		for (j = 0; j < SIZE; j++)
		{
			printf("\t%d ", prod[size * i + j]);
		}
	}*/
	
	printf("\nDone !!! \n");
	return 0;
}

void matMult(int *mat1, int *mat2, int *prod, unsigned int size)
{
	int i, j, k;
	for (i = 0;i < size; i++)
	{
		for (j = 0; j < size; j++)
		{
			prod[size * i + j]=0;
			for(k=0;k<size;k++)
				prod[size * i + j] = prod[size * i + j]+mat1[size * i + k] * mat2[size * k + j];
		}
	}
}
