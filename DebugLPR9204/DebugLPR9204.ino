#include <SoftwareSerial.h>
#define RESET P2_1
#define WAKEUP P2_2
#define SERIAL_BUFFER 40

struct packet_t {
  unsigned int origin; // 受信したデータの送信元ID
  //  unsigned int destination; // 受信したデータの宛先ID
  //  unsigned int message_id; // 受信したデータの識別信号
  unsigned int selector; // 宛先セレクタ
  //  unsigned int rssi; // 受信RSSI/2-130 = dBm
  unsigned int data_length; // 受信データ長
  char *data; // 受信データ内容
  //  unsigned int *route; // 受信したデータの途中経路（宛先IDを常に含む）
};

int countup = 0;
char serial_read[SERIAL_BUFFER];
char send_data[35] = "SKSEND 1 1000 0001 D 0,00.00,00.00";
SoftwareSerial sSerial(P2_3, P2_4); // RX, TX
struct packet_t packet;
//                    01234567890123456789012345678901234


void setup() {
  Serial.begin(9600);
  sSerial.begin(9600); // ソフトウェアシリアルの初期化
  sSerial.println("Ready");
  //  init_lpr9204();
}


void loop() {
  read_serial(1000);
  read_serial(-1);
  //  if ( strcmp(serial_read, "OK") == 0) {
  //  }
  //  if ( event_is("ERXDATA") ) {
  //  }
  //  send_temperature_lpr9204( 0, 33.33, 44.44 );
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

int init_lpr9204() {
  int my_id = 0;
  pinMode(RESET, OUTPUT);
  pinMode(WAKEUP, OUTPUT);
  digitalWrite(RESET, LOW);
  digitalWrite(WAKEUP, LOW);
  delay(10);
  digitalWrite(RESET, HIGH);
  digitalWrite(WAKEUP, HIGH);
  read_serial(-1); // Welcome
  Serial.println("SKINFO");
  for ( int i = 0; i < 3; i++ ) {
    read_serial(-1);
    if ( event_is("EINFO") ) {
      char s[5];
      strncpy(s, serial_read + 6, 4);
      s[4] = '\0';
      my_id = (int)strtol(s, NULL, 16);
      break;
    }
  }
  return my_id;
}

/*
   データを受信待ちにします
   timeoutを超えるとfalseを返して読み込みを中止します
   timeoutに-1を指定すると無制限にデータを待ち続けます
*/
bool read_serial( int timeout ) {
  int t = 0;
  int i = 0;
  serial_read[0] = '\0';
  while ( true ) {
    if (Serial.available()) {
      char c = Serial.read();
      if ( i >= (SERIAL_BUFFER - 1) || c == '\n' ) {
        serial_read[i] = '\0';
        break;
      } else {
        serial_read[i++] = c;
      }
    } else if ( t >= timeout && timeout > 0 ) {
      sSerial.print(t);
      sSerial.println(" times Timeout!");
      return false;
    } else {
      t++;
      delay(1);
    }
  }
  // CRの削除
  if (strlen(serial_read) > 0) {
    serial_read[strlen(serial_read) - 1] = '\0';
  }
  return true;
}

// 01234567890123456789012345678901234
// SKSEND 1 1000 0001 D n,22.22,55.55\0
void send_temperature_lpr9204( int n, double temp, double humi ) {
  send_data[21] = n + '0';
  send_data[22] = ',';
  d22tostr( 23, temp );
  send_data[28] = ',';
  d22tostr( 29, humi );
  Serial.println(send_data);
  sSerial.println(send_data);
}

/*
   2.2fにfscanfします
*/
void d22tostr( int index, double d ) {
  char c1 = (int)(d / 10) % 10 + '0';
  char c2 = (int)d % 10 + '0';
  char c3 = (int)(d * 10) % 10 + '0';
  char c4 = (int)(d * 100) % 10 + '0';
  send_data[index + 0] = c1;
  send_data[index + 1] = c2;
  send_data[index + 2] = '.';
  send_data[index + 3] = c3;
  send_data[index + 4] = c4;
}



