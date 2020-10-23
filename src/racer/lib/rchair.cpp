/*
 * Racer - chair control
 * 17-01-01: Created!
 * 11-11-01: Packet's 'or' member renamed 'orient' because of name conflicts.
 * NOTES:
 * - This module will output car data relevant to control a pneumatic chair
 * for example. Actually, what the receiving end does is not of influence
 * to the sim. However, the data should be sufficient to capture and control
 * a chair for example. Other uses could be some visual display of the
 * data.
 * - Data is sent out in packets; each packet can contain one or more IDs,
 * followed by data. Like: <id> <data...> <id> <data...>
 * For a given ID, the data size is known, although some IDs in the future
 * may use a length (strings for example) to become: <id> <length> <data>.
 * For performance reasons this will not be done in general.
 * A packet contains small chunks of: struct { int16 id; char data[]; }.
 * Defined IDs are:
 *   ID_ACC (1): float acc[3] - the XYZ acceleration of the main body
 *                              in m/s^2. To convert to G-forces, divide
 *                              by 9.81.
 *   ID_OR (2) : float or[9]  - A 3x3 matrix which represents the body
 *                              orientation. The matrix is column-major (?)
 * Data is sent in Intel endianness format.
 * FUTURE:
 * - SGI output may be supported in the future.
 * - QNChannel may be used for higher level message handling.
 * BUGS:
 * - SGI output is currently unsupported. Any attempt to do so will send
 * out data in the wrong endianness.
 * (c) Dolphinity/RvG
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <racer/chair.h>
DEBUG_ENABLE

struct PacketAO
// Default packet with acceleration and orientation
{
  int   time;
  short id0;
  DVector3 acc;
  short id1;
  DMatrix3 orient;
};

/*******
* Ctor *
*******/
RChair::RChair()
{
  flags=0;
  socket=0;
  //address=0;
  chan=0;

  timePerUpdate=RMGR->GetMainInfo()->GetInt("chair.time_per_update");
  nextUpdateTime=0;

  // Auto-create if desired by the user
  if(RMGR->GetMainInfo()->GetInt("chair.enable"))
    Enable();
}
RChair::~RChair()
{
  Disable();
}

void RChair::Enable()
// Turn on chair updates
{
qdbg("RChair:Enable()\n");
  flags|=ENABLE;
  // Open socket for output to the port
  socket=new QNSocket();
  RMGR->GetMainInfo()->GetString("chair.host",host);
  port=RMGR->GetMainInfo()->GetInt("chair.port");
  // Record in log file
  qlog(QLOG_INFO,"Chair control to %s:%d",host.cstr(),port);
  if(!socket->Open(port,QNSocket::UNRELIABLE))
  {
    qwarn("RChair:Enable(); can't open socket at port %d",port);
    // Give up
    QDELETE(socket);
    flags&=~ENABLE;
    return;
  }
  address.SetAttr(AF_INET,INADDR_ANY,port);
  address.GetByName(host);
}

void RChair::Disable()
// Turn off chair updates
{
  flags&=~ENABLE;
  QDELETE(chan);
  QDELETE(socket);
  //QDELETE(address);
}

bool RChair::IsTimeForUpdate()
{
  // Never time for an update if it's not enabled
  if(!IsEnabled())return FALSE;
  if(RMGR->time->GetSimTime()>nextUpdateTime)return TRUE;
  return FALSE;
} 
void RChair::Update()
// It is update time and send out info if it is time.
// Assumes IsTimeForUpdate() has been called and returned TRUE.
{
  RCar *car;

  // Never time for an update if it's not enabled
  if(!IsEnabled())return;

//qdbg("RChair:Update(); simTime=%d, nextUpdateTime=%d\n",
//RMGR->time->GetSimTime(),nextUpdateTime);
  // Calculate next time for an update
  nextUpdateTime+=timePerUpdate;

  // Build a packet with information, if a car is present
  if(RMGR->scene->GetCars()<1)
    return;
  car=RMGR->scene->GetCar(0);
  PacketAO packet;
  packet.time=RMGR->time->GetSimTime();
  packet.id0=ID_ACC;
  packet.acc=*car->GetBody()->GetTotalTorque();
  packet.id1=ID_OR;
  packet.orient=*car->GetBody()->GetRotPosM();

  // Send out the packet
//qdbg("socket=%p\n",socket);
  socket->Write(&packet,sizeof(packet),&address);
//qdbg("  written\n",socket);
}

