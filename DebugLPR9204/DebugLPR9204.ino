#include <SoftwareSerial.h>
#define RESET P2_1
#define WAKEUP P2_2
#define SERIAL_BUFFER 50

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
char send_data[36] = "SKSEND 1 1000 0001 0D 0,00.00,00.00";
SoftwareSerial sSerial(P2_3, P2_4); // RX, TX
struct packet_t packet;
//                    01234567890123456789012345678901234
int packet_id = 0;
int my_id = 0;


void setup() {
  Serial.begin(9600);
  sSerial.begin(9600); // ソフトウェアシリアルの初期化
  sSerial.println("Ready");
  init_lpr9204();
}


void loop() {
  while (1) {
    if ( !read_serial(10 * 1000) ) { // 10秒以上応答がなければbreakする
      break;
    }
    if ( event_is("ERXDATA") ) {
      if ( command_is("RESEND") ) {
        if ( request_for_me(my_id) ) {
          sSerial.println("Resend");
        }
      } else if ( command_is("SLEEP") ) {
        packet_id = get_packet_id(); // n+1される
        int sleep_time = get_sleep_time();
        sSerial.print("Sleep, ");
        sSerial.print(packet_id);
        sSerial.print(" , ");
        sSerial.println(sleep_time);
        break;
      }
    }
  }
}

/*
   再送メッセージが自分宛かどうか確かめます
   01234567890123456789012345678901234567890123456789
   ........FROM DEST MID  SEL  RS LEN DATA
   ERXDATA_0000_0000_0000_0000_00_00_RESEND,0001\0
*/
bool request_for_me( int my_id ) {
  if ( strlen(serial_read) < 45 ) {
    return false;
  }
  char s[5];
  strncpy(s, serial_read + 41, 4);
  s[4] = '\0';
  int dest_id = (int)strtol(s, NULL, 16);
  return ( my_id == dest_id );
}

/*
   SLEEP命令からメッセージIDを取り出します
   01234567890123456789012345678901234567890123456789
   ........FROM DEST MID  SEL  RS LEN DATA
   ERXDATA_0000_0000_0000_0000_00_00_SLEEP_ALL,n,tt\0
*/
int get_packet_id() {
  if ( strlen(serial_read) < 48 ) {
    return -1;
  }
  if ( '0' <= serial_read[44] && serial_read[44] <= '9' ) {
    return serial_read[44] - '0';
  }
  return -1;
}
int get_sleep_time() {
  int sleep_time = 60;
  if ( strlen(serial_read) < 48 ) {
    return sleep_time;
  }
  if ( '0' <= serial_read[46] && serial_read[46] <= '9' ) {
    sleep_time += (serial_read[46] - '0') * 10;
  }
  if ( '0' <= serial_read[47] && serial_read[47] <= '9' ) {
    sleep_time += serial_read[47] - '0';
  }
  return sleep_time;
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
   受信したデータのコマンドがnameであるかを確かめます
   nameの長さはSERIAL_BUFFER以下であることを確認してください
*/
bool command_is( char *name ) {
  int len = strlen(name);
  if ( strlen(serial_read) < len + 34 ) {
    return false;
  }
  for ( int i = 0; i < len; i++ ) {
    if ( name[i] != serial_read[i + 34] ) {
      return (false);
    }
  }
  return true;
}


int init_lpr9204() {
  pinMode(RESET, OUTPUT);
  pinMode(WAKEUP, OUTPUT);
  digitalWrite(RESET, LOW);
  digitalWrite(WAKEUP, LOW);
  delay(10);
  digitalWrite(RESET, HIGH);
  digitalWrite(WAKEUP, HIGH);
  read_serial(1000); // Welcome
  Serial.println("SKINFO");
  // SKINFOの応答はEINFOとOKがあり得る
  for ( int i = 0; i < 2; i++ ) {
    read_serial(1000);
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
  sSerial.println(serial_read);
  return true;
}



/*
   ACKが返ってきたらtrue
*/
bool ack_is_available() {          // 0123456789012345
  if (strlen(serial_read) != 16) { // EACK 1 0002 6231
    return false;
  }
  return (serial_read[5] == '1'); // EACKステータスが1ならtrue
}



/*
  012345678901234567890123456789012345
  SKSEND 1 1000 0001 0D n,22.22, 55.55\0
  char send_data[36] = "SKSEND 1 1000 0001 0D              ";
*/

void send_temperature_lpr9204( int n, double temp, double humi ) {
  send_data[22] = n + '0';
  send_data[23] = ',';
  d22tostr( 24, temp );
  send_data[29] = ',';
  d22tostr( 30, humi );
  Serial.println(send_data);
  //  sSerial.println(send_data);
}

/*
   2.2fにfscanfします
*/
void d22tostr( int index, double d ) {
  char c1, c2, c3, c4, c5;
  if ( d >= 0) {
    c1 = (int)(d / 10) % 10 + '0';
    c2 = (int)d % 10 + '0';
    c3 = '.';
    c4 = (int)(d * 10) % 10 + '0';
    c5 = (int)(d * 100) % 10 + '0';
  } else {
    d *= -1;
    c1 = '-';
    c2 = (int)(d / 10) % 10 + '0';
    c3 = (int)d % 10 + '0';
    c4 = '.';
    c5 = (int)(d * 10) % 10 + '0';
  }
  send_data[index + 0] = c1;
  send_data[index + 1] = c2;
  send_data[index + 2] = c3;
  send_data[index + 3] = c4;
  send_data[index + 4] = c5;
}


