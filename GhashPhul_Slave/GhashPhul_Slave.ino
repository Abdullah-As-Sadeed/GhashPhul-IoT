/* by Abdullah As-Sadeed */



#define Master_Board Serial2



#include "ArduinoJson.h"



#define MQ_2 A0
#define MQ_3 A1
#define MQ_4 A2
#define MQ_5 A3
#define MQ_6 A4
#define MQ_7 A5
#define MQ_8 A6
#define MQ_9 A7
#define MQ_135 A8



float MQ_2_Value, MQ_3_Value, MQ_4_Value, MQ_5_Value, MQ_6_Value, MQ_7_Value, MQ_8_Value, MQ_9_Value, MQ_135_Value;



void setup(void) {
  Master_Board.begin(115200);
}



void loop(void) {
  StaticJsonDocument<1000> Readings;

  MQ_2_Value = analogRead(MQ_2);
  MQ_3_Value = analogRead(MQ_3);
  MQ_4_Value = analogRead(MQ_4);
  MQ_5_Value = analogRead(MQ_5);
  MQ_6_Value = analogRead(MQ_6);
  MQ_7_Value = analogRead(MQ_7);
  MQ_8_Value = analogRead(MQ_8);
  MQ_9_Value = analogRead(MQ_9);
  MQ_135_Value = analogRead(MQ_135);

  Readings["MQ_2"] = String(MQ_2_Value);
  Readings["MQ_3"] = String(MQ_3_Value);
  Readings["MQ_4"] = String(MQ_4_Value);
  Readings["MQ_5"] = String(MQ_5_Value);
  Readings["MQ_6"] = String(MQ_6_Value);
  Readings["MQ_7"] = String(MQ_7_Value);
  Readings["MQ_8"] = String(MQ_8_Value);
  Readings["MQ_9"] = String(MQ_9_Value);
  Readings["MQ_135"] = String(MQ_135_Value);

  Readings["UpTime"] = String(millis());

  serializeJson(Readings, Master_Board);

  delay(100);
}
