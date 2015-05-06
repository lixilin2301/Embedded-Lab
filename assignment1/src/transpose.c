#define SIZE 128

void transpose(int dest[SIZE][SIZE], int src[SIZE][SIZE]) {
	unsigned int i, j;
	for (i = 0; i < SIZE; ++i) {
		for (j = 0; j < SIZE; ++j) {
			dest[j][i] = src[i][j];
		}
	}
}