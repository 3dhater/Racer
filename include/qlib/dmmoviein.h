// QDMMovieIn.h - declaration

#ifndef __QDMMOVIEIN_H
#define __QDMMOVIEIN_H

#include <qlib/dmstream.h>
#include <qlib/movie.h>

class QDMMovieIn : public QDMObject
// A class to get video input into a stream (QDMStream)
// Video input can be composite/camera or screen grabs (!)
{
 protected:
  // Class member variables
  QMovie *movie;
  int flags;

 public:
  QDMMovieIn(string fileName);
  QDMMovieIn(QMovie *mv);
  ~QDMMovieIn();

  int  GetFD();

  QMovie *GetMovie(){ return movie; }
  void    GetSize(int *w,int *h);

  void AddProducerParams(QDMBPool *p);
  void AddConsumerParams(QDMBPool *p);
  void RegisterPool(QDMBPool *pool);

  // Running
  void Iterate();
};

#endif
