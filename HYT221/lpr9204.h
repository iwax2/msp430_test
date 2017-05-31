#ifndef _lpr9204_h
#define _lpr9204_h

#include"arduino.h"
#include<string.h>
#include<stdlib.h>

#define SERIAL_BUFFER 60
#define MAX_RESEND 10

#define RESET P2_1
#define WAKEUP P2_2

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


int init_lpr9204();
bool event_is( char* name );
bool command_is( char* name );
int request_for_me( int my_id );
int get_packet_id();
int get_sleep_time();
bool ack_is_available();
bool read_serial( int timeout );
void parse_data_to_packet();
void print_packet();
int send_temperature_until_ack_lpr9204( int n, double temp, double humi );
void send_temperature_lpr9204( int n, double temp, double humi );
void send_debug_message( int error_no );
void sleep_lpr9204();
void awake_lpr9204();
void d22tostr( int index, double d );
bool blink_times( int times );

#endif
