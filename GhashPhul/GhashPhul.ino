/* by Abdullah As-Sadeed*/

/*
  Board: "ESP32 Dev Module"
  Upload Speed: "921600"
  CPU Frequency: "240MHz (WiFi/BT)"
  Flash Frequency: "40MHz"
  Flash Mode: "DIO"
  Flash Size: "4MB (32Mb)"
  Partition Scheme: "Huge APP (3MB No OTA/1MB SPIFFS)"
  Core Debug Level: "Verbose"
  PSRAM: "Disabled"
  Arduino Runs On: "Core 1"
  Events Run On: "Core 1"
*/


/*
  /home/abdullah_as-sadeed/.arduino15/packages/esp32/hardware/esp32/2.0.5/cores/esp32/HardwareSerial.cpp

  #ifndef RX1
  #if CONFIG_IDF_TARGET_ESP32
  #define RX1 25
  #endif
  #endif

  #ifndef TX1
  #if CONFIG_IDF_TARGET_ESP32
  #define TX1 26
  #endif
  #endif

*/



#define RDM6300_Pin 35
#define Authentication_LED_Pin 2

#define SIM800L__NEO7M Serial1

#define RCWL0516_Pin 13

#define Flame_Sensor_Pin 15

#define Slave_Board Serial2

#define Relay_1_Pin 32
#define Relay_2_Pin 33

#define Buzzer_LED_Pin 4



#include "Arduino.h"
#include "esp_arduino_version.h"

#include "rdm6300.h"

#include "Wire.h"

#include "MPU9250_WE.h"

#include "BH1750.h"

#include "Seeed_BME280.h"

#include "TinyGPSPlus.h"

#include "Lewis.h"

#include "esp_wifi.h"
#include "WiFi.h"

#include "AsyncTCP.h"
#include "ESPAsyncWebServer.h"

#include "ArduinoJson.h"



Rdm6300 RDM6300;

MPU9250_WE MPU9250 = MPU9250_WE(0x68);

BH1750 bh1750(0x23);

BME280 bme280;

TinyGPSPlus GPS;

Lewis Morse_Code;

AsyncWebServer Server(80);
AsyncEventSource Events("/Events");



boolean MPU9250_Status, MPU9250_Magnetometer_Status, BH1750_Status, BME280_Status, BlackBody_Motion, Flame, Called_Yet = 0;

const unsigned int Authenticated_Card = 12940902;

unsigned int Card, Satellites;

unsigned long Last_SSE_Time = 0, SSE_Delay = 500, Last_Call_Time = 0, Call_Delay = 60000;

float Acceleration_X, Acceleration_Y, Acceleration_Z, Resultant_Acceleration, Gyro_X, Gyro_Y, Gyro_Z, Magneto_X, Magneto_Y, Magneto_Z, Light, Temperature_MPU9250, Temperature_BME280, Humidity, Pressure, Altitude_BME280, Latitude, Longitude, Speed, Course, Altitude_GPS, HDOP, MQ_2, MQ_3, MQ_4, MQ_5, MQ_6, MQ_7, MQ_8, MQ_9, MQ_135, Slave_UpTime;

const char* WiFi_SSID = "Sadeed-PC";
const char* WiFi_PassWord = "xxxxxxxxxxx";

const char* Phone_Number = "+880xxxxxxxxxx";

const char* Compile_Date_Time = __DATE__ " " __TIME__;



void Authenticate(void) {
  do {
    if (RDM6300.get_new_tag_id()) {
      Card = RDM6300.get_tag_id();
    }

    digitalWrite(Authentication_LED_Pin, RDM6300.get_tag_id());

    delay(10);
  } while (Card != Authenticated_Card);

  digitalWrite(Authentication_LED_Pin, LOW);
}



String Scan_I2C(void) {
  byte error, address;
  int I2C_Devices_Count = 0;
  String I2C_Scan_Result = "";

  for (address = 0x01; address < 0x7f; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      I2C_Scan_Result += "0x";
      I2C_Scan_Result += String(address, HEX);
      I2C_Scan_Result += "\n";

      I2C_Devices_Count++;
    } else if (error != 2) {
      I2C_Scan_Result += "Error ";
      I2C_Scan_Result += error;
      I2C_Scan_Result += " at address 0x";
      I2C_Scan_Result += String(address, HEX);
      I2C_Scan_Result += "\n";
    }
  }

  if (I2C_Devices_Count == 0) {
    I2C_Scan_Result += "No device";
  }

  return I2C_Scan_Result;
}



void SetUp_MPU9250(void) {
  if (MPU9250.init()) {
    MPU9250_Status = true;
  } else {
    MPU9250_Status = false;
  }

  if (MPU9250.initMagnetometer()) {
    MPU9250_Magnetometer_Status = true;
  } else {
    MPU9250_Magnetometer_Status = false;
  }

  if (MPU9250_Status && MPU9250_Magnetometer_Status) {
    delay(1000);

    MPU9250.autoOffsets();

    MPU9250.enableGyrDLPF();

    MPU9250.setGyrDLPF(MPU9250_DLPF_6);

    MPU9250.setSampleRateDivider(5);

    MPU9250.setGyrRange(MPU9250_GYRO_RANGE_250);

    MPU9250.setAccRange(MPU9250_ACC_RANGE_2G);

    MPU9250.enableAccDLPF(true);

    MPU9250.setAccDLPF(MPU9250_DLPF_6);

    MPU9250.setMagOpMode(AK8963_CONT_MODE_100HZ);

    delay(200);
  };
}



