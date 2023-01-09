/*
      MIDI2CV
      Copyright (C) 2017  Larry McGovern

      This program is free software: you can redistribute it and/or modify
      it under the terms of the GNU General Public License as published by
      the Free Software Foundation, either version 3 of the License, or
      (at your option) any later version.

      This program is distributed in the hope that it will be useful,
      but WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
      GNU General Public License <http://www.gnu.org/licenses/> for more details.
*/

#include <MIDI.h>
#include <SPI.h>
#include <MCP4922.h>
MCP4922 DAC1(11,13,10,9); 
MCP4922 DAC2(11,13,8,9); 
MCP4922 DAC3(11,13,7,9);  // (MOSI,SCK,CS,LDAC) define Connections for MEGA_board, 

// Note priority is set by pins A0 and A2
// Highest note priority: A0 and A2 high (open)
// Lowest note priority:  A0 low (ground), A2 high (open)
// Last note priority:    A2 low (ground)

#define NP_SEL1 A0  // Note priority is set by pins A0 and A2
#define NP_SEL2 A2

#define GATE  2
#define TRIG  3
#define CLOCK 4

#define MIDI0  A1
#define MIDI1  A3
#define MIDI2  A6
#define MIDI3  A7

int MIDI_CHANNEL = 0;
int SWITCH1 = 0;
int SWITCH2 = 0;
int SWITCH3 = 0;
int SWITCH4 = 0;

MIDI_CREATE_DEFAULT_INSTANCE();

void setup()
{
  pinMode(NP_SEL1, INPUT_PULLUP);
  pinMode(NP_SEL2, INPUT_PULLUP);

  pinMode(MIDI0, INPUT_PULLUP);
  pinMode(MIDI1, INPUT_PULLUP);
  pinMode(MIDI2, INPUT_PULLUP);
  pinMode(MIDI3, INPUT_PULLUP);

  pinMode(GATE, OUTPUT);
  pinMode(TRIG, OUTPUT);
  pinMode(CLOCK, OUTPUT);
  digitalWrite(GATE, LOW);
  digitalWrite(TRIG, LOW);
  digitalWrite(CLOCK, LOW);

  SPI.begin();
  MIDI.begin(MIDI_CHANNEL);

  // Set initial pitch bend voltage to 0.5V (mid point).  With Gain = 1X, this is 1023
  // Other DAC outputs will come up as 0V, so don't need to be initialized
  setVoltage2(0, 2047);
}

bool notes[88] = {0};
int8_t noteOrder[20] = {0}, orderIndx = {0};
unsigned long trigTimer = 0;
unsigned int mV;
int velocity, newd2, newerd2;

void loop()
{
  int type, noteMsg, channel, d1, d2;
  static unsigned long clock_timer = 0, clock_timeout = 0;
  static unsigned int clock_count = 0;
  bool S1, S2;

  if ((trigTimer > 0) && (millis() - trigTimer > 20)) {
    digitalWrite(TRIG, LOW); // Set trigger low after 20 msec
    trigTimer = 0;
  }

  if ((clock_timer > 0) && (millis() - clock_timer > 20)) {
    digitalWrite(CLOCK, LOW); // Set clock pulse low after 20 msec
    clock_timer = 0;
  }

  if (digitalRead(MIDI0) == 1) {
    SWITCH1 = 1;
  }
  else {
    SWITCH1 = 0;
  }
  if (digitalRead(MIDI1) == 1) {
    SWITCH2 = 2;
  }
  else {
    SWITCH2 = 0;
  }
  if (digitalRead(MIDI2) == 1) {
    SWITCH3 = 4;
  }
  else {
    SWITCH3 = 0;
  }
  if (digitalRead(MIDI3) == 1) {
    SWITCH4 = 8;
  }
  else {
    SWITCH4 = 0;
  }
  MIDI_CHANNEL = (SWITCH1 + SWITCH2 + SWITCH3 + SWITCH4);


  if (MIDI.read(MIDI_CHANNEL)) {
    byte type = MIDI.getType();
    switch (type) {
      case midi::NoteOn:
      case midi::NoteOff:
        noteMsg = MIDI.getData1() - 24; // A0 = 21, Top Note = 108

        channel = MIDI.getChannel();

        if ((noteMsg < 0) || (noteMsg > 87)) break; // Only 88 notes of keyboard are supported

        if (type == midi::NoteOn) velocity = MIDI.getData2();
        else velocity  = 0;

        if (velocity == 0)  {
          notes[noteMsg] = false;
        }
        else {
          notes[noteMsg] = true;
          // velocity range from 0 to 4095 mV  Left shift d2 by 5 to scale from 0 to 4095,
          // and choose gain = 2X
          int newervol = velocity << 5;
          setVoltage1(mV, newervol); // DAC1, channel 1, gain = 2X
        }

        // Pins NP_SEL1 and NP_SEL2 indictate note priority
        S1 = digitalRead(NP_SEL1);
        S2 = digitalRead(NP_SEL2);

        if (S1 && S2) { // Highest note priority
          commandTopNote();
        }
        else if (!S1 && S2) { // Lowest note priority
          commandBottomNote();
        }
        else { // Last note priority
          if (notes[noteMsg]) {  // If note is on and using last note priority, add to ordered list
            orderIndx = (orderIndx + 1) % 20;
            noteOrder[orderIndx] = noteMsg;
          }
          commandLastNote();
        }
        break;

      case midi::PitchBend:
        d1 = MIDI.getData1();
        d2 = MIDI.getData2(); // d2 from 0 to 127, mid point = 64

        // Pitch bend output from 0 to 1023 mV.  Left shift d2 by 4 to scale from 0 to 2047.
        // With DAC gain = 1X, this will yield a range from 0 to 1023 mV.
        newd2 = d2 << 4;
        setVoltage2(newd2, newerd2); // DAC2, channel 0, gain = 1X

        break;

      case midi::ControlChange:
        d1 = MIDI.getData1();
        d2 = MIDI.getData2(); // From 0 to 127

        // CC range from 0 to 4095 mV  Left shift d2 by 5 to scale from 0 to 4095,
        // and choose gain = 2X
        newerd2 = d2 << 5;
        setVoltage2(newd2, newerd2); // DAC2, channel 1, gain = 2X
        break;

      case midi::AfterTouchChannel:
        d1 = MIDI.getData1();

        // CC range from 0 to 4095 mV  Left shift d2 by 5 to scale from 0 to 4095, 
        // and choose gain = 2X
        int newd1 = d1 << 5;
        setVoltage3(newd1, 2047); // DAC2, channel 1, gain = 2X
        break;
        
      case midi::Clock:
        if (millis() > clock_timeout + 300) clock_count = 0; // Prevents Clock from starting in between quarter notes after clock is restarted!
        clock_timeout = millis();

        if (clock_count == 0) {
          digitalWrite(CLOCK, HIGH); // Start clock pulse
          clock_timer = millis();
        }
        clock_count++;
        if (clock_count == 24) {  // MIDI timing clock sends 24 pulses per quarter note.  Sent pulse only once every 24 pulses
          clock_count = 0;
        }
        break;

      case midi::ActiveSensing:
        break;

      default:
        d1 = MIDI.getData1();
        d2 = MIDI.getData2();
    }
  }
}

