/*
 * spectrum.hpp
 * Frequency-domain plot
 * (c) 2022 @RR_Inyo
 * Released under the MIT lisence
 * https://opensource.org/licenses/mit-license.php
 */

#ifndef _SPECTRUM_
#define _SPECTRUM_

class spectrumPlot {
  public:
    spectrumPlot(int _x0, int _y0, float _u_max);
    ~spectrumPlot();
    void drawFrame();
    void drawLabels();
    void update(float *u);

  private:
    const int N = 64;
    const int width = 128;
    const int height = 45;
    const int colBG = TFT_BLACK;
    const int colFrame = TFT_DARKGREY;
    const int colLabel = TFT_WHITE;
    const int colPlot = TFT_SKYBLUE;
    int x0, y0;
    float u_min, u_max;
    float u_old;
};

#endif
