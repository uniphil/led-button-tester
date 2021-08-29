#include <EEPROM.h>

// for attiny84
//
// clockwise pinout, see http://highlowtech.org/?p=1695

#define ANODE_PIN    0
#define MID_PIN      1
#define CATHODE_PIN  2

#define ANODE_EPS  150  // allowable difference from ADC reading
#define MID_EPS     80
#define CATHODE_EPS 80

#define RESET_PIN    3

#define CONNECT_OK   9
#define CONNECT_NOK 10
#define OPEN_OK      7
#define OPEN_NOK     8
#define CLOSED_OK    6
#define CLOSED_NOK   5

// colours & frame
#define _ 0
#define Y 3
#define G 2
#define R 1
typedef uint8_t Frame[3];

struct ButtonState {
  bool     button_closed;
  bool     drive_anode;  // enable pullup when false
  bool     drive_cathode;
  uint16_t expected_anode;
  uint16_t expected_mid;  // the junction between LED cathod and the switch
  uint16_t expected_cathode;
};

// specs based on one button. voltages measured at the attiny pin.
// anode and cathode are def the wrong terms here -- they mean + and - of the whole switch assembly
// ...mid is the cathode of the actual LED inside
//
//  VCC  | BUTT  | ANODE | AN_V  | AN_b  | MID_V | MID_b | CATH | CATH_V | CATH_b
//       |       |       |       | a/v*10|       | m/v*10|      |        | a/v*2^10
// ------|-------|-------|-------|-------|-------|-------|------|--------|--------
// connection state
//  4.73 | --    | HIGH  | 4.73  | 1024  | 0.00  |   0   | o    | 0.00   |   0      // connection states
//  4.73 | --    | HIGH  | 4.73  | 1024  | 0.00  |   0   | LOW  | 0.00   |   0
//  4.73 | open  | PULL  | 2.50  |  541  | 0.07  |  15   | LOW  | 0.00   |   0
//  4.73 | close | PULL  | 2.44  |  528  | 0.01  |   2   | LOW  | 0.00   |   0
//  4.73 | open  | HIGH  | 4.68  | 1013  | 2.04  | 443   | LOW  | 0.04   |   9      // open states
//  4.73 | open  | HIGH  | 4.70  | 1018  | 2.10  | 455   | o    | 0.80   | 173
//  4.70 | close | HIGH  | 4.43  |  965  | 1.33  | 290   | LOW  | 0.22   |  48      // closed states
//  4.72 | close | HIGH  | 4.66  | 1011  | 1.97  | 542   | o    | 1.72   | 373

//                                       closed,  drive a, drive c,  anode,   mid,  cath
// connection state
const ButtonState B_disconnected     = {  false,    false,    true,   1024,     0,     0};
const ButtonState B_connected_open   = {  false,    false,    true,    541,    15,     0};
const ButtonState B_connected_closed = {   true,    false,    true,    528,     2,     0};
// open states
const ButtonState B_open_driving     = {  false,     true,    true,   1013,   443,     9};
const ButtonState B_open_reading     = {  false,     true,   false,   1018,   455,   173};
// closed states
const ButtonState B_closed_driving   = {   true,     true,    true,    965,   290,    48};
const ButtonState B_closed_reading   = {   true,     true,   false,   1011,   542,   373};

// primary state machine
enum CheckStep {
  Resetting,
  Disconnected,
  ConnectionError,
  Connected,
  ConnectedOpen,
  ConnectedClosed,
  ConnectedPass,
  Oops
};

enum CheckResult { NotChecked, Passed, Failed };

struct CheckState {
  CheckStep   checkStep;
  CheckResult openResult;
  CheckResult closedResult;
} state;

const CheckState defaultState = { Disconnected, NotChecked, NotChecked };

bool adc_within(uint8_t pin, uint16_t expected, uint16_t eps) {
  uint16_t measured = analogRead(pin);
  uint16_t error = (measured > expected) ? (measured - expected) : (expected - measured);
  return error <= eps;
}

