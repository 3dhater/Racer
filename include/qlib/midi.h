// qlib/midi.h

#ifndef __QLIB_MIDI_H
#define __QLIB_MIDI_H

#include <qlib/serial.h>

class QMidi : public QSerial
{
 private:
  int portmanDelay;		// Delay to facilitate writing to Portman PC/S

 protected:
  void WriteSafe(char *p,int len);

 public:
  QMidi(int unit,int flags=0);
  ~QMidi();

  void SetPortmanDelay(int delay);
  int  GetPortmanDelay(){ return portmanDelay; }

  void NoteOn(int channel,int note,int vel);
  void NoteOff(int channel,int note,int vel);
};

#endif

