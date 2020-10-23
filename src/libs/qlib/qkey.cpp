/*
 * QKey - global key functions
 * 14-04-97: Created!
 * FUTURE;
 * - Accented chars; ALT+(F-K) or something
 * (C) MarketGraph/RVG
 */

#include <qlib/keys.h>
#include <stdio.h>

char QKeyToASCII(qkey key)
// Use X? Keymaps etc.
{
  int  mod;
  // Strip out modifiers (NumLock is a tricky one)
  mod=QK_Modifiers(key);
  key=QK_Key(key);
//qdbg("QKeyToASCII(%d)\n",key);
  if(mod==QK_SHIFT)
  { switch(key)
    { case QK_A: return 'A';
#ifndef WIN32
      case QK_A_ACUTE     : return 'Á';
      case QK_A_GRAVE     : return 'À';
      case QK_A_CIRCUMFLEX: return 'Â';
      case QK_A_DIAERESIS : return 'Ä';
      case QK_A_TILDE     : return 'Ã';
      case QK_A_RING      : return 'Å';
#endif
      // AE Æ
      case QK_B: return 'B';
      case QK_C: return 'C';
#ifndef WIN32
      case QK_C_CEDILLA: return 'Ç';
#endif
      case QK_D: return 'D';
      case QK_E: return 'E';
#ifndef WIN32
      case QK_E_ACUTE     : return 'É';
      case QK_E_GRAVE     : return 'È';
      case QK_E_CIRCUMFLEX: return 'Ê';
      case QK_E_DIAERESIS : return 'Ë';
#endif
      case QK_F: return 'F';
      case QK_G: return 'G';
      case QK_H: return 'H';
      case QK_I: return 'I';
#ifndef WIN32
      case QK_I_ACUTE     : return 'Í';
      case QK_I_GRAVE     : return 'Ì';
      case QK_I_CIRCUMFLEX: return 'Î';
      case QK_I_DIAERESIS : return 'Ï';
#endif
      case QK_J: return 'J';
      case QK_K: return 'K';
      case QK_L: return 'L';
      case QK_M: return 'M';
      case QK_N: return 'N';
#ifndef WIN32
      case QK_N_TILDE: return 'Ñ';
#endif
      case QK_O: return 'O';
#ifndef WIN32
      case QK_O_ACUTE     : return 'Ó';
      case QK_O_GRAVE     : return 'Ò';
      case QK_O_CIRCUMFLEX: return 'Ô';
      case QK_O_DIAERESIS : return 'Ö';
      case QK_O_TILDE     : return 'Õ';
      case QK_O_OBLIQUE   : return 'Ø';
#endif
      case QK_P: return 'P';
      case QK_Q: return 'Q';
      case QK_R: return 'R';
      case QK_S: return 'S';
      case QK_T: return 'T';
      case QK_U: return 'U';
#ifndef WIN32
      case QK_U_ACUTE     : return 'Ú';
      case QK_U_GRAVE     : return 'Ù';
      case QK_U_CIRCUMFLEX: return 'Û';
      case QK_U_DIAERESIS : return 'Ü';
#endif
      case QK_V: return 'V';
      case QK_W: return 'W';
      case QK_X: return 'X';
      case QK_Y: return 'Y';
#ifndef WIN32
      case QK_Y_ACUTE     : return 'Ý';
#endif
      // No diaeresis
      case QK_Z: return 'Z';

      case QK_0: case QK_KP_0: return ')';
      case QK_1: case QK_KP_1: return '!';
      case QK_2: case QK_KP_2: return '@';
      case QK_3: case QK_KP_3: return '#';
      case QK_4: case QK_KP_4: return '$';
      case QK_5: case QK_KP_5: return '%';
      case QK_6: case QK_KP_6: return '^';
      case QK_7: case QK_KP_7: return '&';
      case QK_8: case QK_KP_8: return '*';
      case QK_9: case QK_KP_9: return '(';

      case QK_PERIOD: return '>';
      case QK_COMMA: return '<';
      case QK_SLASH: return '?';
      case QK_SEMICOLON: return ':';
      case QK_APOSTROPHE: return '"';
      case QK_BRACKETLEFT: return '{';
      case QK_BRACKETRIGHT: return '}';
      case QK_BACKSLASH: return '|';
      case QK_MINUS: return '_';
      case QK_EQUALS: return '+';
      case QK_GRAVE: return '~';
      //case QK_: return ' ';

      case QK_KP_PLUS: return '+';
      case QK_KP_MINUS: return '-';
      case QK_KP_ASTERISK: return '*';
      case QK_KP_SLASH: return '/';
      case QK_KP_PERIOD: return '.';

      case QK_SPACE: return ' ';
      default: return 0;
    }
  }
  //if(!(mod&QK_SHIFT))
  if(mod==0)
  { switch(key)
    { case QK_A: return 'a';
#ifndef WIN32
      case QK_A_ACUTE     : return 'á';
      case QK_A_GRAVE     : return 'à';
      case QK_A_CIRCUMFLEX: return 'â';
      case QK_A_DIAERESIS : return 'ä';
      case QK_A_TILDE     : return 'ã';
      case QK_A_RING      : return 'å';
#endif
      // ae
      case QK_B: return 'b';
      case QK_C: return 'c';
#ifndef WIN32
      case QK_C_CEDILLA: return 'ç';
#endif
      case QK_D: return 'd';
      case QK_E: return 'e';
#ifndef WIN32
      case QK_E_ACUTE     : return 'é';
      case QK_E_GRAVE     : return 'è';
      case QK_E_CIRCUMFLEX: return 'ê';
      case QK_E_DIAERESIS : return 'ë';
#endif
      case QK_F: return 'f';
      case QK_G: return 'g';
      case QK_H: return 'h';
      case QK_I: return 'i';
#ifndef WIN32
      case QK_I_ACUTE     : return 'í';
      case QK_I_GRAVE     : return 'ì';
      case QK_I_CIRCUMFLEX: return 'î';
      case QK_I_DIAERESIS : return 'ï';
#endif
      case QK_J: return 'j';
      case QK_K: return 'k';
      case QK_L: return 'l';
      case QK_M: return 'm';
      case QK_N: return 'n';
#ifndef WIN32
      case QK_N_TILDE: return 'ñ';
#endif
      case QK_O: return 'o';
#ifndef WIN32
      case QK_O_ACUTE     : return 'ó';
      case QK_O_GRAVE     : return 'ò';
      case QK_O_CIRCUMFLEX: return 'ô';
      case QK_O_DIAERESIS : return 'ö';
      case QK_O_TILDE     : return 'õ';
      case QK_O_OBLIQUE   : return 'ø';
#endif
      case QK_P: return 'p';
      case QK_Q: return 'q';
      case QK_R: return 'r';
      case QK_S: return 's';
      case QK_T: return 't';
      case QK_U: return 'u';
#ifndef WIN32
      case QK_U_ACUTE     : return 'ú';
      case QK_U_GRAVE     : return 'ù';
      case QK_U_CIRCUMFLEX: return 'û';
      case QK_U_DIAERESIS : return 'ü';
#endif
      case QK_V: return 'v';
      case QK_W: return 'w';
      case QK_X: return 'x';
      case QK_Y: return 'y';
#ifndef WIN32
      case QK_Y_ACUTE     : return 'ý';
#ifdef sgi
      // Linux doesn't seem to have this
      case QK_Y_DIAERESIS : return 'ÿ';
#endif
#endif
      case QK_Z: return 'z';

      case QK_0: case QK_KP_0: return '0';
      case QK_1: case QK_KP_1: return '1';
      case QK_2: case QK_KP_2: return '2';
      case QK_3: case QK_KP_3: return '3';
      case QK_4: case QK_KP_4: return '4';
      case QK_5: case QK_KP_5: return '5';
      case QK_6: case QK_KP_6: return '6';
      case QK_7: case QK_KP_7: return '7';
      case QK_8: case QK_KP_8: return '8';
      case QK_9: case QK_KP_9: return '9';

      case QK_PERIOD: return '.';
      case QK_COMMA: return ',';
      case QK_SLASH: return '/';
      case QK_SEMICOLON: return ';';
      case QK_APOSTROPHE: return '\'';
      case QK_BRACKETLEFT: return '[';
      case QK_BRACKETRIGHT: return ']';
      case QK_BACKSLASH: return '\\';
      case QK_MINUS: return '-';
      case QK_EQUALS: return '=';
      case QK_GRAVE: return '`';
      //case QK_: return ' ';
      case QK_SPACE: return ' ';

      case QK_KP_PLUS: return '+';
      case QK_KP_MINUS: return '-';
      case QK_KP_ASTERISK: return '*';
      case QK_KP_SLASH: return '/';
      case QK_KP_PERIOD: return '.';

      default:
#ifdef WIN32
        qdbg("QKeyToASCII(0x%x) not known\n",key);
#endif
        return 0;
    }
  }
  return 0;
}

int QKeyToNumber(int key,int flags)
// Converts QK_0 -> QK_9 to 0..9
// If flags&1, then keypad numbers are converted too.
// Returns -1 if 'key' is not a number.
{
  switch(key)
  {
    case QK_0: return 0;
    case QK_1: return 1;
    case QK_2: return 2;
    case QK_3: return 3;
    case QK_4: return 4;
    case QK_5: return 5;
    case QK_6: return 6;
    case QK_7: return 7;
    case QK_8: return 8;
    case QK_9: return 9;
  }
  if(flags&1)
  {
    // Convert keypad numbers too
    switch(key)
    {
      case QK_KP_0: return 0;
      case QK_KP_1: return 1;
      case QK_KP_2: return 2;
      case QK_KP_3: return 3;
      case QK_KP_4: return 4;
      case QK_KP_5: return 5;
      case QK_KP_6: return 6;
      case QK_KP_7: return 7;
      case QK_KP_8: return 8;
      case QK_KP_9: return 9;
    }
  }
  return -1;
}