bool button_is_disconnected() {
  pinMode(ANODE_PIN, INPUT_PULLUP);
  pinMode(CATHODE_PIN, OUTPUT);
  delay(5);
  bool is_connected = adc_within(ANODE_PIN, 1024, ANODE_EPS);
  pinMode(CATHODE_PIN, INPUT);
  return is_connected;
}

bool button_is_connected() {
  pinMode(ANODE_PIN, INPUT_PULLUP);
  pinMode(CATHODE_PIN, OUTPUT);
  delay(5);
  bool is_connected = adc_within(ANODE_PIN, 535, ANODE_EPS);
  pinMode(CATHODE_PIN, INPUT);
  return is_connected;
}

bool button_is(ButtonState buttonState) {
  bool is = true;

  pinMode(ANODE_PIN, OUTPUT);
  digitalWrite(ANODE_PIN, HIGH);

  if (buttonState.drive_cathode) {
    pinMode(CATHODE_PIN, OUTPUT);
    delay(5);
  } else {
    pinMode(CATHODE_PIN, INPUT);
    delay(5);
    if (!adc_within(CATHODE_PIN, buttonState.expected_cathode, CATHODE_EPS)) is = false;
  }

  if (!adc_within(MID_PIN,     buttonState.expected_mid,     MID_EPS    )) is = false;

  pinMode(ANODE_PIN, INPUT);
  pinMode(CATHODE_PIN, INPUT);
  return is;
}


int save(int i, uint16_t v) {
  EEPROM.put(i, v);
  return i + 2;
}


