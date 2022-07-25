/*
   myFFT.hpp - Fast Fourier Transform (FFT)
   Based on Interface Magazine, CQ Publishing, Sep. 2021, p. 53, List 1
   Ported to C++ using Complex class
   (c) 2021 @RR_Inyo
   Released under the MIT license
   https://opensource.org/licenses/mit-license.php
*/

#ifndef _MYFFT_
#define _MYFFT_

#include <math.h>
#include <CComplex.h>

// DFT function prototype
void DFT(Complex *f, int N);

// FFT function prototypes
void FFT_1(Complex *f, int N);
void FFT_2(Complex *f, int N);
void FFT_3(Complex *f, int N_orig);

#endif