void SetUp_BH1750(void) {
  if (bme280.init()) {
    BME280_Status = true;
  } else {
    BME280_Status = false;
  }
}



void SetUp_BME280(void) {
  if (bh1750.begin(BH1750::CONTINUOUS_HIGH_RES_MODE_2)) {
    BH1750_Status = true;
  } else {
    BH1750_Status = false;
  }
}



void SetUp_Relays(void) {
  pinMode(Relay_1_Pin, OUTPUT);
  pinMode(Relay_2_Pin, OUTPUT);

  digitalWrite(Relay_1_Pin, HIGH);
  digitalWrite(Relay_2_Pin, HIGH);
}



void SetUp_WiFi(void) {
  const char* HostName = "GhashPhul_IoT";

  WiFi.mode(WIFI_STA);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname(HostName);

  WiFi.begin(WiFi_SSID, WiFi_PassWord);

  while (WiFi.status() != WL_CONNECTED) {}
}



void Add_Common_Headers(AsyncWebServerResponse * response) {
  response->addHeader("X-Content-Type-Options", "nosniff");
  response->addHeader("X-Frame-Options", "SAMEORIGIN");
  response->addHeader("Referrer-Policy", "strict-origin");
  response->addHeader("Server", "GhashPhul IoT");
}



void Handle_Error_404(AsyncWebServerRequest * request) {
  String Response_Text = "HTTP 404\n\n";

  Response_Text += "Version: ";
  int version = request->version();
  if (version == 0) {
    Response_Text += "HTTP/1.0\n\n";
  } else if (version == 1) {
    Response_Text += "HTTP/1.1\n\n";
  }
  Response_Text += "Host: ";
  Response_Text += request->host();
  Response_Text += "\n\nURL: ";
  Response_Text += request->url();
  Response_Text += "\n\nMethod: ";
  int method = request->method();
  if (method == HTTP_GET) {
    Response_Text += "GET";
  } else if (method == HTTP_POST) {
    Response_Text += "POST";
  } else if (method == HTTP_DELETE) {
    Response_Text += "DELETE";
  } else if (method == HTTP_PUT) {
    Response_Text += "PUT";
  } else if (method == HTTP_PATCH) {
    Response_Text += "PATCH";
  } else if (method == HTTP_HEAD) {
    Response_Text += "HEAD";
  } else if (method == HTTP_OPTIONS) {
    Response_Text += "OPTIONS";
  }
  Response_Text += "\n\nTotal arguments: ";
  int args = request->args();
  Response_Text += args;
  Response_Text += "\n\n";
  if (args > 0) {
    Response_Text += "Arguments:\n";
    for (int i = 0; i < args; i++) {
      Response_Text += "\t  ";
      Response_Text += request->argName(i).c_str();
      Response_Text += ": ";
      Response_Text += request->arg(i).c_str();
      Response_Text += "\n";
    }
    Response_Text += "\n";
  }
  Response_Text += "Total request headers: ";
  Response_Text += request->headers();

  AsyncWebServerResponse *response = request->beginResponse(404, "text/plain; charset=utf-8", Response_Text);
  Add_Common_Headers(response);
  response->addHeader("Content-Disposition", "inline; filename=\"GhashPhul IoT | Error 404\"");
  request->send(response);
}



void Handle_I2C_Scan(AsyncWebServerRequest * request) {
  AsyncWebServerResponse *response = request->beginResponse(200, "text/html; charset=utf-8", Scan_I2C());
  Add_Common_Headers(response);
  request->send(response);
}



void Handle_Relays(AsyncWebServerRequest * request) {
  String Relay_Number, Relay_State, Response_Text;

  int args = request->args();
  if (args > 0) {
    for (int i = 0; i < args; i++) {
      if (request->argName(i) == "number") {
        Relay_Number = request->arg(i);
      } else if (request->argName(i) == "state") {
        Relay_State = request->arg(i);
      }
    }
  }

  if (Relay_Number == "1") {
    if (Relay_State == "1") {
      digitalWrite(Relay_1_Pin, LOW);

      Response_Text = "Relay 1 On";
    } else if (Relay_State == "0") {
      digitalWrite(Relay_1_Pin, HIGH);

      Response_Text = "Relay 1 Off";
    }
  } else if (Relay_Number == "2") {
    if (Relay_State == "1") {
      digitalWrite(Relay_2_Pin, LOW);

      Response_Text = "Relay 2 On";
    } else if (Relay_State == "0") {
      digitalWrite(Relay_2_Pin, HIGH);

      Response_Text = "Relay 2 Off";
    }
  }

  AsyncWebServerResponse *response = request->beginResponse(200, "text/plain; charset=utf-8", Response_Text);
  Add_Common_Headers(response);
  request->send(response);
}



