// QDMMovieOut.h - declaration

#ifndef __QDMMOVIEOUT_H
#define __QDMMOVIEOUT_H

#include <qlib/dmobject.h>
#include <qlib/movie.h>

class QDMMovieOut : public QDMObject
// A class to get DMbuffers into a movie
// Mostly works with a QDMIC encoder to compress the frames
{
 protected:
  // Class member variables
  QMovie *movie;
  // Creation
  qstring  fileName;
  int      wid,hgt;
  MVid     mid,tid,aid;		// Movie, image track, audio
  MVtimescale tscale;
  MVtime   dur;
  double   rate;
  int      cpix;		// #created pictures

 public:
  QDMMovieOut(cstring fileName=0,int wid=0,int hgt=0);
  ~QDMMovieOut();

  int     GetFD();
  QMovie *GetMovie(){ return movie; }
  //void    GetSize(int *w,int *h);

  void AddProducerParams(QDMBPool *p);      // Get necessary params
  void AddConsumerParams(QDMBPool *p);      // Get necessary params
  void RegisterPool(QDMBPool *pool);

  void SetName(cstring fname);
  void SetSize(int w,int h);
  bool Create(bool addAudio=FALSE);
  bool IsOpen();
  void Close();

  // Receiving (often encoded) buffers + audio (1764 frms/s for PAL 25Hz)
  void Receive(DMbuffer field1,DMbuffer field2=0,short *audioBuf=0,
    int audioFrames=0);
};

#endif
