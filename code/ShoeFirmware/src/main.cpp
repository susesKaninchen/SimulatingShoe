#define CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#define INFLUXDB
#define WiFi_EN
#define DEBUG
//#define LEFT
#include <Wire.h>
#include <ArduinoOTA.h>
#include <esp_task_wdt.h>
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

int vMode = 4;
int modeTimerTicks = 0;
long valveTimer = 0;
int sensorValueH = 0;        // value read from the pot
int sensorValueV = 0;        // value read from the pot
int sensorValueG = 0;        // value read from the pot
int diff = 0;
int base = 0;
int filter = 0;
int threshold = 20;
int state = 3;
int cc = 0;
double baselineH = 0;
double baselineV = 0;
unsigned long startHigh = 0;
unsigned long startLow = 0;
unsigned long laststartHigh = 0;
unsigned long laststartLow = 0;
long clientDelay = 200;
boolean vibrate = true;
enum stepState {
  GROUND,
  UP,
  DOWN,
  STANDING,
  UNDEFINED
};
stepState sState = UNDEFINED;

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
#define WRITE_BUFFER_SIZE 400//MAX_BATCH_SIZE*2
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

#define msg_queue_len 400
int16_t accelX, accelY, accelZ;
int16_t gyroX, gyroY, gyroZ;
struct SensorReading
{
   long ax;
   long ay;
   long az;
   long gx;
   long gy;
   long gz;
   int pressureFront;
   int pressureBack;
   double pressureBaseline;
   unsigned long long time_stamp;
};
static QueueHandle_t msg_queue;

void messure() {
  //sensorValueG = analogRead(sensorGPin);
  sensorValueH = analogRead(sensorHPin)-sensorValueG;
  sensorValueV = analogRead(sensorVPin)-diff-sensorValueG;
  baselineH = baselineH*0.990 + sensorValueH*0.010;
  baselineV = baselineV*0.990 + sensorValueV*0.010;
}

