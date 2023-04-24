#ifndef CONFIG_H
#define CONFIG_H
#define PSRAM_USE

#include <Arduino.h>
#define CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#define INFLUXDB
#define WiFi_EN
#define DEBUG_LVL 0// 0 = Only errors
#define DEBUG_TEMP 3
#define DEBUG_DEBUG 2
#define DEBUG_INFO 1
#define DEBUG_ERROR 0

void print(String s, byte level) {
  print(s.c_str(), level);
}

void print(const char *s, byte level) {
  if (level == DEBUG_ERROR) {
    Serial.println(s);
  }
  #if DEBUG_LVL >= DEBUG_INFO
  if (level == DEBUG_INFO) {
    Serial.println(s);
  }
  #endif
  #if DEBUG_LVL >= DEBUG_DEBUG
  if (level == DEBUG_DEBUG) {
    Serial.println(s);
  }
  #endif
  #if DEBUG_LVL >= DEBUG_TEMP
  if (level == DEBUG_TEMP) {
    Serial.println(s);
  }
  #endif
}

#define LEFT

/* Aus sich vom ESP IMU unten
  Pin  IO  Mosfet
  15  11  1
  14  10  2
  11  7   3
  10  6   4
  9   5   5
  8   4   6
  7   3   7
  6   2   AnalogL Front
  5   1   AnalogR Hacke
  17  13  8
  16  12  9
*/
//Analog
#ifdef LEFT
#define sensorHPin 2//1,32  // Analog input pin that the potentiometer is attached to32
#define sensorVPin 1//2, 33  // Analog input pin that the potentiometer is attached to33
#else
#define sensorHPin 1//1,32  // Analog input pin that the potentiometer is attached to32
#define sensorVPin 2//2, 33  // Analog input pin that the potentiometer is attached to33
#endif
#define MESSUREMENT_SMAPLE_TIME 10//ms
long lastMesurement = 0;
long lastUpload = 0;
int lastQueLen = 0;
//Digital
#ifdef LEFT
#define zuluftHPin 7
#define verbinderPin 12
#define Vibrator0 3
//#define verbinderHPin 5;
#define zuluftVPin 10
#define Vibrator1 4
#define Vibrator2 5
#define Vibrator3 6
#define Vibrator4 13
#define Vibrator5 11
#else
#define zuluftHPin 3
#define verbinderPin 12
#define Vibrator0 7
//#define verbinderHPin 5;
#define zuluftVPin 10
#define Vibrator1 4
#define Vibrator2 5
#define Vibrator3 6
#define Vibrator4 13
#define Vibrator5 11
#endif
#define PWM_FREQ 1500
#define PWM_RES 8

int vibrationStrenth = 200;
int vMode = 4;            // 0 AUS, 1 Random, 2 GND 255, 3 GAIT 255, 4 Asphalt, 5 Gravel, 6 Grass, 7 Sand, 8 GND var, 9 AIR var, 10 continues var
int modeTimerTicks = 0;
long valveTimer = 0;
int sensorValueH = 0;        // value read from the pot
int sensorValueV = 0;        // value read from the pot
int sensorValueG = 0;        // value read from the pot
int diff = 0;
int base = 0;
int filter = 0;
int threshold = 20;
int state = 3;            // 0 PUMP_FRONT, 1 PUMP_BACK, 2 EVEN_OUT, 3 PAUSE, 4 OPEN, 5 Evenout vorever, 6 CALLIBRATE, 7 not defined, <5000 Pump Front to val, >5000 go to value and even out
int cc = 0;
double baselineH = 0;
double baselineV = 0;
unsigned long startHigh = 0;
unsigned long startLow = 0;
unsigned long laststartHigh = 0;
unsigned long laststartLow = 0;
long clientDelay = 200;     // Delay of webserver to update Webrequests
bool vibrate = false;       // Activates ad deavtivates the Vibration Motors
enum stepState {
  GROUND,
  UP,
  DOWN,
  STANDING,
  UNDEFINED
};
stepState sState = UNDEFINED;

#include <Wire.h>
#include <ArduinoOTA.h>
#include <esp_task_wdt.h>

#ifdef WiFi_EN
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <WebServer.h>
#include <uri/UriBraces.h>
#include <uri/UriRegex.h>
WiFiManager wm;
WebServer server(80);
#endif

#ifdef INFLUXDB
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#define INFLUXDB_URL "https://europe-west1-1.gcp.cloud2.influxdata.com"
#define INFLUXDB_TOKEN "-uPns8ptZ32cvyMqNv3c4Ez-ycV5XQYsRv3p3FVPJ7uDPn7n-gI7b88BW9HmxhMNjRySxLZb54-3uKuODIijHg=="
#define INFLUXDB_ORG "marcogabrecht@gmail.com"
#define INFLUXDB_BUCKET "Schusohle"

