/*
 * QVideo - SGI video encapsulation (VL)
 * (c) 1997 MarketGraph/RVG
 */

#include <qlib/video.h>
#include <vl/vl.h>
#include <string.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#if defined(__sgi) || defined(WIN32)

//static VLServer gs;

#undef  DBG_CLASS
#define DBG_CLASS "QVideoServer"

/***************
* QVIDEOSERVER *
***************/
QVideoServer::QVideoServer()
{
  DBG_C("ctor")
#ifdef WIN32
  qerr("QVideoServer ctor nyi/win32");
#else
  svr=vlOpenVideo("");
  if(!svr)
  { vlPerror("QVideoServer::ctor: can't open video daemon\n");
  }
  //gs=svr;
#endif
}

QVideoServer::~QVideoServer()
{
  DBG_C("dtor")
#ifdef WIN32
#else
  if(svr)vlCloseVideo(svr);
#endif
}

void QVideoServer::SaveSystemDefaults()
{
#ifdef WIN32
#else
  vlSaveSystemDefaults(svr);
#endif
}

void QVideoServer::RestoreSystemDefaults()
{
#ifdef WIN32
#else
  vlRestoreSystemDefaults(svr);
#endif
}

string QVideoServer::Reason2Str(int r)
// Returns string indicating reason
{
  static char buf[20];
#ifdef WIN32
  return "VLWin32notexist";
#else
  switch (r)
  {
    case VLStreamBusy:
      return "VLStreamBusy";
    case VLStreamPreempted:
      return "VLStreamPreempted";
    case VLAdvanceMissed:
      return "VLAdvanceMissed";
    case VLStreamAvailable:
      return "VLStreamAvailable";
    case VLSyncLost:
      return "VLSyncLost";
    case VLStreamStarted:
      return "VLStreamStarted";
    case VLStreamStopped:
      return "VLStreamStopped";
    case VLSequenceLost:
      return "VLSequenceLost";
    case VLControlChanged:
      return "VLControlChanged";
    case VLTransferFailed:
      return "VLTransferFailed";
    case VLFrameVerticalRetrace:
      return "VLFrameVerticalRetrace";
    case VLDeviceEvent:
      return "VLDeviceEvent";
      //return (VIDEOIN_SHUTTER_EVENT);
    case VLDefaultSource:
      return "VLDefaultSource";
    case VLControlRangeChanged:
      return "VLControlRangeChanged";
    case VLControlPreempted:
      return "VLControlPreempted";
    case VLControlAvailable:
      return "VLControlAvailable";
    case VLDefaultDrain:
      return "VLDefaultDrain";
    case VLStreamChanged:
      return "VLStreamChanged";
    case VLTransferError:
      return "VLTransferError";
    case VLEvenVerticalRetrace:
      return "VLEvenVerticalRetrace";
    case VLOddVerticalRetrace:
      return "VLOddVerticalRetrace";
    case VLTransferComplete:
      return "VLTransferComplete";
    default:
      sprintf(buf,"VLReason_%d",r);
      return buf;
  }
#endif
}
string QVideoServer::Control2Str(int r)
// Returns string indicating control
{ static char buf[20];
#ifdef WIN32
  return "VL_Win32notexist";
#else
  switch (r)
  {
    case VL_SIZE: return "VL_SIZE";
    case VL_FORMAT: return "VL_FORMAT";
    case VL_CAP_TYPE: return "VL_CAP_TYPE";
    case VL_ZOOM: return "VL_ZOOM";
    case VL_TIMING: return "VL_TIMING";
    case VL_PACKING: return "VL_PACKING";
    case VL_DEFAULT_SOURCE: return "VL_DEFAULT_SOURCE";
    case VL_ORIGIN: return "VL_ORIGIN";
    case VL_OFFSET: return "VL_OFFSET";
    case VL_RATE: return "VL_RATE";
    case VL_BRIGHTNESS: return "VL_BRIGHTNESS";
    case VL_CONTRAST: return "VL_CONTRAST";
    case VL_H_PHASE: return "VL_H_PHASE";
    case VL_V_PHASE: return "VL_V_PHASE";
    case VL_ASPECT: return "VL_ASPECT";
    case VL_COMPRESSION: return "VL_COMPRESSION";
    case VL_SYNC: return "VL_SYNC";
    case VL_SYNC_SOURCE: return "VL_SYNC_SOURCE";
    // General
    case VL_FLICKER_FILTER: return "VL_FLICKER_FILTER";
    case VL_DITHER_FILTER: return "VL_DITHER_FILTER";
    case VL_NOTCH_FILTER: return "VL_NOTCH_FILTER";
    case VL_LAYOUT: return "VL_LAYOUT";
    case VL_FIELD_DOMINANCE: return "VL_FIELD_DOMINANCE";
    case VL_COLORSPACE: return "VL_COLORSPACE";
    // MVP
    case VL_MVP_ZOOMSIZE: return "VL_MVP_ZOOMSIZE";
    default:
      sprintf(buf,"VL_%d",r);
      return buf;
  }
#endif
}

