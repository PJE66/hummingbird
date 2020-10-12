#include "Keyboard.h"
#include "Keycodes.h"
#include "Keymaps.h"

uint64_t Told,Tnow;
uint32_t Map   = 0;
uint32_t oMap  = 0;
uint32_t Idx   = 0;
uint32_t Matrix;
byte     i,cIdx,rIdx,Col,Row,Presses,oPresses;
byte     Down  = 0;
byte     KeySent[32];
byte     Mods  = 0;
byte     oMods = 0;
byte     Key   = 0;
bool     SupressMods = false;
bool     done  = false;
bool     SHIFT = false;
bool     CTRL  = false;
bool     ALT   = false;

/* --------------------------------------------------------------------------------------- Direct Port A Access */
volatile uint32_t *setPinA = &PORT->Group[PORTA].OUTSET.reg;  // Ptr to PortA Data Output Value Set register
volatile uint32_t *clrPinA = &PORT->Group[PORTA].OUTCLR.reg;  // Ptr to PortA Data Output Value Clear register
volatile uint32_t *dirPinA = &PORT->Group[PORTA].DIRSET.reg;  // Ptr to PortA Data Direction Set register
volatile uint32_t *inPinA  = &PORT->Group[PORTA].IN.reg;      // Ptr to PortA Input register

/* -------------------------------------------------------------------------------------------------- User LEDs */
const uint32_t  ShiftLED = (1ul << 17);
const uint32_t  CtrlLED  = (1ul << 18);
const uint32_t  AltLED   = (1ul << 19);

/* +----------------------------------------------------------------------------------------------------------+ */
/* |                              ######  ######## ######## ##     ## ########                                | */
/* |                             ##       ##          ##    ##     ## ##     ##                               | */
/* |                              ######  ######      ##    ##     ## ########                                | */
/* |                                   ## ##          ##    ##     ## ##                                      | */
/* |                              ######  ########    ##     #######  ##                                      | */
/* +----------------------------------------------------------------------------------------------------------+ */
void setup() {
  Keyboard.begin();  
  for (int x=0; x<Rows; x++) pinMode(RowPins[x], INPUT);
  for (int x=0; x<Cols; x++) pinMode(ColPins[x], INPUT_PULLUP);
  *dirPinA = ShiftLED | CtrlLED | AltLED;    // Set LEDs as output
  *setPinA = ShiftLED | CtrlLED | AltLED;    // Turn off (inverted) LEDs
}

/* +----------------------------------------------------------------------------------------------------------+ */
/* |                                 ##        #######   #######  ########                                    | */
/* |                                 ##       ##     ## ##     ## ##     ##                                   | */
/* |                                 ##       ##     ## ##     ## ########                                    | */
/* |                                 ##       ##     ## ##     ## ##                                          | */
/* |                                 ########  #######   #######  ##                                          | */
/* +----------------------------------------------------------------------------------------------------------+ */
void loop() {
/* ------------------------------------------------------------------------------------------------ Scan Matrix */
  oPresses = Presses;
  Presses  = 0;
  Matrix   = 0;
  Idx      = 0;
  for (cIdx = 0; cIdx < Cols; cIdx++) {
    Col = ColPins[cIdx];
    pinMode(Col, OUTPUT);
    digitalWrite(Col, LOW);
    for (rIdx = 0; rIdx < Rows; rIdx++) {    
      Row = RowPins[rIdx];
      pinMode(Row, INPUT_PULLUP);
      delayMicroseconds(250);
      if (!digitalRead(Row)){
        Matrix |= 1 << Decode[Idx];
        Presses++;
      }
      pinMode(Row, INPUT);
      Idx++;      
    }
    pinMode(Col, INPUT);
  }
/* ---------------------------------------------------------------------------------- Extract Mods and Keyfield */
  oMods = Mods;                                               // Save old state of modifiers
  oMap  = Map;                                                // Save old state of keyfield
  Mods  = (Matrix >> 26) & 15;                                // Extract new modifiers
  Map   = (Matrix & 0x03FFFFFF);                              // Extract new keyfield
  if (Map != 0) {                                             // Keyfield Key pressed
    SupressMods = true;                                       // Y: Suppress sending modifier alternate keycode
    Tnow        = millis();                                   //    Save current timer
  }
/* ------------------------------------------------------------------------------------ Press/Release Shift Key */
  if ((Mods & 2) != (oMods & 2)) SHIFT = ((Mods & 2) == 2);
/* -------------------------------------------------------------------------- Handle new keyfield press/release */
  if (Map != oMap) {
    for (i=0; i<26; i++) {
      Idx = (1<<i);
      if ((Map & Idx) != (oMap & Idx)) {
        if ((Map & Idx) != 0) {
          Key = ((Mods & 1) == 0) ? aKeyMap[i] : aAltMap[i];
          if (Key == KEY_LEFT_CTRL) CTRL = !CTRL;
          else if (Key == KEY_LEFT_ALT) ALT = !ALT;
          else {
            if (SHIFT) Keyboard.press(KEY_LEFT_SHIFT);
            if (CTRL)  Keyboard.press(KEY_LEFT_CTRL);
            if (ALT)   Keyboard.press(KEY_LEFT_ALT);
            Keyboard.press(Key);
            if (ALT)   Keyboard.release(KEY_LEFT_ALT);
            if (CTRL)  Keyboard.release(KEY_LEFT_CTRL);
            if (SHIFT) Keyboard.release(KEY_LEFT_SHIFT);
            ALT  = false;
            CTRL = false;
            KeySent[i] = Key;
            Down++;
          }
        } else if ((Key != KEY_LEFT_SHIFT) && (Key != KEY_LEFT_CTRL) && (Key != KEY_LEFT_ALT)) {
          Keyboard.release(KeySent[i]);
          Down--;
        }
      }
    }
  }
  if (SHIFT) *clrPinA = ShiftLED; else *setPinA = ShiftLED;
  if (CTRL)  *clrPinA = CtrlLED; else *setPinA = CtrlLED;
  if (ALT)   *clrPinA = AltLED; else *setPinA = AltLED;
  
/* ------------------------------------------------------------------------- Press & Release single mod key.... */
  if ((Mods < oMods) && !SupressMods) {
    Key = 0;
    switch (oMods) {
    case 1: Key = 26; break;
    case 2: Key = 27; break;
    case 4: Key = 28; break;
    case 8: Key = 29; break;
    }
    if (Key > 0) Keyboard.write(aKeyMap[Key]);
  }

/* ----------------------------------------------------------------------------------------------- All released */
  if (Map == 0) {
    if (Down > 0)  Keyboard.releaseAll();
    if (Mods == 0) SupressMods = false;
    Down = 0;
  }
}