#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"
#define NTP_SERVER1  "pool.ntp.org"
#define NTP_SERVER2  "time.nis.gov"
#define WRITE_PRECISION WritePrecision::MS
#define MAX_BATCH_SIZE 250
#define WRITE_BUFFER_SIZE 500//MAX_BATCH_SIZE*2
#endif

// HTML web page
const char index_html[] PROGMEM = R"***(
<!DOCTYPE html>
<html>
<body>

<h1>Modes</h1>

<form action="/action_page.php">
  <p>Please select the SystemState:</p>
 <input onchange="sendRe(stop)" type="radio" id="stop" name="fav_language" value="stop">
  <label for="html">STOP Upload</label><br>
  <input onchange="sendRe(start)" type="radio" id="start" name="fav_language" value="start">
  <label for="html">START Upload</label><br>
  <input onchange="sendRe(asphalt)" type="radio" id="asphalt" name="fav_language" value="asphalt">
  <label for="html">Asphalte</label><br>
  <input onchange="sendRe(grass)" type="radio" id="grass" name="fav_language" value="grass">
  <label for="html">grass</label><br>
  <input onchange="sendRe(sand)" type="radio" id="sand" name="fav_language" value="sand">
  <label for="html">sand</label><br>
  <input onchange="sendRe(lenolium)" type="radio" id="lenolium" name="fav_language" value="lenolium">
  <label for="html">Lenolium</label><br>
  <input onchange="sendRe(gravel)" type="radio" id="gravel" name="fav_language" value="gravel">
  <label for="html">Gravel</label><br>
  <input onchange="sendRe(evenout)" type="radio" id="evenout" name="fav_language" value="evenout">
  <label for="html">Even out</label><br>
  <input onchange="sendRe(restart)" type="radio" id="restart" name="fav_language" value="restart">
  <label for="html">restart</label><br>
  <input onchange="sendRe(vibrate)" type="radio" id="vibrate" name="fav_language" value="vibrate">
  <label for="html">VibrationToggle</label><br>
  <input onchange="changeMode(mode1)" type="radio" id="modeRadio" name="fav_language" value="modeRadio">
  <input type="text" id="mode1" name="modeValue1" value="0"><br>
  <input onchange="changeMode(mode2)" type="radio" id="modeRadio" name="fav_language" value="modeRadio">
  <input type="text" id="mode2" name="modeValue2" value="0"><br>
  <br><br>
  <span id="status"></span>
  <br>
</form>

<script>
function changeMode(x) {
  document.getElementById("status").innerHTML = "";

  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("status").innerHTML =
      "OK";
    }else {
      document.getElementById("status").innerHTML =
      "ERR";
    }
  };
  xhttp.open("GET", '/parameter/0/value/' + (x.value + ''), true);
  xhttp.send();
}

function sendRe(link) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("status").innerHTML =
      "OK";
    }else {
      document.getElementById("status").innerHTML =
      "ERR";
    }
  };
  xhttp.open("GET", link.id, true);
  xhttp.send();
}
</script>

</body>
</html>
)***";


// define two tasks
TaskHandle_t tloop;
TaskHandle_t tUpload;
void TaskLoop( void *pvParameters );
#ifdef INFLUXDB
void TaskUpload( void *pvParameters );
#endif
int ramLoop = 4096;
int ramUpload = 18384;
int ramWebservice = 4096;


int16_t accelX, accelY, accelZ;
int16_t gyroX, gyroY, gyroZ;
struct SensorReading
{
   long ax;//4bit
   long ay;
   long az;
   long gx;
   long gy;
   long gz;
   int pressureFront;//2bit
   int pressureBack;//2bit
   double pressureBaseline;//8bytes
   unsigned long long time_stamp;//4
};//40bytes
static QueueHandle_t msg_queue;
#ifdef PSRAM_USE
#define msg_queue_len 2000
uint8_t *queueStorageArea = (uint8_t *)ps_malloc(msg_queue_len * sizeof(SensorReading));
StaticQueue_t *queueDataStructure = (StaticQueue_t *)ps_malloc(sizeof(StaticQueue_t));
#elif
#define msg_queue_len 400
uint8_t *queueStorageArea = (uint8_t *)malloc(msg_queue_len * sizeof(SensorReading));
StaticQueue_t *queueDataStructure = (StaticQueue_t *)malloc(sizeof(StaticQueue_t));
#endif

#endif // CONFIG_H