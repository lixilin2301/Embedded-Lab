#include "matMult.h"

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
