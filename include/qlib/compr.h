// qlib/qcompr.h - Compression library

#include <qlib/object.h>
#include <cl.h>
#include <dmedia/cl_cosmo.h>

// Compressor types
#define QC_JPEG_COSMO	CL_JPEG_COSMO

// Transfer modes
#define QCTM_AUTO_1_FIELD	CL_COSMO_VIDEO_TRANSFER_AUTO_1_FIELD
#define QCTM_AUTO_2_FIELD	CL_COSMO_VIDEO_TRANSFER_AUTO_2_FIELD

class QCompDecomp : public QObject
// Base class for compressor/decompressor (never instantiated)
{
 protected:
  CLhandle cprHdl;
  CLbufferHdl dataBuf,frameBuf;		// Compressed/decompressed

 public:
  QCompDecomp();
  virtual ~QCompDecomp();

  CLhandle GetCprHandle(){ return cprHdl; }
};

class QDecompressor : public QCompDecomp
{
 protected:

 public:
  QDecompressor(int cprType);
  ~QDecompressor();

  bool CreateDataBuffer(int size);

  // Parameter setting made more C++
  bool SetImageSize(int wid,int hgt);
  bool SetInternalImageSize(int wid,int hgt);
  bool SetTransferMode(int transferMode);

  bool Start();
  bool End();

  int  QueryFree(void **pFree,int *wrap,int space=0);
  void UpdateHead(int amount);
};

