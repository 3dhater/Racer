/*
 * RMessage - network messages
 * 12-08-01: Created!
 * NOTES:
 * - Handling of messages is done here; for all types.
 * (c) Dolphinity/RvG
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
DEBUG_ENABLE

// Local trace?
#define LTRACE

// Start ID
#define BASE_ID    9668

RMessage::RMessage()
{
  msg=0;
  client=-1;
}
RMessage::~RMessage()
{
}

/******************
* Message sending *
******************/
void RMessage::TargetServer()
// Set the destination address of the message to be the server
{
  msg->GetAddrFrom()->Set(RMGR->network->GetChannel()->GetAddrOut());
}
void RMessage::TargetClient(int n)
// Set the destination address of the message to be client #n
{
  msg->GetAddrFrom()->Set(&RMGR->network->client[n].addr);
}
bool RMessage::Send()
// Send current message.
// Returns FALSE if this failed.
{
#ifdef LTRACE
//qdbg("RMessage:Send()\n");
#endif

  if(!msg)return FALSE;

  if(!RMGR->network->GetChannel()->AddMessage(msg))
    return FALSE;

  return TRUE;
}

/******************
* Client outgoing *
******************/
void RMessage::OutConnectRequest()
// Request machine access to server
{
#ifdef LTRACE
qdbg("RMessage:OutConnectRequest()\n");
#endif
  TargetServer();
  msg->Clear();
  msg->MakeReliable();
  msg->AddChar(ID_CONNECT_REQUEST);
  msg->AddChar(RR_NETWORK_VERSION_MAJOR);
  msg->AddChar(RR_NETWORK_VERSION_MINOR);
  // IP address will be sent no matter what, and will be used
  // to determine the rest
  Send();
}
void RMessage::OutEnterCar(cstring carName,int flags)
// Request a car to be added
{
#ifdef LTRACE
qdbg("RMessage:OutEnterCar()\n");
#endif
  RMSG("Requesting car '%s' to enter field",carName);
  TargetServer();
  msg->Clear();
  msg->MakeReliable();
  msg->AddChar(ID_ENTER_CAR);
  msg->AddString(carName);
  msg->AddInt(flags);
  Send();
}
void RMessage::OutCarState(RCar *car,int flags)
// Send an update of the car state to the server
// Unreliable message (the first after connects probably)
{
#ifdef LTRACE
//qdbg("RMesssage::OutCarState()");
#endif
  TargetServer();
  msg->Clear();
  msg->AddChar(ID_CAR_STATE);
  msg->AddFloats((float*)car->GetBody()->GetPosition(),3);
  msg->AddFloats((float*)car->GetBody()->GetRotPosQ(),4);
  Send();
}
void RMessage::OutCloseConnection()
// Notify server we are leaving
{
#ifdef LTRACE
qdbg("RMessage:OutCloseConnection()\n");
#endif
  TargetServer();
  msg->Clear();
  msg->MakeReliable();
  msg->AddChar(ID_CLOSE_CONNECTION);
  Send();
}
void RMessage::OutRequestCars()
// Request server to send over all cars already on the server
{
#ifdef LTRACE
qdbg("RMessage:OutRequestCars()\n");
#endif
  TargetServer();
  msg->Clear();
  msg->MakeReliable();
  msg->AddChar(ID_REQUEST_CARS);
  Send();
}

/******************
* Client incoming *
******************/
void RMessage::InConnectReply()
// Connect reply (see InConnectRequest() for details)
{
#ifdef LTRACE
qdbg("RMsg:InConnectRequest()\n");
#endif
  bool okToEnter;
  int  reason,id;

  msg->BeginGet();
  msg->GetChar();                 // Skip msg ID
  okToEnter=msg->GetChar();
  reason=(int)msg->GetChar();
  id=msg->GetInt();

#ifdef LTRACE
qdbg("  okToEnter=%d, reason=%d, id=%d\n",okToEnter,reason,id);
#endif
  if(okToEnter==FALSE)
  {
    // Notify user
    RMSG("Can't connect to server");
  }

  // Assign ID to this host (even if not accepted)
  // It will be <=0 if the connect failed
  RMGR->network->SetClientID(id);
}
void RMessage::InNewCar()
// New car enters the field
{
  cstring reasonStr[]=
  { 
    "unknown client","car already entered",
    // Connect refusal
    "no reason","max clients reached","client already accepted",
    "no remote clients accepted","incompatible server"
  };
#ifdef LTRACE
qdbg("RMsg:InNewCar()\n");
#endif
  bool ok;
  char reason;
  char carName[256];
  int  ownerClient;             // Who owns the car?

  msg->BeginGet();
  msg->GetChar();  // ID
  ok=msg->GetChar();
  if(!ok)
  {
    // Car refused (probably after own request to enter car)
    reason=msg->GetChar();
    sprintf(carName,"Car refused (%s)",reason<2?reasonStr[reason]:"???");
    RMSG(carName);
    // Well, that's it then
    return;
  }
  // Car is accepted
  RMSG("Car is ACCEPTED");
  msg->GetString(carName);
  ownerClient=msg->GetChar();

  // Create car
  RCar *car;
  car=new RCar(carName);
  //if(client!=0)
  // Determine whether this car is controlled by a remote computer
  // Note that car0 is always the player-controlled car
  if(RMGR->scene->GetCars()>0)
  {
RMSG("This is a NETWORK car");
    // This is a remote car
    car->SetNetworkCar(TRUE);
    // Mark car for this client (for later removal on close-up)
qdbg("InNewCar(); (owner)client=%d, car=%p\n",ownerClient,car);
    RMGR->network->client[ownerClient].car=car;
  } else
  {
    RMSG("This is a LOCAL car");
  }

  // Place this on the right spot (the next free spot)
  //... determine free spot
  car->Warp(&RMGR->track->gridPos[0]);

  // Add to replay (index will be the next car slot)
  RMGR->replay->RecNewCar(car->GetShortName(),RMGR->scene->GetCars());

  // Add to scene
  RMGR->scene->AddCar(car);

#ifdef OBS_IN_RSCENE
  if(RMGR->scene->GetCars()==1)
  {
    // This is the only car; focus on it
   RMGR->scene->SetCam(0);
   RMGR->scene->SetCamCar(car);
  }
#endif

  // Auto load? I know, it is an odd place to do the load
  if(RMGR->infoDebug->GetInt("state.auto_load"))
    RMGR->LoadState();
//qdbg("  Car is accepted\n");
}