void calibrate() {
  //Open Valves
  digitalWrite(zuluftHPin, HIGH);
  digitalWrite(zuluftVPin, HIGH);
  vTaskDelay(2000 / portTICK_PERIOD_MS);
  digitalWrite(zuluftHPin, LOW);
  digitalWrite(zuluftVPin, LOW);
  vTaskDelay(50 / portTICK_PERIOD_MS);
  
  int MeasurementsToAverage = 20;                // Anzahl der in den Mettelwert aufgenommenen Messungen 
  sensorValueH = 0;
  sensorValueV = 0;
  //sensorValueG = 0;
  for(int i = 0; i < MeasurementsToAverage; ++i)
  {
    sensorValueH += analogRead(sensorHPin);
    sensorValueV += analogRead(sensorVPin);
    //sensorValueG += analogRead(sensorGPin);
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
  sensorValueH /= MeasurementsToAverage;
  sensorValueV /= MeasurementsToAverage;
  sensorValueG /= MeasurementsToAverage;
  diff = sensorValueV - sensorValueH;
  base = sensorValueH;
  baselineH = base;
  baselineV = base;
  startHigh = millis();
  startLow = millis();
  laststartHigh = millis();
  laststartLow = millis();
}

void setVibration(int value) {
  for (int count = 0; count<6; count++) {
    ledcWrite(count, value);
  }
}

void setVibration(int values[]) {
  for (int count = 0; count<6; count++) {
    ledcWrite(count, values[count]);
  }
}

void updateVibrationSystem() {
  if (vMode == 0) {//Aus
    setVibration(0);
  }else if (vMode == 1) {//Random
    if (sensorValueH>baselineH+60||sensorValueV>baselineV+60) {
      setVibration(random(0, 255));
    }else {
      setVibration(0);
    }
  } else if (vMode == 2) {//Beim Auftreten
    if (sensorValueH>baselineH+60||sensorValueV>baselineV+60) {
      setVibration(255);
    }else {
      setVibration(0);
    }
  } else if (vMode == 3) {//entlang des ganges
    if (sensorValueH>baselineH+60||sensorValueV>baselineV+60) {
      int values[] = {255,255,0,255,0,255};
      setVibration(values);
    }else if (sensorValueH>baselineH+60){
      int values[] = {0,0,0,0,255,255};
      setVibration(values);
    }else if (sensorValueV>baselineV+60){
      int values[] = {0,255,255,255,0,0};
      setVibration(values);
    }else {
      setVibration(0);
    }
  } else if (vMode == 4) {//Simulate Asphalt
    if (sensorValueH>baselineH+60||sensorValueV>baselineV+60) {
      if (modeTimerTicks < 5) {
        modeTimerTicks++;
        setVibration(244);
      } else {
        setVibration(0);
      }
    }else {
      modeTimerTicks = 0;
      setVibration(0);
    }
  } else if (vMode == 5) {//Simulate Schotter
    if (sensorValueH>baselineH+60||sensorValueV>baselineV+60) {
      if (modeTimerTicks + random(15) < 15) {
        modeTimerTicks++;
        if (ledcRead(1)) {
          setVibration(255);
        }else {
          modeTimerTicks = 0;
          setVibration(0);
        }
      }
    }else {
      modeTimerTicks = 0;
      setVibration(0);
    }
  } else if (vMode == 6) {//Simulate Gras
    if (sensorValueH>baselineH+60||sensorValueV>baselineV+60) {
      //if (!(modeTimerTicks%(10 + random(10)))) {
      //  modeTimerTicks++;
      //  if (ledcRead(1)) {
          setVibration(55);
      //  }else {
      //    setVibration(0);
      //  }
    }else {
      modeTimerTicks = 0;
      setVibration(0);
    }
  } else if (vMode == 7) {//Simulate Sand
    if (sensorValueH>baselineH+60||sensorValueV>baselineV+60) {
      //if (!(modeTimerTicks%(10 + random(10)))) {
      //  modeTimerTicks++;
      //  if (ledcRead(1)) {
          setVibration(70);
      //  }else {
      //    setVibration(0);
      //  }
    }else {
      modeTimerTicks = 0;
      setVibration(0);
    }
  }
  /*if (sState == STANDING) {
      setVibration(150);
  }else if (sState == GROUND) {
      setVibration(255);
  }else {
      setVibration(0);
  }*/
}

void pumpFront() {
  if (sensorValueH<base-threshold) {
      digitalWrite(zuluftHPin, HIGH);//ZuluftHacke
    }else {
      digitalWrite(zuluftHPin, LOW);//ZuluftHacke
    }
    if (sensorValueV<base-threshold) {
      digitalWrite(zuluftVPin, HIGH);//ZuluftHacke
    }else {
      digitalWrite(zuluftVPin, LOW);//ZuluftHacke
    }
    if (sensorValueV<sensorValueH - threshold) {// von leiner Kammer in große Kammer
      digitalWrite(verbinderPin, HIGH);
    }else {
      digitalWrite(verbinderPin, LOW);
    }
}

void pumpBack() {
  if (sensorValueH<base-threshold) {
      digitalWrite(zuluftHPin, HIGH);//ZuluftHacke
    }else {
      digitalWrite(zuluftHPin, LOW);//ZuluftHacke
    }
    if (sensorValueV<base-threshold) {
      digitalWrite(zuluftVPin, HIGH);//ZuluftHacke
    }else {
      digitalWrite(zuluftVPin, LOW);//ZuluftHacke
    }
    if (sensorValueH<sensorValueV - threshold) {// von leiner Kammer in große Kammer
      digitalWrite(verbinderPin, HIGH);
    }else {
      digitalWrite(verbinderPin, LOW);
    }
}

void evenOut(int millisecends) {
  if (millisecends > 0) {
    if (valveTimer == 0) {
      valveTimer = millis();
    } else {
      if (valveTimer + millisecends < millis()) {
        state = 3;
        valveTimer = 0;
      }
    }
  }
  digitalWrite(verbinderPin, HIGH);
  digitalWrite(zuluftHPin, LOW);
  digitalWrite(zuluftVPin, LOW);
}

void holdValue(int value) {
    if (sensorValueV < value) {
      if (sensorValueV<sensorValueH - threshold) {// von leiner Kammer in große Kammer
        digitalWrite(verbinderPin, HIGH);
      }else {
        digitalWrite(verbinderPin, LOW);
      }
    } else {
      digitalWrite(zuluftHPin, LOW);
      digitalWrite(verbinderPin, LOW);
      digitalWrite(zuluftVPin, LOW);
    }
      if (sensorValueH<base-threshold) {
        digitalWrite(zuluftHPin, HIGH);//ZuluftHacke
      }else {
        digitalWrite(zuluftHPin, LOW);//ZuluftHacke
      }
      if (sensorValueV<base-threshold) {
        digitalWrite(zuluftVPin, HIGH);//ZuluftHacke
      }else {
        digitalWrite(zuluftVPin, LOW);//ZuluftHacke
      }
}

void setValue(int value) {
  if (sensorValueV < value) {
      if (sensorValueV<sensorValueH - threshold) {// von leiner Kammer in große Kammer
        digitalWrite(verbinderPin, HIGH);
      }else {
        digitalWrite(verbinderPin, LOW);
      }
    } else {
      state = 2;
    }
    if (sensorValueH<base-threshold) {
        digitalWrite(zuluftHPin, HIGH);//ZuluftHacke
      }else {
        digitalWrite(zuluftHPin, LOW);//ZuluftHacke
      }
      if (sensorValueV<base-threshold) {
        digitalWrite(zuluftVPin, HIGH);//ZuluftFront
      }else {
        digitalWrite(zuluftVPin, LOW);//ZuluftFront
      }
}

void updateValveSystem() {
  if (state == 0) {// PUMP_FRONT
    pumpFront();
  } else if (state == 1) {// PUMP_BACK
    pumpBack();
  } else if (state == 2) {// EVEN_OUT
     evenOut(1000);
  } else if (state == 3) {// PAUSE
    digitalWrite(zuluftHPin, LOW);
    digitalWrite(verbinderPin, LOW);
    digitalWrite(zuluftVPin, LOW);
  } else if (state == 4) {// OPEN
    digitalWrite(zuluftHPin, HIGH);
    digitalWrite(verbinderPin, LOW);
    digitalWrite(zuluftVPin, HIGH);
  } else if (state == 5) {// Evenout vorever
    evenOut(0);
  } else if (state == 6) {// CALLIBRATE
    calibrate();
    state = 3;
  } else if (state == 7) {//
    
  } else if (state < 5000) {// Go To Value FRONT
    holdValue(state);
  } else {
    setValue(state * 0.1);
  }
  cc++;
  if (cc == 1000) {
    cc = 0;
  }
}

void setupMPU(){
  Wire.beginTransmission(0b1101000); //This is the I2C address of the MPU (b1101000/b1101001 for AC0 low/high datasheet sec. 9.2)
  Wire.write(0x6B); //Accessing the register 6B - Power Management (Sec. 4.28)
  Wire.write(0b00000000); //Setting SLEEP register to 0. (Required; see Note on p. 9)
  Wire.endTransmission();  
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x1B); //Accessing the register 1B - Gyroscope Configuration (Sec. 4.4) 
  Wire.write(0x00011000); //Setting the gyro to full scale +/- 500deg./s 
  /*
    0 = 250 °/s
    1 = 500 °/s
    2 = 1000 °/s
    3 = 2000 °/s
  */
  Wire.endTransmission(); 
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x1C); //Accessing the register 1C - Acccelerometer Configuration (Sec. 4.5) 
  Wire.write(0b00001000); //Setting the accel to +/- 2g
  /*
    0 ± 2g
    1 ± 4g
    2 ± 8g
    3 ± 16g
   */
  Wire.endTransmission(); 
}

