/*
 * spectrum.cpp
 * Frequency-domain plot
 * (c) 2022 @RR_Inyo
 * Released under the MIT lisence
 * https://opensource.org/licenses/mit-license.php
 */

#define LGFX_USE_V1
#include "LGFX_ESP32_ST7789.hpp"
#include "spectrum.hpp"

extern LGFX lcd;

// Constructor
spectrumPlot::spectrumPlot(int _x0, int _y0, float _u_max) {
  // Initializing instant variables
  x0 = _x0;
  y0 = _y0;
  u_max = _u_max;
}

// Destructor
spectrumPlot::~spectrumPlot() {}

// Draw frame
void spectrumPlot::drawFrame() {
  lcd.drawRect(x0 - 1, y0 - 1, width + 2, height + 2, colFrame);
  lcd.drawFastHLine(x0, y0 + height / 2 + 1, width, colFrame);
}

// Draw labels
void spectrumPlot::drawLabels() {
  // Vertical labels
  lcd.setFont(&fonts::Font2);
  lcd.setTextColor(colLabel);
  lcd.setTextDatum(middle_right);
  lcd.drawNumber(static_cast<int>(u_max), x0 - 2, y0);
  lcd.drawNumber(static_cast<int>(u_max / 2), x0 - 2, y0 + height / 2);
  lcd.drawNumber(0, x0 - 2, y0 + height);

  // Horizontal labels
  lcd.setFont(&fonts::Font0);
  lcd.setTextColor(colLabel);
  lcd.setTextDatum(top_center);
  for (int i = 1; i < N / 2; i += 4) {
    lcd.drawNumber(i, x0 + i * 4 + 1, y0 + height + 2);  // Add one for fine tune
  }
}

// Update spectrum
void spectrumPlot::update(float *u) {
  // Clear old waveform and draw zero line, preserve the medium line
  lcd.fillRect(x0, y0, width, height / 2 + 1, colBG);
  lcd.fillRect(x0, y0 + height / 2 + 2, width, height / 2 - 1, colBG);
  lcd.drawFastHLine(x0, y0 + height / 2 + 1, width, colFrame);

  // Plot spectrum
  for (int i = 1; i < N / 2; i++) {
    int y = constrain(static_cast<int>(u[i] / u_max * height), 0, height);
    lcd.drawFastVLine(x0 + i * 4, y0 + height - y, y, colPlot);
    lcd.drawFastVLine(x0 + i * 4 + 1, y0 + height - y, y, colPlot);
  }
}
