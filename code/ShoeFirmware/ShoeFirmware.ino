#define CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#include <Wire.h>
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
6   2   AnalogL
5   1   AnalogR
17  13  8
16  12  9
*/
//Analog
#define sensorHPin 1//1,32  // Analog input pin that the potentiometer is attached to32
#define sensorVPin 2//2, 33  // Analog input pin that the potentiometer is attached to33
#define MESSUREMENT_SMAPLE_TIME 10//ms
long lastMesurement = 0;
long lastUpload = 0;
int lastQueLen = 0;
//Digital
#define zuluftHPin 12
#define verbinderPin 13
#define Vibrator0 4
//#define verbinderHPin 5;
#define zuluftVPin 11
#define Vibrator1 7
#define Vibrator2 3
#define Vibrator3 5
#define Vibrator4 6
#define Vibrator5 10
#define PWM_FREQ 1500
#define PWM_RES 8


int sensorValueH = 0;        // value read from the pot
int sensorValueV = 0;        // value read from the pot
int sensorValueG = 0;        // value read from the pot
int diff = 0;
int base = 0;
int filter = 0;
int threshold = 15;
int state = 0;
int vState = 1;
int cc = 0;
double baseline = 0;
unsigned long startHigh = 0;
unsigned long startLow = 0;
unsigned long laststartHigh = 0;
unsigned long laststartLow = 0;
enum stepState {
  GROUND,
  UP,
  DOWN,
  STANDING,
  UNDEFINED
};
stepState sState = UNDEFINED;
#define INFLUXDB
#define WiFi_EN
//#define DEBUG

#ifdef WiFi_EN
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <uri/UriBraces.h>
#include <uri/UriRegex.h>
WebServer server(80);
#endif

#ifdef INFLUXDB
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#define WIFI_SSID "fablab"
#define WIFI_PASSWORD "fablabfdm"
#define INFLUXDB_URL "https://europe-west1-1.gcp.cloud2.influxdata.com"
#define INFLUXDB_TOKEN "-uPns8ptZ32cvyMqNv3c4Ez-ycV5XQYsRv3p3FVPJ7uDPn7n-gI7b88BW9HmxhMNjRySxLZb54-3uKuODIijHg=="
#define INFLUXDB_ORG "marcogabrecht@gmail.com"
#define INFLUXDB_BUCKET "Schusohle"

#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"
#define NTP_SERVER1  "pool.ntp.org"
#define NTP_SERVER2  "time.nis.gov"
#define WRITE_PRECISION WritePrecision::MS
#define MAX_BATCH_SIZE 100
#define WRITE_BUFFER_SIZE 300

InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
// Links oder Rechts
Point sensorStatus("L");// L, R
#endif

// define two tasks
void TaskLoop( void *pvParameters );
void TaskUpload( void *pvParameters );

#define msg_queue_len 800
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
  baseline = baseline*0.990 + sensorValueH*0.010;
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
  baseline = base;
  startHigh = millis();
  startLow = millis();
  laststartHigh = millis();
  laststartLow = millis();
}

void setVibration(int value) {
  for (int count = 0; count<=5; count++) {
    ledcWrite(count, value);
  }
}

void setVibration(int values[]) {
  for (int count = 0; count<=5; count++) {
    ledcWrite(count, values[count]);
  }
}

void updateVibrationSystem() {
  if (sState == STANDING) {
      setVibration(150);
  }else if (sState == GROUND) {
      setVibration(255);
  }else {
      setVibration(0);
  }
}

