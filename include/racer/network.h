// racer/network.h - Network/multiplayer facilities

#ifndef __RACER_NETWORK_H
#define __RACER_NETWORK_H

#include <nlib/message.h>
#include <nlib/socket.h>
#include <nlib/channel.h>

// Version numbers to keep track of incompatible servers
// in the future. Normally, a minor change should remain
// compatible. Major version differences will not be
// allowed (incompatible).
// Revisions:
// 1.1 = v0.4.8beta
// 1.2 = v0.4.8
#define RR_NETWORK_VERSION_MAJOR   1
#define RR_NETWORK_VERSION_MINOR   2

class RCar;

class RMessage
// A message, containing a variety of information
{
  QNMessage *msg;
  int        client;         // Index of client

 public:
  enum MsgID
  {
    ID_CONNECT_REQUEST,      // Machine wants to connect to server
    ID_ENTER_CAR,            // Request for a new car to enter the game
    ID_CONNECT_REPLY,        // Accepted? (also Server Out)
    ID_NEW_CAR,              // Whether car is accepted
    ID_STATUS_REQUEST,       // Request info on what
    ID_STATUS_REPLY,
    ID_CAR_STATE,            // Update of car state
    ID_CLOSE_CONNECTION,     // Client is going away
    // Client out
    ID_REQUEST_CARS,         // Request all cars already on host
    ID_MAX
  };

 public:
  RMessage();
  ~RMessage();

  // Attribs
  QNMessage *GetMessage(){ return msg; }
  void SetMessage(QNMessage *_msg){ msg=_msg; }

  // Target address
  void TargetServer();
  void TargetClient(int n);

  // Client messages - outgoing
  void OutConnectRequest();
  void OutStatusRequest();
  void OutEnterCar(cstring carName,int flags);
  void OutCarState(RCar *car,int flags);
  void OutCloseConnection();
  void OutRequestCars();

  // Client messages - incoming
  void InConnectReply();    // bool *yn,int *id);
  void InNewCar();

  // Server messages - outgoing
  void OutConnectReply(bool yn,int id,int reason);
  void OutStatusReply();

  // Server messages - incoming
  void InConnectRequest();
  void InEnterCar();
  void InCarState();
  void InCloseConnection();
  void InRequestCars();

  // All messages
  void Interpret();

  // Communication
  bool Send();
};

class RClient
// A client connecting to the server
{
 public:
  QNAddress addr;              // Actual IP address
  int       id;                // Server handed out this ID
  int       cars;              // Cars entered in the game
  rfloat    ping;              // Average ping time
  RCar     *car;               // The client's car

  RClient(){ Reset(); }
  void Reset()
  {
    id=0;
    ping=0;
    cars=0;
    car=0;
  }
};

class RNetwork
// Keeps a record of the network mode the program is in
{
 public:
  enum Max
  {
    MAX_CLIENT=50,             // Max #attached machines (theoretical!)
    MAX_MESSAGE=100            // Max #total message in the system
  };
  enum Flags
  {
    // Note that a single machine may be both client AND server
    // at the same time.
    IS_SERVER=1,               // Machine can host other machines
    IS_CLIENT=2,               // Machine is a client
    ALLOW_REMOTE=4             // Allow remote machines to connect
  };

 protected:
  // Network links
  QNSocket *socket;            // Communication socket - all comms go thru this
  QNChannel *chan;             // Channel to send things across

#ifdef ND
  // Preallocated messages
  RMessage *msg[MAX_MESSAGE];
#endif

  // Prefs
  bool    enable;              // Attempt network play?
  int     port;                // Which network port to use?
  qstring serverName;          // Hostname of server:
  int     flags;
  int     timeOutConnect;      // Seconds to try to connect

  // State - client
  int     clientID;            // ID given to the machine by the server (>0)
  // State - server
 public:
  RClient client[MAX_CLIENT];  // Client info
  int     clients;

 public:
  RNetwork();
  ~RNetwork();

  // Attribs
  bool    IsEnabled(){ return enable; }
  int     GetPort(){ return port; }
  cstring GetServerName(){ return serverName; }
  QNChannel *GetChannel(){ return chan; }
  void    SetClientID(int n){ clientID=n; }
  int     GetClientID(){ return clientID; }

  // Resetting
  void ResetClients();

  // Global message sharing
  RMessage *GetGlobalMessage();

  bool    IsServer(){ if(flags&IS_SERVER)return TRUE; return FALSE; }
  bool    IsClient(){ if(flags&IS_CLIENT)return TRUE; return FALSE; }
  bool    AllowsRemote(){ if(flags&ALLOW_REMOTE)return TRUE; return FALSE; }

  bool Create();
  void Destroy();
  void ReadPrefs();

  // Clients
  int  FindClient(QNAddress *addr);

  // High-level
  void Handle();
  bool ConnectToServer();
  bool Disconnect();
};

#endif

