/*
 * QIVS - SysCom's IVS voting system - server control
 * 24-11-00: Created!
 * NOTES:
 * - QIVS acts as a client of the IVS server
 * (c) MarketGraph/RvG
 */

#include <qlib/ivs.h>
#include <bstring.h>
//#include <process.h>
#ifdef linux
#include <sys/time.h>
#endif
#include <unistd.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#undef  DBG_CLASS
#define DBG_CLASS "QIVS"

/************
* CTOR/DTOR *
************/
QIVS::QIVS()
{
  DBG_C("ctor")
  socket=0;
#ifdef OBS
  keypads=100;
  votes=0;
#endif
  maxPoll=10;
  // Come up with an application ID
  appID=getpid();
}
QIVS::~QIVS()
{
  DBG_C("dtor")
  Close();
}

/**********
* Helpers *
**********/
static void cvtShorts(ushort *buffer,long n)
// Reverse byte ordering of 'n' shorts
{
  short i;
  unsigned short swrd;

  for(i=0;i<n;i++)
  { swrd=*buffer;
    *buffer++=(swrd>>8)|(swrd<<8);
  }
}

/*************
* Open/close *
*************/
bool QIVS::Open(cstring host,int port)
{
  socket=new QClientSocket();
  if(!socket->Open(host,port))
  {
    qerr("Can't open IVS at %s:%d",host,port);
    return FALSE;
  }
  return TRUE;
}

void QIVS::Close()
{
  if(socket)
  { delete socket;
    socket=0;
  }
}

int QIVS::GetFD()
// Return file descriptor for network reading/writing
// Returns -1 in case of failure
{
  DBG_C("GetFD")
  
  if(!socket)return -1;
  return socket->GetFD();
}

/*************
* Read/Write *
*************/
bool QIVS::SendCommand(QIVS_MsgHdr *hdr,int params,char *param)
// Send a command to the IVS server
{
  DBG_C("SendCommand")

  int fd,n,len,msgLen;
  char msg[1024];
  
  fd=GetFD();
  len=sizeof(*hdr);
  // Calculate length of message
  msgLen=len+params*sizeof(char);
  hdr->len=msgLen;
  hdr->appID=appID;
  hdr->nr=0;
  // Combine into 1 message
  memcpy(msg,hdr,sizeof(*hdr));
  memcpy(&msg[sizeof(*hdr)],param,params);
hdr->DbgPrint("QIVS:SendCommand");
  hdr->ToHost();
  n=write(fd,msg,msgLen);
  // Re-reverse byte ordering to keep structure 'constant'
  hdr->ToHost();
  if(n!=msgLen)
  {
    qwarn("QIVS:SendCommand(); could not send all chars");
    return FALSE;
  }
  return TRUE;
}
bool QIVS::RecvCommand(QIVS_MsgHdr *hdr,int *params,char *param)
// Receive a command from the IVS server
// Returns FALSE if no message could be read within some time
{
  DBG_C("RecvCommand")

  short msgLen;
  int fd,len;
  fd_set fdset;
  struct timeval timeout;
  char msg[1024];
  
  fd=GetFD();
  FD_ZERO(&fdset);
  FD_SET(fd,&fdset);
  bzero(&timeout,sizeof(timeout));
  //timeout.tv_usec=500000;
  timeout.tv_usec=500;
  select(fd+1,&fdset,0,0,&timeout);
  if(FD_ISSET(fd,&fdset))
  {
    // Something to read
    len=read(fd,&msgLen,sizeof(msgLen));
    if(len>=0)
    {
      // We have read message length
      *(short*)msg=msgLen;
#ifndef WIN32
      cvtShorts((ushort*)&msgLen,1);
#endif
//qdbg("RecvCmd; message length %d\n",msgLen);
    }
    // Read rest of message in 'msg'
    *params=msgLen-sizeof(QIVS_MsgHdr)/sizeof(char);
    len=read(fd,&msg[2],msgLen-2);
    if(len>=0)
    {
//qdbg("QIVS:RecvCommand; params=%d\n",*params);
      // Get header
      memcpy(hdr,msg,sizeof(QIVS_MsgHdr));
      hdr->ToHost();
//hdr->DbgPrint("QIVS:RecvCommand");
      // Get params
      if(param)
      {
        int i;
        for(i=0;i<*params;i++)
        {
          param[i]=msg[sizeof(QIVS_MsgHdr)+i];
        }
      }
    }
  } else
  {
    // Timed out
    return FALSE;
  }
  return TRUE;
}

