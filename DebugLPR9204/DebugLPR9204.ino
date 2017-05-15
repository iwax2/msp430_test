#include "lpr9204.h"
#include <SoftwareSerial.h>

char serial_read[SERIAL_BUFFER];
//                    012345678901234567890123456789012345
//                    SKSEND 1 1000 0001 0D n,22.22,55.55\0
SoftwareSerial sSerial(P2_5, P2_4); // RX, TX
//SoftwareSerial sSerial(10, 11); // RX, TX
double humidity = 98.76;
double temperature = 12.34;
int packet_id = 0;

void setup() {
  pinMode(RED_LED, OUTPUT);
  digitalWrite(RED_LED, LOW);
  Serial.begin(115200); // Communication with LPR9204
  sSerial.begin(9600); // ソフトウェアシリアルの初期化
  sSerial.println("Ready");
  delay(100);
  sSerial.println("start initialyze");
  init_lpr9204();
  sSerial.println("start Setup");
  setup_lpr9204();
  sSerial.println("start loop");
}

void loop() {
  digitalWrite(RED_LED, LOW);
  Serial.flush();
  Serial.print("SKSEND 1 1000 0001 1 ");
  Serial.println(packet_id);
  packet_id = (packet_id + 1) % 10;
  read_serial(1000); // Welcome
  read_serial(1000); // Welcome
  read_serial(1000); // Welcome
  Serial.flush();
  Serial.println("SKINFO");
  read_serial(1000); // Welcome
  read_serial(1000); // Welcome
  read_serial(1000); // Welcome
  Serial.flush();
  digitalWrite(RED_LED, HIGH);
  delay(1000);
}


void setup_lpr9204() {
  Serial.flush();
  Serial.println("SKSREG S01 0020"); // ID
  Serial.flush();
  Serial.println("SKSREG S08 34"); // 周波数
  Serial.flush();
  Serial.println("SKSREG S20 1"); // オートロード機能フラグ
  Serial.flush();
  Serial.println("SKSREG S26 4"); // 0:9600 4:115200
  Serial.flush();
  Serial.println("SKSAVE");
  Serial.flush();
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
  Serial.flush();
  Serial.println("SKINFO");
  read_serial(1000); // INFO
  Serial.flush();
  return my_id;
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