/*****************
* EVENT HANDLING *
*****************/
int QVideoServer::NextEvent(VLEvent *ev)
// Busy waits if no event is present
// Returns 0 if ok
{
#ifdef WIN32
  return 1;
#else
  int r;
  r=vlNextEvent(svr,ev);
  if(r)vlPerror("QVideoServer::NextEvent failed");
  return r;
#endif
}
int QVideoServer::PeekEvent(VLEvent *ev)
// Get next event without taking it off the queue
{
#ifdef WIN32
  return 1;
#else
  int r;
  r=vlPeekEvent(svr,ev);
  if(r)vlPerror("QVideoServer::PeekEvent failed");
  return r;
#endif
}
int QVideoServer::CheckEvent(VLEventMask mask,VLEvent *ev)
{
#ifdef WIN32
  return 1;
#else
  int r;
  r=vlCheckEvent(svr,mask,ev);
  if(r)vlPerror("QVideoServer::CheckEvent failed");
  return r;
#endif
}
int QVideoServer::Pending()
// Returns the #events pending in the VL queue
{
#ifdef WIN32
  return 0;
#else
  return vlPending(svr);
#endif
}
cstring QVideoServer::EventToName(int type)
{
#ifdef WIN32
  return "QVSWin32notexist";
#else
  return vlEventToName(type);
#endif
}

#ifndef WIN32
// No video support for Win32

#undef  DBG_CLASS
#define DBG_CLASS "QVideoNode"

/*************
* QVIDEONODE *
*************/
QVideoNode::QVideoNode(QVideoServer *s,int nodeType,int devType,int devnr)
{
  DBG_C("ctor")
  path=0;			// Not connected to a path
  server=s;
  node=vlGetNode(s->GetSGIServer(),nodeType,devType,devnr);
  if(!node)
  { vlPerror("QVideoNode ctor: can't get node");
  }
}
QVideoNode::~QVideoNode()
{
}

int QVideoNode::GetFD()
// Retrieve file descriptor to select() on
// This is new since VL_65, because fd's are now used on specific nodes
// instead of paths; QVideoPath::GetFD() is now obsolete, because
// the VL interface doesn't provide it
// Returns 0 if the fd couldn't be fetched; fd 0 is NEVER a VL fd.
// (should return -1, but kept to 0 for historical reasons)
{ int n;
  n=vlNodeGetFd(server->GetSGIServer(),path->GetSGIPath(),node);
  if(n<0)		// n<=0 can also be used
  { vlPerror("Can't get FD for node");
    return 0;
  }
  return n;
}
void QVideoNode::SetPath(QVideoPath *_path)
{ path=_path;
}

bool QVideoNode::SetControl(int ctrlType,VLControlValue *val)
{
  if(vlSetControl(server->GetSGIServer(),path->GetSGIPath(),node,ctrlType,val))
  { //vlPerror("QVideoNode::SetControl failed");
    qerr("QVideoNode::SetControl(%s) failed; %s",
      server->Control2Str(ctrlType),vlStrError(vlGetErrno()));
    return FALSE;
  }
  return TRUE;
}

