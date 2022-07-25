/*
   Volquo_ESP32.ino
   Volquo, a voltage quality monitor, for ESP32
   (c) 2022 @RR_Inyo
   Released under the MIT lisence
   https://opensource.org/licenses/mit-license.php
*/

#define LGFX_USE_V1
#include "LGFX_ESP32_ST7789.hpp"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <CComplex.h>
#include "dfilt.hpp"
#include "hmeter.hpp"
#include "tdplot.hpp"
#include "spectrum.hpp"
#include "myFFT.hpp"

//----------------------------------------------------------------------
// Global variables and objects
//----------------------------------------------------------------------

// Lovyan GFX
LGFX lcd;

// WiFi settings
const int N_SSID = 2;
const char* ssid[N_SSID] = {"Put_your_SSID_1_here", "Put_your_SSID_2_here"};
const char* password[N_SSID] = {"Put_your_password_1_here", "Put_your_password_2_here"};
uint32_t T_WIFI_TIMEOUT_MS = 10000;
bool online = false;

// Declare constants and variables for NTP servers, date-time structure, and string
const char* server1 = "ntp.nict.jp";
const char* server2 = "time.google.com";
const char* server3 = "ntp.jst.mfeed.ad.jp";
const long JST = 3600L * 9;
const int summertime = 0;
struct tm tm;
int tm_sec_old = 0;
char datebuf[32];

// Define constants and variables for Google Spreadsheet API constants and variables
const char* apiURL = "Put_your_Google_Spreadsheet_Web_App_URL_here";
const int tPost = 10;
int httpCode;
HTTPClient http;
bool postingNow = false;

// Define constants and variables for A-D conversion, sampling, and digital filters
const int N_CYCLE = 8;                    // Number of cycles to record on buffer
const int N_SAMPLE = 64 * N_CYCLE;        // Size of buffer
const int N_PLOT = 64;                    // Size of buffer to undergo plotting and FFT
const int N_ZCW = 8;                      // Size of zero-cross detection window
const int GPIO_AD = 35;                   // ESP32 GPIO to connect mains voltage signal
const float AD_VCC = 3.3;                 // A-D converter VCC
const float AD_FULLSCALE = 4096.0;        // A-D converter fullscale
const float TURN_RATIO = 100.0 / 7.931;   // Turn ratio of the transformer
const float V_DIV = 120.0 / 1920.0;       // Voltage divider ratio
const float T_SAMPLE_US = 312.5;          // Sampling frequency
const float ZETA = 0.707;                 // Damping factor of both HPF and LPF
const float OMEGA_N_HPF = 2 * PI * 1;     // Natural angular frequency of HPF
const float OMEGA_N_LPF = 2 * PI * 500;   // Natural angular frequency of LPF
int i = 0;                                // Sampling index
int v_raw[N_SAMPLE];                      // Raw sampling data
float v_scaled[N_SAMPLE];                 // Scaled sampling data
float v_hpf[N_SAMPLE];                    // HPF'ed data, scaled to 100-volt mains
float v_lpf[N_SAMPLE];                    // HPF'ed and LPF'ed data, scaled to 100-volt mains
float s[N_PLOT];                          // Spectrum in percentage
SecondOrderHPF hpf(T_SAMPLE_US / 1e6, ZETA, OMEGA_N_HPF);
SecondOrderLPF lpf(T_SAMPLE_US / 1e6, ZETA, OMEGA_N_LPF);

int v_raw_max = 0;
int v_raw_min = 4095;

// Define class and instance for zero-cross detection and frequency calculation
class zeroCrossing {
  public:
    zeroCrossing();
    int N;
    float V_pos, V_neg;
};

zeroCrossing::zeroCrossing() {
  int N = 0;
  float V_neg = 0;
  float V_pos = 0;
}

const int N_ZEROCROSS = 100;
zeroCrossing zc[N_ZEROCROSS + 1];
int i_zc = 0;
int k = 0;

// Define constants and varianles for frequency, RMS voltage, and THD caculation results
const int N_MAF = 20;
int m = 0;

float f_buf[N_MAF];
float f_MAF = 0.0;
float f_MAF_old = 0.0;
const float f_alert_U = 50.1;
const float f_alert_L = 49.9;

float V_rms_buf[N_MAF];
float V_rms_MAF = 0.0;
float V_rms_MAF_old = 0.0;
const float V_rms_alert_U = 105.0;
const float V_rms_alert_L = 96.0;

float THD_buf[N_MAF];
float THD_MAF = 0.0;
float THD_MAF_old = 0.0;
const float THD_alert = 10.0;

// Plot setting
uint32_t t0;
const int T_PLOT_MS = 50;