void Handle_WiFi_Scan(AsyncWebServerRequest * request) {
  String WiFi_Scan_Result = "";

  int n = WiFi.scanComplete();
  if (n == -2) {
    WiFi.scanNetworks(true);
  } else if (n) {
    WiFi_Scan_Result += "<table>\n<thead>\n<tr>\n<th>RSSI</th>\n<th>SSID</th>\n<th>BSSID</th>\n<th>Channel</th>\n<th>Encryption</th>\n</tr>\n</thead>\n<tbody>\n";
    for (int i = 0; i < n; ++i) {
      WiFi_Scan_Result += "<tr>\n<td>";
      WiFi_Scan_Result += WiFi.RSSI(i);
      WiFi_Scan_Result += "</td>\n<td>";
      WiFi_Scan_Result += WiFi.SSID(i);
      WiFi_Scan_Result += "</td>\n<td>";
      WiFi_Scan_Result += WiFi.BSSIDstr(i);
      WiFi_Scan_Result += "</td>\n<td>";
      WiFi_Scan_Result += WiFi.channel(i);
      WiFi_Scan_Result += "</td>\n<td>";
      int encryption = WiFi.encryptionType(i);
      if (encryption == WIFI_AUTH_OPEN) {
        WiFi_Scan_Result += "Open";
      } else if (encryption == WIFI_AUTH_WEP) {
        WiFi_Scan_Result += "WEP";
      } else if (encryption == WIFI_AUTH_WPA_PSK) {
        WiFi_Scan_Result += "WPA PSK";
      } else if (encryption == WIFI_AUTH_WPA2_PSK) {
        WiFi_Scan_Result += "WPA2 PSK";
      } else if (encryption == WIFI_AUTH_WPA_WPA2_PSK) {
        WiFi_Scan_Result += "WPA WPA2 PSK";
      } else if (encryption == WIFI_AUTH_WPA2_ENTERPRISE) {
        WiFi_Scan_Result += "WPA2 Enterprise";
      }
      WiFi_Scan_Result += "</td>\n</tr>\n";
    }
    WiFi_Scan_Result += "</tbody>\n</table>\n";

    WiFi.scanDelete();
    if (WiFi.scanComplete() == -2) {
      WiFi.scanNetworks(true);
    }
  }
  AsyncWebServerResponse *response = request->beginResponse(200, "text/html; charset=utf-8", WiFi_Scan_Result);
  Add_Common_Headers(response);
  request->send(response);
}



void Get_Slave_Readings(void) {
  while (Slave_Board.available()) {
    StaticJsonDocument<1000> Slave_Readings;

    DeserializationError JSON_Error = deserializeJson(Slave_Readings, Slave_Board);

    if (JSON_Error == DeserializationError::Ok) {
      MQ_2 = Slave_Readings["MQ_2"].as<float>();
      MQ_3 = Slave_Readings["MQ_3"].as<float>();
      MQ_4 = Slave_Readings["MQ_4"].as<float>();
      MQ_5 = Slave_Readings["MQ_5"].as<float>();
      MQ_6 = Slave_Readings["MQ_6"].as<float>();
      MQ_7 = Slave_Readings["MQ_7"].as<float>();
      MQ_8 = Slave_Readings["MQ_8"].as<float>();
      MQ_9 = Slave_Readings["MQ_9"].as<float>();
      MQ_135 = Slave_Readings["MQ_135"].as<float>();

      Slave_UpTime = Slave_Readings["UpTime"].as<float>();
    }
  }
}



void Call_About_Flame(void) {
  if (Flame) {
    if (!Called_Yet || ((millis() - Last_Call_Time) > Call_Delay)) {
      SIM800L__NEO7M.print("ATD+ ");
      SIM800L__NEO7M.print(Phone_Number);
      SIM800L__NEO7M.print(";\r\n");

      Called_Yet = 1;
      Last_Call_Time = millis();
    }
  }
}



void Take_Auto_Actions(void) {
  Call_About_Flame();
}



void Get_Readings(void) {
  if (MPU9250_Status && MPU9250_Magnetometer_Status) {

    xyzFloat acceleration = MPU9250.getGValues();
    xyzFloat gyro = MPU9250.getGyrValues();
    xyzFloat magneto = MPU9250.getMagValues();
    Resultant_Acceleration = MPU9250.getResultantG(acceleration);

    Acceleration_X = acceleration.x;
    Acceleration_Y = acceleration.y;
    Acceleration_Z = acceleration.z;
    Gyro_X = gyro.x;
    Gyro_Y = gyro.y;
    Gyro_Z = gyro.z;
    Magneto_X = magneto.x;
    Magneto_Y = magneto.y;
    Magneto_Z = magneto.z;

    Temperature_MPU9250 = MPU9250.getTemperature();
  } else {
    Acceleration_X = 0;
    Acceleration_Y = 0;
    Acceleration_Z = 0;
    Resultant_Acceleration = 0;
    Gyro_X = 0;
    Gyro_Y = 0;
    Gyro_Z = 0;
    Magneto_X = 0;
    Magneto_Y = 0;
    Magneto_Z = 0;
    Temperature_MPU9250 = 0;
  }

  if (BH1750_Status && bh1750.measurementReady()) {
    Light = bh1750.readLightLevel();
  } else {
    Light = 0;
  }

  if (BME280_Status) {
    Temperature_BME280 = bme280.getTemperature();
    Humidity = bme280.getHumidity();
    Pressure = bme280.getPressure();
    Altitude_BME280 = bme280.calcAltitude(Pressure);
  } else {
    Temperature_BME280 = 0;
    Humidity = 0;
    Pressure = 0;
    Altitude_BME280 = 0;
  }

  while (SIM800L__NEO7M.available()) {
    GPS.encode(SIM800L__NEO7M.read());
  }

  Latitude = GPS.location.lat();
  Longitude = GPS.location.lng();
  Speed = GPS.speed.kmph();
  Course = GPS.course.deg();
  Altitude_GPS = GPS.altitude.meters();
  Satellites = GPS.satellites.value();
  HDOP = GPS.hdop.value();

  BlackBody_Motion = digitalRead(RCWL0516_Pin);

  Flame = !digitalRead(Flame_Sensor_Pin);

  do {
    Get_Slave_Readings();
  } while (MQ_2 == 0 && MQ_3 == 0 && MQ_4 == 0 && MQ_5 == 0 && MQ_6 == 0 && MQ_7 == 0 && MQ_8 == 0 && MQ_9 == 0 && MQ_135 == 0 && Slave_UpTime == 0);

  Take_Auto_Actions();
}



