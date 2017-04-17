#include <Wire.h>
#include "lpr9204.h"
#define HYT_ADDR 0x28
#define BAUDRATE 9600
#include <SoftwareSerial.h>

char serial_read[SERIAL_BUFFER];
//                    012345678901234567890123456789012345
//                    SKSEND 1 1000 0001 0D n,22.22,55.55\0
char send_data[36] = "SKSEND 1 1000 0001 0D              ";
struct packet_t packet;
//SoftwareSerial sSerial(P2_3, P2_4); // RX, TX
SoftwareSerial sSerial(10, 11); // RX, TX
double humidity = 98.76;
double temperature = 12.34;
int packet_id = 0;
int my_id = 0;

void setup() {
  pinMode(RED_LED, OUTPUT);
  digitalWrite(RED_LED, LOW);
  Serial.begin(BAUDRATE); // Communication with LPR9204
  sSerial.begin(9600); // ソフトウェアシリアルの初期化
  sSerial.println("Ready");
  while (my_id == 0) {
    my_id = init_lpr9204();
  }
  sSerial.print("myID is :");
  sSerial.println(my_id);
  //  Wire.setModule(0);
  //  Wire.begin();
}

void loop() {
  int sleep_time = 10;
  awake_lpr9204();
  sSerial.println("920 Wakeup");
  digitalWrite(RED_LED, HIGH);
  delay(100);
  //  get_temperature_by_wire();
  int no_resend = send_temperature_until_ack_lpr9204( packet_id, temperature, humidity );

  while (1) {
    if ( !read_serial(30 * 1000) ) { // 30秒以上応答がなければbreakする
      sleep_time = 30;
      packet_id = (packet_id + 1) % 10; // n+1される
      break;
    }
    if ( event_is("ERXDATA") ) {
      if ( command_is("RSEND") ) {
        //        blink_times(5);
        int pid = request_for_me(my_id); // packet_IDは0～9
        if ( pid >= 0 ) {
          send_temperature_until_ack_lpr9204( pid, temperature, humidity );
        }
      } else if ( command_is("SLEEP") ) {
        int p = get_packet_id();
        blink_times(p + 1);
        if ( p < 0 ) {
          packet_id = (packet_id + 1) % 10; // n+1される
        } else {
          packet_id = p;
        }
        int s = get_sleep_time();
        if ( s == 0 ) {
          sleep_time = 50;
        } else {
          sleep_time = s;
        }
        break;
      }
    }
  }
  delay(1000);
  digitalWrite(RED_LED, LOW);
  sleep_lpr9204();
  for ( int i = 0; i < 100; i++ ) {
    delay( sleep_time * 10 );
  }
}



bool get_temperature_by_wire() {
  bool is_available = false;
  Wire.beginTransmission(HYT_ADDR);   // Begin transmission with given device on I2C bus
  Wire.requestFrom(HYT_ADDR, 4);      // Request 4 bytes
  if (Wire.available() == 4) {
    int b1 = Wire.read();
    int b2 = Wire.read();
    int b3 = Wire.read();
    int b4 = Wire.read();

    int rawHumidity = b1 << 8 | b2;
    rawHumidity =  (rawHumidity &= 0x3FFF);
    humidity = 100.0 / pow(2, 14) * rawHumidity;

    b4 = (b4 >> 2); // Mask away 2 least significant bits see HYT 221 doc
    int rawTemperature = b3 << 6 | b4;
    temperature = 165.0 / pow(2, 14) * rawTemperature - 40;

    //    sSerial.print(humidity);
    //    sSerial.print("% - Temperature: ");
    //    sSerial.println(temperature);
    is_available = true;
  } else {
    //    Serial.println("Not enough bytes available on wire.");
    is_available = false;
  }
  Wire.endTransmission();           // End transmission and release I2C bus
  return (is_available);
}

bool blink_times( int times ) {
  for ( int i = 0; i < times; i++ ) {
    digitalWrite(RED_LED, HIGH);
    delay(200);
    digitalWrite(RED_LED, LOW);
    delay(200);
  }
  digitalWrite(RED_LED, HIGH);

}