/******************
* Server outgoing *
******************/
void RMessage::OutConnectReply(bool yn,int id,int reason)
// Sends message ID_CONNECT_REPLY with ok, id and reason.
// reason: 0=no particular reason, 1=out of space, 2=client already connected
// Decision is already made by the caller.
{
#ifdef LTRACE
qdbg("RMsg:OutConnectReply(ok=%d,id=%d,reason=%d)\n",yn,id,reason);
#endif

  // Reply message
  msg->Clear();
  msg->AddChar(ID_CONNECT_REPLY);
  msg->AddChar(yn);
  msg->AddChar(reason);
  msg->AddInt(id);
  Send();
}

/******************
* Server incoming *
******************/
void RMessage::InConnectRequest()
// Someone wants to connect.
// Returns OK or not OK in a net message with a reason.
// Reason: 0=ok, 1=max clients reached, 2=ok AGAIN, 3=no remote clients
// allowed, 4=incompatible server (version mismatch)
// (3 means that the server doesn't want network play)
{
#ifdef LTRACE
qdbg("RMsg:InConnectRequest()\n");
#endif
  bool okToEnter;
  int  reason=0,id=0;
  int  i,vMajor,vMinor;
  RNetwork *nw=RMGR->network;

  // Handle request to connect to this game
  msg->BeginGet();
  msg->GetChar();     // ID
  vMajor=msg->GetChar();
  vMinor=msg->GetChar();
  RCON->printf("Incoming connect request from %s, v%d.%d",
    msg->GetAddrFrom()->ToString(),vMajor,vMinor);

//qdbg("  clients=%d\n",nw->clients);

  if(!nw->AllowsRemote())
  {
    // Can't join in; this is a local game
    okToEnter=FALSE;
    reason=3;
  } else if(vMajor!=RR_NETWORK_VERSION_MAJOR)
  {
    // Incompatible network version
    okToEnter=FALSE;
    reason=4;
  } else if(nw->clients>=RNetwork::MAX_CLIENT)
  {
    // No more space
    okToEnter=FALSE;
    reason=1;
  } else
  {
    // Client already active/seen?
    i=nw->FindClient(msg->GetAddrFrom());
    if(i>=0)
    {
      // Grant access (again); it may be that the
      // client thought it dropped a packet, so be gentle
      okToEnter=TRUE;
      id=BASE_ID+i;
      reason=2;
      goto send_reply;
    }
    // New client
    // Grant access, hey, happy to have you here
    okToEnter=TRUE;
//qdbg("  clients=%d\n",nw->clients);
    i=nw->clients;
    id=BASE_ID+i;
    nw->client[i].addr=*msg->GetAddrFrom();
    nw->client[i].Reset();
    nw->clients++;
    RMSG("New client '%s'",nw->client[i].addr.ToString());
  }

 send_reply:
  // Reply to client whether connect was ok
  OutConnectReply(okToEnter,id,reason);
}
void RMessage::InEnterCar()
// Client requests a new car to enter the field
// If refused, reasons are:
//   0 - unknown client
//   1 - client already has a car
{
#ifdef LTRACE
qdbg("RMessage:InEnterCar()\n");
#endif
  char carName[256];
  int  flags;
  int  i,reason;

  msg->BeginGet();
  msg->GetChar();     // ID
  msg->GetString(carName);
  flags=msg->GetInt();
#ifdef LTRACE
qdbg("  car '%s', flags=%d\n",carName,flags);
#endif

  if(client==-1)
  {
    reason=0;
   deny:
//qdbg("  DENY car\n");
    // Unknown client; must connect first; refuse car
    msg->Clear();
    msg->MakeReliable();
    msg->AddChar(ID_NEW_CAR);
    msg->AddChar(FALSE);
    msg->AddChar(reason);
    Send();
  } else
  {
//qdbg("  check, client=%d\n",client);
    RClient *cl=&RMGR->network->client[client];
//qdbg("  cl=%p\n",cl);
    if(cl->cars>0)
    {
      // Client already has a car in
      reason=1;
      goto deny;
    }
qdbg("  ACCEPT car\n");
    msg->Clear();
    msg->MakeReliable();
    msg->AddChar(ID_NEW_CAR);
    msg->AddChar(TRUE);
    msg->AddString(carName);
    msg->AddChar(client);
    // Send to all clients; get the car on everbody's machine
    for(i=0;i<RMGR->network->clients;i++)
    {
      TargetClient(i);
      Send();
    }

    // Car is in; notice this here already
    cl->cars++;

  }
}
void RMessage::InCarState()
// A client sends us his state
{
//qdbg("RMessage:InCarState(); client=%d\n",client);
  if(client==0)
  {
    // This is just us, the client AND server. Ignore.
    return;
  }

  if(client<RMGR->scene->GetCars())
  {
qdbg("RMessage:InCarState(); client=%d\n",client);
    // Bad client (bad packet, perhaps a left-over
    // from a previous run)
    if(client<0)return;

    RCar *car;

    car=RMGR->scene->GetCar(client);
    msg->BeginGet();
    msg->GetChar();  // ID
    msg->GetFloats((float*)car->GetBody()->GetPosition(),3);
    msg->GetFloats((float*)car->GetBody()->GetRotPosQ(),4);
    car->GetBody()->CalcMatrixFromQuat();
  }
}
void RMessage::InCloseConnection()
// Client closes connection
{
  int i;

#ifdef LTRACE
qdbg("RMsg: connection closing by client %d\n",client);
#endif

  RCON->printf("%s closes connection.\n",msg->GetAddrFrom()->ToString());
  // Take client's cars out of game
qdbg("Client car=%p\n",RMGR->network->client[client].car);
  for(i=0;i<RMGR->scene->cars;i++)
  {
qdbg("  car%d=%p?\n",i,RMGR->scene->car[i]);
    if(RMGR->scene->car[i]==RMGR->network->client[client].car)
    {
qdbg("Found car %d to be of this client\n");
      // Remove car from list (should be in RScene)
      for(i=i+1;i<RMGR->scene->cars;i++)
      {
        RMGR->scene->car[i-1]=RMGR->scene->car[i];
      }
      RMGR->scene->cars--;
      break;
    }
  }
  // Take client out of network
  for(i=client+1;i<RMGR->network->clients;i++)
  {
    RMGR->network->client[i-1]=RMGR->network->client[i];
  }
  RMGR->network->clients--;
}
void RMessage::InRequestCars()
// Client wants to know about all cars on the server
{
  int i;
#ifdef LTRACE
qdbg("RMessage:InRequestCars()\n");
#endif

  for(i=0;i<RMGR->scene->cars;i++)
  {
    msg->Clear();
    msg->MakeReliable();
    msg->AddChar(ID_NEW_CAR);
    msg->AddChar(TRUE);
    msg->AddString(RMGR->scene->car[i]->GetName());
    // BUG; send correct cliend id for this car
    msg->AddChar(0);
    //Send();
qdbg("Send car %d (%s)\n",i,RMGR->scene->car[i]->GetName());
  }
}

