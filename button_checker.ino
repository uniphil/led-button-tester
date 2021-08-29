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

enum Result {Pending, Passed, Failed} button_open, button_closed;

enum ConnectionState { Disconnected, Connected, ConnectionError } connectionState;

enum CheckState {
  JustChillin,
  Testable,
  Testing,
  Oops,
} state;

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
  if (connectionState != Connected) return;
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
  if (connectionState != Connected) return;
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

CheckState next(CheckState current) { switch (current) {
  case JustChillin:
    if (is_disconnected()) return current;
    else return Testable;

  case Testable:
    if (is_disconnected()) return JustChillin;
    else if (digitalRead(BUTTON_PIN) == LOW) return Testing;
    else return current;

  case Testing:
    if (is_disconnected()) return JustChillin;
    if (button_open == Pending) {
      if (is_button_open()) {
        button_open = Passed;
      }
    }
//      } else if (is_button_closed()) {
//        // special error case -- possible open/close confusion OR actual error
//      } else {
//        button_open = Failed;
//      }
//    } else {
//      if (is_button_closed()) {
//        button_closed = Passed;
//      } else {
//        button_closed = Failed;
//      }
//    }
    return current;

  default:
    return Oops;
}}

void resetStuff() {
  pinMode(ANODE_PIN, INPUT);
  pinMode(CATHODE_PIN, INPUT);
  state = JustChillin;
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
  switch (state) {
    case JustChillin: {
      bool on = (t & 0b100000000);
      showFrame(on ? Y : _, _, _);
      return;
    }
    case Testable: {
      bool con = is_connected();
      digitalWrite(CONNECT_OK,  con);
      digitalWrite(CONNECT_NOK, !con);
      float ts = (float)t / 1000 * PI * 4;
      float ts2 = ts + PI;
      analogWrite(OPEN_OK, (sin(ts)/2 + 0.5) * 8);
      analogWrite(OPEN_NOK, (cos(ts)/2 + 0.5) * 32);
      analogWrite(CLOSED_OK, (sin(ts2)/2 + 0.5) * 8);
      analogWrite(CLOSED_NOK, (cos(ts2)/2 + 0.5) * 32);
      return;
    }
    case Testing: {
      bool on = (t & 0b100000000);
      uint8_t c_con = is_connected() ? G : R;
      uint8_t c_open = on ? Y : _;
      uint8_t c_closed = on ? _ : Y;
      if (button_open == Passed) {
        c_open = G;
      }
//      if (button_open == Pending && is_button_closed()) {
//        c_open = on ? Y : _;
//        c_closed = on ? _ : Y;
//      }
//      uint8_t c_open = (button_open == Pending) ? (on ? Y : _) : (button_open == Passed) ? G : R;
//      uint8_t c_closed = _;
//      if (button_open != Pending) {
//        c_closed = (button_closed == Pending) ? (on ? Y : _) : (button_closed == Passed) ? G : R;
//      }
      showFrame(c_con, c_open, c_closed);
      return;
    }
    case Oops: {
      // slow flash
      uint8_t c = (t & 0b1000000000) ? R : _;
      showFrame(c, c, c);
      return;
    }
  }
  
//  if (connectionState == Disconnected) {
//    bool on =  !(t & 0b110000000);
//    bool ong = !(t & 0b111100000);
//    showFrame((on?(ong?R:Y):_), _, _);
//    return;
//  } else if (connectionState == ConnectionError) {
//    bool on = t & 0b11100000;
//    showFrame((on?R:_), _, _);
//    return;
//  }
//
//  if (button_open == Passed && button_closed == Passed) {
//    Frame seq[12] = {{G,G,G}, {_,G,G}, {G,_,G}, {G,G,_}, {G,G,G}, {G,G,G},
//                     {G,G,G}, {G,G,_}, {G,_,G}, {_,G,G}, {G,G,G}, {G,G,G}};
//    showSeq(seq, 12, 50);
//    return;
//  }
//
//  uint8_t connection_c = G;
//  uint8_t button_open_c = _;
//  uint8_t button_closed_c = _;
//  
//  if (button_open == Passed) {
//    button_open_c = G;
//  }
//
//  if (button_closed == Passed) {
//    button_closed_c = G;
//  }
//
//  showFrame(connection_c, button_open_c, button_closed_c);

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
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  resetStuff();
}

void loop() {
  state = next(state);
//  updateConnectionState();
//  updateButtonStatus();
  present(0, millis());
}
