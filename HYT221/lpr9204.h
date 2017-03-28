#ifndef _lpr9204_h
#define _lpr9204_h

#define SERIAL_BUFFER 100

#define WAKEUP_SEC 0
#define SLEEP_SEC 10
#define RESET P2_1
#define WAKEUP P2_2

#define MAX_RESEND 10

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


int init_lpr9204();
bool event_is( char* name );
bool command_is( char* name );
bool request_for_me( int my_id );
int get_message_id();
int get_sleep_time();
bool ack_is_available();
void read_serial();
void parse_data_to_packet();
void print_packet();
void send_temperature_until_ack_lpr9204( int n, double temp, double humi );
void send_temperature_lpr9204( int n, double temp, double humi );
void sleep_lpr9204();
void awake_lpr9204();

#endif
