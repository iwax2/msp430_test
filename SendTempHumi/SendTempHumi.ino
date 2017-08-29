#include <Wire.h>
#include "lpr9204.h"
#define HYT_ADDR 0x28
#define BAUDRATE 9600

double humidity = 98.76;
double temperature = 12.34;
int my_id = 0;

void setup() {
  pinMode(RED_LED, OUTPUT);
  digitalWrite(RED_LED, HIGH);
  Serial.begin(BAUDRATE); // Communication with LPR9204
  while (my_id == 0) {
    my_id = init_lpr9204();
  }
  my_id = my_id % 10;
  Wire.setModule(0);
  Wire.begin();
}

void loop() {
  digitalWrite(RED_LED, LOW);
  get_temperature_by_wire();
  send_temperature_lpr9204( my_id, temperature, humidity );
  digitalWrite(RED_LED, HIGH);
  delay(1000);
}



bool get_temperature_by_wire() {
  bool is_available = false;
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

    //    sSerial.print(humidity);
    //    sSerial.print("% - Temperature: ");
    //    sSerial.println(temperature);
    is_available = true;
  } else {
    //    Serial.println("Not enough bytes available on wire.");
    is_available = false;
  }
  Wire.endTransmission();           // End transmission and release I2C bus
  return (is_available);
}


