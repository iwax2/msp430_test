#include <Wire.h>
#define HYT_ADDR 0x28
#define LED RED_LED


double humidity;
double temperature;

void setup() {
  Wire.setModule(0);
  Wire.begin();             // Join I2c Bus as master
  Serial.begin(9600);
  pinMode(LED, OUTPUT);
  Serial.println("Start transimission");
  digitalWrite(LED, LOW);   // turn the LED on (HIGH is the voltage level)
  delay(1000);
}


void loop() {
  digitalWrite(LED, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);
  Wire.beginTransmission(HYT_ADDR);   // Begin transmission with given device on I2C bus
  Wire.requestFrom(HYT_ADDR, 4);      // Request 4 bytes
  if (Wire.available() == 4) {
    int b1 = Wire.read();
    int b2 = Wire.read();
    int b3 = Wire.read();
    int b4 = Wire.read();
    int rawHumidity = b1 << 8 | b2;
    rawHumidity =  (rawHumidity &= 0x3FFF);
    humidity = 100.0 / pow(2, 14) * rawHumidity;
    b4 = (b4 >> 2); // Mask away 2 least significant bits see HYT 221 doc
    int rawTemperature = b3 << 6 | b4;
    temperature = 165.0 / pow(2, 14) * rawTemperature - 40;
    Serial.print(humidity);
    Serial.print("% - Temperature: ");
    Serial.println(temperature);
    Wire.endTransmission();           // End transmission and release I2C bus
    if ( temperature > 10 ) {
      digitalWrite(LED, LOW);
    }
  }
  else {
    Serial.println("Not enough bytes available on wire.");
  }

  delay(1000);
}