const char* xhtml PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<!-- by Abdullah As-Sadeed -->
<html lang="en-US" xmlns="http://www.w3.org/1999/xhtml">
<head>
  <meta charset="utf-8"/>
  <title>GhashPhul IoT</title>
  <link rel="icon" href="data:;base64,iVBORw0KGgo="/>
  <meta name="viewport" content="width=device-width, initial-scale=1"/>
  <meta name="author" content="Abdullah As-Sadeed"/>
  <meta name="description" content="GhashPhul IoT"/>
<style>
* {
  box-sizing: border-box;
  user-select: none;
  -webkit-tap-highlight-color: transparent;
}
*:focus {
  outline: none;
}
html {
  scroll-behavior: smooth;
}
body {
  margin: 0;
  text-align: center;
  line-height: 1.6;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
}
.topnav {
  overflow: hidden;
  background-color: #50B8B4;
  color: white;
}
.cards {
  max-width: 800px;
  margin: 50px auto;
  display: grid;
  grid-gap: 2rem;
  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
}
.card {
  box-shadow: 2px 2px 12px 1px rgba(140, 140, 140, 0.5);
  overflow-x: auto;
}
.card_heading {
  font-weight: bold;  
}
.alert_box {
  visibility: hidden;
  background-color: black;
  color: white;
  text-align: center;
  border-radius: 5px;
  padding: 10px 20px 10px 20px;
  position: fixed;
  z-index: 1;
  left: 50%%;
  transform: translate(-50%%, 0);
  bottom: 20px;
}

.alert_box.show {
  visibility: visible;
  animation: alert_box_in 0.5s, alert_box_out 0.5s 2.5s;
}

@keyframes alert_box_in {
  from {
    opacity: 0;
  }

  to {
    opacity: 1;
  }
}

@keyframes alert_box_out {
  from {
    opacity: 1;
  }

  to {
    opacity: 0;
  }
}
button {
  cursor: pointer;
}
table {
  width: 100%%;
}
th, td {
  padding: 10px 20px 10px 20px;
}
#details {
  text-align: left;
  padding-left: 20px;
  padding-right: 20px;
}
#I2C_Scan_Result {
  white-space: pre-wrap;
}
#BlackBody_Motion {
  display: none;
  height: 40px;
  width: 40px;
  position: fixed;
  bottom: 10px;
  left: 10px;
  z-index: 1;
  border-radius: 50%%;
  justify-content: center;
  align-items: center;
  background-color: green;
  color: white;
  font-weight: bold;
}
#Flame {
  display: none;
  height: 40px;
  width: 40px;
  position: fixed;
  bottom: 10px;
  right: 10px;
  z-index: 1;
  border-radius: 50%%;
  justify-content: center;
  align-items: center;
  background-color: red;
  color: white;
  font-weight: bold;
}
</style>
</head>
<body lang="en-US">
<div class="topnav">
  <h1>GhashPhul IoT</h1>
</div>
<div class="cards">
  <div class="card" id="details">
  <p>
    <span><b>Chip Model:</b> %CHIP_MODEL%</span>
    <br/>
    <span><b>Chip Revision:</b> %CHIP_REVISION%</span>
    <br/>
    <span><b>Chip Cores:</b> %CHIP_CORES%</span>
    <br/>
    <span><b>Chip ID:</b> %CHIP_ID%</span>
    <br/>
    <span><b>MAC Address:</b> %MAC_ADDRESS%</span>
    <br/>
    <span><b>Host Name:</b> %HOST_NAME%</span>
    <br/>
    <span><b>Local IP Address:</b> %LOCAL_IP_ADDRESS%</span>
    <br/>
    <span><b>Compiled On:</b> %COMPILE_DATE_TIME%</span>
  </p>
  </div>
</div>
<div class="cards">
  <div class="card">
    <p class="card_heading">Uptime (Master)</p>
    <p><span id="UpTime">%UPTIME%</span> s</p>
  </div>
  <div class="card">
    <p class="card_heading">Uptime (Slave)</p>
    <p><span id="Slave_UpTime">%SLAVE_UPTIME%</span> s</p>
  </div>
