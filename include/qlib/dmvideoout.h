// QDMVideoOut.h - declaration

#ifndef __QDMVIDEOOUT_H
#define __QDMVIDEOOUT_H

#include <qlib/dmobject.h>
#include <qlib/video.h>

class QDMVideoOut : public QDMObject
// A class to get video input into a stream (QDMStream)
// Video input can be composite/camera or screen grabs (!)
{
 protected:
  // Class member variables
  QVideoNode *srcNode;			// Source picture (Video/Screen)
  QVideoNode *drnNode;			// Memory buffer (DMbuffer)
  QVideoPath *pathOut;			// Video output

 public:
  QDMVideoOut();
  ~QDMVideoOut();

  int  GetFD();
  QVideoNode *GetSourceNode(){ return srcNode; }
  QVideoNode *GetDrainNode(){ return drnNode; }
  QVideoPath *GetPath(){ return pathOut; }

  //void AddProducerParams(QDMBPool *p);    // Video in
  void AddConsumerParams(QDMBPool *p);    // Video out
  void RegisterPool(QDMBPool *pool);

  void GetSize(int *w,int *h);
  int  GetTransferSize();

  void SetSize(int w,int h);		// ?
  void SetOffset(int x,int y);
  void SetOrigin(int x,int y);
  void SetZoomSize(int w,int h);	// O2 zoom (not camera)
  void SetLayout(int n);
  void SetPacking(int n);
  void SetFormat(int n);
  void SetTiming(int n);
  void SetCaptureType(int n);
  void SetGenlock(bool b);
  void Start();

  void GetEvent(VLEvent *event);
  void Send(DMbuffer buf);
};

#endif