bool QVideoNode::SetControl(int typ,int n)
{ VLControlValue val;
  val.intVal=n;
  return SetControl(typ,&val);
}
bool QVideoNode::SetControl(int typ,int x,int y)
{ VLControlValue val;
  val.xyVal.x=x;
  val.xyVal.y=y;
  return SetControl(typ,&val);
}
bool QVideoNode::GetIntControl(int typ,int *n)
{ VLControlValue val;
  if(vlGetControl(server->GetSGIServer(),path->GetSGIPath(),node,typ,&val))
  { //vlPerror("QVideoNode::GetControl failed");
    qerr("QVideoNode::GetIntControl(%s) failed; %s",
      server->Control2Str(typ),vlStrError(vlGetErrno()));
    return FALSE;
  }
  *n=val.intVal;
  return TRUE;
}
bool QVideoNode::GetXYControl(int typ,int *x,int *y)
{ VLControlValue val;
  if(vlGetControl(server->GetSGIServer(),path->GetSGIPath(),node,typ,&val))
  { //vlPerror("QVideoNode::GetControl failed");
    qerr("QVideoNode::GetXYControl(%s) failed; %s",
      server->Control2Str(typ),vlStrError(vlGetErrno()));
    return FALSE;
  }
  *x=val.xyVal.x;
  *y=val.xyVal.y;
  return TRUE;
}

bool QVideoNode::SetOrigin(int x,int y)
{ return SetControl(VL_ORIGIN,x,y);
}
bool QVideoNode::SetSize(int wid,int hgt)
{ return SetControl(VL_SIZE,wid,hgt);
}
bool QVideoNode::SetWindow(QWindow *win)
{ //return SetControl(VL_WINDOW,(int)XtWindow(wg->GetX11Widget()));
  return SetControl(VL_WINDOW,win->GetQXWindow()->GetXWindow());
}
bool QVideoNode::Freeze(bool onOff)
{ VLControlValue val;
  val.boolVal=onOff;
  return SetControl(VL_FREEZE,&val);
  //return SetControl(VL_FREEZE,(int)onOff);
}

int QVideoNode::GetFilled()
{
  if(server==0||path==0)return 0;
  return vlGetFilledByNode(server->GetSGIServer(),path->GetSGIPath(),node);
}

/*************
* QVIDEOPATH *
*************/
#undef  DBG_CLASS
#define DBG_CLASS "QVideoPath"

QVideoPath::QVideoPath(QVideoServer *s,int dev,QVideoNode *src,QVideoNode *drn)
{
  DBG_C("ctor")
  srv=s;
  path=vlCreatePath(s->GetSGIServer(),dev,src->GetSGINode(),drn->GetSGINode());
  //path=vlCreatePath(gs,VL_ANY,src->GetSGINode(),drn->GetSGINode());
  //printf("path=%p\n",path);
  if(path<0)
  { vlPerror("QVideoPath: can't create video path");
    printf("  (error %d)\n",path);
    path=0;
  } else
  { // Connect to nodes
    src->SetPath(this);
    drn->SetPath(this);
  }
}
QVideoPath::~QVideoPath()
{
  DBG_C("dtor")
  if(path)vlDestroyPath(srv->GetSGIServer(),path);
}

bool QVideoPath::Setup(int ctrlUsage,int streamUsage,QVideoPath *path2)
{ VLPath paths[2];
  int n;
  
  DBG_C("Setup")
  DBG_ARG_I(ctrlUsage);
  DBG_ARG_I(streamUsage);
  DBG_ARG_P(path2);
  
  n=1;
  paths[0]=path;
  if(path2)
  { paths[n]=path2->GetSGIPath();
    n++;
  }
  //if(vlSetupPaths(srv->GetSGIServer(),&path,1,ctrlUsage,streamUsage)<0)
  if(vlSetupPaths(srv->GetSGIServer(),paths,n,ctrlUsage,streamUsage)<0)
  { vlPerror("Can't setup path");
    return FALSE;
  }
  return TRUE;
}

bool QVideoPath::SelectEvents(VLEventMask evMask)
{
  if(vlSelectEvents(srv->GetSGIServer(),path,evMask)<0)
  { vlPerror("Can't select events mask");
    return FALSE;
  }
  return TRUE;
}

int  QVideoPath::GetTransferSize()
{
  return vlGetTransferSize(srv->GetSGIServer(),path);
}

#ifdef OBS_FDS_BASED_ON_NODE_SINCE_VL_65
int QVideoPath::GetFD()
{ int n;
  if(vlPathGetFD(srv->GetSGIServer(),path,&n)<0)
  { vlPerror("Can't get FD for path");
    return 0;
  }
  return n;
}
#endif

