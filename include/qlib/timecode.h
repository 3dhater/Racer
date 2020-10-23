// qlib/timecode.h

#ifndef __QLIB_TIMECODE_H
#define __QLIB_TIMECODE_H

#include <qlib/object.h>
#include <dmedia/dm_timecode.h>

class QTimeCode : public QObject
{
  string ClassName(){ return "timecode"; }
 public:
  // Timecode types; determines rate and dropframe
  // Note that PAL uses 25 nodrop,
  //           NTSC uses 30 or 29.97 or derivatives
  //           M-PAL (Brazil) uses 29.97 8-field drop
  //           FILM uses 24 nodrop
  //           HDTV uses 60 nodrop or 59.94 8-field drop
  //           and more...
  enum Type
  { TC_PAL25=DM_TC_25_ND,
    TC_NTSC30_ND=DM_TC_30_ND,
    TC_FILM24=DM_TC_24_ND
  };

 protected:
  DMtimecode ostc;
  int        type;

 public:
  QTimeCode(int type=TC_PAL25);
  ~QTimeCode();

  DMtimecode *GetOSTC(){ return &ostc; }
  int         GetType(){ return type; }

  bool    ToString(string d);
  bool    FromString(cstring src);
  cstring GetString();
  void    Reset();
  bool    Set(int frame);
  bool    Set(QTimeCode *tc);
  bool    Add(QTimeCode *tc,int *overflow=0);
  bool    Add(int frames,int *overunderflow=0);
  int     FramesBetween(QTimeCode *tc);

  void    DbgPrint(cstring s);
};

#endif

