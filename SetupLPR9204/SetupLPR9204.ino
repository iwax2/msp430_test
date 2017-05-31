#define RESET P2_1
#define WAKEUP P2_2

void setup() {
  pinMode(RED_LED, OUTPUT);
  digitalWrite(RED_LED, HIGH);
  Serial.begin(9600);
//  Serial.begin(115200);
  setup_lpr9204();
  digitalWrite(RED_LED, LOW);
}

void loop() {
  digitalWrite(RED_LED, LOW);
  Serial.flush();
  Serial.println("SKSEND 1 1000 0001 1 1");
  Serial.flush();
  delay(1000);
  digitalWrite(RED_LED, HIGH);
  delay(1000);
}


void setup_lpr9204() {
  pinMode(RESET, OUTPUT);
  pinMode(WAKEUP, OUTPUT);
  digitalWrite(RESET, LOW);
  digitalWrite(WAKEUP, LOW);
  delay(10);
  digitalWrite(RESET, HIGH);
  digitalWrite(WAKEUP, HIGH);
  delay(1000);
  
  Serial.flush();
  Serial.println("SKSREG S01 0009"); // ID
  Serial.flush();
  Serial.println("SKSREG S08 34"); // 周波数
  Serial.flush();
  Serial.println("SKSREG S20 1"); // オートロード機能フラグ
  Serial.flush();
  Serial.println("SKSREG S26 0"); // 0:9600 4:115200
  Serial.flush();
  Serial.println("SKSAVE");
  Serial.flush();
}

