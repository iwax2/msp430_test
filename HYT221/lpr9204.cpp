#include "lpr9204.h"

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
   SLEEP命令からメッセージIDを取り出します
   01234567890123456789012345678901234567890123456789
   ........FROM DEST MID  SEL  RS LEN DATA
   ERXDATA_0000_0000_0000_0000_00_00_SLEEP_ALL,n,tt\0
*/
int get_message_id() {
  if ( strlen(serial_read) < 48 ) {
    return -1;
  }
  if ( '0' <= serial_read[44] && serial_read[44] <= '9' ) {
    return serial_read[44]-'0';
  }
  return -1;
}
int get_sleep_time() {
  int sleep_time = 60;
  if ( strlen(serial_read) < 48 ) {
    return sleep_time;
  }
  if ( '0' <= serial_read[46] && serial_read[46] <= '9' ) {
    sleep_time += (serial_read[46]-'0')*10;
  }
  if ( '0' <= serial_read[47] && serial_read[47] <= '9' ) {
    sleep_time += serial_read[47]-'0';
  }
  return sleep_time;
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
   データを受信待ちにします（受信するまで戻りません）
*/
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
  // CRの削除
  if (strlen(serial_read) > 0) {
    serial_read[strlen(serial_read) - 1] = '\0';
  }
}


/*
   発生したERXDATAイベントを解析し，packet構造体に格納します
*/
void parse_data_to_packet() {     // 01234567890123456789012345678901234567890123456789
  char s[5];                      //         FROM DEST MID  SEL  RS LEN DATA RUT1 RUT2
  strncpy(s, serial_read + 7, 4); // ERXDATA_0000_0000_0000_0000_00_00_00000_0000_0000
  s[4] = '\0';
  packet.origin = (unsigned int)strtol(s, NULL, 16);
  strncpy(s, serial_read + 13, 4);
  s[4] = '\0';
  packet.destination = (unsigned int)strtol(s, NULL, 16);
  strncpy(s, serial_read + 18, 4);
  s[4] = '\0';
  packet.message_id = (unsigned int)strtol(s, NULL, 16);
  strncpy(s, serial_read + 23, 4);
  s[4] = '\0';
  packet.selector = (unsigned int)strtol(s, NULL, 16);
  strncpy(s, serial_read + 28, 2);
  s[2] = '\0';
  packet.rssi = (unsigned int)strtol(s, NULL, 16);
  strncpy(s, serial_read + 31, 2);
  s[2] = '\0';
  packet.data_length = (unsigned int)strtol(s, NULL, 16);
  free(packet.data);
  packet.data = (char *)malloc(sizeof(char) * (packet.data_length + 1));
  strncpy(packet.data, serial_read + 34, packet.data_length);
  packet.data[packet.data_length] = '\0';
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

/*
   本来はソフトウェアシリアルで確かめる（そのうち実装する）
*/
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

int send_temperature_until_ack_lpr9204( int n, double temp, double humi ) {
  int no_resend = 0;
  while ( no_resend < MAX_RESEND ) {
    send_temperature_lpr9204( int n, double temp, double humi ); // データを送る
    for ( int m = 0; m < 2; m++ ) {
      read_serial(); // ACKを待つ。615B OKなどの応答もある
      if ( event_is("EACK") ) {
        break;
      }
    }
    if ( ack_is_available() ) {
      break;
    }
    no_resend++;
  }
  return (no_resend);
}

void send_temperature_lpr9204( int n, double temp, double humi ) {
  char ch_n = n + '0';
  Serial.print("SKSEND 1 1000 0001 D ");
  Serial.print(ch_n);
  Serial.print(",");
  Serial.print(temp, 2);
  Serial.print(",");
  Serial.print(humi, 2);
  Serial.println();
}

void sleep_lpr9204() {
  Serial.println("SKSLEEP");
}

void awake_lpr9204() {
  digitalWrite(WAKEUP, LOW);
  delay(10); // min 5msのWAKEUP入力が必要（立ち上がりエッジで起動）
  digitalWrite(WAKEUP, HIGH);
  delay(10); // max 5msで起動
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
  Serial.begin(115200); // Communication with LPR9204
  Serial.println("SKINFO");
  for ( int i = 0; i < MAX_RESEND; i++ ) {
    read_serial();
    if ( event_is("EINFO") ) {
      char s[5];
      strncpy(s, serial_read + 5, 4);
      s[4] = '\0';
      my_id = (int)strtol(s, NULL, 16);
      break;
    } else if ( event_is("OK") ) {
      Serial.println("SKINFO");
    }
  }
  return my_id;
}