</div>
<div class="cards">
  <div class="card">
    <p class="card_heading">Acceleration</p>
    <p><b>X:</b> <span id="Acceleration_X">%Acceleration_X%</span> g</p>
    <p><b>Y:</b> <span id="Acceleration_Y">%Acceleration_Y%</span> g</p>
    <p><b>Z:</b> <span id="Acceleration_Z">%Acceleration_Z%</span> g</p>
    <p><b>Resultant:</b> <span id="Resultant_Acceleration">%Resultant_Acceleration%</span> g</p>
  </div>
  <div class="card">
    <p class="card_heading">Gyro</p>
    <p><b>X:</b> <span id="Gyro_X">%Gyro_X%</span> deg/s</p>
    <p><b>Y:</b> <span id="Gyro_Y">%Gyro_Y%</span> deg/s</p>
    <p><b>Z:</b> <span id="Gyro_Z">%Gyro_Z%</span> deg/s</p>
  </div>
  <div class="card">
    <p class="card_heading">Compass</p>
    <p><b>X:</b> <span id="Magneto_X">%Magneto_X%</span> µT</p>
    <p><b>Y:</b> <span id="Magneto_Y">%Magneto_Y%</span> µT</p>
    <p><b>Z:</b> <span id="Magneto_Z">%Magneto_Z%</span> µT</p>
  </div>
  <div class="card">
    <p class="card_heading">Temperature (MPU9250)</p>
    <p><span id="Temperature_MPU9250">%Temperature_MPU9250%</span> &deg;C</p>
  </div>
</div>
<div class="cards">
  <div class="card">
    <p class="card_heading">Light</p>
    <p><span id="Light">%LIGHT%</span> lux</p>
  </div>
</div>
<div class="cards">
  <div class="card">
    <p class="card_heading">Temperature (BME280)</p>
    <p><span id="Temperature_BME280">%Temperature_BME280%</span> &deg;C</p>
  </div>
  <div class="card">
    <p class="card_heading">Humidity</p>
    <p><span id="Humidity">%HUMIDITY%</span> &percnt;</p>
  </div>
  <div class="card">
    <p class="card_heading">Pressure</p>
    <p><span id="Pressure">%PRESSURE%</span> Pa</p>
  </div>
  <div class="card">
    <p class="card_heading">Altitude</p>
    <p><span id="Altitude_BME280">%ALTITUDE%</span> m</p>
  </div>
</div>
<div class="cards">
  <div class="card">
    <p class="card_heading">Location</p>
    <p><b>Latitude:</b> <span id="Latitude">%LATITUDE%</span> &deg;</p>
    <p><b>Longitude:</b> <span id="Longitude">%LONGITUDE%</span> &deg;</p>
  </div>
  <div class="card">
    <p class="card_heading">Speed</p>
    <p><span id="Speed">%SPEED%</span> m/s</p>
  </div>
  <div class="card">
    <p class="card_heading">Course</p>
    <p><span id="Course">%COURSE%</span> &deg;</p>
  </div>
  <div class="card">
    <p class="card_heading">Altitude (GPS)</p>
    <p><span id="Altitude_GPS">%ALTITUDE_GPS%</span> m</p>
  </div>
  <div class="card">
    <p><b>Satellites:</b> <span id="Satellites">%SATELLITES%</span></p>
    <p><b>HDOP:</b> <span id="HDOP">%HDOP%</span></p>
  </div>
</div>
<div class="cards">
  <div class="card">
    <p class="card_heading">MQ 2</p>
    <p><span id="MQ_2">%MQ_2%</span></p>
  </div>
    <div class="card">
    <p class="card_heading">MQ 3</p>
    <p><span id="MQ_3">%MQ_3%</span><p>
  </div>
    <div class="card">
    <p class="card_heading">MQ 4</p>
    <p><span id="MQ_4">%MQ_4%</span></p>
  </div>
    <div class="card">
    <p class="card_heading">MQ 5</p>
    <p><span id="MQ_5">%MQ_5%</span></p>
  </div>
    <div class="card">
    <p class="card_heading">MQ 6</p>
    <p><span id="MQ_6">%MQ_6%</span></p>
  </div>
    <div class="card">
    <p class="card_heading">MQ 7</p>
    <p><span id="MQ_7">%MQ_7%</span></p>
  </div>
    <div class="card">
    <p class="card_heading">MQ 8</p>
    <p><span id="MQ_8">%MQ_8%</span></p>
  </div>
    <div class="card">
    <p class="card_heading">MQ 9</p>
    <p><span id="MQ_9">%MQ_9%</span></p>
  </div>
    <div class="card">
    <p class="card_heading">MQ 135</p>
    <p><span id="MQ_135">%MQ_135%</span></p>
  </div>
</div>
<div class="cards">
  <div class="card">
    <p class="card_heading">Relay 1</p>
    <table>
      <tbody>
        <tr>
          <td>
            <button type="button" class="relay_button" data-number="1" data-state="1">On</button>
          </td>
          <td>
            <button type="button" class="relay_button" data-number="1" data-state="0">Off</button>
          </td>
        </tr>
      </tbody>
    </table>
   </div>
  <div class="card">
    <p class="card_heading">Relay 2</p>
    <table>
      <tbody>
        <tr>
          <td>
            <button type="button" class="relay_button" data-number="2" data-state="1">On</button>
          </td>
          <td>
            <button type="button" class="relay_button" data-number="2" data-state="0">Off</button>
          </td>
        </tr>
      </tbody>
    </table>
   </div>
