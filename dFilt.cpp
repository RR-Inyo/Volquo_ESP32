/*
  dFilt.cpp
  My digital filter, including HPFs
  (c) 2022 @RR_Inyo
  Released under the MIT lisence
  https://opensource.org/licenses/mit-license.php
*/

#include <math.h>
#include "dFilt.hpp"

/*
  First-order low-pass filter
*/

// Constructor
FirstOrderLPF::FirstOrderLPF(float Ts, float omega_n) {
  // Set coefficients based on Bilinear transform (Tustin's method)
  a1 = (2 - omega_n * Ts) / (2 + omega_n * Ts);
  b0 = omega_n * Ts / (omega_n * Ts + 2);
  b1 = b0;

  // Initialize old values
  y_old = 0;
  u_old = 0;
}

// Destructor
FirstOrderLPF::~FirstOrderLPF(void) {}

// Apply digital filter
float FirstOrderLPF::apply(float u) {
  float y = b0 * u + b1 * u_old + a1 * y_old;
  y_old = y;
  u_old = u;
  return y;
}

/*
  First-order high-pass filter
*/

// Constructor
FirstOrderHPF::FirstOrderHPF(float Ts, float omega_n) {
  // Set coefficients based on Bilinear transform (Tustin's method)
  a1 = (2 - omega_n * Ts) / (2 + omega_n * Ts);
  b0 = -omega_n * Ts / (omega_n * Ts + 2);
  b1 = -b0;

  // Initialize old values
  y_old = 0;
  u_old = 0;
}

// Destructor
FirstOrderHPF::~FirstOrderHPF(void) {}

// Apply digital filter
float FirstOrderHPF::apply(float u) {
  float y = b0 * u + b1 * u_old + a1 * y_old;
  y_old = y;
  u_old = u;
  return y;
}

/*
  Second-order low-pass filter
*/

// Constructor
SecondOrderLPF::SecondOrderLPF(float Ts, float zeta, float omega_n) {
  // Set coefficients based on Bilinear transform (Tustin's method)
  m = 4.0 * zeta / (omega_n * Ts);
  n = 4.0 / pow(omega_n * Ts, 2.0);

  a1 = 2.0 * (1.0 - n) / (1.0 + m + n);
  a2 = (1.0 - m + n) / (1.0 + m + n);
  b0 = 1.0 / (1.0 + m + n);
  b1 = 2.0 / (1.0 + m + n);
  b2 = b0;

  // Initialize old values
  y_old[0] = y_old[1] = 0;
  u_old[0] = u_old[1] = 0;
}

// Destructor
SecondOrderLPF::~SecondOrderLPF(void) {}

// Apply digital filter
float SecondOrderLPF::apply(float u) {
  float y = b0 * u + b1 * u_old[0] + b2 * u_old[1] - a1 * y_old[0] - a2 * y_old[1];
  y_old[1] = y_old[0];
  y_old[0] = y;
  u_old[1] = u_old[0];
  u_old[0] = u;
  return y;
}

/*
  Second-order high-pass filter
*/

// Constructor
SecondOrderHPF::SecondOrderHPF(float Ts, float zeta, float omega_n) {
  // Set coefficients based on Bilinear transform (Tustin's method)
  m = 4.0 * zeta / (omega_n * Ts);
  n = 4.0 / pow(omega_n * Ts, 2.0);

  a1 = 2.0 * (1.0 - n) / (1.0 + m + n);
  a2 = (1.0 - m + n) / (1.0 + m + n);
  b0 = n / (1.0 + m + n);
  b1 = -2.0 * n / (1.0 + m + n);
  b2 = b0;

  // Initialize old values
  y_old[0] = y_old[1] = 0;
  u_old[0] = u_old[1] = 0;
}

// Destructor
SecondOrderHPF::~SecondOrderHPF(void) {}

// Apply digital filter
float SecondOrderHPF::apply(float u) {
  float y = b0 * u + b1 * u_old[0] + b2 * u_old[1] - a1 * y_old[0] - a2 * y_old[1];
  y_old[1] = y_old[0];
  y_old[0] = y;
  u_old[1] = u_old[0];
  u_old[0] = u;
  return y;
}
