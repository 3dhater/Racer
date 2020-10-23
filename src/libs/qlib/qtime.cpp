/*
 * QTime - Standard time functions
 * NOTES:
 * - I believe it takes into account timezone stuff etc.
 * (C) 16-10-98 MarketGraph/RVG
 */

#include <time.h>
#include <qlib/debug.h>
DEBUG_ENABLE

string QCurrentDate(string format)
// Returns DD-MM-YYYY or the format as specified in strftime()
{
  time_t t;
  static char buf[40];
  time(&t);
  strftime(buf,sizeof(buf),format?format:"%d-%m-%Y",localtime(&t));
  return buf;
}

string QCurrentTime(string format)
// Returns HH:MM:SS or the format as specified in strftime()
// (uses 24h system)
{
  time_t t;
  static char buf[40];
  time(&t);
  strftime(buf,sizeof(buf),format?format:"%H:%M:%S",localtime(&t));
  return buf;
}