void recordAccelRegisters() {
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x3B); //Starting register for Accel Readings
  Wire.endTransmission();
  Wire.requestFrom(0b1101000,6); //Request Accel Registers (3B - 40)
  while(Wire.available() < 6);
  accelX = Wire.read()<<8|Wire.read(); //Store first two bytes into accelX
  accelY = Wire.read()<<8|Wire.read(); //Store middle two bytes into accelY
  accelZ = Wire.read()<<8|Wire.read(); //Store last two bytes into accelZ
  //processAccelData();
}//gForceX = accelX / 16384.0;


void recordGyroRegisters() {
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x43); //Starting register for Gyro Readings
  Wire.endTransmission();
  Wire.requestFrom(0b1101000,6); //Request Gyro Registers (43 - 48)
  while(Wire.available() < 6);
  gyroX = Wire.read()<<8|Wire.read(); //Store first two bytes into accelX
  gyroY = Wire.read()<<8|Wire.read(); //Store middle two bytes into accelY
  gyroZ = Wire.read()<<8|Wire.read(); //Store last two bytes into accelZ
  //processGyroData();
}//  rotX = gyroX / 131.0;

void classifyMovement() {//  GROUND,  UP,  DOWN,  STANDING,  UNDEFINED
  // Pressure change on Chambers
  if (sensorValueH > baselineH + 60) {
    sState = GROUND;
  }else if (sensorValueH > baselineH+20) {
    sState = STANDING;
  }else if (false) {
    sState = UP;
  }else {
    sState = UNDEFINED;
  }
}