void QIVS::Flush()
// Flush all pending commands
{
  QIVS_MsgHdr hdr;
  char param[100];
  int  params;
  int  count=0;
  
  while(RecvCommand(&hdr,&params,param))count++;
qdbg("QIVS:Flush(); flushed %d commands\n",count);
}

/***************
* Higher level *
***************/
bool QIVS::RequestID(int *id)
// Request ID of keyreader's placed key
// Returns FALSE in case of failure (and sets id to 0 in that case)
{
  DBG_C("RequestID")
  
  QIVS_MsgHdr hdr;
  char param[100];
  int  params;
  
  // ID_REQ
  hdr.cmd=0x1;
  hdr.subCmd=0x40;
  SendCommand(&hdr);
  if(!RecvCommand(&hdr,&params,param))
  {
    *id=0;
    return FALSE;
  }
  *id=1;
  return TRUE;
}

bool QIVS::ReadCommand()
// Just reads an incoming command
{
  DBG_C("ReadCommand")
  
  QIVS_MsgHdr hdr;
  char param[100];
  int  params;
  
  if(!RecvCommand(&hdr,&params,param))
  {
    return FALSE;
  }
  // Check for votes
  if(hdr.cmd==0x2&&hdr.subCmd==0x42)
  {
    int key;
    if(params>=5)key=param[4];
    else         key=-1;
qdbg("Vote; key='%c'\n",key);
#ifdef OBS
qdbg("  params: %x,%x,%x,%x,%x\n",param[0],param[1],param[2],param[3]
,param[4]);
#endif
  }
  return TRUE;
}

/*********
* VOTING *
*********/
void QIVS::SetKeypads(int n)
{
  result.countKeypads=n;
}
void QIVS::SetMaxPoll(int n)
{
  maxPoll=n;
}

bool QIVS::OpenVoting(int max,bool yn)
// Opens a vote; people can press now
// Returns FALSE in case of failure
{
  DBG_C("OpenVoting")
  
  QIVS_MsgHdr hdr;
  char param[100];
  int  params,i;
  
  // OPEN_REQ
  hdr.cmd=0x3;
  hdr.subCmd=0x40;
  params=6;
  param[0]=1;        // TimesToVote
  param[1]=0;        // Red button function
  // First/last key
  if(yn)
  {
    param[2]=11;     // 'Y' key
    param[3]=13;     // '?' key
  } else
  {
    // Normal MC
    param[2]=1;
    param[3]=max;
  }
  param[4]=0;         // UseKey (0=no key, !0=key)
  param[5]=0;         // Time registration (UseTime)
  SendCommand(&hdr,params,param);
#ifdef OBS
  if(!RecvCommand(&hdr,&params,param))
  {
    return FALSE;
  }
#endif
  
  // Start voting numbers
  result.ResetVoting();
  return TRUE;
}
bool QIVS::CloseVoting()
// Closes a vote
// Returns FALSE in case of failure
{
  DBG_C("OpenVoting")
  
  QIVS_MsgHdr hdr;
  char param[100];
  int  params;
  
  // OPEN_REQ
  hdr.cmd=0x4;
  hdr.subCmd=0x40;
  SendCommand(&hdr);
#ifdef OBS_FORGET_IT
  if(!RecvCommand(&hdr,&params,param))
  {
    return FALSE;
  }
#endif
  return TRUE;
}
int QIVS::KeyToIndex(char c)
// Returns index of 'c'; '1' => 0 etc.
// This is used to get voting results neatly in an array
{
  switch(c)
  {
    case '1': return 0;
    case '2': return 1;
    case '3': return 2;
    case '4': return 3;
    case '5': return 4;
    case '6': return 5;
    case '7': return 6;
    case '8': return 7;
    case '9': return 8;
    case '0': return 9;
    default: return 15;         // Unused; don't let caller crash
  }
}

