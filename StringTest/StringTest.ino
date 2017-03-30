#define SERIAL_BUFFER 100
struct packet_t {
  unsigned int origin; // 受信したデータの送信元ID
  unsigned int destination; // 受信したデータの宛先ID
  unsigned int message_id; // 受信したデータの識別信号(ランダム値)
  unsigned int selector; // 宛先セレクタ（データの内容を示す識別子）
  unsigned int rssi; // 受信RSSI/2-130 = dBm
  unsigned int data_length; // 受信データ長（100バイトまで）
  char *data; // 受信データ内容
  unsigned int *routes; // 受信したデータの途中経路（宛先IDを常に含む）
  unsigned int no_routes; // ホップ数
};

char serial_read[SERIAL_BUFFER];
struct packet_t packet;
char send_data[35] = "SKSEND 1 1000 0001 D 0,00.00,00.00";

double temp = 20.00;
double humi = 90.03;
int packet_id = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("Ready");
  packet.data = NULL;
  packet.routes = NULL;
}

void loop() {
  /*
    Serial.println("Loop start");
    //                           FROM DEST MID  SEL  RS LEN DATA
    strcpy(serial_read, "ERXDATA 0001 0002 6231 1000 DD 05 12345"); // example
    if ( event_is("ERXDATA") ) {
    parse_data_to_packet();
    print_packet();
    }
    delay(10000);
    strcpy(serial_read, "EACK 0 0002 6231"); // unreachableのexample
    // いったん送って，ackが返ってくるまで再送する
    if ( event_is("EACK") ) {
    if ( packet_is_unreachable() ) {
      Serial.println("Resend");
    } else {
      Serial.println("wait for SLEEP signal");
    }
    }
    delay(1000);
    strcpy(serial_read, "EACK 1 0002 6231"); // ACKが返ってきたときのexample
    if ( event_is("EACK") ) {
    if ( packet_is_unreachable() ) {
      Serial.println("Resend");
    } else {
      Serial.println("wait for SLEEP signal");
    }
    }
    delay(1000);
    strcpy(serial_read, "ERXDATA 0001 0002 6231 1000 DD 09 SLEEP_ALL"); // 全ノードSLEEPを受信したとき
    if ( event_is("ERXDATA") ) {
    parse_data_to_packet();
    print_packet();
    }
    if ( strcmp(packet.data, "SLEEP_ALL") == 0 ) {
    Serial.println("start SLEEP");
    } else {
    Serial.print("unknown packet: ");
    Serial.println(packet.data);
    }
    delay(1000);
    strcpy(serial_read, "ERXDATA 0001 0002 6231 1000 DD 0A SLEEP_0001"); // 対象ノードがSLEEPを受信したとき
    if ( event_is("ERXDATA") ) {
    parse_data_to_packet();
    print_packet();
    }
    if ( strcmp(packet.data, "SLEEP_ALL") == 0 ) {
    Serial.println("start SLEEP");
    } else {
    Serial.print("unknown packet: ");
    Serial.println(packet.data);
    }
    delay(1000);
    strcpy(serial_read, "ERXDATA 0001 0002 6231 1000 DD 0A SLEEP_ALL,1,55"); // 対象ノードがSLEEPを受信したとき
    if ( command_is("SLEEP") ) {
    Serial.println("start SLEEP");
    } else {
    Serial.println("cannot SLEEP");
    }
    delay(10000);
  strcpy(serial_read, "ERXDATA 0001 0002 6231 1000 DD 0A RESEND,0002"); // 再送命令を受けたとき
  if ( command_is("RESEND") ) {
    if ( request_for_me(3) ) {
      Serial.println("Resend");
    } else {
      Serial.println("Wait");
    }
  }
  delay(10000);
  strcpy(serial_read, "ERXDATA 0001 0002 6231 1000 DD 0A RESEND,000A"); // 再送命令を受けたとき
  if ( command_is("RESEND") ) {
    if ( request_for_me(10) ) {
      Serial.println("Resend");
    } else {
      Serial.println("Wait");
    }
  }
  */
  send_temperature_lpr9204( packet_id, temp, humi );
  packet_id = (packet_id+1)%10;
  temp += 0.12;
  humi += 0.232;

  delay(1000);
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
bool command_is( char* name ) {
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
   発生したERXDATAイベントを解析し，packet構造体に格納します
*/
void parse_data_to_packet() { //     01234567890123456789012345678901234567890123456789
  char s[5];                  //             FROM DEST MID  SEL  RS LEN DATA RUT1 RUT2
  strncpy(s, serial_read + 7, 4); // ERXDATA_0000_0000_0000_0000_00_00_00000_0000_0000
  s[4] = '\0';
  Serial.print("From: ");
  Serial.println(s);
  packet.origin = (unsigned int)strtol(s, NULL, 16);
  strncpy(s, serial_read + 13, 4);
  s[4] = '\0';
  Serial.print("Dest: ");
  Serial.println(s);
  packet.destination = (unsigned int)strtol(s, NULL, 16);
  strncpy(s, serial_read + 18, 4);
  s[4] = '\0';
  Serial.print("MessageID: ");
  Serial.println(s);
  packet.message_id = (unsigned int)strtol(s, NULL, 16);
  strncpy(s, serial_read + 23, 4);
  s[4] = '\0';
  Serial.print("Selector: ");
  Serial.println(s);
  packet.selector = (unsigned int)strtol(s, NULL, 16);
  strncpy(s, serial_read + 28, 2);
  s[2] = '\0';
  Serial.print("RSSI: ");
  Serial.println(s);
  packet.rssi = (unsigned int)strtol(s, NULL, 16);
  strncpy(s, serial_read + 31, 2);
  s[2] = '\0';
  Serial.print("DataLength: ");
  Serial.println(s);
  packet.data_length = (unsigned int)strtol(s, NULL, 16);
  free(packet.data);
  packet.data = (char *)malloc(sizeof(char) * (packet.data_length + 1));
  strncpy(packet.data, serial_read + 34, packet.data_length);
  packet.data[packet.data_length] = '\0';
  Serial.print("Data: ");
  Serial.println(packet.data);
  // ROUTE1からの処理
  free(packet.routes);
  packet.routes = NULL;
  packet.no_routes = 0;
  if ( strlen(serial_read) > (34 + packet.data_length + 1) ) { // スペースの分+1
    int str_begin = 34 + packet.data_length + 1;
    int str_end = strlen(serial_read);
    int str_length = str_end - str_begin;
    char *str_route = (char *)malloc(sizeof(char) * (str_length + 1));
    strncpy(str_route, serial_read + str_begin, (str_length + 1));
    Serial.print("str_route -1 : ");
    Serial.println(str_route[str_length - 1], HEX);
    Serial.print("str_route    : ");
    Serial.println(str_route[str_length], HEX);
    packet.no_routes = (int)(str_length / 5);
    packet.routes = (unsigned int *)malloc(sizeof(int) * packet.no_routes);
    char *tp = strtok(str_route, " ");
    packet.routes[0] = (unsigned int)strtol(tp, NULL, 16);
    int i = 1;
    while ( tp != NULL && i < packet.no_routes ) {
      tp = strtok(NULL, " ");
      if ( tp != NULL) {
        packet.routes[i++] = (unsigned int)strtol(tp, NULL, 16);
      }
    }
  }
}


void print_packet() {
  Serial.print("From ID: ");
  Serial.println(packet.origin, HEX);
  Serial.print("Destination ID: ");
  Serial.println(packet.destination, HEX);
  Serial.print("Message ID: ");
  Serial.println(packet.message_id, HEX);
  Serial.print("Selector: ");
  Serial.println(packet.selector, HEX);
  Serial.print("RSSI: ");
  Serial.println(packet.rssi, HEX);
  Serial.print("Data length: ");
  Serial.println(packet.data_length, HEX);
  Serial.print("DATA: ");
  Serial.println(packet.data);
  if ( packet.no_routes > 0 ) {
    for ( int i = 0; i < packet.no_routes; i++ ) {
      Serial.print("Route ");
      Serial.print(i);
      Serial.print(" : ");
      Serial.println(packet.routes[i]);
    }
  } else {
    Serial.println("There is no route information.");
  }
}

/*
   Ackが返ってこない(NACK)状態の時は再送させるため，unreachableならtrue
*/
bool packet_is_unreachable() {     // 0123456789012345
  if (strlen(serial_read) != 16) { // EACK 1 0002 6231
    return true;
  }
  return (serial_read[5] == '0'); // EACKステータスが0ならtrue
}


// 01234567890123456789012345678901234
// SKSEND 1 1000 0001 D n,22.22,55.55\0
void send_temperature_lpr9204( int n, double temp, double humi ) {
  send_data[21] = n + '0';
  send_data[22] =',';
  d22tostr( 23, temp );
  send_data[28] =',';
  d22tostr( 29, humi );
  Serial.println(send_data);
}

/*
   2.2fにfscanfします
*/
void d22tostr( int index, double d ) {
  char c1 = (int)(d / 10) % 10 + '0';
  char c2 = (int)d % 10 + '0';
  char c3 = (int)(d * 10) % 10 + '0';
  char c4 = (int)(d * 100) % 10 + '0';
  send_data[index+0] = c1;
  send_data[index+1] = c2;
  send_data[index+2] = '.';
  send_data[index+3] = c3;
  send_data[index+4] = c4;
}




