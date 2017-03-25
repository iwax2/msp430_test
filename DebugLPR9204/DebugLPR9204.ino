#include <SoftwareSerial.h>
#define RESET P2_1
#define WAKEUP P2_2
#define SERIAL_BUFFER 100

struct packet_t {
  unsigned int origin; // 受信したデータの送信元ID
  unsigned int destination; // 受信したデータの宛先ID
  unsigned int message_id; // 受信したデータの識別信号
  unsigned int selector; // 宛先セレクタ
  unsigned int rssi; // 受信RSSI/2-130 = dBm
  unsigned int data_length; // 受信データ長
  char *data; // 受信データ内容
  unsigned int *route; // 受信したデータの途中経路（宛先IDを常に含む）
};

int countup = 0;
char serial_read[SERIAL_BUFFER];
SoftwareSerial sSerial(P2_3, P2_4); // RX, TX
struct packet_t packet;

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
  read_serial();
  sSerial.println(serial_read);
  if ( strcmp(serial_read, "OK") == 0) {
    //    Serial.println("SKPING 0001");
  }
  if ( event_is("ERXDATA") ) {
    parse_data();
  }
}

/*
   発生したイベントがnameであるかを確かめます
   nameの長さはSERIAL_BUFFER以下であることを確認してください
*/
bool event_is( char* name ) {
  int len = strlen(name);
  for ( int i = 0; i < len; i++ ) {
    if ( name[i] != serial_read[i] ) {
      return (false);
    }
  }
  return true;
}

/*
   発生したERXDATAイベントを解析し，packet構造体に格納します
*/
void parse_data() {

}

/*
   16進文字列をunsigned intに変換します
*/
unsigned int hexstr2int( char* hexstr ) {
  unsigned int data = 0;
  unsigned int base = 1;
  int len = strlen(hexstr);
  for ( int i = 0; i < len; i++ ) {
    if ( '0' <= hexstr[i] && hexstr[i] <= '9'  ) {
      data += (hexstr[i] - '0') * base;
    }
    base *= 16;
  }
  return (data);
}
/*
   unsigned intを指定した文字列数の16進文字列に変換します
   hexstr = (char *)malloc(sizeof(char)*len+1)
*/
char* int2hexstr( char* hexstr, int len, unsigned int data ) {
  for ( int i = 0; i < len; i++ ) {
    if ( data > 0 ) {
      hexstr[i] = data % 16;
      data /= 16;
    } else {
      hexstr[i] = '0';
    }
  }

}

void read_serial() {
  serial_read[0] = '\0';
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

