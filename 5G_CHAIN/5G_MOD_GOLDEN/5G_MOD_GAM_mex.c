#include "mex.h"
#include "5G_MOD_GAM_functions.h"
#include "5G_GAM_MAPPING.h"
#include <complex.h>

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) 
{
    // Check for proper number of arguments
    if (nrhs < 1) {
        mexErrMsgIdAndTxt("GAM:nrhs", "At least one input required (function name).");
    }
    
    // Get the function name
    if (!mxIsChar(prhs[0])) {
        mexErrMsgIdAndTxt("GAM:notString", "First input must be a string (function name).");
    }
    char *func_name = mxArrayToString(prhs[0]);
    
    // Dispatch to the appropriate function
    if (strcmp(func_name, "modulate_GAM") == 0) {
        // modulate_GAM(int *bits, float complex *symbols, int numbits, int BPS)
        if (nrhs != 3) {
            mexErrMsgIdAndTxt("GAM:modulate", "modulate_GAM requires 2 inputs: bits array and BPS.");
        }
        
        // Get input bits
        if (!mxIsInt32(prhs[1]) && !mxIsDouble(prhs[1])) {
            mexErrMsgIdAndTxt("GAM:invalidBits", "Bits array must be int32 or double.");
        }
        double *bits_in = mxGetPr(prhs[1]);
        int numbits = mxGetNumberOfElements(prhs[1]);
        
        // Get BPS
        int BPS = (int)mxGetScalar(prhs[2]);
        
        // Prepare output symbols array
        int numsymbols = numbits / BPS;
        if (numbits % BPS != 0) {
            mexErrMsgIdAndTxt("GAM:invalidBitLength", "Number of bits must be multiple of BPS.");
        }
        plhs[0] = mxCreateDoubleMatrix(numsymbols, 1, mxCOMPLEX);
        double *symbols_real = mxGetPr(plhs[0]);
        double *symbols_imag = mxGetPi(plhs[0]);
        
        // Convert bits to int array
        int *bits = (int *)mxMalloc(numbits * sizeof(int));
        for (int i = 0; i < numbits; i++) {
            bits[i] = (int)bits_in[i];
        }
        
        // Create complex array for modulation
        float complex *symbols = (float complex *)mxMalloc(numsymbols * sizeof(float complex));
        
        // Call modulation function
        int result = modulate_GAM(bits, symbols, numbits, BPS);
        
        // Convert complex symbols to MATLAB format
        for (int i = 0; i < numsymbols; i++) {
            symbols_real[i] = (double)creal(symbols[i]);
            symbols_imag[i] = (double)cimag(symbols[i]);
        }
        
        // Free memory
        mxFree(bits);
        mxFree(symbols);
        
    } else if (strcmp(func_name, "demodulate_GAM") == 0) {
        // demodulate_GAM(float complex *symbols, int *bits, int numsymb, int BPS)
        if (nrhs != 3) {
            mexErrMsgIdAndTxt("GAM:demodulate", "demodulate_GAM requires 2 inputs: symbols array and BPS.");
        }
        
        // Get input symbols
        if (!mxIsComplex(prhs[1])) {
            mexErrMsgIdAndTxt("GAM:invalidSymbols", "Symbols array must be complex.");
        }
        double *symbols_real = mxGetPr(prhs[1]);
        double *symbols_imag = mxGetPi(prhs[1]);
        int numsymbols = mxGetNumberOfElements(prhs[1]);
        
        // Get BPS
        int BPS = (int)mxGetScalar(prhs[2]);
        
        // Prepare output bits array
        int numbits = numsymbols * BPS;
        plhs[0] = mxCreateDoubleMatrix(numbits, 1, mxREAL);
        double *bits_out = mxGetPr(plhs[0]);
        
        // Create complex array for demodulation
        float complex *symbols = (float complex *)mxMalloc(numsymbols * sizeof(float complex));
        for (int i = 0; i < numsymbols; i++) {
            symbols[i] = symbols_real[i] + I * symbols_imag[i];
        }
        
        // Create bits array
        int *bits = (int *)mxMalloc(numbits * sizeof(int));
        
        // Call demodulation function
        int result = demodulate_GAM(symbols, bits, numsymbols, BPS);
        
        // Convert bits to MATLAB format
        for (int i = 0; i < numbits; i++) {
            bits_out[i] = (double)bits[i];
        }
        
        // Free memory
        mxFree(symbols);
        mxFree(bits);
        
    } else if (strcmp(func_name, "normGAM") == 0) {
        // normGAM(float complex *inout, int length)
        if (nrhs != 2) {
            mexErrMsgIdAndTxt("GAM:normalize", "normGAM requires 1 input: symbols array.");
        }
        
        // Get input symbols
        if (!mxIsComplex(prhs[1])) {
            mexErrMsgIdAndTxt("GAM:invalidSymbols", "Symbols array must be complex.");
        }
        double *symbols_real = mxGetPr(prhs[1]);
        double *symbols_imag = mxGetPi(prhs[1]);
        int numsymbols = mxGetNumberOfElements(prhs[1]);
        
        // Create complex array for normalization
        float complex *symbols = (float complex *)mxMalloc(numsymbols * sizeof(float complex));
        for (int i = 0; i < numsymbols; i++) {
            symbols[i] = symbols_real[i] + I * symbols_imag[i];
        }
        
        // Call normalization function
        int result = normGAM(symbols, numsymbols);
        
        // Prepare output symbols array (same as input)
        plhs[0] = mxCreateDoubleMatrix(numsymbols, 1, mxCOMPLEX);
        double *out_real = mxGetPr(plhs[0]);
        double *out_imag = mxGetPi(plhs[0]);
        
        // Convert complex symbols to MATLAB format
        for (int i = 0; i < numsymbols; i++) {
            out_real[i] = (double)creal(symbols[i]);
            out_imag[i] = (double)cimag(symbols[i]);
        }
        
        // Free memory
        mxFree(symbols);
        
    } else {
        mexErrMsgIdAndTxt("GAM:unknownFunction", "Unknown function name. Valid options are: 'modulate_GAM', 'demodulate_GAM', 'normGAM'");
    }
    
    mxFree(func_name);
}