</div>
<div class="cards">
  <div class="card">
    <p class="card_heading">WiFi Access Points</p>
    <p>
      <span id="WiFi_Scan_Result"></span>
    </p>
    <p>
      <button type="button" id="Scan_WiFi">Scan</button>
    </p>
   </div>
</div>
<div class="cards">
  <div class="card">
    <p class="card_heading">I2C Devices</p>
    <p>
      <span id="I2C_Scan_Result"></span>
    </p>
    <p>
      <button type="button" id="Scan_I2C">Scan</button>
    </p>
   </div>
</div>
<div id="BlackBody_Motion" title="Blackbody Motion">M</div>
<div id="Flame" title="Flame">F</div>
<script>
function Show_Alert(message) {
  if (typeof message !== "undefined") {
    if (message !== "") {
      var alert_box = document.createElement("div");
      alert_box.innerHTML = message;
      alert_box.classList.add("alert_box");
      alert_box.classList.add("show");
      document.body.append(alert_box);
      setTimeout(function () {
        alert_box.classList.remove("show");
      }, 3000);
      setTimeout(function () {
        alert_box.remove();
      }, 4000);
    } else {
      return false;
    }
  } else {
    return false;
  }
}

document.oncontextmenu = function (contextmenu) {
  contextmenu.preventDefault();
  Show_Alert("Context menu is not allowed");
  return false;
};

document.onkeydown = function (keyboard) {
  if (
    keyboard.key == "F12" ||
    (keyboard.ctrlKey && keyboard.shiftKey && keyboard.key == "i") ||
    (keyboard.ctrlKey && keyboard.shiftKey && keyboard.key == "j") ||
    (keyboard.ctrlKey && keyboard.key == "u")
  ) {
    keyboard.preventDefault();
    Show_Alert("Developer tools are not allowed");
    return false;
  } else if (
    keyboard.Code == "PrintScreen" ||
    (keyboard.ctrlKey && keyboard.key == "p")
  ) {
    keyboard.preventDefault();
    Show_Alert("Printing is not allowed");
    return false;
  } else if (keyboard.ctrlKey && keyboard.key == "s") {
    keyboard.preventDefault();
    Show_Alert("Saving is not allowed");
    return false;
  }
};

if (!!window.EventSource) {
  var source = new EventSource("/Events");
  source.addEventListener(
    "open",
    function () {
      Show_Alert("Connected");
    },
    false
  );
  source.addEventListener(
    "error",
    function (e) {
      if (e.target.readyState != EventSource.OPEN) {
        Show_Alert("Disconnected");
      }
    },
    false
  );
  source.addEventListener(
    "json",
    function (crude_json) {      
      var UpTime_Milliseconds = parseInt(crude_json.lastEventId);
      var Parsed_JSON = JSON.parse(crude_json.data);
      
      var UpTime = (UpTime_Milliseconds / 1000).toFixed(2);
      
      document.getElementById("UpTime").innerHTML = UpTime;
      
      document.getElementById("Acceleration_X").innerHTML = Parsed_JSON.Acceleration_X;
      document.getElementById("Acceleration_Y").innerHTML = Parsed_JSON.Acceleration_Y;
      document.getElementById("Acceleration_Z").innerHTML = Parsed_JSON.Acceleration_Z;
      document.getElementById("Resultant_Acceleration").innerHTML = Parsed_JSON.Resultant_Acceleration;
      
      document.getElementById("Gyro_X").innerHTML = Parsed_JSON.Gyro_X;
      document.getElementById("Gyro_Y").innerHTML = Parsed_JSON.Gyro_Y;
      document.getElementById("Gyro_Z").innerHTML = Parsed_JSON.Gyro_Z;
      
      document.getElementById("Magneto_X").innerHTML = Parsed_JSON.Magneto_X;
      document.getElementById("Magneto_Y").innerHTML = Parsed_JSON.Magneto_Y;
      document.getElementById("Magneto_Z").innerHTML = Parsed_JSON.Magneto_Z;

      document.getElementById("Light").innerHTML = Parsed_JSON.Light;

      document.getElementById("Temperature_MPU9250").innerHTML = Parsed_JSON.Temperature_MPU9250;

      document.getElementById("Temperature_BME280").innerHTML = Parsed_JSON.Temperature_BME280;

      document.getElementById("Humidity").innerHTML = Parsed_JSON.Humidity;
      document.getElementById("Pressure").innerHTML = Parsed_JSON.Pressure;
      document.getElementById("Altitude_BME280").innerHTML = Parsed_JSON.Altitude_BME280;

      document.getElementById("Latitude").innerHTML = Parsed_JSON.Latitude;
      document.getElementById("Longitude").innerHTML = Parsed_JSON.Longitude;
      document.getElementById("Speed").innerHTML = Parsed_JSON.Speed;
      document.getElementById("Course").innerHTML = Parsed_JSON.Course;
      document.getElementById("Altitude_GPS").innerHTML = Parsed_JSON.Altitude_GPS;
      document.getElementById("Satellites").innerHTML = Parsed_JSON.Satellites;
      document.getElementById("HDOP").innerHTML = Parsed_JSON.HDOP;

      document.getElementById("MQ_2").innerHTML = Parsed_JSON.MQ_2;
      document.getElementById("MQ_3").innerHTML = Parsed_JSON.MQ_3;
      document.getElementById("MQ_4").innerHTML = Parsed_JSON.MQ_4;
      document.getElementById("MQ_5").innerHTML = Parsed_JSON.MQ_5;
      document.getElementById("MQ_6").innerHTML = Parsed_JSON.MQ_6;
      document.getElementById("MQ_7").innerHTML = Parsed_JSON.MQ_7;
      document.getElementById("MQ_8").innerHTML = Parsed_JSON.MQ_8;
      document.getElementById("MQ_9").innerHTML = Parsed_JSON.MQ_9;
      document.getElementById("MQ_135").innerHTML = Parsed_JSON.MQ_135;

      document.getElementById("Slave_UpTime").innerHTML = (parseInt(Parsed_JSON.Slave_UpTime) / 1000).toFixed(2);

      if(parseInt(Parsed_JSON.BlackBody_Motion) == 1) {
      document.getElementById("BlackBody_Motion").style.display = "flex";
      } else {
      document.getElementById("BlackBody_Motion").style.display = "none";
      }

      if(parseInt(Parsed_JSON.Flame) == 1) {
      document.getElementById("Flame").style.display = "flex";
      } else {
      document.getElementById("Flame").style.display = "none";
      }
    },
    false
  );
}

  Array.prototype.forEach.call(
    document.querySelectorAll(".relay_button"),
    function (relay_button) {
      relay_button.onclick = function() {
      fetch("/relays?number=" + relay_button.dataset.number + "&state=" + relay_button.dataset.state, {
    method: "GET",
  })
    .then(function (response) {
      return response.text();
    })
    .then(function (realy_state) {
      Show_Alert(realy_state);
    });    
        }
    }
  );