bool QVideoPath::BeginTransfer()
{
#ifdef ND
  VLTransferDescriptor xferDesc;
  xferDesc.mode=VL_TRANSFER_MODE_CONTINUOUS;
  xferDesc.count=1;
  xferDesc.delay=0;
  xferDesc.trigger=VLTriggerImmediate;
#endif
  DBG_C("BeginTransfer")
  //if(vlBeginTransfer(srv->GetSGIServer(),path,1,&xferDesc))
  if(vlBeginTransfer(srv->GetSGIServer(),path,0,NULL))
  { vlPerror("Can't begin transfer");
    return FALSE;
  }
  return TRUE;
}
bool QVideoPath::EndTransfer()
{ vlEndTransfer(srv->GetSGIServer(),path);
  return TRUE;
}

bool QVideoPath::AddNode(QVideoNode *node)
{
  if(vlAddNode(srv->GetSGIServer(),path,node->GetSGINode()))
  { vlPerror("Can't add node");
    return FALSE;
  }
  node->SetPath(this);
  return TRUE;
}

/*********
* PREFAB *
*********/

/********
* COSMO *
********/
QVideoCosmoLink::QVideoCosmoLink(QVideoServer *s)
{ VLDevice *devPtr;
  VLDevList devList;
  VLControlValue val;
  int n;

  srv=s;
  devNode=src=drnScreen=drnVideo=0;
  timingSrc=timingDrn=dataTiming=0;
  pathTiming=pathVideo=0;

  // Create paths
  
  vlGetDeviceList(s->GetSGIServer(),&devList);

  printf("QVideoCosmoLink: devices:\n");
  for(n=0;n<devList.numDevices;n++)
  { devPtr=&(devList.devices[n]);
    printf("  '%s'\n",devPtr->name);
    if(!strcmp(devPtr->name,"ev1"))break;
    else devPtr=0;
  }
  if(!devPtr)
  { fprintf(stderr, "IndyVideo/Galileo not found\n");
    return;
  }

  // Set timing to internal sync
  devNode=new QVideoNode(s,VL_DEVICE,0,VL_ANY);
  pathVideo=new QVideoPath(s,devPtr->dev,devNode,devNode);
  pathVideo->Setup(VL_SHARE,VL_READ_ONLY);
  devNode->GetIntControl(VL_SYNC,&n);
  if(n==VL_EV1_SYNC_SLAVE)
    devNode->SetControl(VL_SYNC,VL_SYNC_INTERNAL);
  delete pathVideo;

  // Setup the timing path (IndyVideo/Galileo -> Cosmo)
  timingSrc=new QVideoNode(s,VL_SRC,VL_SCREEN,VL_ANY);
  timingDrn=new QVideoNode(s,VL_DRN,VL_VIDEO,2);
  pathTiming=new QVideoPath(s,devPtr->dev,timingSrc,timingDrn);

  pathTiming->Setup(VL_SHARE,VL_SHARE);
  pathTiming->BeginTransfer();

  // setup the data transfer path (Cosmo -> IndyVideo/Galileo --> screen,vid) 
  src=new QVideoNode(s,VL_SRC,VL_VIDEO,1);
  drnScreen=new QVideoNode(s,VL_DRN,VL_SCREEN,VL_ANY);
  drnVideo=new QVideoNode(s,VL_DRN,VL_VIDEO,0);
  dataTiming=new QVideoNode(s,VL_DRN,VL_VIDEO,2);

  //path=new QVideoPath(s,devPtr->dev,src,drn);
  pathVideo=new QVideoPath(s,devPtr->dev,src,drnVideo);

  //path->AddNode(drnScreen);
  pathVideo->AddNode(dataTiming);

  pathVideo->Setup(VL_SHARE,VL_SHARE);

  printf("Cosmo paths created\n");
}
QVideoCosmoLink::~QVideoCosmoLink()
{
  if(devNode)delete devNode;
  if(src)delete src;
  //etc.

  if(pathTiming)delete pathTiming;
  if(pathVideo)delete pathVideo;
}

bool QVideoCosmoLink::Show()
{ pathVideo->BeginTransfer();
  return TRUE;
}
bool QVideoCosmoLink::Hide()
{ pathVideo->EndTransfer();
  return TRUE;
}

/* void BeginCosmo()
{
  drn->SetWidget(gw);
  drn->SetOrigin(0,0);
  drn->SetSize(768,576);
  path->BeginTransfer();
}*/

#endif
// Win32

#endif

