#ifdef MATLAB_MEX_FILE
#include "mex.h"
#include "5G_CHANNEL_DIST_NOISE_functions.h"
#include <string.h>
#include <time.h>

// Función para añadir ruido AWGN
void mexAddAWGNNoise(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
    // Verificación de argumentos
    if (nrhs != 3) {
        mexErrMsgIdAndTxt("CHANNEL:inputError",
            "Se requieren 3 argumentos: symbols, snr_db, avg_power");
    }

    // Validación de tipos de entrada
    if (!mxIsComplex(prhs[0]) {
        mexErrMsgIdAndTxt("CHANNEL:inputError",
            "El primer argumento debe ser un arreglo complejo");
    }
    if (!mxIsScalar(prhs[1]) {
        mexErrMsgIdAndTxt("CHANNEL:inputError",
            "snr_db debe ser escalar");
    }
    if (!mxIsScalar(prhs[2])) {
        mexErrMsgIdAndTxt("CHANNEL:inputError",
            "avg_power debe ser escalar");
    }

    // Obtener parámetros de entrada
    int numsymb = mxGetNumberOfElements(prhs[0]);
    float complex *symbols = (float complex *)mxMalloc(numsymb * sizeof(float complex));

    // Copiar datos de MATLAB
    double *real_part = mxGetPr(prhs[0]);
    double *imag_part = mxGetPi(prhs[0]);
    for (int i = 0; i < numsymb; i++) {
        symbols[i] = real_part[i] + (imag_part ? imag_part[i] : 0) * I;
    }

    float snr_db = (float)mxGetScalar(prhs[1]);
    float avg_power = (float)mxGetScalar(prhs[2]);

    // Inicializar semilla aleatoria
    srand((unsigned int)time(NULL));

    // Aplicar ruido AWGN
    add_awgn_noise(symbols, numsymb, snr_db, avg_power);

    // Preparar salida
    plhs[0] = mxCreateDoubleMatrix(mxGetM(prhs[0]), mxGetN(prhs[0]), mxCOMPLEX);
    double *real_out = mxGetPr(plhs[0]);
    double *imag_out = mxGetPi(plhs[0]);

    for (int i = 0; i < numsymb; i++) {
        real_out[i] = creal(symbols[i]);
        imag_out[i] = cimag(symbols[i]);
    }

    mxFree(symbols);
}

// Función para calcular potencia promedio
void mexCalcAvgPower(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
    if (nrhs != 1 || !mxIsComplex(prhs[0])) {
        mexErrMsgIdAndTxt("CHANNEL:inputError",
            "calc_avg_power requiere 1 vector complejo (symbols).");
    }

    int numsymb = mxGetNumberOfElements(prhs[0]);
    float complex *symbols = (float complex *)mxMalloc(numsymb * sizeof(float complex));

    // Copiar datos de MATLAB
    double *real_part = mxGetPr(prhs[0]);
    double *imag_part = mxGetPi(prhs[0]);
    for (int i = 0; i < numsymb; i++) {
        symbols[i] = real_part[i] + (imag_part ? imag_part[i] : 0) * I;
    }

    // Calcular potencia promedio
    float avg_power = calc_avg_power(symbols, numsymb);
    plhs[0] = mxCreateDoubleScalar((double)avg_power);

    mxFree(symbols);
}

// Punto de entrada MEX principal
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
    if (nrhs < 1 || !mxIsChar(prhs[0])) {
        mexErrMsgIdAndTxt("CHANNEL:functionError",
            "Primer argumento debe ser el nombre de la función.");
    }

    char *function_name = mxArrayToString(prhs[0]);

    if (strcmp(function_name, "add_awgn_noise") == 0) {
        mexAddAWGNNoise(nlhs, plhs, nrhs - 1, prhs + 1);
    }
    else if (strcmp(function_name, "calc_avg_power") == 0) {
        mexCalcAvgPower(nlhs, plhs, nrhs - 1, prhs + 1);
    }
    else {
        mexErrMsgIdAndTxt("5G_CHANNEL:functionError",
            "Función no reconocida. Opciones válidas:\n"
            "'add_awgn_noise', 'calc_avg_power'");
    }

    mxFree(function_name);
}
#endif