var Scan_WiFi_Button = document.getElementById("Scan_WiFi");
Scan_WiFi_Button.onclick = function () {
  Scan_WiFi_Button.innerHTML = "Scanning";

  fetch("/Scan_WiFi", {
    method: "GET",
  })
    .then(function (response) {
      return response.text();
    })
    .then(function (WiFi_Scan_Result) {
      if(WiFi_Scan_Result == "") {
      Scan_WiFi_Button.click();
      } else {
      document.getElementById("WiFi_Scan_Result").innerHTML = WiFi_Scan_Result;
      Scan_WiFi_Button.innerHTML = "Scan";
      Show_Alert("WiFi Scanned");
      }
    });
};

var Scan_I2C_Button = document.getElementById("Scan_I2C");
Scan_I2C_Button.onclick = function () {
  Scan_I2C_Button.innerHTML = "Scanning";
  fetch("/Scan_I2C", {
    method: "GET",
  })
    .then(function (response) {
      return response.text();
    })
    .then(function (I2C_Scan_Result) {
      document.getElementById("I2C_Scan_Result").innerHTML = I2C_Scan_Result;
      Scan_I2C_Button.innerHTML = "Scan";
      Show_Alert("I2C Bus Scanned");
    });
};
</script>
</body>
</html>)rawliteral";



String Process_Page(const String & var) {
  int chip_id = 0;

  for (int i = 0; i < 17; i = i + 8) {
    chip_id |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }

  Get_Readings();

  if (var == "CHIP_MODEL") {
    return String(ESP.getChipModel());
  } else if (var == "CHIP_REVISION") {
    return String(ESP.getChipRevision());
  } else if (var == "CHIP_CORES") {
    return String(ESP.getChipCores());
  } else if (var == "CHIP_ID") {
    return String(chip_id);
  } else if (var == "MAC_ADDRESS") {
    return String(WiFi.macAddress());
  } else if (var == "HOST_NAME") {
    return String(WiFi.getHostname());
  } else if (var == "LOCAL_IP_ADDRESS") {
    return String(WiFi.localIP().toString().c_str());
  } else if (var == "COMPILE_DATE_TIME") {
    return String(Compile_Date_Time);
  } else if (var == "UPTIME") {
    return String(millis() / 1000);
  } else if (var == "SLAVE_UPTIME") {
    return String(Slave_UpTime);
  } else if (var == "Acceleration_X") {
    return String(Acceleration_X);
  } else if (var == "Acceleration_Y") {
    return String(Acceleration_Y);
  } else if (var == "Acceleration_Z") {
    return String(Acceleration_Z);
  } else if (var == "Resultant_Acceleration") {
    return String(Resultant_Acceleration);
  } else if (var == "Gyro_X") {
    return String(Gyro_X);
  } else if (var == "Gyro_Y") {
    return String(Gyro_Y);
  } else if (var == "Gyro_Z") {
    return String(Gyro_Z);
  } else if (var == "Magneto_X") {
    return String(Magneto_X);
  } else if (var == "Magneto_Y") {
    return String(Magneto_Y);
  } else if (var == "Magneto_Z") {
    return String(Magneto_Z);
  } else if (var == "LIGHT") {
    return String(Light);
  } else if (var == "Temperature_MPU9250") {
    return String(Temperature_MPU9250);
  } else if (var == "Temperature_BME280") {
    return String(Temperature_BME280);
  } else if (var == "HUMIDITY") {
    return String(Humidity);
  } else if (var == "PRESSURE") {
    return String(Pressure);
  } else if (var == "ALTITUDE") {
    return String(Altitude_BME280);
  } else if (var == "LATITUDE") {
    return String(Latitude);
  } else if (var == "LONGITUDE") {
    return String(Longitude);
  } else if (var == "SPEED") {
    return String(Speed);
  } else if (var == "COURSE") {
    return String(Course);
  } else if (var == "ALTITUDE_GPS") {
    return String(Altitude_GPS);
  } else if (var == "SATELLITES") {
    return String(Satellites);
  } else if (var == "HDOP") {
    return String(HDOP);
  } else if (var == "MQ_2") {
    return String(MQ_2);
  } else if (var == "MQ_3") {
    return String(MQ_3);
  } else if (var == "MQ_4") {
    return String(MQ_4);
  } else if (var == "MQ_5") {
    return String(MQ_5);
  } else if (var == "MQ_6") {
    return String(MQ_6);
  } else if (var == "MQ_7") {
    return String(MQ_7);
  } else if (var == "MQ_8") {
    return String(MQ_8);
  } else if (var == "MQ_9") {
    return String(MQ_9);
  } else if (var == "MQ_135") {
    return String(MQ_135);
  }

  return String();
}