// Define instances for horizontal meters, time-domain plot, and frequency-domain spectrum
hMeter hmFreq(108, 40, 49.0, 51.0);
hMeter hmVolt(108, 85, 90.0, 110.0);
tdPlot tdWaveform(104, 125, -200.0, 200.0);
spectrumPlot fdSpectrum(104, 185, 4);

// Define task handlers
TaskHandle_t taskHandle_sample, taskHandle_GSS;

//----------------------------------------------------------------------
// Functions
//----------------------------------------------------------------------

// Timer ISR
void IRAM_ATTR onTimer() {
  BaseType_t taskWoken;
  xTaskNotifyFromISR(taskHandle_sample, 0, eIncrement, &taskWoken);
}

// Task run by Timer ISR:
// Sampling of voltage and detection of zero-crossings
void sampleVoltage(void *pvParameters) {
  uint32_t ulNotifiedValue;
  while (true) {
    // Wait for notification
    xTaskNotifyWait(0, 0, &ulNotifiedValue, portMAX_DELAY);

    // Sample analog signal and apply filters
    v_raw[i] = analogRead(GPIO_AD);

    v_raw_max = v_raw[i] > v_raw_max ? v_raw[i] : v_raw_max;
    v_raw_min = v_raw[i] < v_raw_min ? v_raw[i] : v_raw_min;

    v_scaled[i] = static_cast<float>((v_raw[i] / AD_FULLSCALE * AD_VCC - AD_VCC / 2.0) / V_DIV * TURN_RATIO);
    v_hpf[i] = hpf.apply(v_scaled[i]);
    v_lpf[i] = lpf.apply(v_hpf[i]);
    //    v_hpf[i] = v_scaled[i];
    //     v_lpf[i] = v_hpf[i];

    // Detect zero-crossing
    i_zc++;
    int i_pre;
    i_pre = i > 0 ? i - 1 : N_SAMPLE - 1;
    if (v_lpf[i_pre] < 0 && v_lpf[i] >= 0) {
      // Check zero-crossing timing
      if (i_zc > N_PLOT - N_ZCW && i_zc < N_PLOT + N_ZCW) {
        // Save zero-crossing information
        zc[k].N = i_zc;
        zc[k].V_neg = v_lpf[i_pre];
        zc[k].V_pos = v_lpf[i];
        k = (k + 1) % N_ZEROCROSS;
      }
      i_zc = 0;
    }

    // Increment index
    i = (i + 1) % N_SAMPLE;
  }
}

// Calculation of frequency from zero-crosing information
float calcFreq() {
  float T = 0;

  // Calculate time for all zero-crossings in class
  T += zc[0].V_pos / (zc[0].V_pos - zc[0].V_neg);
  for (int j = 1; j < N_ZEROCROSS; j++) {
    T += zc[j].N;
  }
  T -= zc[N_ZEROCROSS - 1].V_neg / (zc[N_ZEROCROSS - 1].V_pos - zc[N_ZEROCROSS - 1].V_neg);
  T *= T_SAMPLE_US * 1e-6;

  // Calculate frequency and return
  float f = 1.0 / T * (N_ZEROCROSS - 1);
  return f;
}

// Calculation of RMS value
float calcRMS() {
  // Calculate RMS value
  float V_rms = 0;
  for (int j = 0; j < N_SAMPLE; j++) {
    V_rms += pow(v_lpf[j], 2);
  }
  V_rms /= N_SAMPLE;
  V_rms = sqrt(V_rms);
  return V_rms;
}

// Task to post frequency, RMS voltage, and harmonic spectum data to Google Spreadsheet API
void postGSS(void *pvParameters) {
  uint32_t ulNotifiedValue;
  char pubMessage[256];
  while (true) {
    // Wait for notification
    xTaskNotifyWait(0, 0, &ulNotifiedValue, portMAX_DELAY);

    // Flag up
    postingNow = true;
 
    // Create JSON message
    StaticJsonDocument<500> doc;
    JsonObject object = doc.to<JsonObject>();

    // Stuff data to JSON message and serialize it
    object["sensedDate"] = datebuf;
    object["freq"] = f_MAF;
    object["v_rms"] = V_rms_MAF;
    object["v3"] = s[3];
    object["v5"] = s[5];
    object["v7"] = s[7];
    object["v9"] = s[9];
    object["v11"] = s[11];
    object["thd"] = THD_MAF;
    serializeJson(doc, pubMessage);

    // Post to Google Spreadsheet API
    http.begin(apiURL);
    httpCode = http.POST(pubMessage);

    // Flag down
    postingNow = false;

    // Wait
    delay(100);
  }
}

//----------------------------------------------------------------------
// The "setup" Function
//----------------------------------------------------------------------

