#include <SoftwareSerial.h>
#define RESET P2_1
#define WAKEUP P2_2
#define SERIAL_BUFFER 100

int countup = 0;
char serial_read[SERIAL_BUFFER];
SoftwareSerial sSerial(P2_3, P2_4); // RX, TX

void setup() {
  pinMode(RESET, OUTPUT);
  pinMode(WAKEUP, OUTPUT);
  digitalWrite(RESET, LOW);
  delay(100);
  digitalWrite(RESET, HIGH);
  digitalWrite(WAKEUP, HIGH);
  Serial.begin(9600);
  sSerial.begin(9600); // ソフトウェアシリアルの初期化
  sSerial.println("Ready");
}


void loop() {
  if ( strcmp(serial_read, "OK") == 0) {
//  if ( serial_read[0] == 'o') {
    sSerial.print(serial_read[0], HEX);
    sSerial.print(serial_read[1], HEX);
    sSerial.print(serial_read[2], HEX);
    sSerial.print(serial_read[3], HEX);
    sSerial.print(serial_read[4], HEX);
    sSerial.print(serial_read[5], HEX);
    sSerial.print(serial_read[6], HEX);
    sSerial.print(serial_read[7], HEX);
    sSerial.print(serial_read[8], HEX);
    sSerial.println(serial_read[0], HEX);
    Serial.println("SKPING 0001");
  }
  serial_read[0] = '\0';
  read_serial();
  sSerial.println(serial_read);
}

void read_serial() {
  int i = 0;
  while (true) {
    if (Serial.available()) {
      char c = Serial.read();
      if ( i >= (SERIAL_BUFFER - 1) || c == '\n' ) {
        serial_read[i] = '\0';
        break;
      } else {
        serial_read[i++] = c;
      }
    }
  }
  /*
  sSerial.println(serial_read);
  sSerial.print("Before: ");
  char l = strlen(serial_read)%10+'0';
  sSerial.println(l);
  sSerial.print("After : ");
 l = strlen(serial_read)%10+'0';
  sSerial.println(l);
  sSerial.println("Finish read_serial");
  */
  // CRの削除
  if (strlen(serial_read) > 0) {
    serial_read[strlen(serial_read) - 1] = '\0';
  }
}

void send_test() {
  char str[24] = "SKSEND 1 1000 0001 2 0";
  char c = (countup++ % 10) + '0';
  str[22] = c;
  sSerial.println(str);
  sSerial.listen();
  while (sSerial.available() < 1);
  while (sSerial.available()) {
    Serial.print(sSerial.read(), HEX);
    Serial.print(' ');
    delay(100);
  }
  Serial.println("Finish transmission");
  //  sSerial.print("SKSEND 1 1000 0002 1 " + c);
  //  Serial.println(sSerial.read());
  delay(1000);
  //  sSerial.println("SKPING 0001");
  //  Serial.println(sSerial.read());
  //  sSerial.println("SKPING 0002");
  //  Serial.println(sSerial.read());
  delay(1000);
  //sleep_all();
  while (sSerial.available() < 1);
  while (sSerial.available()) {
    Serial.print(sSerial.read());
    Serial.print('_');
    delay(100);
  }
  Serial.println("Next transmission");

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

