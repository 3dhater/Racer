// QDMVideoIn.h - declaration

#ifndef __QDMVIDEOIN_H
#define __QDMVIDEOIN_H

#include <qlib/dmstream.h>
#include <qlib/video.h>

class QDMVideoIn : public QDMObject
// A class to get video input into a stream (QDMStream)
// Video input can be composite/camera or screen grabs (!)
{
 protected:
  // Class member variables
  QVideoNode *srcVid;			// Source picture (Video/Screen)
  QVideoNode *drnMem;			// Memory buffer (DMbuffer)
  QVideoPath *pathIn;			// Video input
  int events;				// Events to catch

 public:
  QDMVideoIn(int source=VL_MVP_VIDEO_SOURCE_CAMERA,int _class=VL_VIDEO);
  ~QDMVideoIn();

  int  GetFD();
  QVideoPath *GetPath(){ return pathIn; }
  QVideoNode *GetSourceNode(){ return srcVid; }
  QVideoNode *GetDrainNode(){ return drnMem; }

  //void AddPoolParams(QDMBPool *p);      // Get necessary params
  void AddProducerParams(QDMBPool *p);    // Video in
  //void AddConsumerParams(QDMBPool *p);    // Video in
  void RegisterPool(QDMBPool *pool);

  void GetSize(int *w,int *h);
  int  GetTransferSize();

  void SetSize(int w,int h);		// ?
  void SetOffset(int x,int y);
  void SetOrigin(int x,int y);
  void SetZoomSize(int w,int h);	// O2 zoom (not camera)
  void SetLayout(int n);
  void SetPacking(int n);
  void SetTiming(int n);
  void SetCaptureType(int n);
  void Start();
  void Stop();

  // Events
  void AddEvents(int vlMask);
  void RemoveEvents(int vlMask);
  bool GetEvent(VLEvent *event);
};

#endif