/*******************
* Global interpret *
*******************/
void RMessage::Interpret()
{
  char id;

  // Check for a message to be associated
  if(!msg)return;

  // Dispatch message handler
  id=*msg->GetBuffer();

  // Try to find client that sent this
  client=RMGR->network->FindClient(msg->GetAddrFrom());
  if(client==-1&&id!=ID_CONNECT_REQUEST)
  {
    // Unknown clients may not do anything accept
    // try to connect
//qdbg("## RMessage: refuse msg id=%d (unknown client)\n",id);
    return;
  }

#ifdef LTRACE
if(id!=ID_CAR_STATE)
 qdbg("## RMessage:Interpret(); id %d, from '%s' (client %d)\n",
 id,msg->GetAddrFrom()->ToString(),client);
#endif
  // Server commands
  if(RMGR->network->IsServer())
  {
    switch(id)
    {
      // Server commands
      case ID_CONNECT_REQUEST : InConnectRequest(); goto done;
      case ID_ENTER_CAR       : InEnterCar(); goto done;
      case ID_CAR_STATE       : InCarState(); goto done;
      case ID_CLOSE_CONNECTION: InCloseConnection(); goto done;
      case ID_REQUEST_CARS    : InRequestCars(); goto done;
    }
  }

  // Client commands (may split into 2 id switches?)
  if(RMGR->network->IsClient())
  {
    switch(id)
    {
      case ID_CONNECT_REPLY: InConnectReply(); break;
      case ID_NEW_CAR      : InNewCar(); break;
    }
  }

 done:
  return;
}

