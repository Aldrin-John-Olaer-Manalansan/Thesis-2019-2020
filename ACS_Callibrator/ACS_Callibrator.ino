#include "ACS712.h"

/*
  This example shows how to measure the power consumption
  of devices in 230V electrical system
  or any other system with alternative current
*/

// We have 30 amps version sensor connected to A1 pin of arduino
// Replace with your version if necessary
ACS712 sensor1(ACS712_05B, A0);
ACS712 sensor2(ACS712_05B, A1);
float ACS_Callibrator[2];
void setup() {
  pinMode(6,OUTPUT);
  pinMode(7,OUTPUT);
  digitalWrite(6,LOW);
  digitalWrite(7,LOW);
  Serial.begin(9600);

  // This method calibrates zero point of sensor,
  // It is not necessary, but may positively affect the accuracy
  // Ensure that no current flows through the sensor at this moment
  delay(3000);
  sensor1.setZeroPoint(sensor1.calibrate());
  sensor2.setZeroPoint(sensor2.calibrate());
  delay(1000);
  ACS_Callibrator[0]=sensor1.getCurrentAC(60);
  ACS_Callibrator[1]=sensor2.getCurrentAC(60);
}

void loop() {
  static unsigned long timer=millis(),analog0=0,analog1=0;
  static double mA1=0.0,mA2=0.0;
  static uint16_t samples=0;
  mA1+=sensor1.getCurrentAC(60)-ACS_Callibrator[0];
  mA2+=sensor2.getCurrentAC(60)-ACS_Callibrator[1];
  analog0+=analogRead(A0);
  analog1+=analogRead(A1);
  samples++;
    //Serial.println(String("analog1:") + analogRead(A0));
   // Serial.println(String("analog2:") + analogRead(A0));
  /*// To calculate the power we need voltage multiplied by current
  double A=sensor1.getCurrentAC(60)-ACS_Callibrator[0];
  Serial.println(String("A1 = ") + A + " mAmps");
  double P = 220.0 * A/1000.0;
  Serial.println(String("P1 = ") + P + " Watts");
  A=sensor2.getCurrentAC(60)-ACS_Callibrator[1];
  Serial.println(String("A2 = ") + A + " mAmps");
  P = 220.0 * A/1000.0;
  Serial.println(String("P2 = ") + P + " Watts"); */

    Serial.println(String(" analog1:") + analogRead(A0));
    Serial.println(String(" analog2:") + analogRead(A1));
    Serial.println(String("average analog1:") + analog0/samples);
    Serial.println(String("average analog2:") + analog1/samples);
  if (millis()-timer>60e3)
  {
    Serial.println(String("average analog1:") + analog0/samples);
    Serial.println(String("average analog2:") + analog1/samples);
    double value=mA1/samples;
    Serial.println(String("average noise1:") + value);
    value=mA2/samples;
    Serial.println(String("average noise2:") + value);
    Serial.print(digitalRead(6));
    Serial.println(digitalRead(7));
    digitalWrite(6,digitalRead(6)?LOW:HIGH);
    digitalWrite(7,digitalRead(7)?LOW:HIGH);
    delay(3000);
    timer=millis();
    mA1=0.0; mA2=0.0;analog0=0;analog1=0;samples=0;;
  }
}