/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/

void TaskWebserver(void*pvParameters) {
  #ifdef LEFT
    ArduinoOTA.setHostname("ShoeL");
    #else
    ArduinoOTA.setHostname("ShoeR");
    #endif
    #ifdef DEBUG
  Serial.println("Setze Ota NAme");
  #endif
  ArduinoOTA
    .onStart([]() {
      clientDelay = 1;
      vTaskDelete(tUpload);
      vTaskDelete(tloop);
    })
    .onEnd([]() {
      clientDelay = 200;
      ESP.restart();
    });
    #ifdef DEBUG
  Serial.println("StarteOta");
  #endif
  ArduinoOTA.begin();
  #ifdef DEBUG
  Serial.println("OTA Gestartet");
  #endif
  server.on("/", HTTP_GET, [](){
    server.send_P(200, "text/html", index_html);
  });
  server.on(UriRegex("^\\/parameter\\/([0-9]+)\\/value\\/([0-9]+)$"), []() {
    String parameter = server.pathArg(0);
    String value = server.pathArg(1);
    state = value.toInt();
    server.send(200, "text/plain", "Set " + parameter + " to " + value);
  });
  server.on(UriRegex("^\\/vibrate\\/([0-5])$"), []() {
    String parameter = server.pathArg(0);
    int value = parameter.toInt();
    server.send(200, "text/plain", "Vibrate Motor " + String(value) + "");
    vTaskDelete(tloop);
    ledcWrite(value, 255);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    ledcWrite(value, 0);
    xTaskCreatePinnedToCore(TaskLoop,  "TaskLoop",  ramLoop,  NULL, configMAX_PRIORITIES - 1,  &tloop,  ARDUINO_RUNNING_CORE);
    //xTaskCreatePinnedToCore(TaskUpload,  "TaskUpload",  ramUpload,  NULL,  1,  &tUpload,  ARDUINO_RUNNING_CORE);
  });
  server.on("/stop", []() {
    server.send(200, "text/plain", "Stoppe Upload und ValveSystem");
    state = 3;
    vTaskDelete(tUpload);
  });
  server.on("/start", []() {
    server.send(200, "text/plain", "Stoppe Upload und ValveSystem");
    state = 2600;
    #ifdef INFLUXDB
    xTaskCreatePinnedToCore(TaskUpload,  "TaskUpload",  ramUpload,  NULL,  1,  &tUpload,  ARDUINO_RUNNING_CORE);
    #endif
  });
  server.on("/status", []() {
    server.send(200, "text/plain", "Die letzte Messung dauerte: " + String(lastMesurement) + "microsekunden<br>Der letzte Upload dauerte: " + String(lastUpload) + "microsekunden und hatte " + String(lastQueLen) + " Elemente.<br>Freier Heep: " + String(ESP.getFreeHeap()));
  });
  server.on("/vibrate", []() {
    vibrate = !vibrate;
    server.send(200, "text/plain", "Vibrate: " + String(vibrate));
  });
  server.on("/asphalt", []() {
    vibrate = true;
    vMode = 4;
    state = 38000;
    server.send(200, "text/plain", "Simulating Asphalt");
  });
  server.on("/grass", []() {
    vibrate = true;
    vMode = 6;
    state = 5;
    server.send(200, "text/plain", "Simulating Grass");
  });
  server.on("/sand", []() {
    vibrate = true;
    vMode = 7;
    state = 5;
    server.send(200, "text/plain", "Simulating Sand");
  });
  server.on("/lenolium", []() {
    vibrate = false;
    state = 32000;
    server.send(200, "text/plain", "Simulating Sand");
  });
  server.on("/gravel", []() {
    vibrate = true;
    vMode = 5;
    state = 34000;
    server.send(200, "text/plain", "Simulating Sand");
  });
  server.on("/evenout", []() {
    server.send(200, "text/plain", "Evenout");
    state = 2;
  });
  server.on("/restart", []() {
    server.send(200, "text/plain", "Starte Neu");
    vTaskDelay(100 / portTICK_PERIOD_MS);
    ESP.restart();
  });
  server.begin();
  for (;;)
  {
    server.handleClient();
    ArduinoOTA.handle();
    vTaskDelay(clientDelay / portTICK_PERIOD_MS);
  }
}


