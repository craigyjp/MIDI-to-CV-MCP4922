# MIDI-to-CV with channel switching and Aftertouch using MCP4922 dual DACs

Based on my earlier MIDI to CV clone of the Elkyam MIDI to CV using MCP4822, these are in short supply so I swapped them to MCP4922

This repository contains the code and schematic for a DIY MIDI to CV converter. I installed this converter into a home-built analog synthesizer.

- The MIDI to CV converter includes the following outputs:

- Note CV output (88 keys, 1V/octave) using a 12-bit DAC
- Note priority (highest note, lowest note, or last note) selectable with jumper or 3-way switch
- Pitch bend CV output (1.0v +/- 1.0V) octave up or down
- Velocity CV output (0 to 5V)
- Control Change CV output (0 to 2V) suitable for a CEM or AS  based VCA
- Aftertouch output CV (0-2V) can be increased with an opamp like the velocity.
- Trigger output (5V, 20 msec pulse for each new key played)
- Gate output (5V when any key depressed)
- Clock output (1 clock per quarter note, 20 msec 5V pulses)
- Read the MIDI channel from 4 toggle switches, can be replaced with a HEX rotary switch or bank of 4 dip switches.

- Really needs a 4096mV reference for the AREF pins and recalibrating.

When building and testing the Elkyam MIDI to CV I noticed the voltage was out and it seems he subtracts 21 from the note, I removed this and fixed the bottom note failure. I also added 4 toggle switches so that I could change the channel from OMNI to 1-15 (not 16 unfortunately)
