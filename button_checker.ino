// for attiny84
//
// clockwise pinout, see http://highlowtech.org/?p=1695
// thresholds: https://docs.google.com/spreadsheets/d/18AQomssA9ao9b4hix50sLytVfsuXtkYp12ck_SXKnsM/edit#gid=0

#define ANODE_PIN    0
#define MID_PIN      1
#define CATHODE_PIN  2  // always input

#define BUTTON_PIN    3

#define CONNECT_OK   9
#define CONNECT_NOK 10
#define OPEN_OK      7  // pwm
#define OPEN_NOK     8  // pwm
#define CLOSED_OK    6  // pwm
#define CLOSED_NOK   5  // pwm

// colours & frame
#define _ 0
#define Y 3
#define G 2
#define R 1
typedef uint8_t Frame[3];


// THE BUSINESS
bool is_disconnected() {
  pinMode(ANODE_PIN, INPUT_PULLUP);
  delayMicroseconds(10);
  uint16_t ano_val = analogRead(ANODE_PIN);
  uint16_t cath_val = analogRead(CATHODE_PIN);
  return 1000 < ano_val && cath_val <= 2;
}

bool is_connected() {
  pinMode(ANODE_PIN, INPUT_PULLUP);
  delayMicroseconds(10);
  uint16_t ano_val = analogRead(ANODE_PIN);
  uint16_t mid_val = analogRead(MID_PIN);
  uint16_t cath_val = analogRead(CATHODE_PIN);
  
  return 524 < ano_val && ano_val <= 584 &&
          2 < mid_val   && mid_val <= 14 &&
          3 < cath_val  && cath_val <= 36;
}

bool is_button_open() {
  pinMode(ANODE_PIN, OUTPUT);
  digitalWrite(ANODE_PIN, HIGH);
  delayMicroseconds(10);
  uint16_t mid_val = analogRead(MID_PIN);
  uint16_t cath_val = analogRead(CATHODE_PIN);
  return (
    140 < mid_val  && mid_val <= 205 &&
    436 < cath_val && cath_val <= 475
  );
}

bool is_button_closed() {
  pinMode(ANODE_PIN, OUTPUT);
  digitalWrite(ANODE_PIN, HIGH);
  delayMicroseconds(10);
  uint16_t mid_val = analogRead(MID_PIN);
  uint16_t cath_val = analogRead(CATHODE_PIN);
  return (  // button closed
    334 < mid_val  && mid_val <= 394 &&
    399 < cath_val && cath_val <= 439
  );
}

// GOOOOOO