void commandTopNote()
{
  int topNote = 0;
  bool noteActive = false;

  for (int i = 0; i < 88; i++)
  {
    if (notes[i]) {
      topNote = i;
      noteActive = true;
    }
  }

  if (noteActive)
    commandNote(topNote);
  else // All notes are off, turn off gate
    digitalWrite(GATE, LOW);
}

void commandBottomNote()
{
  int bottomNote = 0;
  bool noteActive = false;

  for (int i = 87; i >= 0; i--)
  {
    if (notes[i]) {
      bottomNote = i;
      noteActive = true;
    }
  }

  if (noteActive)
    commandNote(bottomNote);
  else // All notes are off, turn off gate
    digitalWrite(GATE, LOW);
}

void commandLastNote()
{

  int8_t noteIndx;

  for (int i = 0; i < 20; i++) {
    noteIndx = noteOrder[ mod(orderIndx - i, 20) ];
    if (notes[noteIndx]) {
      commandNote(noteIndx);
      return;
    }
  }
  digitalWrite(GATE, LOW); // All notes are off
}

// Rescale 88 notes to 4096 mV:
//    noteMsg = 0 -> 0 mV
//    noteMsg = 87 -> 4096 mV
// DAC output will be (4095/87) = 47.069 mV per note, and 564.9655 mV per octive
// Note that DAC output will need to be amplified by 1.77X for the standard 1V/octave

#define NOTE_SF 36.995f // This value can be tuned if CV output isn't exactly 1V/octave

void commandNote(int noteMsg) {
  digitalWrite(GATE, HIGH);
  digitalWrite(TRIG, HIGH);
  trigTimer = millis();

  mV = (unsigned int) ((float) (noteMsg) * NOTE_SF + 0.5);
  int newvel = velocity << 5;
  setVoltage1(mV, newvel);  // DAC1, channel 0, gain = 2X
}

// setVoltage -- Set DAC voltage output
// dacpin: chip select pin for DAC.  Note and velocity on DAC1, pitch bend and CC on DAC2
// channel: 0 (A) or 1 (B).  Note and pitch bend on 0, velocity and CC on 2.
// gain: 0 = 1X, 1 = 2X.
// mV: integer 0 to 4095.  If gain is 1X, mV is in units of half mV (i.e., 0 to 2048 mV).
// If gain is 2X, mV is in units of mV

void setVoltage1(unsigned int DAC1channel1, int DAC1channel2)
{
  DAC1.Set(DAC1channel1,DAC1channel2); 
}

void setVoltage2(int DAC2channel1, int DAC2channel2)
{
  DAC2.Set(DAC2channel1,DAC2channel2); 
}

void setVoltage3(int DAC3channel1, int DAC3channel2)
{
  DAC3.Set(DAC3channel1,DAC3channel2); 
}

int mod(int a, int b)
{
  int r = a % b;
  return r < 0 ? r + b : r;
}