/**************************
* Old voting manipulation *
**************************/
void QIVS::SetVotes(int n)
// To manually modify votes 
{
  result.countVotes=n;
}
int QIVS::GetVotesFor(int n)
{
  return result.GetVotesFor(n);
}
void QIVS::SetVotesFor(int n,int count)
// Looks more like manipulation, this
{
  result.SetVotesFor(n,count);
}

/**********
* Polling *
**********/
bool QIVS::PollVoting()
// Keep up with incoming messages that indicate votes
// Polls until no messages are present anymore, or the number
// of messages processed exceeds 'maxPoll' (see SetMaxPoll()).
// Returns TRUE if some votes were seen
{
  DBG_C("PollVoting")
  
  QIVS_MsgHdr hdr;
  char param[100];
  union theID
  { char c[4];
    long id;
  } anID;
  int  params;
  bool r=FALSE;
  int  countMessages=0;
  
 next_vote:
  if(!RecvCommand(&hdr,&params,param))
  {
    return r;
  }
  
  // Check for votes
  if(hdr.cmd==0x2&&hdr.subCmd==0x42)
  {
    int  key,index;
    long id;
    
    if(params>=5)key=param[4];
    else         key=-1;
    // Votes!
    r=TRUE;
    index=KeyToIndex(key);
    result.countVotes++;
    result.voteCount[index]++;
    // Store keypad ID to remember who voted what
    // Note the id's are from a PC, so big endian is used
    anID.c[0]=param[3];
    anID.c[1]=param[2];
    anID.c[2]=param[1];
    anID.c[3]=param[0];
    id=anID.id;
    result.SetVoteForKeypad(id,index);
qdbg("Vote; key='%c' for id %d\n",key,id);
    
//#ifdef OBS
qdbg("  params: %x,%x,%x,%x,%x\n",param[0],param[1],param[2],param[3],
  param[4]);
//#endif
    if(++countMessages<maxPoll)
    {
      // Check for more votes
      goto next_vote;
    } else
    {
      // A lot of messages waiting, but make sure the main thread
      // gets to do some work too; return here.
    }
  }
  return TRUE;
}

/*****************
* Message header *
*****************/
#undef  DBG_CLASS
#define DBG_CLASS "QIVS_MsgHdr"

void QIVS_MsgHdr::ToHost()
// Reverses bytes if on non-PC (big-endian?)
{
#ifdef WIN32
  // No need to swap on Win32; byte ordering is ok
#else
  cvtShorts((ushort*)&len,1);
  cvtShorts((ushort*)&appID,1);
  cvtShorts((ushort*)&nr,1);
#endif
}

void QIVS_MsgHdr::DbgPrint(cstring s)
// Dumps structure to debug output
{
  DBG_C("DbgPrint")
  
  qdbg("%s: len=%d, cmd=0x%x, sub=0x%x, appID=%d, nr=%d\n",
    s,len,cmd,subCmd,appID,nr);
}

/*************
* QIVSResult *
*************/
void QIVSResult::SetVotes(int n)
// To manually modify votes.
{
  countVotes=n;
}
int QIVSResult::GetVotesFor(int n)
{
  QASSERT_0(n>=0&&n<QIVS_MAX_CHOICE);
  return voteCount[n];
}
void QIVSResult::SetVotesFor(int n,int count)
// Looks more like manipulation, this.
{
  QASSERT(n>=0&&n<QIVS_MAX_CHOICE);
  voteCount[n]=count;
}
int QIVSResult::GetVoteCountForGroup(int choice,int group,QIVSGrouping *grp)
// Returns #votes for choice 'choice', voted by group 'group'
{
  int i,g;
  int count=0;
  
qdbg("QIVSResult:GVCFG(choice=%d, group=%d)\n",choice,group);
  // Look at all keypads
  for(i=0;i<countKeypads;i++)
  {
//qdbg("  [%d]=group %d, choice %d\n",i,grp->GetGroup(i),voteForKeypad[i]);
    if(keypadID[i]==-1)continue;
    
    // Wrong group?
    g=grp->GetGroupForID(keypadID[i]);
    if(g!=group)continue;
    
    // Wrong choice?
    if(voteForKeypad[i]!=choice)continue;
    
    count++;
  }
qdbg("QIVSResult:GetVoteCountForGroup(%d,%d)=%d\n",choice,group,count);
  return count;
}
void QIVSResult::ResetVoting()
// Reset variables for a new voting.
// Number of keypads is untouched.
{
  int i;
  
  countVotes=0;
  for(i=0;i<QIVS_MAX_CHOICE;i++)
  {
    voteCount[i]=0;
  }
  for(i=0;i<QIVS_MAX_KEYPAD;i++)
  {
    voteForKeypad[i]=-1;
  }
}
void QIVSResult::ResetKeypadIDs()
// Reset all keypad ID's
// Is really never really necessary
{
  int i;
  for(i=0;i<QIVS_MAX_KEYPAD;i++)
  {
    keypadID[i]=-1;
  }
}

