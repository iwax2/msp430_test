#include <SoftwareSerial.h>
#define WAKEUP_SEC 0
#define SLEEP_SEC 10
#define RESET P2_1
#define WAKEUP P2_2

int i = 0;
SoftwareSerial sSerial(P2_3, P2_4); // RX, TX

void setup() {
  Serial.begin(9600);
  Serial.println("Ready");
  sSerial.begin(9600); // ソフトウェアシリアルの初期化
  pinMode(RESET, OUTPUT);
  pinMode(WAKEUP, OUTPUT);
  digitalWrite(RESET, LOW);
  digitalWrite(WAKEUP, LOW);
  delay(10);
  digitalWrite(RESET, HIGH);
  digitalWrite(WAKEUP, HIGH);
  delay(1000);
}


void loop() {
  char str[24] = "SKSEND 1 1000 0001 1 0";
  char c = (i++ % 10) + '0';
  str[0] = c;
  Serial.println(c);
  sSerial.println(str);
  //  Serial.println(sSerial.read());
  //  sSerial.print("SKSEND 1 1000 0002 1 " + c);
  //  Serial.println(sSerial.read());
  delay(1000);
  //  sSerial.println("SKPING 0001");
  //  Serial.println(sSerial.read());
  //  sSerial.println("SKPING 0002");
  //  Serial.println(sSerial.read());
  delay(1000);
  //sleep_all();
}

void sleep_all() {
  //  Serial.println("SKSYNCREQ 0001");
  //  delay(100);
  sSerial.println("SKSLEEP");
  delay(7000);
  //  Serial.println("SKSEND 1 1000 0001 B 00.00 00.00"); // テスト
  //  delay(2900);
  digitalWrite(WAKEUP, LOW);
  delay(1000); // min 5msのWAKEUP入力が必要（立ち上がりエッジで起動）
  digitalWrite(WAKEUP, HIGH);
  delay(1000); // max 5msで起動
}

