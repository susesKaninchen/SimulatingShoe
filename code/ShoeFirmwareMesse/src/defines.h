#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
// CONFIG_FREERTOS_UNICORE and ARDUINO_RUNNING_CORE are already defined by the framework

// ── Debug ─────────────────────────────────────────────────────────────────────
#define DEBUG_LVL       2
#define DEBUG_ERROR_LVL 1
#define DEBUG_INFO_LVL  2
#define DEBUG_DEBUG_LVL 3

void print(const char *s, byte level) {
    if (level <= DEBUG_LVL) Serial.println(s);
}
void printS(String s, byte level) { print(s.c_str(), level); }

// ── WiFi credentials (trade show) ─────────────────────────────────────────────
#define WIFI_SSID "fablab"
#define WIFI_PASS "fablabfdm"
#define HOSTNAME  "SimShoe"

// ── Shoe side (comment out LEFT for right shoe) ────────────────────────────────
#define LEFT

// ── Pin mapping ───────────────────────────────────────────────────────────────
//  Viewed from ESP/IMU below:
//  Pin  IO  Function
//  15   11  Mosfet 1
//  14   10  Mosfet 2
//  11    7  Mosfet 3
//  10    6  Mosfet 4
//   9    5  Mosfet 5
//   8    4  Mosfet 6
//   7    3  Mosfet 7
//   6    2  Analog Front
//   5    1  Analog Heel
//  17   13  Mosfet 8
//  16   12  Mosfet 9

// Analog pressure sensors
#ifdef LEFT
#define sensorHPin 2   // Heel
#define sensorVPin 1   // Front
#else
#define sensorHPin 1   // Heel
#define sensorVPin 2   // Front
#endif

// Digital valve pins
#ifdef LEFT
#define zuluftHPin   7    // Heel air inlet
#define verbinderPin 12   // Chamber connector
#define zuluftVPin   10   // Front air inlet
#define Vibrator0    3
#define Vibrator1    4
#define Vibrator2    5
#define Vibrator3    6
#define Vibrator4    13
#define Vibrator5    11
#else
#define zuluftHPin   3    // Heel air inlet
#define verbinderPin 12   // Chamber connector
#define zuluftVPin   10   // Front air inlet
#define Vibrator0    7
#define Vibrator1    4
#define Vibrator2    5
#define Vibrator3    6
#define Vibrator4    13
#define Vibrator5    11
#endif

// PWM config for vibration motors
#define PWM_FREQ 1500
#define PWM_RES  8

// ── Measurement timing ────────────────────────────────────────────────────────
#define MEASUREMENT_SAMPLE_TIME 25   // ms

// ── Shared global state ───────────────────────────────────────────────────────
long  lastMeasurement = 0;

// Pressure
int   sensorValueH = 0;       // Heel raw ADC
int   sensorValueV = 0;       // Front raw ADC
int   sensorValueG = 0;       // Baseline/ground reference
float baselineH = 0;          // Slow-moving heel average
float baselineV = 0;          // Slow-moving front average
int   diff = 0;
int   base = 0;
int   threshold = 20;

// Valve state machine
// 0: Pump Front   1: Pump Back    2: Even Out (timed)   3: Pause
// 4: Open         5: Even Out ∞   6: Calibrate
// < 5000: holdValue(state)        >= 5000: setValue(state * 0.1)
int   state = 3;
int   cc = 0;
long  valveTimer = 0;

// IMU
int16_t accelX, accelY, accelZ;
int16_t gyroX,  gyroY,  gyroZ;

// Step detection
enum StepState { GROUND, UP, DOWN, STANDING, UNDEFINED_STEP };
StepState sState = UNDEFINED_STEP;

// Vibration
int  vibrationStrength = 200;
volatile int  vMode = 4;   // 4 = Asphalt default at startup
int  modeTimerTicks = 0;
volatile bool vibrate = false;

// Webserver client delay (ms between handleClient calls)
long clientDelay = 50;

// ── Includes ──────────────────────────────────────────────────────────────────
#include <Wire.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <uri/UriBraces.h>
#include <uri/UriRegex.h>

WebServer server(80);

// Vibrator pin lookup (index 0-5 → GPIO) — used by vibration.h and web_ui.h
static const int VIBRATOR_PINS[6] = {Vibrator0, Vibrator1, Vibrator2, Vibrator3, Vibrator4, Vibrator5};

// ── Chart ring buffer ─────────────────────────────────────────────────────────
// Stores every 25ms sample so the web client gets full resolution per poll.
// At 25ms sample rate and 500ms poll interval → ~20 new samples per request.
// Buffer holds 120 samples (~3 s) so slow browsers don't lose data.
#define CHART_BUF_SIZE 120

struct ChartSample {
    int16_t prH;   // heel pressure relative to baseline
    int16_t prV;   // front pressure relative to baseline
    int16_t ax;    // accelX / 100
    int16_t ay;
    int16_t az;
};

ChartSample chartBuf[CHART_BUF_SIZE];
volatile int chartBufHead  = 0;   // next write position
volatile int chartBufFill  = 0;   // how many valid entries (0..CHART_BUF_SIZE)

// Called from TaskLoop (25 ms tick)
inline void chartBufPush(const ChartSample &s) {
    chartBuf[chartBufHead] = s;
    chartBufHead = (chartBufHead + 1) % CHART_BUF_SIZE;
    if (chartBufFill < CHART_BUF_SIZE) chartBufFill++;
}

// Returns sample i (0 = oldest, chartBufFill-1 = newest)
inline ChartSample chartBufGet(int i) {
    int idx = (chartBufHead - chartBufFill + i + CHART_BUF_SIZE * 2) % CHART_BUF_SIZE;
    return chartBuf[idx];
}

// ── Task handles ──────────────────────────────────────────────────────────────
TaskHandle_t tloop;
void TaskLoop(void *pvParameters);

int ramLoop       = 4096;
int ramWebservice = 8192;   // extra for JSON buffer + server framework

#endif // CONFIG_H
