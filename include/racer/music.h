// racer/music.h

#ifndef __RACER_MUSIC_H
#define __RACER_MUSIC_H

class RMusicManager
// Music container which knows about songs to play
{
 public:
  enum Section
  {
    RACE,                      // During racing
    REPLAY,
    MENU,
    MAX_SECTION
  };
 protected:
  int  curSection;
  int  songCount[MAX_SECTION];
  int  songNr[MAX_SECTION];    // For Round-Robin playback
  bool playing;
  bool reqNext;

#ifdef Q_USE_FMOD
  FSOUND_STREAM *streamMusic;
#endif

 public:
  RMusicManager();
  ~RMusicManager();

  // Attribs
  bool NextSongRequested(){ return reqNext; }
  void RequestNextSong(){ reqNext=TRUE; }

  // Init
  cstring SectionToString(int section);
  void    Create();

  // Playback
  void CloseSong();

  // High-level
  void Play(int section,int song);
  void PlayRandom(int section);
  void PlayNext();
  void Stop();
};

#endif