void Handle_Root(AsyncWebServerRequest * request) {
  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html; charset=utf-8", xhtml, Process_Page);
  Add_Common_Headers(response);
  request->send(response);
}



void Assign_URLs(void) {
  Server.on("/Scan_I2C", HTTP_GET, Handle_I2C_Scan);

  Server.on("/relays", HTTP_GET, Handle_Relays);

  Server.on("/Scan_WiFi", HTTP_GET, Handle_WiFi_Scan);

  Server.on("/", HTTP_GET, Handle_Root);
}



void SetUp_Server(void) {
  Assign_URLs();

  Server.addHandler(&Events);

  Server.onNotFound(Handle_Error_404);

  Server.begin();
}



void setup(void) {
  Serial.begin(115200);

  RDM6300.begin(RDM6300_Pin);
  pinMode(Authentication_LED_Pin, OUTPUT);
  Authenticate();

  Wire.begin();

  // Serial.println(Scan_I2C());

  SetUp_MPU9250();

  SetUp_BH1750();

  SetUp_BME280();

  SIM800L__NEO7M.begin(9600);

  pinMode(RCWL0516_Pin, INPUT);

  pinMode(Flame_Sensor_Pin, INPUT);

  Slave_Board.begin(115200);

  SetUp_Relays();

  Morse_Code.begin(Buzzer_LED_Pin);

  SetUp_WiFi();

  SetUp_Server();

  Morse_Code.print("S");
}



void loop(void) {
  if ((millis() - Last_SSE_Time) > SSE_Delay) {
    StaticJsonDocument<1000> Readings;

    Get_Readings();

    Readings["Acceleration_X"] = String(Acceleration_X);
    Readings["Acceleration_Y"] = String(Acceleration_Y);
    Readings["Acceleration_Z"] = String(Acceleration_Z);
    Readings["Resultant_Acceleration"] = String(Resultant_Acceleration);

    Readings["Gyro_X"] = String(Gyro_X);
    Readings["Gyro_Y"] = String(Gyro_Y);
    Readings["Gyro_Z"] = String(Gyro_Z);

    Readings["Magneto_X"] = String(Magneto_X);
    Readings["Magneto_Y"] = String(Magneto_Y);
    Readings["Magneto_Z"] = String(Magneto_Z);

    Readings["Light"] = String(Light);

    Readings["Temperature_MPU9250"] = String(Temperature_MPU9250);

    Readings["Temperature_BME280"] = String(Temperature_BME280);

    Readings["Humidity"] = String(Humidity);
    Readings["Pressure"] = String(Pressure);
    Readings["Altitude_BME280"] = String(Altitude_BME280);

    Readings["Latitude"] = String(Latitude);
    Readings["Longitude"] = String(Longitude);
    Readings["Speed"] = String(Speed);
    Readings["Course"] = String(Course);
    Readings["Altitude_GPS"] = String(Altitude_GPS);
    Readings["Satellites"] = String(Satellites);
    Readings["HDOP"] = String(HDOP);

    Readings["BlackBody_Motion"] = String(BlackBody_Motion);

    Readings["Flame"] = String(Flame);

    Readings["MQ_2"] = String(MQ_2);
    Readings["MQ_3"] = String(MQ_3);
    Readings["MQ_4"] = String(MQ_4);
    Readings["MQ_5"] = String(MQ_5);
    Readings["MQ_6"] = String(MQ_6);
    Readings["MQ_7"] = String(MQ_7);
    Readings["MQ_8"] = String(MQ_8);
    Readings["MQ_9"] = String(MQ_9);
    Readings["MQ_135"] = String(MQ_135);

    Readings["Slave_UpTime"] = String(Slave_UpTime);

    String JSON_String;
    serializeJson(Readings, JSON_String);

    Last_SSE_Time = millis();
    Events.send(JSON_String.c_str(), "json", Last_SSE_Time);
  }
}



/* Links:

  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

  https://github.com/me-no-dev/AsyncTCP

  https://github.com/me-no-dev/ESPAsyncWebServer
  
  https://github.com/arduino12/rdm6300

  https://raw.githubusercontent.com/RuiSantosdotme/Random-Nerd-Tutorials/master/Projects/ESP32/ESP32_Server_Sent_Events.ino

*/
