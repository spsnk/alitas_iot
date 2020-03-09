#include <SoftwareSerial.h>
#include <SevSeg.h>

const byte MORE  = A0;
const byte LESS  = A1;
const byte SET   = A2;
const byte RELAY = A3;
const byte BTX   = A4;
const byte BRX   = A5;
bool ON = false;

const unsigned int DEBOUNCE = 50;
const byte b_pins[] = {MORE, LESS, SET};
const byte NB = 3;
bool button_state [NB];
bool last_button_state [NB];
unsigned long b_p_millis [NB];
unsigned long h_p_millis [NB];

unsigned long timer = 0;
unsigned long p_millis = 0;
byte sec = 0;
int hold_delay = 400;

SevSeg sevseg; //Instantiate a seven segment controller object
SoftwareSerial BT(BTX, BRX);

void setup()
{

  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, LOW);

  byte numDigits = 4;
  byte digitPins[] = {2, 3, 4, 5}; //Digits: 1,2,3,4 <--put one resistor (ex: 220 Ohms, or 330 Ohms, etc, on each digit pin)
  byte segmentPins[] = {6, 7, 8, 9, 10, 11, 12, 13}; //Segments: A,B,C,D,E,F,G,Period

  for (int i = 0; i < NB; i++) {
    pinMode(b_pins[i], INPUT_PULLUP);
    button_state[i] = HIGH;
    last_button_state[i] = HIGH;
    b_p_millis[i] = 0;
    h_p_millis[i] = 0;
  }
  sevseg.begin(COMMON_CATHODE, numDigits, digitPins, segmentPins);
  sevseg.setBrightness(90);
  BT.begin(9600);
}

void loop()
{
  if ( BT.available() ) {
    char data = BT.read();
    switch ( data ) {
      case 'g':
      case 'G':
        if ( timer > 0 ) {
          BT.print(timer / 60);
          BT.print(":");
          BT.println(timer % 60);
        } else {
          BT.println("0");
        }
        break;
      case 'S':
      case 's':
        BT.println(ON);
        break;
      case '+':
      case 'm':
      case 'M':
        timer += 10;
        break;
      case '-':
      case 'l':
      case 'L':
        timer = timer < 10 ? 0 : timer - 10;
        break;
      case 'O':
        ON = true;
        break;
      case 'F':
        ON = false;
        break;
      default:
        BT.write("Invalid command: '");
        BT.write(data);
        BT.write("'\n");
        BT.flush();
        break;
    }
  }
  digitalWrite(RELAY, ON);
  unsigned long c_millis = millis();
  if ( ON ) {
    if (timer <= 0) {
      ON = false;
      timer = 0;
    } else {
      //sevseg.setNumber(timer/3600*100 + timer/60%60,2);
      sevseg.setNumber(timer / 60 * 100 + timer % 60, 2);
      if (c_millis - p_millis >= 60000) {
        timer--;
        p_millis = millis();
      }
    }
    if (check_button(SET))
    {
      timer = 0;
    }
  } else {
    if (timer == 0) {
      if ( c_millis - p_millis <= 1000 ) {
        sevseg.setChars(" .   ");
      } else if ( c_millis - p_millis <= 2000) {
        sevseg.setChars("  .  ");
      } else if ( c_millis - p_millis <= 3000) {
        sevseg.setChars("   . ");
      } else if ( c_millis - p_millis <= 4000) {
        sevseg.setChars("    .");
      } else {
        p_millis = millis();
      }
    } else {
      sevseg.setNumber(timer / 60 * 100 + timer % 60, 2);
    }
    if (check_button(MORE))
    {
      timer += 10;
    }
    else if (check_button(LESS))
    {
      timer = timer < 10 ? 0 : timer - 10;
    }
    else if (check_button(SET))
    {
      ON = true;
      p_millis = millis();
    }
  }
  sevseg.refreshDisplay();
}

bool check_button( byte pin )
{
  unsigned long now = millis();
  bool ret = false;
  for (int i = 0 ; i < NB && !ret ; i++)
  {
    if (pin == b_pins[i])
    {
      button_state[i] = digitalRead(b_pins[i]);
      if ((button_state[i] == HIGH) && (last_button_state[i] == LOW))
      {
        b_p_millis[i] = now;
      }
      else if ((button_state[i] == LOW) && (last_button_state[i] == HIGH) && (now - b_p_millis[i] > DEBOUNCE))
      {
        ret = true;
        h_p_millis[i] = now;
        hold_delay = 500;
      }
      else if ((button_state[i] == LOW) && (last_button_state[i] == LOW) && (now - h_p_millis[i] > hold_delay))
      {
        ret = true;
        h_p_millis[i] = now;
        if (hold_delay > 90) {
          hold_delay /= 1.5;
        }
      }
      last_button_state[i] = button_state[i];
    }
  }
  return ret;
}
