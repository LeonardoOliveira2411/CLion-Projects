#ifdef MATLAB_MEX_FILE
#include "mex.h"
#include "5G_DATASOURCE_functions.h"

// Función para generar bits aleatorios
void mexGenerateRandomBits(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
    if (nrhs != 1 || !mxIsScalar(prhs[0])) {
        mexErrMsgIdAndTxt("5G_DATASOURCE:inputError",
            "generate_random_bits requiere 1 escalar (numbits).");
    }
    int numbits = mxGetScalar(prhs[0]);
    plhs[0] = mxCreateNumericMatrix(1, numbits, mxINT32_CLASS, mxREAL);
    int *bits = (int *)mxGetData(plhs[0]);
    generate_random_bits(bits, numbits);
}

// Función para calcular BER
void mexCalculateBER(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
    if (nrhs != 2 || !mxIsInt32(prhs[0]) || !mxIsInt32(prhs[1]) ||
        mxGetNumberOfElements(prhs[0]) != mxGetNumberOfElements(prhs[1])) {
        mexErrMsgIdAndTxt("5G_DATASOURCE:inputError",
            "calculate_ber requiere 2 vectores int32 del mismo tamaño.");
    }
    int *tx_bits = (int *)mxGetData(prhs[0]);
    int *rx_bits = (int *)mxGetData(prhs[1]);
    int numbits = mxGetNumberOfElements(prhs[0]);
    float ber = calculate_ber(tx_bits, rx_bits, numbits);
    plhs[0] = mxCreateDoubleScalar((double)ber);
}

// Punto de entrada MEX
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
    // Obtener el nombre de la función llamada (alternativa sin mxGetFunctionName)
    const char *function_name = (nrhs > 0 && mxIsChar(prhs[0])) ? mxArrayToString(prhs[0]) : NULL;

    if (function_name == NULL) {
        mexErrMsgIdAndTxt("5G_DATASOURCE:functionError",
            "Primer argumento debe ser el nombre de la función ('generate_random_bits' o 'calculate_ber').");
    }

    // Redirigir a la función correspondiente
    if (strcmp(function_name, "generate_random_bits") == 0) {
        mexGenerateRandomBits(nlhs, plhs, nrhs - 1, prhs + 1); // Ignorar el primer argumento (nombre)
    }
    else if (strcmp(function_name, "calculate_ber") == 0) {
        mexCalculateBER(nlhs, plhs, nrhs - 1, prhs + 1); // Ignorar el primer argumento (nombre)
    }
    else {
        mexErrMsgIdAndTxt("5G_DATASOURCE:functionError",
            "Función no reconocida. Use 'generate_random_bits' o 'calculate_ber'.");
    }

    if (function_name != NULL) {
        mxFree((void *)function_name);
    }
}
#endif