void showFrame(uint8_t connectColour, uint8_t openColour, uint8_t closedColour) {
  digitalWrite(CONNECT_OK, connectColour & 0b10);
  digitalWrite(CONNECT_NOK, connectColour & 0b01);
  digitalWrite(OPEN_OK, openColour & 0b10);
  digitalWrite(OPEN_NOK, openColour & 0b01);
  digitalWrite(CLOSED_OK, closedColour & 0b10);
  digitalWrite(CLOSED_NOK, closedColour & 0b01);
}
void showSeq(Frame seq[], uint8_t n, uint8_t t) {
  for (int i = 0; i < n; i++) {
    showFrame(seq[i][0], seq[i][1], seq[i][2]);
    delay(t);
  }
}
void setup() {
  pinMode(CONNECT_OK, OUTPUT);
  pinMode(CONNECT_NOK, OUTPUT);
  pinMode(OPEN_OK, OUTPUT);
  pinMode(OPEN_NOK, OUTPUT);
  pinMode(CLOSED_OK, OUTPUT);
  pinMode(CLOSED_NOK, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void wait_for_button() {
  showFrame(_, _, _);
  while(digitalRead(BUTTON_PIN)) {
    uint32_t t = millis();
    uint8_t c = _;
    if (is_connected()) c = G;
    else if (!is_disconnected()) c = R;
    showFrame(t & 0b1000000 ? c : _, _, _);
  }
  Frame seq[13] = {{_,_,_}, {R,_,_}, {R,R,_},
                   {R,R,R}, {Y,R,R}, {Y,Y,R},
                   {Y,Y,Y}, {G,Y,Y}, {G,G,Y},
                   {G,G,G}, {_,G,G}, {_,_,G}, {_,_,_}};
  showSeq(seq, 13, 5);
  showSeq(seq, 13, 5);
}

bool checkConnection() {
  if (is_disconnected()) {
    for (int i = 0; i < 4; i++) {
      digitalWrite(CONNECT_NOK, HIGH);
      delay(100);
      digitalWrite(CONNECT_NOK, LOW);
      delay(100);
    }
    delay(300);
    return false;
  } else if (is_connected()) {
    for (int i = 0; i < 2; i++) {
      digitalWrite(CONNECT_OK, HIGH);
      delay(100);
      digitalWrite(CONNECT_OK, LOW);
      delay(50);
    }
    delay(50);
    digitalWrite(CONNECT_OK, HIGH);
    return true;
  } else {
    for (int i = 0; i < 8; i++) {
      digitalWrite(CONNECT_NOK, HIGH);
      delay(100);
      digitalWrite(CONNECT_NOK, LOW);
      delay(100);
    }
    delay(300);
    digitalWrite(CONNECT_NOK, HIGH);
    return false;
  }
}

bool checkForOpen() {
  while (true) {
    if (is_button_open()) {
      for (int i = 0; i < 2; i++) {
        digitalWrite(OPEN_OK, HIGH);
        delay(100);
        digitalWrite(OPEN_OK, LOW);
        delay(50);
      }
      delay(50);
      digitalWrite(OPEN_OK, HIGH);
      return true;
    } else if (is_disconnected()) {
      return false;
    } else {
      digitalWrite(OPEN_NOK, HIGH);
      delay(100);
      digitalWrite(OPEN_NOK, LOW);
      if (is_button_closed()) {
        digitalWrite(CLOSED_OK, HIGH);
        digitalWrite(CLOSED_NOK, HIGH);
      }
      delay(100);
      digitalWrite(CLOSED_OK, LOW);
      digitalWrite(CLOSED_NOK, LOW);
    }
  }
}

bool checkForClosed() {
  while (true) {
    if (is_button_closed()) {
      for (int i = 0; i < 2; i++) {
        digitalWrite(CLOSED_OK, HIGH);
        delay(100);
        digitalWrite(CLOSED_OK, LOW);
        delay(50);
      }
      digitalWrite(CLOSED_OK, HIGH);
      delay(200);
      digitalWrite(CLOSED_OK, LOW);
      return true;
    } else if (is_disconnected()) {
      return false;
    } else {
      digitalWrite(CLOSED_NOK, HIGH);
      if (is_button_open()) {
        analogWrite(CLOSED_OK, 80);
        pinMode(ANODE_PIN, OUTPUT);
        digitalWrite(ANODE_PIN, HIGH);
      }
      delay(100);
      digitalWrite(CLOSED_NOK, LOW);
      digitalWrite(CLOSED_OK, LOW);
      pinMode(ANODE_PIN, INPUT_PULLUP);
      delay(100);
    }
  }
}

void happyPingPong() {
  Frame seq[12] = {{G,G,G}, {_,G,G}, {G,_,G}, {G,G,_}, {G,G,G},
                   {G,G,G}, {G,G,_}, {G,_,G}, {_,G,G}, {G,G,G}};
  showSeq(seq, 10, 40);
  delay(300);
}

void loop() {
  wait_for_button();
  if (!checkConnection()) return;
  if (!checkForOpen()) return;
  if (!checkForClosed()) return;
  while (is_connected() && digitalRead(BUTTON_PIN)) {
    happyPingPong();
  }
}