void setup() {
  // Initialize LovyanGFX
  lcd.begin();
  lcd.setRotation(3);
  lcd.setColorDepth(8);

  // Start WiFi, try multiple SSIDs
  lcd.setFont(&fonts::FreeMono9pt7b);
  lcd.setCursor(0, 0);
  for (int i = 0; i < N_SSID; i++) {
    lcd.println("Connecting to:");
    lcd.println(ssid[i]);
    uint32_t tWiFi = millis();

    // Wait for connecton
    WiFi.begin(ssid[i], password[i]);
    while (WiFi.status() != WL_CONNECTED) {
      delay(250);
      lcd.print('.');
      if (millis() - tWiFi > T_WIFI_TIMEOUT_MS) {
        lcd.println("\r\nConnection failed...\r\n");
        break;
      }
    }
    
    // Confirm connectionm, exit loop if connected
    if (WiFi.status() == WL_CONNECTED) {
      break;
    }
  }

  // Confirm connection
  if (WiFi.status() == WL_CONNECTED) {
    lcd.print("\r\nWifi connected!\r\nIP address:\r\n");
    lcd.println(WiFi.localIP());
    online = true;
  }
  else {
    lcd.println("\r\nGo offline!");
    online = false;
  }

  // Keep screen for a while
  delay(1000);
  lcd.clear();

  // Synchronize real-time clock to NTP server
  if (online) {
    configTime(JST, summertime, server1, server2, server3);
  }

  hmFreq.drawFrame();
  hmFreq.drawLabels();

  hmVolt.drawFrame();
  hmVolt.drawLabels();

  tdWaveform.drawFrame();
  tdWaveform.drawLabels();

  fdSpectrum.drawFrame();
  fdSpectrum.drawLabels();

  xTaskCreateUniversal(
    sampleVoltage,        // Function to be run as a task
    "sampleVoltage",      // Task name
    8192,                 // Stack memory
    NULL,                 // Parameter
    24,                   // Priority
    &taskHandle_sample,   // Task handler
    PRO_CPU_NUM           // Core to run the task
  );

  // Create task to POST measurement results to Google Spreadsheet web app, if online
  if (online) {    
    xTaskCreateUniversal(
      postGSS,              // Function to be run as a task
      "postGSS",            // Task name
      8192,                 // Stack memory
      NULL,                 // Parameter
      10,                   // Priority
      &taskHandle_GSS,      // Task handler
      PRO_CPU_NUM           // Core to run the task
    );
  }

  // Create timer interrupt
  hw_timer_t * timer = NULL;
  timer = timerBegin(0, 40, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 625, true);
  timerAlarmEnable(timer);

  // Wait for 10 ms for safety
  delay(10);
}

//----------------------------------------------------------------------
// The "loop" Function
//----------------------------------------------------------------------

