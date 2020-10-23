// racer/console.h

#ifndef __RACER_CONSOLE_H
#define __RACER_CONSOLE_H

class RConsole
{
 public:
  enum Max
  {
    MAX_LINE=10,
    MAX_LINE_WID=80
  };
  enum Flags
  {
    HIDDEN=1
  };
 protected:
  char text[MAX_LINE][MAX_LINE_WID];
  int  x,y;
  int  maxLine;
  int  flags;

 protected:
  void ShiftLines();

 public:
  RConsole();
  ~RConsole();

  bool IsHidden(){ if(flags&HIDDEN)return TRUE; return FALSE; }
  void Hide(){ flags|=HIDDEN; }
  void Show(){ flags&=~HIDDEN; }

  void printf(cstring fmt,...);

  void Paint();
};

#endif

