// for attiny84
//
// clockwise pinout, see http://highlowtech.org/?p=1695
// thresholds: https://docs.google.com/spreadsheets/d/18AQomssA9ao9b4hix50sLytVfsuXtkYp12ck_SXKnsM/edit#gid=0

#define ANODE_PIN    0
#define MID_PIN      1
#define CATHODE_PIN  2  // always input

#define ANODE_EPS  150  // allowable difference from ADC reading
#define MID_EPS     80
#define CATHODE_EPS 80

#define RESET_PIN    3

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

enum Result {Pending, Passed, Failed} button_open, button_closed;

enum ConnectionState { Disconnected, Connected, ConnectionError } connectionState;

//enum CheckState {
//  Waiting
//}

void resetStuff() {
  pinMode(ANODE_PIN, INPUT);
  pinMode(CATHODE_PIN, INPUT);
  connectionState = Disconnected;
  button_open = Pending;
  button_closed = Pending;
  Frame seq[13] = {{_,_,_}, {R,_,_}, {R,R,_},
                   {R,R,R}, {Y,R,R}, {Y,Y,R},
                   {Y,Y,Y}, {G,Y,Y}, {G,G,Y},
                   {G,G,G}, {_,G,G}, {_,_,G}, {_,_,_}};
  showSeq(seq, 13, 9);
  showSeq(seq, 13, 9);
}

bool adc_within(uint8_t pin, uint16_t expected, uint16_t eps) {
  uint16_t measured = analogRead(pin);
  uint16_t error = (measured > expected) ? (measured - expected) : (expected - measured);
  return error <= eps;
}

void updateConnectionState() {
  pinMode(ANODE_PIN, INPUT_PULLUP);
  delayMicroseconds(10);
  uint16_t ano_val = analogRead(ANODE_PIN);
  uint16_t mid_val = analogRead(MID_PIN);
  uint16_t cath_val = analogRead(CATHODE_PIN);
  
  if (
    524 < ano_val && ano_val <= 584 &&
    2 < mid_val   && mid_val <= 14 &&
    3 < cath_val  && cath_val <= 36
   ) {
      connectionState = Connected;
      return;
  } else if (
    1000 < ano_val &&
    cath_val <= 2
  ) {
    if (connectionState != Disconnected) {
      resetStuff();
    }
  }
  connectionState = ConnectionError;
  return;
}

void updateButtonStatus() {
  if (connectionState != Connected) return;
  pinMode(ANODE_PIN, OUTPUT);
  digitalWrite(ANODE_PIN, HIGH);
  delayMicroseconds(10);
  uint16_t mid_val = analogRead(MID_PIN);
  uint16_t cath_val = analogRead(CATHODE_PIN);
  if (  // button open
    140 < mid_val  && mid_val <= 205 &&
    436 < cath_val && cath_val <= 475
  ) {
    button_open = Passed;
    return;
  }
  if (  // button closed
    334 < mid_val  && mid_val <= 394 &&
    399 < cath_val && cath_val <= 439
  ) {
    button_closed = Passed;
    return;
  }
  // error
}

void showFrame(Frame frame) {
  showFrame(frame[0], frame[1], frame[2]);
}
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
    showFrame(seq[i]);
    delay(t);
  }
}

uint8_t resultColour(Result res, uint32_t t) {
  if (res == Passed) return G;
  else if (res == Failed) return R;
  return _;
}

void present(int x, uint32_t t) {
  if (connectionState == Disconnected) {
    bool on =  !(t & 0b110000000);
    bool ong = !(t & 0b111100000);
    showFrame((on?(ong?R:Y):_), _, _);
    return;
  } else if (connectionState == ConnectionError) {
    bool on = t & 0b11100000;
    showFrame((on?R:_), _, _);
    return;
  }

  if (button_open == Passed && button_closed == Passed) {
    Frame seq[12] = {{G,G,G}, {_,G,G}, {G,_,G}, {G,G,_}, {G,G,G}, {G,G,G},
                     {G,G,G}, {G,G,_}, {G,_,G}, {_,G,G}, {G,G,G}, {G,G,G}};
    showSeq(seq, 12, 50);
    return;
  }

  uint8_t connection_c = G;
  uint8_t button_open_c = _;
  uint8_t button_closed_c = _;
  
  if (button_open == Passed) {
    button_open_c = G;
  }

  if (button_closed == Passed) {
    button_closed_c = G;
  }

  showFrame(connection_c, button_open_c, button_closed_c);

//  if (
//  else {
//    showFrame(G, _, _);
//  }
//  switch (state.checkStep) {
//  case Resetting: {
//    Frame seq[13] = {{_,_,_}, {R,_,_}, {R,R,_},
//                     {R,R,R}, {Y,R,R}, {Y,Y,R},
//                     {Y,Y,Y}, {G,Y,Y}, {G,G,Y},
//                     {G,G,G}, {_,G,G}, {_,_,G}, {_,_,_}};
//    showSeq(seq, 13, 9);
//    showSeq(seq, 13, 9);
//    break;
//  }
//  case Disconnected: {
//    bool on = !(t & 0b110000000);
//    showFrame((on?Y:_), _, _);
//    break;
//  }
//  case ConnectionError: {
//    showFrame(R, _, _);
//    delay(300);
////    bool on = t & 0b11100000;
////    showFrame((on?R:_), _, _);
////    break;
////    Frame seq[6] = {{_,_,_}, {Y,_,_}, {Y,Y,_}, {_,Y,Y}, {_,_,Y}, {_,_,_}};
////    showSeq(seq, 6, 32);
////    break;
//  }
//  case Connected: {
//    Frame seq[6] = {{_,_,_}, {Y,_,_}, {Y,Y,_}, {_,Y,Y}, {_,_,Y}, {_,_,_}};
//    showSeq(seq, 6, 100);
//    break;
//  }
//  case ConnectedOpen: {
//    bool on = t & 0b1100000;
//    showFrame(G, (on?Y:_), resultColour(state.closedResult, t));
//    break;
//  }
//  case ConnectedClosed: {
//    bool on = t & 0b1100000;
//    showFrame(G, resultColour(state.openResult, t), (on?Y:_));
//    break;
//  }
//  case ConnectedPass: {
//    Frame seq[12] = {{G,G,G}, {_,G,G}, {G,_,G}, {G,G,_}, {G,G,G}, {G,G,G},
//                     {G,G,G}, {G,G,_}, {G,_,G}, {_,G,G}, {G,G,G}, {G,G,G}};
//    showSeq(seq, 12, 50);
//    break;
//  }
//  case Oops: {
//    // slow flash
//    uint8_t c = (t & 0b1000000000) ? R : _;
//    showFrame(c, c, c);
//    break;
//  }
//  default: {
//    uint8_t c = (t & 0b100000) ? Y : _;
//    showFrame(c, c, c);
//  }
//  }
}

void setup() {
  pinMode(CONNECT_OK, OUTPUT);
  pinMode(CONNECT_NOK, OUTPUT);
  pinMode(OPEN_OK, OUTPUT);
  pinMode(OPEN_NOK, OUTPUT);
  pinMode(CLOSED_OK, OUTPUT);
  pinMode(CLOSED_NOK, OUTPUT);
  pinMode(RESET_PIN, INPUT_PULLUP);
  resetStuff();
}

void loop() {
  if (digitalRead(RESET_PIN) == LOW) resetStuff();
  updateConnectionState();
  updateButtonStatus();
  present(0, millis());
}