void updateValveSystem() {
  if (state == 0) {// PUMP_FRONT
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
  } else if (state == 1) {// PUMP_BACK
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
  } else if (state == 2) {// EVEN_OUT
     digitalWrite(verbinderPin, HIGH);
     if (cc%100 == 0) {
      state = 3;
    }
  } else if (state == 3) {// PAUSE
    digitalWrite(zuluftHPin, LOW);
    digitalWrite(verbinderPin, LOW);
    digitalWrite(zuluftVPin, LOW);
  } else if (state == 4) {// OPEN
    digitalWrite(zuluftHPin, HIGH);
    digitalWrite(verbinderPin, LOW);
    digitalWrite(zuluftVPin, HIGH);
  } else if (state == 5) {// EVEN_OUT
    
  } else if (state == 6) {// CALLIBRATE
    calibrate();
    state = 3;
  } else if (state == 7) {// KeineAhung
    
  } else {// Go To Value FRONT
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
    if (sensorValueV < state) {
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
  Wire.write(0b00011000); //Setting the accel to +/- 2g
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
  if (sensorValueH > baseline + 60) {
    sState = GROUND;
  }else if (sensorValueH > baseline+20) {
    sState = STANDING;
  }else if (false) {
    sState = UP;
  }else {
    sState = UNDEFINED;
  }
}

void setup() {
  #ifdef DEBUG
  Serial.begin(115200);
  #endif
  Wire.begin();
  Wire.setClock(400000);
  setupMPU();
  
#ifdef INFLUXDB
#ifdef WiFi_EN
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  #ifdef DEBUG
  Serial.print("Connecting to wifi");
  #endif
  while (WiFi.status() != WL_CONNECTED) {
    #ifdef DEBUG
    Serial.print(".");
    vTaskDelay(500 / portTICK_PERIOD_MS);
    #endif
  }
#ifdef DEBUG
  Serial.println(WiFi.localIP());                                                                                                                                                                                          
#endif
#endif
  // Sync time
  timeSync(TZ_INFO, NTP_SERVER1, NTP_SERVER2);
  #ifdef DEBUG
  Serial.println("time Fertig");
  #endif
  client.setWriteOptions(WriteOptions().writePrecision(WRITE_PRECISION).batchSize(MAX_BATCH_SIZE).bufferSize(WRITE_BUFFER_SIZE));
  #ifdef DEBUG
  Serial.println("client Fertig");
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
  xTaskCreatePinnedToCore(TaskLoop,  "TaskLoop",  8192,  NULL, configMAX_PRIORITIES - 1,  NULL,  ARDUINO_RUNNING_CORE);
  #ifdef DEBUG
  Serial.println("task1 Fertig");
  #endif
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  xTaskCreatePinnedToCore(TaskUpload,  "TaskUpload",  8192,  NULL,  1,  NULL,  ARDUINO_RUNNING_CORE);
  xTaskCreatePinnedToCore(TaskWebserver,  "TaskWebserver",  4192,  NULL,  1,  NULL,  ARDUINO_RUNNING_CORE);
  #ifdef DEBUG
  Serial.println("task2 Fertig");
  #endif
  #ifdef WiFi_EN
  server.on(UriRegex("^\\/parameter\\/([0-9]+)\\/value\\/([0-9]+)$"), []() {
    String parameter = server.pathArg(0);
    String value = server.pathArg(1);
    server.send(200, "text/plain", "Set " + parameter + " to " + value);
  });
  server.on("/status", []() {
    server.send(200, "text/plain", "Die letzte Messung dauerte: " + String(lastMesurement) + "microsekunden<br>Der letzte Upload dauerte: " + String(lastUpload) + "microsekunden und hatte " + String(lastQueLen) + " Elemente.");
  });
  server.begin();
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

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/

void TaskLoop(void *pvParameters)  // This is a task.
{
  (void) pvParameters;
  esp_task_wdt_init(30, false);
  #ifdef DEBUG
  Serial.println("Start Loop");
  #endif
  Point temptime("t");
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
    updateVibrationSystem();// Setze die Vibration
    SensorReading pointValue = {accelX, accelY, accelZ, gyroX, gyroY, gyroZ,sensorValueV, sensorValueH, baseline, getTimeStamp(&tv,3)};
    // Punkt wird erstellt mit allen gelesenen werten
    if (xQueueSend(msg_queue, (void *)&pointValue, 0) != pdTRUE) {
      #ifdef DEBUG
      Serial.println("Queue full");
      #endif
    }
    lastMesurement = micros() - tempMessure;
    //Serial.println("messureFin");
  }
}

void TaskWebserver(void*pvParameters) {
  for (;;)
  {
    server.handleClient();
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}

void TaskUpload(void *pvParameters)  // This is a task.
{
  (void) pvParameters;
  esp_task_wdt_init(30, false);
  #ifdef DEBUG
  Serial.println("Start uploadtask");
  #endif
  SensorReading item;
  long tempUploadTime = 0;
  for (;;)
  {
    int xD = 0;
    while (xQueueReceive(msg_queue, (void *)&item, 0) == pdTRUE) {
      xD++;
      //Serial.println(item);
#ifdef INFLUXDB
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
      #ifdef WiFi_EN
      #else
      #ifdef DEBUG
      Serial.println(client.pointToLineProtocol(sensorStatus));
      #endif
      #endif
      sensorStatus.clearFields();
#endif
    }
    lastQueLen = xD;
#ifdef INFLUXDB
    
#ifdef WiFi_EN
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
#else
    vTaskDelay(900 / portTICK_PERIOD_MS);
#endif
#ifdef DEBUG
    Serial.println("END Uploadet");
    #endif
#endif
  vTaskDelay(20 / portTICK_PERIOD_MS);
    //vTaskSuspend( NULL );
  }
}
