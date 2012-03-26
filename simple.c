#include <stdio.h>
#include "mex.h"

void mexFunction(int n_inputs, mxArray* inputs[],
                 int n_outputs, const mxArray* outputs[])
{
  fprintf(stderr, "inputs: %d\noutputs: %d\n", n_inputs, n_outputs);
  fprintf(stderr, "input arrays: %p\noutput arrays: %p\n", inputs, outputs);
}