/*******************
* Keypad functions *
*******************/
void QIVSResult::SetVoteForKeypad(long id,int index)
{
  int n;
  
  if(id==-1)return;
  
  // Find the place to store the vote
  n=GetIndexFromKeypad(id);
  // If the id was not yet found, store it at a free spot
  if(n==-1)
  { n=GetFreeKeypadIndex();
    keypadID[n]=id;
  }
  
  // Check if we can store this info
  if(n<0||n>=QIVS_MAX_KEYPAD)
  { qwarn("QIVSResult:SetVoteForKeypad(0x%x,%d) no space for new ids",
      id,index);
    return;
  }

  // Store the exact vote for this keypad
  voteForKeypad[n]=index;
qdbg("QIVSResult: voteForKeypad[id %d]=%d\n",id,index);
}
int QIVSResult::GetIndexFromKeypad(long id)
// Given a keypad id, return the index into the voting array
// Returns -1 if not found
{
  int i;
  for(i=0;i<QIVS_MAX_KEYPAD;i++)
  {
    if(keypadID[i]==id)return i;
  }
  return -1;
}
int QIVSResult::GetFreeKeypadIndex()
// Returns a slot where you can put a new keypad id
// Returns -1 if all full
{
  int i;
  for(i=0;i<QIVS_MAX_KEYPAD;i++)
  {
    if(keypadID[i]==-1)return i;
  }
  return -1;
}

/*****************
* QIVSResult I/O *
*****************/
bool QIVSResult::Load(cstring fname)
{
  FILE *fp;
  fp=fopen(fname,"rb");
  if(!fp){ qwarn("QIVSResult: can't load '%s'",fname); return FALSE; }
  fread(&countKeypads,1,sizeof(int),fp);
  fread(&countVotes,1,sizeof(int),fp);
  fread(voteForKeypad,QIVS_MAX_KEYPAD,sizeof(char),fp);
  fread(keypadID,QIVS_MAX_KEYPAD,sizeof(long),fp);
  fread(voteCount,QIVS_MAX_CHOICE,sizeof(int),fp);
  fclose(fp);
  return TRUE;
}
bool QIVSResult::Save(cstring fname)
{
  FILE *fp;
  fp=fopen(fname,"wb");
  if(!fp){ qwarn("QIVSResult: can't save '%s'",fname); return FALSE; }
  fwrite(&countKeypads,1,sizeof(int),fp);
  fwrite(&countVotes,1,sizeof(int),fp);
  fwrite(voteForKeypad,QIVS_MAX_KEYPAD,sizeof(char),fp);
  fwrite(keypadID,QIVS_MAX_KEYPAD,sizeof(long),fp);
  fwrite(voteCount,QIVS_MAX_CHOICE,sizeof(int),fp);
  fclose(fp);
  return TRUE;
}

