/*
 * hmeter.hpp
 * Horizontal meter
 * (c) 2022 @RR_Inyo
 * Released under the MIT lisence
 * https://opensource.org/licenses/mit-license.php
 */

#ifndef _HMETER_
#define _HMETER_

class hMeter {
  public:
    hMeter(int _x0, int _y0, float _u_min, float _u_max);
    ~hMeter();
    void drawFrame();
    void drawLabels(); 
    void update(float u);

  private:
    const int width = 120;
    const int height = 24;
    const int colBG = TFT_BLACK;
    const int colFrame = TFT_DARKGREY;
    const int colLabel = TFT_WHITE;
    const int colHand = TFT_RED;
    int x0, y0, x_old;
    float u_min, u_max;
};

#endif
