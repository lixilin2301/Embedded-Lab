#include "convert.h"
#include <stdlib.h>
#include <stdio.h>

int main () {
	int inputs[8] = {213124, 42, 128, 32001};
	int outputs[8] = {-1, -1, -1, -1};
	int i;
	char buf[256];	// Will be our message buffer

	char *pin = buf;	// Input pointer
	char *pout = buf;	// Output pointer
	pin = conv_ints_bytes(inputs, pin, 4);		// Convert to the message buffer
	pout = conv_bytes_ints(pout, outputs, 4);	// Read from the message buffer

	// Test equality
	int succes = 1;
	for (i = 0; i < 4; ++i) {
		if (inputs[i] != outputs[i]) {
			succes = 0;
		}
	}
	printf("%s\n", succes ? "Succes" : "Fail");
}