CheckState next(CheckState current, bool forceReset) {
  if (forceReset) {
    int i = 0;
    pinMode(ANODE_PIN, INPUT_PULLUP);
    pinMode(CATHODE_PIN, INPUT);
    i = save(i, analogRead(ANODE_PIN));
    i = save(i, analogRead(MID_PIN));
    i = save(i, analogRead(CATHODE_PIN));
    pinMode(CATHODE_PIN, OUTPUT);
    i = save(i, analogRead(ANODE_PIN));
    i = save(i, analogRead(MID_PIN));
    i = save(i, analogRead(CATHODE_PIN));
    pinMode(ANODE_PIN, OUTPUT);
    digitalWrite(ANODE_PIN, HIGH);
    pinMode(CATHODE_PIN, INPUT);
    i = save(i, analogRead(ANODE_PIN));
    i = save(i, analogRead(MID_PIN));
    i = save(i, analogRead(CATHODE_PIN));
    pinMode(CATHODE_PIN, OUTPUT);
    i = save(i, analogRead(ANODE_PIN));
    i = save(i, analogRead(MID_PIN));
    i = save(i, analogRead(CATHODE_PIN));
    return { Resetting };
  }

  switch (current.checkStep) {
    case Resetting:
      return defaultState;
  
    case Disconnected:
      if (button_is_disconnected())
        return current;
      else if (button_is_connected())
        return { Connected, NotChecked, NotChecked };
      else
        return { ConnectionError, NotChecked, NotChecked };

    case ConnectionError:
      if (button_is_disconnected())
        return { Resetting };
      if (button_is_connected())
        return { Connected, NotChecked, NotChecked };
      else
        return current;

    case Connected:
      if (button_is_disconnected())
        return { Resetting };
      else if (button_is(B_open_driving))
        return { ConnectedOpen, NotChecked, NotChecked };
      else if (button_is(B_closed_driving)) 
        return { ConnectedClosed, NotChecked, NotChecked };
      else
        return current;


    case ConnectedOpen:
      if (button_is_disconnected())
        return { Resetting };
      return current;

    case ConnectedClosed:
      if (button_is_disconnected())
        return { Resetting };
      return current;

//    case ConnectedClosed
//    case ConnectedOpen:
//    case ConnectedOpenError:
//    case ConnectedClosedOk:
//      if (button_is(B_open_reading) && button_is(B_open_driving)) {
//        if (current.closedResult == Passed) return { ConnectedPass };
//        else return { ConnectedOpenOk, Passed, current.closedResult };
//      } else
//        return { ConnectedOpenError, Failed, current.closedResult };

//    case ConnectedClosed:
//    case ConnectedClosedError:
//    case ConnectedOpenOk:
//      if (button_is(B_closed_reading) && button_is(B_closed_driving)) {
//        if (current.openResult == Passed) return { ConnectedPass };
//        else return { ConnectedClosedOk, current.openResult, Passed };
//      } else
//        return { ConnectedClosedError, current.closedResult, Failed };

    case ConnectedPass:
      if (button_is_disconnected())
        return { Resetting };
      else
        return current;

    default: {
      return { Oops };
    }
  }
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

uint8_t resultColour(CheckResult res, uint32_t t) {
  if (res == Passed) return G;
  else if (res == Failed) return R;
  return _;
}

void present(CheckState state, uint32_t t) { switch (state.checkStep) {
  case Resetting: {
    Frame seq[13] = {{_,_,_}, {R,_,_}, {R,R,_},
                     {R,R,R}, {Y,R,R}, {Y,Y,R},
                     {Y,Y,Y}, {G,Y,Y}, {G,G,Y},
                     {G,G,G}, {_,G,G}, {_,_,G}, {_,_,_}};
    showSeq(seq, 13, 9);
    showSeq(seq, 13, 9);
    break;
  }
  case Disconnected: {
    bool on = !(t & 0b110000000);
    showFrame((on?Y:_), _, _);
    break;
  }
  case ConnectionError: {
    showFrame(R, _, _);
    delay(300);
//    bool on = t & 0b11100000;
//    showFrame((on?R:_), _, _);
//    break;
//    Frame seq[6] = {{_,_,_}, {Y,_,_}, {Y,Y,_}, {_,Y,Y}, {_,_,Y}, {_,_,_}};
//    showSeq(seq, 6, 32);
//    break;
  }
  case Connected: {
    Frame seq[6] = {{_,_,_}, {Y,_,_}, {Y,Y,_}, {_,Y,Y}, {_,_,Y}, {_,_,_}};
    showSeq(seq, 6, 100);
    break;
  }
  case ConnectedOpen: {
    bool on = t & 0b1100000;
    showFrame(G, (on?Y:_), resultColour(state.closedResult, t));
    break;
  }
  case ConnectedClosed: {
    bool on = t & 0b1100000;
    showFrame(G, resultColour(state.openResult, t), (on?Y:_));
    break;
  }
  case ConnectedPass: {
    Frame seq[12] = {{G,G,G}, {_,G,G}, {G,_,G}, {G,G,_}, {G,G,G}, {G,G,G},
                     {G,G,G}, {G,G,_}, {G,_,G}, {_,G,G}, {G,G,G}, {G,G,G}};
    showSeq(seq, 12, 50);
    break;
  }
  case Oops: {
    // slow flash
    uint8_t c = (t & 0b1000000000) ? R : _;
    showFrame(c, c, c);
    break;
  }
  default: {
    uint8_t c = (t & 0b100000) ? Y : _;
    showFrame(c, c, c);
  }
}}

void setup() {
//  for (int i = 0; i < 255; i++) {
//    // this performs as EEPROM.write(i, i)
//    EEPROM.update(i, i);
//  }
  pinMode(CONNECT_OK, OUTPUT);
  pinMode(CONNECT_NOK, OUTPUT);
  pinMode(OPEN_OK, OUTPUT);
  pinMode(OPEN_NOK, OUTPUT);
  pinMode(CLOSED_OK, OUTPUT);
  pinMode(CLOSED_NOK, OUTPUT);
  pinMode(RESET_PIN, INPUT_PULLUP);
}

void loop() {
  present(state, millis());
  state = next(state, digitalRead(RESET_PIN) == LOW);
}
