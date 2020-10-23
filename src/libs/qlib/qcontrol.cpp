/*
 * QControl - control generator for control values
 * 18-12-96: Created!
 * NOTES:
 * - A control is built around an 'int' value.
 *   Use it to automatically move the int value through a path of key values
 *   featuring easein and out.
 *   You can have a callback function to be notified of any changes.
 * BUGS:
 * - flags usage between QFX and QControl!
 * (C) MarketGraph/RVG
 */

#include <stdio.h>
#include <stdlib.h>
#include <qlib/control.h>
#include <qlib/debug.h>
DEBUG_ENABLE

/***********
* QCONTROL *
***********/
QControl::QControl(int _controlID)
  : QFX(0)
{ int i;
  controlID=_controlID;
  flags=QCONTROLF_DBLBUF;			// $BUG!
  for(i=0;i<QC_MAXKEY;i++)
    keyTime[i]=keyValue[i]=keyFlags[i]=0;
  keys=0;
  curTime=0;
  loopType=0;
  loopsToGo=0;
  cbFunc=0;
  cbPtr=0;
  vCurrent=0;
}

QControl::~QControl()
{ //if(env)delete env;
}

#ifdef OBS
/** Old style **/
void QControl::Vary(int _type,int _from,int _to,int _iterations,int _delay)
{
  //printf("QControl::Vary(%p; its=%d)\n",this,_iterations);
  //if(!env)env=new QEnvelope();
  //env->Vary(_type,_from,_to,_iterations,_delay);
  Done(FALSE);
}
void QControl::VaryTo(int _type,int _to,int _iterations,int _delay)
{ //Vary(_type,GetCurrent(),_to,_iterations,_delay);
}
void QControl::Go(int to,int delay)
{ //if(!env)env=new QEnvelope();
  //env->Go(to,delay);
  Done(FALSE);
}
#endif

void QControl::Set(int v)
// Shortcut to hard-set the value
// All keyframes are lost
{ //qdbg("QControl::Set(%d)\n",v);
  keys=0;
  vCurrent=v;
  flags|=QCONTROLF_EQUALIZE;
  //if(cbFunc)cbFunc(this,cbPtr,vCurrent);
  Done(FALSE);
}

void QControl::Key(int frame,int value,char kflags)
{ if(frame<0)
  { qwarn("QControl::Key was given keyframe<0");
    return;
  }
  //qdbg("QCtl(%p):Key(f%d, v%d)\n",this,frame,value);
  if(keys==0)
  { // Create 2 keys; one for the start, and one for the endpoint
    keyTime[0]=0;
    keyValue[0]=vCurrent;
    keyFlags[0]=0;
    keyTime[1]=frame;
    keyValue[1]=value;
    keyFlags[1]=kflags;
    keys=2;
    curTime=0;
  } else
  { // Now, we'll assume the key is in the future of all keys
    // Check for placement of key
    for(int i=0;i<keys;i++)
    { if(keyTime[i]==frame)
      { // Force key to go here, and flush all following keys
        // (this is bad key programming practice)
        keys=i; break;
      }
    }
    if(keys==QC_MAXKEY)
    { qwarn("Max control keys reached (%d)",QC_MAXKEY);
      return;
    }
    keyTime[keys]=frame;
    keyValue[keys]=value;
    keyFlags[keys]=kflags;
    keys++;
  }
  flags&=~QCONTROLF_EQUALIZE;
  //printf("  Key: flags=%d\n",flags);
  Done(FALSE);
  //printf("  Key: flags=%d (df)\n",flags);
}

void QControl::Loop(int loopType,int loops)
// loops=-1 => loop forever
{ //if(env)env->Loop(loopType,loops);
}
void QControl::Skip(int n)
{ //if(env)env->Skip(n);
}

void QControl::Iterate()
{ int vFrom,vTo,frames;
  int i,key;			// Key to go to
  int frameInSegment;		// Relative to start of key segment
  int oldv;

  if(IsDone())return;
  //printf("QCtrl(%p) Iterate; flags=%d\n",this,flags);
  if(flags&QCONTROLF_EQUALIZE)
  { // Repeat the last value one more time
    //qdbg("  Equalize (%d)\n",QCONTROLF_EQUALIZE);
    if(cbFunc)cbFunc(this,cbPtr,vCurrent);
    flags&=~QCONTROLF_EQUALIZE;
    Done(TRUE);
    keys=0;
    return;
  }

  /*qdbg("QCtl: Iterate, curTime=%d, vCurrent=%d, flags=%d\n",
    curTime,vCurrent,flags);*/
  if(keys<2)
  { qwarn("QControl::Iterate: no keys, but not done (BUG)");
    Done(TRUE);
    return;
  }
  /*printf("keys: %d; ",keys);
  for(key=0;key<keys;key++)printf(" t%d,v%d",keyTime[key],
    keyValue[key]);
  printf("\n");*/

  // Find the key we are moving to
  for(key=0;key<keys;key++)
    if(curTime<keyTime[key])break;
  if(key==keys)
  { // End of keyframes reached; because of dblbuffering, equalize
    flags|=QCONTROLF_EQUALIZE;
    //Done(TRUE);
    //keys=0;
    return;
  }
  if(key==0){ printf("BUG: QControl:Iterate key==0\n"); return; }

  frames=keyTime[key]-keyTime[key-1];
  vFrom=keyValue[key-1];
  vTo=keyValue[key];
  frameInSegment=curTime-keyTime[key-1];
  oldv=vCurrent;

  if(vFrom==vTo||frames<2)
  { vCurrent=vTo;
  } else
  { // Linear interpolation
    vCurrent=vFrom+(vTo-vFrom)*frameInSegment/(frames-1);
  }

  // Notify control owner
  if(cbFunc!=0)
  { if(curTime==0||vCurrent!=oldv)
      cbFunc(this,cbPtr,vCurrent);
  }
  curTime++;
}

void QControl::SetCallback(QControlCallback func,void *clientPtr)
{ cbPtr=clientPtr;
  cbFunc=func;
}

/*void QControl::SetController(QGel *object,int _control)
{ obj=object;
  control=_control;
}*/

int QControl::GetCurrent()
{ return vCurrent;
  //if(env)return env->GetCurrent();
}
int QControl::Get()
{ //if(env)return env->GetCurrent();
  return vCurrent;
}

