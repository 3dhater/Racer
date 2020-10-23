// qlib/alphabuf.h

#ifndef __QLIB_ALPHABUF_H
#define __QLIB_ALPHABUF_H
 
#include <qlib/canvas.h>
#include <qlib/common/dmedia.h>

class QAlphaBuf : public QDrawable
// Hidden alpha drawing buffer, to be output to video
{
 public:
  string ClassName(){ return "alphabuf"; }
  enum Flags
  {
    GENLOCK=1,
    TRIGGER=2                   // Must trigger video stream
  };
  enum
  { MAX_BUFFER=100              // Max #buffers to use
  };
  
 protected:
  int wid,hgt;
  int flags;
  int backBufferIndex;          // Buffer index that is painted on
  int buffers;                  // Used buffers (2 normally)
  int maxBufferIndex;           // Current limit on buffers used
                                // Used in movie playback to get more buffers
  // DMedia
  QDMPBuffer  *pbuf[MAX_BUFFER];
  QDMVideoOut *vout;
  QDMBPool    *bufPool;
  
  QDMBuffer   *dmBuf[MAX_BUFFER];
  
  int transfersComplete;
  
 public:
  QAlphaBuf(int wid=720,int hgt=576,int buffers=2);
  ~QAlphaBuf();

  void SetGenlock(bool yn);
  
  bool Create();
  void Destroy();
  
  GLXDrawable GetGLXDrawable();
  
  int GetWidth(){ return wid; }
  int GetHeight(){ return hgt; }
  int GetBuffers(){ return buffers; }
  int GetMaxBufferIndex(){ return maxBufferIndex; }
  int GetBackBufferIndex(){ return backBufferIndex; }

  void Trigger();
  void HandleEvents();
  void Swap();
  void Equalize(QRect *r=0);
  
  // Painting
  void Select();

  // Swapping control
  void HoldBuffers(int n);
};

#endif

