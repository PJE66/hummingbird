#include "Keyboard.h"
#include "Keycodes.h"
#include "Keymaps.h"

// #define Debug

uint64_t Told,Tnow,Tdown;
uint32_t Map   = 0;
uint32_t oMap  = 0;
uint32_t dMap  = 0;
uint32_t Idx   = 0;
uint32_t Matrix;
byte     i,cIdx,rIdx,Col,Row,Presses,oPresses;
byte     Down  = 0;
byte     KeySent[32];
byte     Mods  = 0;
byte     oMods = 0;
byte     Key   = 0;
byte     Chord = 0;
bool     SupressMods = false;
bool     done  = false;
bool     SHIFT = false;
bool     CTRL  = false;
bool     ALT   = false;
bool     GUI   = false;
bool     firstpass;
byte     Set   = 0;

enum states { S_Idle, S_Wait, S_Chord, S_Key };

states  Seq = S_Idle;

/* --------------------------------------------------------------------------------------- Direct Port A Access */
volatile uint32_t *setPinA = &PORT->Group[PORTA].OUTSET.reg;  // Ptr to PortA Data Output Value Set register
volatile uint32_t *clrPinA = &PORT->Group[PORTA].OUTCLR.reg;  // Ptr to PortA Data Output Value Clear register
volatile uint32_t *dirPinA = &PORT->Group[PORTA].DIRSET.reg;  // Ptr to PortA Data Direction Set register
volatile uint32_t *inPinA  = &PORT->Group[PORTA].IN.reg;      // Ptr to PortA Input register

/* -------------------------------------------------------------------------------------------------- User LEDs */
const uint32_t  ShiftLED = (1ul << 17);
const uint32_t  CtrlLED  = (1ul << 18);
const uint32_t  AltLED   = (1ul << 19);

void PressKey(byte Key, bool SHIFT, bool CTRL, bool ALT, bool GUI, bool Press ) {
  if (SHIFT) Keyboard.press(KEY_LEFT_SHIFT);
  if (CTRL)  Keyboard.press(KEY_LEFT_CTRL);
  if (ALT)   Keyboard.press(KEY_LEFT_ALT);
  if (GUI)   Keyboard.press(KEY_LEFT_GUI);
  if (Press) Keyboard.press(Key);
  else Keyboard.write(Key);
  if (GUI)   Keyboard.release(KEY_LEFT_GUI);
  if (ALT)   Keyboard.release(KEY_LEFT_ALT);
  if (CTRL)  Keyboard.release(KEY_LEFT_CTRL);
  if (SHIFT) Keyboard.release(KEY_LEFT_SHIFT); 
}

byte DecodeChord(uint32_t Map, byte Set) {
  byte     Chord = 0;
  uint32_t Mask  = 3;
  byte     Loop  = 0;

#ifdef Debug
  Keyboard.print(Map,16);
  Keyboard.print(" ");
#endif

  while ((Loop < 25) && (Chord == 0)) {
    if (Mask == Map) {
      switch (Set) {
      case 0: Chord = a2Chords0[Loop]; break;
      case 1: Chord = a2Chords1[Loop]; break;
      }
    }
    Loop++;
    Mask <<= 1;
  }
  if (Chord == 1) Chord = 0;
  return Chord;
}

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
        Key = Decode[Idx];
        Matrix |= 1 << Key;
        if (Key < 26) Presses++;
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
  if (Map != 0) SupressMods = true;                           // Suppress sending modifier alternate keycode
  Tnow  = millis();                                           // Save current timer
/* ------------------------------------------------------------------------------------ Press/Release Shift Key */
  if ((Mods & 2) != (oMods & 2)) SHIFT = ((Mods & 2) == 2);
  Set = ((Mods & 1) == 0) ? 0 : 1;
/* -------------------------------------------------------------------------- Handle new keyfield press/release */
  switch (Seq) {
/* ------------------------------------------------------------------------------------------ Wait for keypress */
  case S_Idle:
    if (Presses > 0) {
      Chord = (Presses > 1) ? DecodeChord(Map, Set) : 0;
      Tdown = Tnow;
      Seq   = S_Wait;
    }
    break;
/* -------------------------------------------------------------------------------------- Short Delay for Chord */
  case S_Wait:
    if (Presses > oPresses) {
      Chord = DecodeChord(Map, Set);
      Tdown = Tnow;
      Seq   = S_Chord;
    } 
    if (((Tnow - Tdown) > 30) || (Map == 0)) {
      firstpass = true;
      Seq = (Chord == 0) ? S_Key : S_Chord;    
    }
    break;
/* ---------------------------------------------------------------------------------------------- Handle Chords */
  case S_Chord:
    if (Presses > oPresses) {
      Chord = DecodeChord(Map, Set);
    } else if (Map == 0) {
      PressKey(Chord, SHIFT, CTRL, ALT, GUI, false);
      Chord = 0;
      ALT   = false;
      CTRL  = false;
      GUI   = false;
      Seq   = S_Idle;
    }
    break;
/* --------------------------------------------------------------------------------------- Handle Standard Keys */
  case S_Key:
    if (firstpass) {
      Map       = oMap;
      oMap      = 0;
      firstpass = false;
    }
    for (i=0; i<26; i++) {
      Idx = (1<<i); 
      if ((Map & Idx) != (oMap & Idx)) {
        if ((Map & Idx) != 0) {
          Key = ((Mods & 1) == 0) ? aKeyMap[i] : aAltMap[i];
          if (Key == KEY_LEFT_CTRL) CTRL = !CTRL;
          else if (Key == KEY_LEFT_ALT) ALT = !ALT;
          else if (Key == KEY_LEFT_GUI) ALT = !GUI;
          else {
            PressKey(Key, SHIFT, CTRL, ALT, GUI, true);
            ALT  = false;
            CTRL = false;
            GUI  = false;
            KeySent[i] = Key;
            Down++;
          } 
        } else if ((Key != KEY_LEFT_SHIFT) && (Key != KEY_LEFT_CTRL) && (Key != KEY_LEFT_ALT)) {
          Keyboard.release(KeySent[i]);
          Down--;
        }
      }
    }
    if (Map == 0) Seq = S_Idle;
    break;
  }
/* -------------------------------------------------------------------------------------------- Handle Mod LEDS */
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
    Chord = 0;
    Down  = 0;
    Seq   = S_Idle;
  }
}