void loop() {
  // Wait for next plot timing
  while (millis() - t0 < T_PLOT_MS) {
    delay(1);
  }
  t0 = millis();

  // Show clock
  if (online) {
    if (getLocalTime(&tm)) {
      lcd.setTextColor(TFT_WHITE, TFT_BLACK);
      lcd.setFont(&fonts::FreeMono9pt7b);
      lcd.setCursor(4, 4);
      lcd.setTextDatum(top_left);
      sprintf(datebuf, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
      lcd.printf(datebuf);
    }
  }

  // Copy data from buffer
  float vp[N_SAMPLE];
  int i0 = i + 1 % N_SAMPLE;
  for (int j = 0; j < N_SAMPLE; j++) {
    vp[j] = v_hpf[(i0 + j) % N_SAMPLE];
  }

  // Calculate frequency and RMS voltage
  float f = calcFreq();
  float V_rms = calcRMS();

  // Find zero-crossing to start plotting
  bool crossedLow = false;
  int j, j0;
  j = 1;
  while (true) {
    if (vp[j - 1] < 0 - 20 && vp[j] >= -20) {
      crossedLow = true;
    }
    if (crossedLow && vp[j] >= 20) {
      j0 = j;
      break;
    }
    j++;
    if (j >= N_SAMPLE) {
      j0 = 0;
      break;
    }
  }

  // Plot time-domain waveform
  tdWaveform.update(&vp[j0]);

  // Copy data as complex number, taking average of four cycles
  Complex uc[N_PLOT];
  float v_tmp = 0;
  for (int j = 0; j < N_PLOT; j++) {
    v_tmp = 0.0;
    for (int k = 0; k < N_CYCLE; k++) {
      v_tmp += vp[k * N_PLOT + j];
    }
    v_tmp /= N_CYCLE;
    uc[j].set(v_tmp, 0.0);
  }

  // Perform FFT
  FFT_1(uc, N_PLOT);

  // Calculate abosolute value spectrum and THD
  float THD;
  for (int j = 0; j < N_PLOT; j++) {
    // Spectrum
    s[j] = uc[j].modulus() / uc[1].modulus() * 100;

    // Add up squares from second up to 31st harmonics
    if (j > 1 && j < N_PLOT / 2) {
      THD += pow(uc[j].modulus(), 2);
    }
  }
  // Calculate THD
  THD = sqrt(THD) / uc[1].modulus() * 100;

  // Plot spectrum bar graph
  fdSpectrum.update(s);

  // Prepare for moving average
  f_buf[m] = f;
  V_rms_buf[m] = V_rms;
  THD_buf[m] = THD;
  m = (m + 1) % N_MAF;

  // Preserve old values
  f_MAF_old = f_MAF;
  V_rms_MAF_old = V_rms_MAF;
  THD_MAF_old = THD_MAF;
  
  f_MAF = 0;
  V_rms_MAF = 0;
  THD_MAF = 0;
  for (int j = 0; j < N_MAF; j++) {
    f_MAF += f_buf[j];
    V_rms_MAF += V_rms_buf[j];
    THD_MAF += THD_buf[j];
  }
  f_MAF /= N_MAF;
  V_rms_MAF /= N_MAF;
  THD_MAF /= N_MAF;

  // Show frequency
  // Change color if alert level reached
  if (f_MAF > f_alert_U || f_MAF < f_alert_L) {
    lcd.setFont(&fonts::FreeMonoBold12pt7b);
    lcd.setTextColor(TFT_RED, TFT_BLACK);
  }
  else {
    lcd.setFont(&fonts::FreeMono12pt7b);
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  }

  // Write frequency data to LCD
  lcd.setTextDatum(bottom_right);
  lcd.drawFloat(f_MAF, 3, 88, 52);
  lcd.setTextDatum(top_right);
  lcd.setFont(&fonts::FreeMono9pt7b);
  lcd.drawString("Hz", 84, 52);
  hmFreq.update(f_MAF);

  // Show voltage
  // Delete the 3rd (hundred's) digit if getting smaller
  if (V_rms_MAF_old >= 99.995 && V_rms_MAF < 99.995) {
    lcd.fillRect(88 - 14 * 6, 97 - 22, 14, 22, TFT_BLACK);
  }

  // Change color if alert level reached
  if (V_rms_MAF > V_rms_alert_U || V_rms_MAF < V_rms_alert_L) {
    lcd.setFont(&fonts::FreeMonoBold12pt7b);
    lcd.setTextColor(TFT_RED, TFT_BLACK);
  }
  else {
    lcd.setFont(&fonts::FreeMono12pt7b);
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  }

  // Write RMS voltage data to LCD
  lcd.setTextDatum(bottom_right);
  lcd.drawFloat(V_rms_MAF, 2, 88, 97);
  lcd.setTextDatum(top_right);
  lcd.setFont(&fonts::FreeMono9pt7b);
  lcd.drawString("V", 84, 97);
  hmVolt.update(V_rms_MAF);

  // Show THD
  // Delete the 2rd and 3rd (ten's and hundred's) digits if getting smaller
  if (THD_MAF_old >= 10.0 && THD_MAF < 10.0) {
    lcd.fillRect(80 - 14 * 6, 213 - 22, 14 * 2, 22, TFT_BLACK);
  }
  
  // Change color if alert level reached
  if (THD_MAF > THD_alert) {
    lcd.setFont(&fonts::FreeMonoBold12pt7b);
    lcd.setTextColor(TFT_RED, TFT_BLACK);
  }
  else {
    lcd.setFont(&fonts::FreeMono12pt7b);
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  }

  // Write THD data to LCD
  lcd.setTextDatum(bottom_right);
  lcd.drawFloat(THD_MAF, 2, 80, 213);
  lcd.setTextDatum(top_right);
  lcd.setFont(&fonts::FreeMono9pt7b);
  lcd.drawString("THD%", 76, 215);

  // Show Google Spreadsheet API status
  if (postingNow) {
    lcd.setCursor(228, 4);
    lcd.setTextColor(TFT_WHITE, TFT_DARKGREEN);
    lcd.print("G");
  }
  else {
    lcd.fillRect(228, 4, 11, 15, TFT_BLACK);
  }

  // Post to Google Spreadsheet periodically
  if (online) {
    if (tm.tm_sec % tPost == 0 && tm.tm_sec != tm_sec_old) {
      xTaskNotify(taskHandle_GSS, 0, eIncrement);
      tm_sec_old = tm.tm_sec;
      //postGSS();
    }
  }

  // Put small delay
  delay(5);
}
