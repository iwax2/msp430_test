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

void setup() {
  Serial.begin(9600);
  Serial.println("Ready");
  packet.data = NULL;
  packet.routes = NULL;
}

void loop() {
  //                           FROM DEST MID  SEL  RS LEN DATA
  strcpy(serial_read, "ERXDATA 0001 0002 6231 1000 DD 05 12345"); // example
  parse_data_to_packet();
  print_packet();
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
   発生したERXDATAイベントを解析し，packet構造体に格納します
*/
void parse_data_to_packet() {           //     01234567890123456789012345678901234567890123456789
  char s[5];                  //             FROM DEST MID  SEL  RS LEN DATA RUT1 RUT2
  strncpy(s, serial_read + 7, 4); // ERXDATA_0000_0000_0000_0000_00_00_00000_0000_0000
  s[4] = '\0';
  packet.origin = (unsigned int)strtol(s, NULL, 16);
  strncpy(s, serial_read + 12, 4);
  s[4] = '\0';
  packet.destination = (unsigned int)strtol(s, NULL, 16);
  strncpy(s, serial_read + 17, 4);
  s[4] = '\0';
  packet.message_id = (unsigned int)strtol(s, NULL, 16);
  strncpy(s, serial_read + 22, 4);
  s[4] = '\0';
  packet.selector = (unsigned int)strtol(s, NULL, 16);
  strncpy(s, serial_read + 27, 2);
  s[2] = '\0';
  packet.rssi = (unsigned int)strtol(s, NULL, 16);
  strncpy(s, serial_read + 30, 2);
  s[2] = '\0';
  packet.data_length = (unsigned int)strtol(s, NULL, 16);
  free(packet.data);
  packet.data = (char *)malloc(sizeof(char) * (packet.data_length + 1));
  strncpy(packet.data, serial_read + 33, packet.data_length);
  packet.data[packet.data_length] = '\0';
  // ROUTE1からの処理
  free(packet.routes);
  packet.routes = NULL;
  packet.no_routes = 0;
  if ( strlen(serial_read) > (34 + packet.data_length) ) {
    int str_begin = 34 + packet.data_length;
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