void QIVSResult::GetFromString(cstring s)
// Retrieve voting results from a string
// For example: s="20 50 30"
{
  cstring t;
  int  total,n,i;
  int  count[QIVS_MAX_CHOICE];
  char buf[200];
  
  // Reset
  for(i=0;i<QIVS_MAX_CHOICE;i++)
    count[i]=0;
    
//qdbg("QIVSResult::GetFromString(%s)\n",s);
  strcpy(buf,s);
  n=0;
  for(t=strtok(buf," ");t;t=strtok(0," "))
  {
//qdbg("token '%s'\n",t);
    count[n]=Eval(t);
    if(++n>=QIVS_MAX_CHOICE)break;
  }
  // Calc total
  for(i=total=0;i<QIVS_MAX_CHOICE;i++)
    total+=count[i];
  
  // Manually set vote state
//qdbg("Total votes: %d\n",total);
  countVotes=total;
  for(i=0;i<QIVS_MAX_CHOICE;i++)
  {//qdbg("  votes for %d: %d\n",i,count[i]);
    voteCount[i]=count[i];
  }
}
void QIVSResult::ToString(string s,int maxChoice)
// Create a string from the votes
// 's' must be at least 100 bytes long.
// No more than QIVS_MAX_CHOICE will be put into the string (or maxChoice)
{
  int i;
  char buf[100],buf2[100];
  
  buf2[0]=0;
  for(i=0;i<QIVS_MAX_CHOICE&&i<maxChoice;i++)
  {
    if(i>0)strcat(buf2," ");
    sprintf(buf,"%d",voteCount[i]);
    strcat(buf2,buf);
  }
  strcpy(s,buf2);
}

/***********************
* QIVSResult debugging *
***********************/
void QIVSResult::DbgPrint(cstring s)
{
  int i;
  qdbg("QIVSResult; %s\n",s);
  qdbg("  keypads=%d, votes=%d, votes are:\n",countKeypads,countVotes);
  for(i=0;i<QIVS_MAX_KEYPAD;i++)
  {
    if(keypadID[i]==-1)continue;
    qdbg("  keypad id %d voted index %d\n",keypadID[i],voteForKeypad[i]);
  }
  for(i=0;i<QIVS_MAX_CHOICE;i++)
    qdbg("  summary choice %d = %d\n",i,voteCount[i]);
}

/***************
* QIVSGrouping *
***************/
void QIVSGrouping::CreateFromResult(QIVSResult *r)
// Create grouping based on a voting result
// The votes are directly used as groups. This class is more
// here for verboseness.
{
  int i;
  
  count=r->countKeypads;
  for(i=0;i<count;i++)
  {
    group[i]=r->voteForKeypad[i];
  }
  // Clear the rest
  for(;i<QIVS_MAX_KEYPAD;i++)
    group[i]=-1;

  // Copy keypad id's
  for(i=0;i<QIVS_MAX_KEYPAD;i++)
    keypadID[i]=r->keypadID[i];
}
#ifdef ND_DO_NOT_USE
int QIVSGrouping::GetGroup(int n)
// Old way of accessing the array directly
{
  if(n<0)return -1;
  if(n>=count)return -1;
  return group[n];
}
#endif
int QIVSGrouping::GetGroupForID(long id)
// Returns group for keypad with id 'id'
// Returns -1 if the keypad was not found (the voter probably never voted)
{
  int i;
  
  // Any valid id used?
  if(id==-1)return -1;
  
  for(i=0;i<QIVS_MAX_KEYPAD;i++)
  {
    if(id==keypadID[i])return group[i];
  }
  return -1;
}

bool QIVSGrouping::Load(cstring fname)
{
  FILE *fp;
  fp=fopen(fname,"rb");
  if(!fp){ qwarn("QIVSGrouping: can't load '%s'",fname); return FALSE; }
  fread(&count,1,sizeof(int),fp);
  fread(group,QIVS_MAX_KEYPAD,sizeof(char),fp);
  fread(keypadID,QIVS_MAX_KEYPAD,sizeof(long),fp);
  fclose(fp);
  return TRUE;
}
bool QIVSGrouping::Save(cstring fname)
{
  FILE *fp;
  fp=fopen(fname,"wb");
  if(!fp){ qwarn("QIVSGrouping: can't save '%s'",fname); return FALSE; }
  fwrite(&count,1,sizeof(int),fp);
  fwrite(group,QIVS_MAX_KEYPAD,sizeof(char),fp);
  fwrite(keypadID,QIVS_MAX_KEYPAD,sizeof(long),fp);
  fclose(fp);
  return TRUE;
}

int QIVSGrouping::GetGroupSize(int grp)
// Returns #voters that are in group 'grp'
{
  int i,sz=0;
  for(i=0;i<count;i++)
  {
    if(group[i]==grp)sz++;
  }
qdbg("QIVSGroup::GetGroupSize(%d) = %d\n",grp,sz);
  return sz;
}