/*
   LPR9204の初期設定
*/
int init_lpr9204() {
  int my_id = 0;
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
   発生したイベントがnameであるかを確かめます
   nameの長さはSERIAL_BUFFER以下であることを確認してください
*/
bool event_is( char *name ) {
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


/*
   再送メッセージが自分宛かどうか確かめます
   01234567890123456789012345678901234567890123456789
   ........FROM DEST MID  SEL  RS LEN  CMD,DEST,n
   ERXDATA_0000_0000_0000_0000_00_00_RSEND,0001,n\0
*/
int request_for_me( int my_id ) {
  if ( strlen(serial_read) < 46 ) {
    return -1;
  }
  char s[5];
  strncpy(s, serial_read + 40, 4);
  s[4] = '\0';
  int dest_id = (int)strtol(s, NULL, 16);
  if ( dest_id == my_id ) {
    return ( get_packet_id() );
  } else {
    return -1;
  }
}

/*
   SLEEP/RSEND命令からメッセージIDを取り出します
   01234567890123456789012345678901234567890123456789
   ........FROM DEST MID  SEL  RS LEN DATA
   ERXDATA_0000_0000_0000_0000_00_00_SLEEP_ALL0,n,tt\0
   ERXDATA_0000_0000_0000_0000_00_00_RSEND,0001,n\0
*/
int get_packet_id() {
  if ( strlen(serial_read) < 46 ) {
    return -1;
  }
  if ( '0' <= serial_read[45] && serial_read[45] <= '9' ) {
    return serial_read[45] - '0';
  }
  return -1;
}

/*
   SLEEP命令からSLEEP時間を取り出します
   01234567890123456789012345678901234567890123456789
   ........FROM DEST MID  SEL  RS LEN DATA
   ERXDATA_0000_0000_0000_0000_00_00_SLEEP_ALL0,n,tt\0
*/
int get_sleep_time() {
  int sleep_time = 0;
  if ( strlen(serial_read) < 48 ) {
    return sleep_time;
  }
  if ( '0' <= serial_read[47] && serial_read[47] <= '9' ) {
    sleep_time += (serial_read[47] - '0') * 10;
  }
  if ( '0' <= serial_read[48] && serial_read[48] <= '9' ) {
    sleep_time += serial_read[48] - '0';
  }
  return sleep_time;
}


/*
   ACKが返ってきたらtrue
*/
bool ack_is_available() {        // 0123456789012345
  if (strlen(serial_read) < 6) { // EACK 1 0002 6231
    return false;
  }
  if ( serial_read[5] == '1' ) {
    return true;
  } else {
    return false;
  }
}

/*
   データを受信待ちにします
   timeoutを超えるとfalseを返して読み込みを中止します
   timeoutに-1を指定すると無制限にデータを待ち続けます
   timeout=10,000で15秒ぐらいなので/1.5してdelayに合わせる
*/
bool read_serial( int timeout ) {
  int t = 0;
  int i = 0;
  if ( timeout > 1 ) {
    timeout = (int)(timeout / 1.5);
  }
  serial_read[0] = '\0';
  while ( true ) {
    if ( Serial.available() > 0) {
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
  sSerial.print("920 Says> ");
  sSerial.println(serial_read);
  return true;
}

/*
   Ackが返ってくるまで温湿度を再送します（MAX_RESENDまで）
*/
int send_temperature_until_ack_lpr9204( int n, double temp, double humi ) {
  int no_resend = 0;
  while ( no_resend < MAX_RESEND ) {
    sSerial.print("Current resend time is :");
    sSerial.println(no_resend);
    send_temperature_lpr9204( n, temp, humi ); // データを送る
    for ( int m = 0; m < 5; m++ ) {
      read_serial(1000); // ACKを待つ。エコーバックや615B OK, ERXDATAなどの応答もある
      if ( event_is("EACK") ) {
        break;
      }
    }
    if ( ack_is_available() ) {
      break;
    }
    delay(100);
    no_resend++;
  }
  sSerial.print("Number of sent packets until ack is available :");
  sSerial.println(no_resend);
  return (no_resend);
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
  Serial.flush();
  Serial.println(send_data);
  Serial.flush();
  sSerial.print("MSP Sends> ");
  sSerial.println(send_data);
}

void send_debug_message( int error_no ) {
  Serial.flush();
  if ( error_no == 0 ) {
    Serial.println("SKSEND 1 1000 0001 32 Debug:");
  } else if ( error_no == 1 ) {
    Serial.println("SKSEND 1 1000 0001 32 Sleep");
  } else if ( error_no == 2 ) {
    Serial.println("SKSEND 1 1000 0001 32 Resend");
  }
  Serial.flush();
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


void sleep_lpr9204() {
  sSerial.print("MSP Sends> ");
  sSerial.println("SKDEEPSLEEP"); // SKSLEEPはESLEEPやEWAKEを返さないと書いてあるけど返ってきてる気がする
  Serial.flush();
  Serial.println("SKDEEPSLEEP");
  Serial.flush();
}

void awake_lpr9204() {
  digitalWrite(WAKEUP, LOW);
  delay(10); // min 5msのWAKEUP入力が必要（立ち上がりエッジで起動）
  digitalWrite(WAKEUP, HIGH);
  delay(10); // max 5msで起動
  while ( read_serial( 1000 ) ); // SLEEP時に発行したSKSLEEPを受け取る
}

