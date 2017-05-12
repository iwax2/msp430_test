#ifndef _lpr9204_h
#define _lpr9204_h

#include"arduino.h"
#include<string.h>
#include<stdlib.h>

#define SERIAL_BUFFER 60
#define MAX_RESEND 10

#define RESET P2_1
#define WAKEUP P2_2
//#define RED_LED 13
//#define RESET 2
//#define WAKEUP 3



int init_lpr9204();
bool read_serial( int timeout );
void sleep_lpr9204();
void awake_lpr9204();

#endif
