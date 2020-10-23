/*
 * Racer - Music
 * 05-12-01: Created!
 * NOTES:
 * - Switching to the next stream is a bit weird; closing and opening
 * a new stream from within the end callback leads to a crash (on Linux),
 * so we must set a flag for someone to pick up. In this case, that is
 * RManager, which periodically checks the flag and saves us. Not too clean.
 * BUGS:
 * - FMOD seems to select a new channel number at every song, though I tried
 * to close the stream and channel for sure. Don't know how quickly that
 * will lead to resource constipation though.
 * FUTURE:
 * - When started random, keep shuffling?
 * (c) Dolphinity/RvG
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
DEBUG_ENABLE

// Hm... could do better than this ;-)
static int chn;

RMusicManager::RMusicManager()
{
  int i;

  curSection=MENU;
  playing=FALSE;
#ifdef Q_USE_FMOD
  streamMusic=0;
#endif
  for(i=0;i<MAX_SECTION;i++)
  {
    songCount[i]=0;
    songNr[i]=0;
  }
  reqNext=FALSE;
}
RMusicManager::~RMusicManager()
{
  CloseSong();
}

/**********
* Helpers *
**********/
#ifdef Q_USE_FMOD
signed char cbEnd(FSOUND_STREAM *stream,void *buf,int len,int param)
// Called when the stream ends (end of song)
{
qdbg("cbEnd()\n");
//FSOUND_Stream_Play(FSOUND_FREE,stream);
//return FALSE;
  // Start next song
  //RMGR->musicMgr->PlayNext();
  RMGR->musicMgr->RequestNextSong();

  // Return value is ignored
  return TRUE;
}
#endif

/*******
* Init *
*******/
cstring RMusicManager::SectionToString(int section)
// Converts enum to string
{
  switch(section)
  {
    case RACE  : return "race";
    case REPLAY: return "replay";
    case MENU  : return "menu";
    default: return "unknownsection";
  }
}

void RMusicManager::Create()
// Call this before trying to start music.
// This reads the songs.
{
  int i,n;
  char buf[256];

  // Pre-read the number of songs available for each section.
  // This is done to avoid having to detect the song counts later on,
  // when we're in the middle of a race perhaps.
  for(i=0;i<MAX_SECTION;i++)
  {
    for(n=0;;n++)
    {
      sprintf(buf,"music.%s.song%d",SectionToString(i),n+1);
      if(!RMGR->GetMainInfo()->PathExists(buf))
        break;
    }
    songCount[i]=n;
//qdbg("Section %s has %d songs.\n",SectionToString(i),n);
  }
}

/***********
* Playback *
***********/
void RMusicManager::CloseSong()
// Close the current song
{
#ifdef Q_USE_FMOD
  if(streamMusic)
  {
qdbg("Stop & close; stream=%p\n",streamMusic);
    FSOUND_StopSound(chn);
    FSOUND_Stream_Stop(streamMusic);
    FSOUND_Stream_Close(streamMusic);
    streamMusic=0;
  }
#endif
}

void RMusicManager::Play(int section,int song)
// Play a song from the section
{
  char    buf[256];
  qstring s;
  int     volume;

//qdbg("RMusicManager:Play(section %d, song %d)\n",section,song);

  // Checks
  if(song<0||song>songCount[section])
  { qwarn("RMM:Play(); Song %d out of range",song);
    return;
  }

  // Stop the old song
  CloseSong();

  // Remember settings
  curSection=section;
  songNr[curSection]=song;
  // Don't keep on requesting new songs
  reqNext=FALSE;

  // Get song volume (0..100)
  sprintf(buf,"music.%s.volume%d",SectionToString(section),song+1);
  volume=RMGR->GetMainInfo()->GetInt(buf,100);

  // Get song name
  sprintf(buf,"music.%s.song%d",SectionToString(section),song+1);
  RMGR->GetMainInfo()->GetString(buf,s);
qdbg("RMusicManager:Play(%d,%d) => %s\n",section,song,s.cstr());

  // Open the song and play it
#ifdef Q_USE_FMOD
  sprintf(buf,"data/music/%s",s.cstr());
  streamMusic=FSOUND_Stream_OpenFile(buf,FSOUND_LOOP_OFF|FSOUND_2D,0);
  if(!streamMusic)
  {
    qwarn("Can't open music (%s) for section '%s'",
      buf,SectionToString(section));
  } else
  {
    // Play
    int chn=FSOUND_Stream_Play(FSOUND_FREE,streamMusic);
qdbg("  playing at chn=%d, volume=%d\n",chn,volume);

#ifdef linux
    // Match frequency to audio system (needed on Linux still?)
    FSOUND_SetFrequency(chn,RMGR->audio->GetFrequency());
#else
    // On Windows, the above seems to get the reverse effect; playing
    // at the wrong sampling frequency.
    // On SGI, no FMOD is present.
#endif

    // Set volume
    FSOUND_SetVolume(chn,volume*255/100);
    // Get notified when the song ends
    if(!FSOUND_Stream_SetEndCallback(streamMusic,cbEnd,0))
      qerr("FSOUND_Stream_SetEndCallback() failed");
//qdbg("  play stream %p\n",streamMusic);
  }

#endif
}

void RMusicManager::PlayRandom(int section)
{
  int n;
  if(section<0||section>=MAX_SECTION)
  {
    qwarn("RMM:PlayRandom(); section %d out of range",section);
    return;
  }
  if(!songCount[section])
  {
    // No songs available
    return;
  }

  n=rand()%songCount[section];
  Play(section,n);
}
void RMusicManager::PlayNext()
// Called mainly from the end callback function, this selects
// the next song to play.
{
  int n;

qdbg("RMM:PlayNext()\n");

  // Any music?
  if(!songCount[curSection])return;

  // Use the next song (not random)
  n=(songNr[curSection]+1)%songCount[curSection];
  Play(curSection,n);
}

void RMusicManager::Stop()
// Stop playing the current song
{
#ifdef Q_USE_FMOD
//qdbg("RMM:Stop()\n");
//qwarn("RMM:Stop()\n");
  if(streamMusic)
    FSOUND_Stream_Stop(streamMusic);
#endif
}

