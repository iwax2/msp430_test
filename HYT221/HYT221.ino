#include <Wire.h>
#include "lpr9204.h"
#define HYT_ADDR 0x28
#define BAUDRATE 9600

double humidity = 98.76;
double temperature = 12.34;
int packet_id = 0;
int my_id = 0;

void setup() {
  pinMode(RED_LED, OUTPUT);
  digitalWrite(RED_LED, LOW);
  Serial.begin(BAUDRATE); // Communication with LPR9204
  while (my_id == 0) {
    my_id = init_lpr9204();
  }
  Wire.setModule(0);
  Wire.begin();
}

void loop() {
  int sleep_time = 10;
  awake_lpr9204();
  digitalWrite(RED_LED, LOW);
  delay(100);
  get_temperature_by_wire();
  int no_resend = send_temperature_until_ack_lpr9204( packet_id, temperature, humidity );

  while (1) {
    if ( !read_serial(30 * 1000) ) { // 30秒以上応答がなければbreakする
      sleep_time = 30;
      packet_id = (packet_id + 1) % 10; // n+1される
      blink_times(10);
      break;
    }
    if ( event_is("ERXDATA") ) {
      if ( command_is("RSEND") ) {
        blink_times(3);
        int pid = request_for_me(my_id); // packet_IDは0～9
        if ( pid >= 0 ) {
          send_temperature_until_ack_lpr9204( pid, temperature, humidity );
        }
      } else if ( command_is("SLEEP") ) {
        int p = get_packet_id();
        if ( p < 0 ) {
          packet_id = (packet_id + 1) % 10; // n+1される
        } else {
          packet_id = p;
        }
        int s = get_sleep_time();
        if ( s == 0 ) {
          sleep_time = 50;
        } else {
          sleep_time = s;
        }
        break;
      }
    }
  }
  delay(1000);
  sleep_lpr9204();
  digitalWrite(RED_LED, HIGH);
  for ( int i = 0; i < 100; i++ ) {
    delay( sleep_time * 10 );
  }
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


