//#include <SoftwareSerial.h>
#include <Wire.h>
#include "lpr9204.h"
#define HYT_ADDR 0x28
#define BAUDRATE 9600

double humidity;
double temperature;
int packet_id = 0;
int my_id = 0;
//SoftwareSerial sSerial(P2_3, P2_4); // RX, TX

void setup() {
  Serial.begin(BAUDRATE); // Communication with LPR9204
  //  sSerial.begin(9600); // ソフトウェアシリアルの初期化
  //  sSerial.println("Ready");
  delay(1000);
  while (my_id == 0) {
    my_id = init_lpr9204();
  }
  Wire.setModule(0);
  Wire.begin();
  pinMode(RED_LED, OUTPUT);
  digitalWrite(RED_LED, HIGH);
}

void loop() {
  int sleep_time = 1;
  awake_lpr9204();
  get_temperature_by_wire();
  int no_resend = send_temperature_until_ack_lpr9204( packet_id, temperature, humidity );
  
  /*
    while (1) {
    read_serial();
    sSerial.println(get_serial_read());
    if ( event_is("ERXDATA") ) {
      if ( command_is("RESEND") ) {
        if ( request_for_me(my_id) ) {
          send_temperature_until_ack_lpr9204( packet_id, temperature, humidity );
        }
      } else if ( command_is("SLEEP") ) {
        packet_id = get_packet_id(); // n+1される
        sleep_time = get_sleep_time();
        break;
      }
    }
    delay(100);
    }
    delay(1000);
    sleep_lpr9204();
  */
  delay( sleep_time * 1000 );
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