void TaskUpload(void *pvParameters)  // This is a task.
{
  (void) pvParameters;
  esp_task_wdt_init(30, false);
  #ifdef DEBUG
  Serial.println("Start uploadtask");
  #endif
  #ifdef INFLUXDB
  InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
  
// Links oder Rechts
#ifdef LEFT
  Point sensorStatus("L");// L, R
#else
  Point sensorStatus("R");// L, R
#endif
client.setWriteOptions(WriteOptions().writePrecision(WRITE_PRECISION).batchSize(MAX_BATCH_SIZE).bufferSize(WRITE_BUFFER_SIZE));
  #endif
  SensorReading item;
  long tempUploadTime = 0;
#ifdef INFLUXDB
  for (;;)
  {
    int xD = 0;
    while (xQueueReceive(msg_queue, (void *)&item, 0) == pdTRUE && xD < MAX_BATCH_SIZE) {
      xD++;
      //Serial.println(item);
      sensorStatus.addField("ax", item.ax);
      sensorStatus.addField("ay", item.ay);
      sensorStatus.addField("az", item.az);
      sensorStatus.addField("gx", item.gx);
      sensorStatus.addField("gy", item.gy);
      sensorStatus.addField("gz", item.gz);
      sensorStatus.addField("pV", item.pressureFront);
      sensorStatus.addField("pH", item.pressureBack);
      sensorStatus.addField("pB", item.pressureBaseline);
      sensorStatus.setTime(item.time_stamp);
      client.writePoint(sensorStatus);
      //Serial.println(client.pointToLineProtocol(sensorStatus));
      sensorStatus.clearFields();
    }
    lastQueLen = xD;
#ifdef DEBUG
    Serial.print("Start Upload von : ");
    Serial.println(xD);
    #endif
    tempUploadTime = micros();
    if (!client.flushBuffer()) {
      #ifdef DEBUG
      Serial.print("InfluxDB flush failed: ");
      Serial.println(client.getLastErrorMessage());
      Serial.print("Full buffer: ");
      Serial.println(client.isBufferFull() ? "Yes" : "No");
      #endif
    }
    lastUpload = micros() - tempUploadTime;
#ifdef DEBUG
    Serial.println("END Uploadet");
    #endif
  }
  #endif
}


void TaskLoop(void *pvParameters)  // This is a task.
{
  (void) pvParameters;
  esp_task_wdt_init(30, false);
  #ifdef DEBUG
  Serial.println("Start Loop");
  #endif
  long tempMessure = 0;
  struct timeval tv; // Erstelle die Zeit
  TickType_t xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();
  for (;;) // A Task shall never return or exit.
  {
    vTaskDelayUntil(&xLastWakeTime, MESSUREMENT_SMAPLE_TIME / portTICK_PERIOD_MS);
    // Sorge für eine Konstante Messung alle 10 MS
    tempMessure = micros();// Messe die Microsekunden die eine Messung benötigt
    gettimeofday(&tv, NULL);// hole die aktuelle Zeit
    recordAccelRegisters();// Hole Accelerometer Werte
    recordGyroRegisters();// Hole Gyroscope Messwerte
    messure();// Messe den druck der Kammern
    classifyMovement();// Classifiziere die Bewegung
    updateValveSystem();// Setze die Ventile
    if (vibrate) {
      updateVibrationSystem();// Setze die Vibration
    }
    #ifdef INFLUXDB
    SensorReading pointValue = {accelX, accelY, accelZ, gyroX, gyroY, gyroZ,sensorValueV, sensorValueH, baselineH, getTimeStamp(&tv,3)};
    // Punkt wird erstellt mit allen gelesenen werten
    if (xQueueSend(msg_queue, (void *)&pointValue, 0) != pdTRUE) {
      #ifdef DEBUG
      Serial.println("Queue full");
      #endif
    }
    #endif
    lastMesurement = micros() - tempMessure;
    //Serial.println("messureFin");
  }
}

