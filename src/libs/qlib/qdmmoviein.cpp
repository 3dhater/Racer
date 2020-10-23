/*
 * QDMMovieIn - movie generating DMbuffers
 * 03-09-98: Created!
 * (C) MG/RVG
 */

#include <qlib/dmmoviein.h>
#include <qlib/debug.h>
DEBUG_ENABLE

// Movie owned by user
#define F_USERMOVIE	1

QDMMovieIn::QDMMovieIn(string fileName)
  : QDMObject()
{
  //DEBUG_C(qdbg("QDMMovieIn ctor\n"));
  movie=new QMovie(fileName);
  flags=0;
}
QDMMovieIn::QDMMovieIn(QMovie *mv)
  : QDMObject()
{
  movie=mv;
  flags=F_USERMOVIE;
}

QDMMovieIn::~QDMMovieIn()
{
  if(!(flags&F_USERMOVIE))
    delete movie;
}

/*******
* POOL *
*******/
void QDMMovieIn::AddProducerParams(QDMBPool *p)
// Get necessary params to use this input object
{
}
void QDMMovieIn::AddConsumerParams(QDMBPool *p)
{}
void QDMMovieIn::RegisterPool(QDMBPool *pool)
{
}
int QDMMovieIn::GetFD()
{
  return -1;
  //return pathIn->GetFD();
}

void QDMMovieIn::GetSize(int *w,int *h)
{ QMovieTrack *img;
  img=movie->GetImageTrack();
  *w=img->GetImageWidth();
  *h=img->GetImageHeight();
}

void QDMMovieIn::Iterate()
{
  qdbg("QDMMovieIn::Iterate()\n");
  movie->SetCurFrame(10);
  //movie->RenderToDMB(
}

