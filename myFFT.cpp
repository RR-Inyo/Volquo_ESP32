/*
   myFFT.cpp - Fast Fourier Transform (FFT) library implementation for practice
   (c) 2021 @RR_Inyo
   Released under the MIT license
   https://opensource.org/licenses/mit-license.php
*/

#include "myFFT.hpp"

// Prototype of internal-use functions
static void _FFT_2_butterfly(Complex *f, int N);

// Discrete Fourier Transform (DFT), straight-forward
// Ported from my implementation to Arduino IDE
void DFT(Complex *f, int N) {
  // Declare complex array to store output
  Complex F[N];

  // Direct calculation of Fourier coefficients
  for (int k = 0; k < N; k++) {
    for (int i = 0; i < N; i++) {
      // Prepare complex sinusoid, rotation operator
      Complex W;
      W.polar(1.0, -2 * M_PI * k * i / N);

      // Perform multiply-accumulate operation
      F[k] += W * f[i];
    }
  }

  // Overwrite input with output
  for (int i = 0; i < N; i++) {
    f[i] = F[i];
  }
}

// Fast Fourier Transform (FFT), recursive, simple permutation
// Ported from Interface Magazine, CQ Publishing, Sep. 2021, p. 53, List 1
// Through my porting to Python, and also ported from Arduino IDE
void FFT_1(Complex *f, int N) {
  if (N > 1) {
    int n = N / 2;
    for (int i = 0; i < n; i++) {
      // Prepare complex sinusoid, rotation operator
      Complex W;
      W.polar(1.0, -2 * M_PI * i / N);

      // Butterfly computation
      Complex f_tmp = f[i] - f[n + i];
      f[i] += f[n + i];
      f[n + i] = W * f_tmp;
    }

    // Recursively call this function itself
    FFT_1(&f[0], n);  // First half
    FFT_1(&f[n], n);  // Second half

    // Simple permutaton
    Complex F[N];
    for (int i = 0; i < n; i++) {
      F[2 * i] = f[i];
      F[2 * i + 1] = f[n + i];
    }
    for (int i = 0; i < N; i++) {
      f[i] = F[i];
    }
  }
}

// Fast Fourier Transform (FFT), recursive, bit-reverse
// Ported from Interface Magazine, CQ Publishing, Sep. 2021, p. 53, List 2
// Through my porting to Python, and also ported from Arduino IDE
void FFT_2(Complex *f, int N) {
  // Perform butterfly computations
  _FFT_2_butterfly(f, N);

  // Bit-reversal permutation
  int i = 0;
  for (int j = 1; j < N - 1; j++) {
    for (int k = N >> 1; k > (i ^= k); k >>= 1);
    if (j < i) {
      Complex f_tmp = f[j];
      f[j] = f[i];
      f[i] = f_tmp;
    }
  }
}

static void _FFT_2_butterfly(Complex *f, int N) {
  if (N > 1) {
    int n = N / 2;
    for (int i = 0; i < n; i++) {
      // Prepare complex sinusoid, rotation operator
      Complex W;
      W.polar(1.0, -2 * M_PI * i / N);

      // Butterfly computation
      Complex f_tmp = f[i] - f[n + i];
      f[i] += f[n + i];
      f[n + i] = W * f_tmp;
    }

    // Recursively call this function itself
    _FFT_2_butterfly(&f[0], n);  // First half
    _FFT_2_butterfly(&f[n], n);  // Second half
  }
}

// Fast Fourier Transform (FFT), non-recursive, bit-reverse reordering
// Ported from my code in Python
void FFT_3(Complex *f, int N_orig) {
  // Calculate number of data divisions
  int m = int(log2(N_orig));

  // FFT core calculation
  for (int s = 0; s < m; s++) {
    int div = int(pow(2, s));  // Number of divided deta segments
    int N = N_orig / div;  // Number of data points in each data segment
    int n = N / 2;

    // Butterfly computations of each segment
    for (int j = 0; j < int(pow(2, s)); j++) {
      for (int i = 0; i < n; i++) {
        // Prepare complex sinusoid, rotation operator
        Complex W;
        W.polar(1.0, -2 * M_PI * i / N);

        // Butterfly computation
        Complex f_tmp = f[N * j + i] - f[N * j + n + i];
        f[N * j + i] += f[N * j + n + i];
        f[N * j + n + i] = W * f_tmp;
      }
    }
  }

  // Bit-reversal permutation
  int i = 0;
  for (int j = 1; j < N_orig - 1; j++) {
    for (int k = N_orig >> 1; k > (i ^= k); k >>= 1);
    if (j < i) {
      Complex f_tmp = f[j];
      f[j] = f[i];
      f[i] = f_tmp;
    }
  }
}
