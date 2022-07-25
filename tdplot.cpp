/*
 * tdplot.cpp
 * Time-domain plot
 * (c) 2022 @RR_Inyo
 * Released under the MIT lisence
 * https://opensource.org/licenses/mit-license.php
 */

#define LGFX_USE_V1
#include "LGFX_ESP32_ST7789.hpp"
#include "tdplot.hpp"

extern LGFX lcd;

// Constructor
tdPlot::tdPlot(int _x0, int _y0, float _u_min, float _u_max) {
  // Initializing instant variables
  x0 = _x0;
  y0 = _y0;
  u_min = _u_min;
  u_max = _u_max;
}

// Destructor
tdPlot::~tdPlot() {}

// Draw frame
void tdPlot::drawFrame() {
  lcd.drawRect(x0 - 1, y0 - 1, width + 2, height + 2, colFrame);
  lcd.drawFastHLine(x0, y0 + height / 2 + 1, width, colFrame);
}

// Draw labels
void tdPlot::drawLabels() {
  lcd.setFont(&fonts::Font2);
  lcd.setTextColor(colLabel);
  lcd.setTextDatum(middle_right);
  lcd.drawNumber(static_cast<int>(u_max), x0 - 2, y0);
  lcd.drawNumber(static_cast<int>((u_max + u_min) / 2), x0 - 2, y0 + height / 2);
  lcd.drawNumber(static_cast<int>(u_min), x0 - 2, y0 + height);
}

// Update waveform
void tdPlot::update(float *u) {
  // Clear old waveform and draw zero line, preserve the medium line
  lcd.fillRect(x0, y0, width, height / 2 + 1, colBG);
  lcd.fillRect(x0, y0 + height / 2 + 2, width, height / 2 - 1, colBG);
  lcd.drawFastHLine(x0, y0 + height / 2 + 1, width, colFrame);

  // Plot first point
  int y = constrain(static_cast<int>((u[0] - u_min) / (u_max - u_min) * height), 1, height - 1);
  lcd.drawPixel(x0, y0 + height - y, colPlot);
  int y_old = y;

  // Plot rest of points;
  for (int i = 1; i < N; i++) {
    int y = constrain(static_cast<int>((u[i] - u_min) / (u_max - u_min) * height), 1, height - 1);
    lcd.drawLine(x0 + (i - 1) * 2, y0 + height - y_old, x0 + i * 2, y0 + height - y, colPlot);
    y_old = y;
  }

  // Final point
  y = constrain(static_cast<int>((u[0] - u_min) / (u_max - u_min) * height), 1, height - 1);
  lcd.drawLine(x0 + width - 2, y0 + height - y_old, x0 + width, y0 + height - y, colPlot);
}
