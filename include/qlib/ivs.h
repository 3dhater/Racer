// qlib/ivs.h - Voting system

#ifndef __QLIB_IVS_H
#define __QLIB_IVS_H

#include <qlib/socket.h>

#define QIVS_DEF_PORT	7171
// Max #keypads to store
#define QIVS_MAX_KEYPAD	200

// Max #choices on keypad (and some extra for sentinels)
#define QIVS_MAX_CHOICE 16

// QIVS notion of nothing voted
#define QIVS_NONE	(-1)

struct QIVS_MsgHdr
// Message header for all messages
// Parameters can follow
// Don't use structure too directly when sending,
// because byte ordering issues are around.
{
  short len;
  short appID;
  char  cmd;
  char  subCmd;
  short nr;                // Message number

  void ToHost();		// Reverses byte ordering
  void DbgPrint(cstring s);
};

struct QIVSGrouping;

struct QIVSResult
// The basic result of a voting
// Useful to store results for later usage, or to combine later on
{
  int  countKeypads;		// Number of possible votes (=#keypads)
  int  countVotes;		// Number of votes actually cast
  char voteForKeypad[QIVS_MAX_KEYPAD];   // What was voted
  long keypadID[QIVS_MAX_KEYPAD]; // ID's of keypads; -1 = no ID
  int  voteCount[QIVS_MAX_CHOICE];       // Summary of votes

  // Ctor
  QIVSResult()
  { int i;
    countKeypads=0; countVotes=0;
    for(i=0;i<QIVS_MAX_KEYPAD;i++)
    { keypadID[i]=-1;
      voteForKeypad[i]=-1;
    }
    for(i=0;i<QIVS_MAX_CHOICE;i++)
      voteCount[i]=0;
  }

  // Methods
  int  GetVotes(){ return countVotes; }
  void SetVotes(int n);
  int  GetVotesFor(int choice);
  void SetVotesFor(int choice,int count);
  void SetVoteForKeypad(long id,int index);	// Store vote for 1 keypad
  int  GetVoteForKeypad(long id);
  int  GetKeypadID(int index);
  void GetFromString(cstring s);
  void ToString(string s,int maxChoice=QIVS_MAX_CHOICE);

  int  GetIndexFromKeypad(long id);
  int  GetFreeKeypadIndex();
  int  GetVoteCountForGroup(int choice,int group,QIVSGrouping *grp);

  void ResetVoting();
  void ResetKeypadIDs();

  // I/O
  bool Load(cstring fname);
  bool Save(cstring fname);

  // Debugging
  void DbgPrint(cstring s);
};

struct QIVSGrouping
// Splitting the keypads into groups
// Can be directly derived from an IVS result
// (the votes ARE the groups)
{
  int  count;			// Number of keypads in this grouping
  char group[QIVS_MAX_KEYPAD];	// Which group the voter belongs to
  long keypadID[QIVS_MAX_KEYPAD]; // ID's of keypads

  // Methods
  void CreateFromResult(QIVSResult *r);
  //int  GetGroup(int n);		// Return group for keypad/voter
  int  GetGroupForID(long id);	// Return group for explicit keypad
  int  GetGroupSize(int grp);   // Returns #members in group 'grp'

  // I/O
  bool Load(cstring fname);
  bool Save(cstring fname);
};

class QIVS
{

  QClientSocket *socket;
  int            appID;		// Id of our application
  // Voting
  QIVSResult result;		// The last result
#ifdef OBS
  int keypads;                  // Number of *possible* votes
  int votes;                    // Votes sofar
  int voteCount[16];		// Votes until now (per key)
#endif
  int maxPoll;			// Max #commands to poll at once

 public:
  QIVS();
  ~QIVS();

  // Attributes
  int    GetFD();
  inline QClientSocket *GetSocket(){ return socket; }
  QIVSResult *GetResult(){ return &result; }

  bool Open(cstring host,int port=QIVS_DEF_PORT);
  void Close();

  // Sending/receiving commands
  bool SendCommand(QIVS_MsgHdr *hdr,int params=0,char *param=0);
  bool RecvCommand(QIVS_MsgHdr *hdr,int *params,char *param);
  void Flush();

  // High level
  bool RequestID(int *id);
  bool ReadCommand();

  // Voting
  void SetKeypads(int count);
  int  GetKeypads(){ return result.countKeypads; }
  void SetMaxPoll(int count);
  int  GetMaxPoll(){ return maxPoll; }
  bool OpenVoting(int max=10,bool fYesNo=FALSE);
  bool CloseVoting();
  bool PollVoting();
  int  KeyToIndex(char key);
  int  GetVotes(){ return result.countVotes; }
  void SetVotes(int n);
  int  GetVotesFor(int n);
  void SetVotesFor(int n,int count);
};

#endif
