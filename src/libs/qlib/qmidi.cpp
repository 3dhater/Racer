/*
 * MIDI
 * 14-02-2001: Created! (15:36:42)
 * NOTES:
 * - Portman PC/S has problems; separate Write() calls per-byte
 * seem to work to get bytes out.
 * (C) MarketGraph/RvG
 */

#include <qlib/midi.h>
#include <qlib/debug.h>
#pragma hdrstop
DEBUG_ENABLE

// Portman PC/S uses funny protocol
#define USE_PORTMAN_PROTOCOL

#define DEFAULT_PORTMAN_DELAY 10

// Portman uses fast upload speed
#define DEFAULT_BPS 115200

/*********
* C/Dtor *
*********/
QMidi::QMidi(int iunit,int iflags)
  : QSerial(iunit,DEFAULT_BPS,iflags)
{
  portmanDelay=DEFAULT_PORTMAN_DELAY;
}
QMidi::~QMidi()
{
}

/**********************************
* Specific hardware-related stuff *
**********************************/
void QMidi::SetPortmanDelay(int delay)
// The Portman PC/S uses 115200 upstream speed. However nice, when
// sending things too fast from the O2, it seems to miss bytes quite
// quickly.
// The delay here is used inbetween *each byte written*!
// A value of 1 to 10 is supposed to be nice. Check with a MIDI device
// how good it works (or rather, test on a MIDI hexdump device, like
// the Amiga).
{
  portmanDelay=delay;
}

/******************
* Writing to MIDI *
******************/
void QMidi::WriteSafe(char *p,int len)
{
#ifdef USE_PORTMAN_PROTOCOL
  for(int i=0;i<len;i++)
  { Write(&p[i],1);
    QNap(portmanDelay);
  }
#else
  Write(p,len);
#endif
}

/********
* Notes *
********/
void QMidi::NoteOn(int channel,int note,int vel)
// channel=1..16, note/vel=0..127
{
  char buf[3];
  
  QASSERT_V(channel>=1&&channel<=16);
  QASSERT_V(note>=0&&note<=127);
  QASSERT_V(vel>=0&&vel<=127);
  
  buf[0]=0x90+channel-1;
  buf[1]=note;
  buf[2]=vel;
  
  WriteSafe(buf,3);
}
void QMidi::NoteOff(int channel,int note,int vel)
// channel=1..16, note/vel=0..127
{
  char buf[3];
  
  QASSERT_V(channel>=1&&channel<=16);
  QASSERT_V(note>=0&&note<=127);
  QASSERT_V(vel>=0&&vel<=127);
  
  buf[0]=0x80+channel-1;
  buf[1]=note;
  buf[2]=vel;
  
  WriteSafe(buf,3);
}
