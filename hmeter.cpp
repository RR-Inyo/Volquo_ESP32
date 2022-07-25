/*
 * hmeter.cpp
 * Horizontal meter
 * (c) 2022 @RR_Inyo
 * Released under the MIT lisence
 * https://opensource.org/licenses/mit-license.php
 */

#define LGFX_USE_V1
#include "LGFX_ESP32_ST7789.hpp"
#include "hmeter.hpp"

extern LGFX lcd;

// Constructor
hMeter::hMeter(int _x0, int _y0, float _u_min, float _u_max) {
  // Initializing instant variables
  x0 = _x0;
  y0 = _y0;
  x_old = 0;
  u_min = _u_min;
  u_max = _u_max;
}

// Destructor
hMeter::~hMeter() {}

// Draw frame
void hMeter::drawFrame() {
  
  // Draw horizontal line
  lcd.drawFastHLine(x0, y0, width, colFrame);

  // Draw large ticks
  lcd.drawFastVLine(x0, y0 - height / 2, height + 1, colFrame);
  lcd.drawFastVLine(x0 + width / 2, y0 - height / 2, height + 1, colFrame);
  lcd.drawFastVLine(x0 + width, y0 - height / 2, height + 1, colFrame);

  // Draw medium ticks
  lcd.drawFastVLine(x0 + width / 4, y0 - height / 4, height / 2 + 1, colFrame);
  lcd.drawFastVLine(x0 + 3 * width / 4, y0 - height / 4, height / 2 + 1, colFrame);

  // Draw small ticks
  for (int i = 0; i < 20; i++) {
    lcd.drawFastVLine(x0 + i * width / 20, y0 - height / 8, height / 4 + 1, colFrame);
  }  
}

// Print labels
void hMeter::drawLabels() {  
  lcd.setFont(&fonts::Font2);
  lcd.setTextColor(colLabel);
  lcd.setTextDatum(top_center);
  lcd.drawNumber(static_cast<int>(u_min), x0, y0 + height / 2 + 2);
  lcd.drawNumber(static_cast<int>((u_max + u_min) / 2), x0 + width / 2, y0 + height / 2 + 2);
  lcd.drawNumber(static_cast<int>(u_max), x0 + width, y0 + height / 2 + 2);
}

// Update and draw hand (needle)
void hMeter::update(float u) {
  lcd.drawFastVLine(x0 + x_old, y0 - height / 2, height, colBG);
  drawFrame();
  int x = constrain(static_cast<int>((u - u_min) / (u_max - u_min) * width), 0, width);
  lcd.drawFastVLine(x0 + x, y0 - height / 2, height, colHand);
  x_old = x;
}