void setup() {
  #ifdef DEBUG
  Serial.begin(115200);
  #endif
  Wire.begin();
  Wire.setClock(400000);
  setupMPU();
  
#ifdef WiFi_EN
    bool res;
    #ifdef LEFT
    res = wm.autoConnect("ShoeL","shoeshoe"); // password protected ap
    #else
    res = wm.autoConnect("ShoeR","shoeshoe"); // password protected ap
    #endif
#ifdef DEBUG
  Serial.println(WiFi.localIP());                                                                                                                                                                                          
#endif
  // Sync time
  #ifdef INFLUXDB
  timeSync(TZ_INFO, NTP_SERVER1, NTP_SERVER2);
  #endif
  #ifdef DEBUG
  Serial.println("time Fertig");
  #endif
#endif
  msg_queue = xQueueCreate(msg_queue_len, sizeof(SensorReading));
  #ifdef DEBUG
  Serial.println("Queue Fertig");
  #endif
  // Gardware
  //messure();
  //Serial.println("messure Fertig");
  pinMode(zuluftHPin, OUTPUT);//ZuluftHacke
  pinMode(verbinderPin, OUTPUT);//VerbinderV
  pinMode(zuluftVPin, OUTPUT);//ZuluftV
  ledcSetup(0, PWM_FREQ, PWM_RES);
  ledcAttachPin(Vibrator0, 0);
  ledcWrite(0, 0);
  ledcSetup(1, PWM_FREQ, PWM_RES);
  ledcAttachPin(Vibrator1, 1);
  ledcWrite(1, 0);
  ledcSetup(2, PWM_FREQ, PWM_RES);
  ledcAttachPin(Vibrator2, 2);
  ledcWrite(2, 0);
  ledcSetup(3, PWM_FREQ, PWM_RES);
  ledcAttachPin(Vibrator3, 3);
  ledcWrite(3, 0);
  ledcSetup(4, PWM_FREQ, PWM_RES);
  ledcAttachPin(Vibrator4, 4);
  ledcWrite(4, 0);
  ledcSetup(5, PWM_FREQ, PWM_RES);
  ledcAttachPin(Vibrator5, 5);
  ledcWrite(5, 0);
  digitalWrite(zuluftHPin, LOW);
  digitalWrite(verbinderPin, LOW);
  digitalWrite(zuluftVPin, LOW);
  #ifdef DEBUG
  Serial.println("pins Fertig");
  #endif
  vTaskDelay(50 / portTICK_PERIOD_MS);
  //messure();// Remove inductive nois
  #ifdef DEBUG
  Serial.println("mesure2 Fertig");
  #endif
  vTaskDelay(1 / portTICK_PERIOD_MS);
  calibrate();
  #ifdef DEBUG
  Serial.println("callibarate Fertig");
  #endif
  xTaskCreatePinnedToCore(TaskLoop,  "TaskLoop",  ramLoop,  NULL, configMAX_PRIORITIES - 1,  &tloop,  ARDUINO_RUNNING_CORE);
  #ifdef DEBUG
  Serial.println("task1 Fertig");
  #endif
  #ifdef INFLUXDB
  vTaskDelay(10 / portTICK_PERIOD_MS);
  xTaskCreatePinnedToCore(TaskUpload,  "TaskUpload",  ramUpload,  NULL,  1,  &tUpload,  ARDUINO_RUNNING_CORE);
  #endif
  #ifdef DEBUG
  Serial.println("task2 Fertig");
  #endif
  #ifdef WiFi_EN
  xTaskCreatePinnedToCore(TaskWebserver,  "TaskWebserver",  ramWebservice,  NULL,  1,  NULL,  ARDUINO_RUNNING_CORE);
  #endif
  #ifdef DEBUG
  Serial.println("Webserver Fertig");
  #endif
}

void loop()
{
  esp_task_wdt_init(30, false);
  vTaskDelete(NULL);
}
