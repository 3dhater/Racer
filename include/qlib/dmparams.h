// dmparams.h - declaration

#ifndef __QLIB_DMPARAMS_H
#define __QLIB_DMPARAMS_H

#include <dmedia/dm_params.h>
#include <qlib/video.h>

class QDMParams
{
 protected:
  DMparams *p;

 public:
  QDMParams();
  ~QDMParams();

  // Internal things
  DMparams *GetDMparams(){ return p; }

  // Get
  int GetInt(const char *name);
  int GetEnum(const char *name);
  const char *GetString(const char *name);
  double GetFloat(const char *name);
  DMfraction GetFract(const char *name);

  // Set
  void SetImageDefaults(int wid,int hgt,DMimagepacking packing);
  void SetAudioDefaults(int bits=16,int freq=44100,int channels=2);
  void SetInt(const char *name,int n);
  void SetFloat(const char *name,double n);
  void SetEnum(const char *name,int n);
  void SetString(const char *name,const char *v);

  // Debug
  void DbgPrint();
};

#endif
