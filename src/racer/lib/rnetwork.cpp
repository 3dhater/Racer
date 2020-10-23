/*
 * RNetwork - network messages
 * 12-08-01: Created!
 * (c) Dolphinity/RvG
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
DEBUG_ENABLE

// Local trace?
#define LTRACE

// Messages
static QNMessage msg;
static RMessage  rmsg;

RNetwork::RNetwork()
{
  // Init
  socket=0;
  chan=0;
  flags=0;

  // Client
  clientID=0;
  // Server
  clients=0;

  // Get prefs; may not to re-get later on in the game
  // when the player plays around with the settings
  ReadPrefs();

  timeOutConnect=RMGR->GetMainInfo()->GetInt("multiplayer.timeout_connect");

  // Init global message for localhost communication
  // Should get it from libQN (qnLocalAddr) actually
  msg.GetAddrFrom()->SetAttr(AF_INET,INADDR_ANY,port);
  msg.GetAddrFrom()->GetByName("localhost");
  rmsg.SetMessage(&msg);

  // Hardcoded prefs
  //...

#ifdef LTRACE
  qdbg("RNetwork: %s:%d (enabled=%d)\n",serverName.cstr(),port,enable);
#endif
}                 
RNetwork::~RNetwork()
{
  Destroy();
}                 

RMessage *RNetwork::GetGlobalMessage()
// Provides access to the 1 allocated global message
{
  return &rmsg;
}
 
void RNetwork::ResetClients()
// Reset to state with no connected clients. Unhook all clients.
{
  int i;

#ifdef LTRACE
qdbg("RNetwork:ResetClients()\n");
#endif

  //for(i=0;i<clients;i++)
  for(i=0;i<MAX_CLIENT;i++)
  {
    client[i].Reset();
  }
  // Restart accepting clients
  clients=0;
  clientID=0;
}

/*****************
* Create/Destroy *
*****************/
bool RNetwork::Create()
// Setup all the stuff required for networking (client/server)
{
#ifdef LTRACE
qdbg("RNetwork:Create()\n");
#endif
  // Open a UDP socket
  socket=new QNSocket();
  if(!socket->Open(port,QNSocket::UNRELIABLE))
  {
    qerr("Can't open network socket at port %d",port);
    enable=FALSE;
    return FALSE;
  }
  // Set non-blocking to avoid blocking on read/write
  socket->SetNonBlocking(TRUE);

  // Detect whether the server is the local machine
  // Should compare the local host actual machine name to the
  // server address actually (IP numbers), but we'll take
  // a shortcut for now.
  if(!strcmp(serverName,"localhost"))
  {
    flags|=IS_SERVER;
  }
  // All machines currently are clients as well (no fulltime
  // server-only; there's always client activity as well)
  flags|=IS_CLIENT;

  // Setup a channel that is the transport from the local host
  // to the remote host (which may still be on the same machine)
  chan=new QNChannel();
  chan->AttachSocket(socket);
  // Declare remote host as the target for communications
  chan->SetTarget(serverName,port);

  return TRUE;
}
void RNetwork::Destroy()
{
  // Make sure to delete channel before the socket, in case
  // of multithreading
  QDELETE(chan);
  QDELETE(socket);
}
void RNetwork::ReadPrefs()
// Reads the preferences from the main ini file (racer.ini)
// Note that is called every time you run a game (to allow for
// switching network play to non-network play)
{
  enable=(bool)RMGR->GetMainInfo()->GetInt("multiplayer.enable");
  port=RMGR->GetMainInfo()->GetInt("multiplayer.port");
  RMGR->GetMainInfo()->GetString("multiplayer.server",serverName);
  // Remote connects (be a server to incoming clients?)
  if(RMGR->GetMainInfo()->GetInt("multiplayer.allow_remote"))
    flags|=ALLOW_REMOTE;
}

/*********
* Handle *
*********/
void RNetwork::Handle()
// Poll network events
{
  // Handle network traffic
  chan->Handle();

  // Any packets arrived?
  if(chan->GetMessage(&msg))
  {
    // Handle it!
    rmsg.Interpret();
  }
}

/**********
* Clients *
**********/
int RNetwork::FindClient(QNAddress *addr)
// Based on the IP address, find the client index
// Returns -1 if not found.
{
  int i;
  for(i=0;i<clients;i++)
  {
    if(addr->Compare(&client[i].addr))
      return i;
  }
  // Not found; may be a new client
  return -1;
}

/*************
* High-level *
*************/
bool RNetwork::ConnectToServer()
// Attempt to connect to the server. Returns TRUE if succesful
{
qdbg("RNetwork:ConnectToServer(); clients=%d\n",clients);

  // Clear ID
  clientID=0;

  // Send out request
  rmsg.OutConnectRequest();

  // Wait for reply
  QTimer tmr;
  tmr.Start();
qdbg("RNetwork: waiting for connecting id...\n");
  while(tmr.GetMilliSeconds()<timeOutConnect)
  {
    // Listen
    Handle();

    // If an acknowledge came in, we can break
    if(clientID)break;
  }
  if(clientID<=0)
  {
    // Could not connect
    return FALSE;
  }

qdbg("RNetwork: succesfully connected to server.\n");
  // Success!
  return TRUE;
}
bool RNetwork::Disconnect()
{
  rmsg.OutCloseConnection();
  return TRUE;
}

