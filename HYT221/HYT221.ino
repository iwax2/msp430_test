#include <Wire.h>
//#include <SoftwareSerial.h>
#define HYT_ADDR 0x28
#define WAKEUP_SEC 0
#define SLEEP_SEC 10
#define RESET P2_1
#define WAKEUP P2_2

char s[12];
double humidity;
double temperature;
//SoftwareSerial mySerial(P2_1, P2_2); // RX, TX

void setup() {
  Wire.setModule(0);
  Wire.begin();             // Join I2c Bus as master
  Serial.begin(115200);
  //  Serial.begin(9600); // ハードウェアシリアルを準備
  //  Serial.println("Ready");
  //  mySerial.begin(9600); // ソフトウェアシリアルの初期化
  delay(100);
  Serial.println("SKSREG S1 0002");
  delay(100);
  Serial.println("SKSREG S8 34");
  delay(100);
  Serial.println("SKPING 0001");
  delay(100);
//  Serial.println("SKSETPS A 0"); // 0秒でwakeup、10秒でsleepの間欠動作
  pinMode(RESET, OUTPUT);
  pinMode(WAKEUP, OUTPUT);
  digitalWrite(RESET, HIGH);
  digitalWrite(WAKEUP, HIGH);
}


void loop() {
  wire_hyt221();
  sleep_all();
}

void sleep_all() {
  Serial.println("SKSYNCREQ 0001");
  delay(100);
  Serial.println("SKSLEEP");
  delay(30000);
  Serial.println("SKSEND 1 1000 0001 B 00.00 00.00"); // テスト
  delay(29000);
  digitalWrite(WAKEUP, LOW);
  delay(10); // min 5msのWAKEUP入力が必要（立ち上がりエッジで起動）
  digitalWrite(WAKEUP, HIGH);
  delay(5); // max 5msで起動
}

void wire_hyt221() {
  Wire.beginTransmission(HYT_ADDR);   // Begin transmission with given device on I2C bus
  Wire.requestFrom(HYT_ADDR, 4);      // Request 4 bytes
  // Read the bytes if they are available
  // The first two bytes are humidity the last two are temperature
  if (Wire.available() == 4) {
    int b1 = Wire.read();
    int b2 = Wire.read();
    int b3 = Wire.read();
    int b4 = Wire.read();

    // combine humidity bytes and calculate humidity
    int rawHumidity = b1 << 8 | b2;
    // compound bitwise to get 14 bit measurement first two bits
    // are status/stall bit (see intro text)
    rawHumidity =  (rawHumidity &= 0x3FFF);
    humidity = 100.0 / pow(2, 14) * rawHumidity;

    // combine temperature bytes and calculate temperature
    b4 = (b4 >> 2); // Mask away 2 least significant bits see HYT 221 doc
    int rawTemperature = b3 << 6 | b4;
    temperature = 165.0 / pow(2, 14) * rawTemperature - 40;

    //    Serial.print(humidity);
    //    Serial.print("% - Temperature: ");
    //    Serial.println(temperature);
  }
  else {
    //    Serial.println("Not enough bytes available on wire.");
  }
  send_temperature();
  Wire.endTransmission();           // End transmission and release I2C bus
}

void send_temperature() {
  Serial.print("SKSEND 1 1000 0001 B ");
  Serial.print(temperature, 2);
  Serial.print(",");
  Serial.print(humidity, 2);
  Serial.println();
}


