// qlib/keys.h - Key definitions
// 23-02-00: accented keys added
// 28-07-00: Win32 keys (VK_xxx)

#ifndef __QLIB_KEYS_H
#define __QLIB_KEYS_H

#include <qlib/object.h>
#ifdef WIN32
#include <winsock.h>
#include <windows.h>
#endif
#include <X11/keysym.h>

// Keys
#define Q_X11_KEYS
#ifdef  Q_X11_KEYS

typedef int qkey;

// Modifiers (empirically established!)
#define QK_Modifiers(k)	((k)&0x7FFF0000)
#define QK_Key(k)	((k)&0x0000FFFF)
#define QK_SHIFT	0x00010000
#define QK_CAPS		0x00020000
#define QK_CTRL		0x00040000
#define QK_ALT		0x00080000

// Modifier keys as they come in from the OS
#ifdef WIN32
// Win32 doesn't distinguish left and right for modifier keys
#define _QK_LSHIFT	VK_SHIFT
#define _QK_RSHIFT	VK_SHIFT
//#define _QK_CAPS	0x000000
#define _QK_LCTRL	VK_CONTROL
#define _QK_RCTRL	VK_CONTROL
#define _QK_LALT	VK_MENU
#define _QK_RALT	VK_MENU
#else
#define _QK_LSHIFT	0xffe1
#define _QK_RSHIFT	0xffe2
//#define _QK_CAPS	0x000000
#define _QK_LCTRL	0xffe3
#define _QK_RCTRL	0xffe4
#define _QK_LALT	0xffe9
#define _QK_RALT	0xffea
#endif

// ^^ Might become more explicit; LCTRL, RCTRL, LSHIFT, LALT etc...

#ifdef WIN32
#define QK_PAUSE	XK_Pause
#define QK_SPACE	VK_SPACE
#define QK_ENTER	VK_RETURN
#define QK_TAB		VK_TAB
#define QK_ESC		VK_ESCAPE
#define QK_DEL		VK_DELETE
#define QK_BS     VK_BACK
#else
// X11
#define QK_PAUSE	XK_Pause
#define QK_SPACE	XK_space
#define QK_ENTER	XK_Return
#define QK_TAB		XK_Tab
#define QK_ESC		XK_Escape
#define QK_DEL		XK_Delete
#define QK_BS		XK_BackSpace
#endif

// Keypad
#ifdef WIN32
#define QK_KP_ENTER	VK_RETURN
#define QK_KP_0		VK_NUMPAD0
#define QK_KP_1		VK_NUMPAD1
#define QK_KP_2		VK_NUMPAD2
#define QK_KP_3		VK_NUMPAD3
#define QK_KP_4		VK_NUMPAD4
#define QK_KP_5		VK_NUMPAD5
#define QK_KP_6		VK_NUMPAD6
#define QK_KP_7		VK_NUMPAD7
#define QK_KP_8		VK_NUMPAD8
#define QK_KP_9		VK_NUMPAD9
#define QK_KP_SLASH	VK_DIVIDE
#define QK_KP_ASTERISK	VK_MULTIPLY
#define QK_KP_MINUS	VK_SUBTRACT
#define QK_KP_PLUS	VK_ADD
#define QK_KP_PERIOD	VK_DECIMAL
#else
// SGI
#define QK_KP_ENTER	XK_KP_Enter
#define QK_KP_0		XK_KP_Insert
#define QK_KP_1		XK_KP_End
#define QK_KP_2		XK_KP_Down
#define QK_KP_3		XK_KP_Page_Down
#define QK_KP_4		XK_KP_Left
#define QK_KP_5		XK_KP_Begin
#define QK_KP_6		XK_KP_Right
#define QK_KP_7		XK_KP_Home
#define QK_KP_8		XK_KP_Up
#define QK_KP_9		XK_KP_Page_Up
#define QK_KP_SLASH	XK_KP_Divide
#define QK_KP_ASTERISK	XK_KP_Multiply
#define QK_KP_MINUS	XK_KP_Subtract
#define QK_KP_PLUS	XK_KP_Add
#define QK_KP_PERIOD	XK_KP_Decimal
#endif

#ifdef WIN32
#define QK_HOME		VK_HOME
#define QK_END		VK_END
#define QK_LEFT		VK_LEFT
#define QK_UP		VK_UP
#define QK_RIGHT	VK_RIGHT
#define QK_DOWN		VK_DOWN
#define QK_PAGEUP	VK_PRIOR
#define QK_PAGEDOWN	VK_NEXT
#define QK_INSERT	VK_INSERT
#else
#define QK_HOME		XK_Home
#define QK_END		XK_End
#define QK_LEFT		XK_Left
#define QK_UP		XK_Up
#define QK_RIGHT	XK_Right
#define QK_DOWN		XK_Down
#define QK_PAGEUP	XK_Page_Up
#define QK_PAGEDOWN	XK_Page_Down
#define QK_INSERT	XK_Insert
#endif

#ifdef WIN32
#define QK_A		'A'
#define QK_B		'B'
#define QK_C		'C'
#define QK_D		'D'
#define QK_E		'E'
#define QK_F		'F'
#define QK_G		'G'
#define QK_H		'H'
#define QK_I		'I'
#define QK_J		'J'
#define QK_K		'K'
#define QK_L		'L'
#define QK_M		'M'
#define QK_N		'N'
#define QK_O		'O'
#define QK_P		'P'
#define QK_Q		'Q'
#define QK_R		'R'
#define QK_S		'S'
#define QK_T		'T'
#define QK_U		'U'
#define QK_V		'V'
#define QK_W		'W'
#define QK_X		'X'
#define QK_Y		'Y'
#define QK_Z		'Z'
#else
#define QK_A		XK_a
#define QK_B		XK_b
#define QK_C		XK_c
#define QK_D		XK_d
#define QK_E		XK_e
#define QK_F		XK_f
#define QK_G		XK_g
#define QK_H		XK_h
#define QK_I		XK_i
#define QK_J		XK_j
#define QK_K		XK_k
#define QK_L		XK_l
#define QK_M		XK_m
#define QK_N		XK_n
#define QK_O		XK_o
#define QK_P		XK_p
#define QK_Q		XK_q
#define QK_R		XK_r
#define QK_S		XK_s
#define QK_T		XK_t
#define QK_U		XK_u
#define QK_V		XK_v
#define QK_W		XK_w
#define QK_X		XK_x
#define QK_Y		XK_y
#define QK_Z		XK_z
#endif

// Accented keys (created by combining ALT-: and QK_E for example)
#ifdef WIN32
// Win32 accented keys
#define QK_A_GRAVE      0
#define QK_A_ACUTE      0
#define QK_A_CIRCUMFLEX 0
#define QK_A_TILDE      0
#define QK_A_DIAERESIS  0
#define QK_A_RING       0
#define QK_C_CEDILLA    0
#define QK_E_GRAVE      0
#define QK_E_ACUTE      0
#define QK_E_CIRCUMFLEX 0
#define QK_E_DIAERESIS  0
#define QK_I_GRAVE      0
#define QK_I_ACUTE      0
#define QK_I_CIRCUMFLEX 0
#define QK_I_DIAERESIS  0
#define QK_N_TILDE      0
#define QK_O_GRAVE      0
#define QK_O_ACUTE      0
#define QK_O_CIRCUMFLEX 0
#define QK_O_DIAERESIS  0
#define QK_O_TILDE      0
#define QK_O_OBLIQUE    0
#define QK_U_GRAVE      0
#define QK_U_ACUTE      0
#define QK_U_CIRCUMFLEX 0
#define QK_U_DIAERESIS  0
#define QK_Y_ACUTE      0
#define QK_Y_DIAERESIS  0

#else

// SGI X11 accented keys
#define QK_A_GRAVE	XK_Agrave
#define QK_A_ACUTE	XK_Aacute
#define QK_A_CIRCUMFLEX	XK_Acircumflex
#define QK_A_TILDE	XK_Atilde
#define QK_A_DIAERESIS	XK_Adiaeresis
#define QK_A_RING	XK_Aring
#define QK_C_CEDILLA	XK_Ccedilla
#define QK_E_GRAVE	XK_Egrave
#define QK_E_ACUTE	XK_Eacute
#define QK_E_CIRCUMFLEX	XK_Ecircumflex
#define QK_E_DIAERESIS	XK_Ediaeresis
#define QK_I_GRAVE	XK_Igrave
#define QK_I_ACUTE	XK_Iacute
#define QK_I_CIRCUMFLEX	XK_Icircumflex
#define QK_I_DIAERESIS	XK_Idiaeresis
#define QK_N_TILDE	XK_Ntilde
#define QK_O_GRAVE	XK_Ograve
#define QK_O_ACUTE	XK_Oacute
#define QK_O_CIRCUMFLEX	XK_Ocircumflex
#define QK_O_DIAERESIS	XK_Odiaeresis
#define QK_O_TILDE	XK_Otilde
#define QK_O_OBLIQUE	XK_Ooblique
#define QK_U_GRAVE	XK_Ugrave
#define QK_U_ACUTE	XK_Uacute
#define QK_U_CIRCUMFLEX	XK_Ucircumflex
#define QK_U_DIAERESIS	XK_Udiaeresis
#define QK_Y_ACUTE	XK_Yacute
#define QK_Y_DIAERESIS	XK_Ydiaeresis

#endif

// Top row US keybd
#ifdef WIN32
#define QK_0		'0'
#define QK_1		'1'
#define QK_2		'2'
#define QK_3		'3'
#define QK_4		'4'
#define QK_5		'5'
#define QK_6		'6'
#define QK_7		'7'
#define QK_8		'8'
#define QK_9		'9'
#else
#define QK_0		XK_0
#define QK_1		XK_1
#define QK_2		XK_2
#define QK_3		XK_3
#define QK_4		XK_4
#define QK_5		XK_5
#define QK_6		XK_6
#define QK_7		XK_7
#define QK_8		XK_8
#define QK_9		XK_9
#endif

// Special chars
#ifdef WIN32
#define QK_PERIOD	0xBE
#define QK_COMMA	0xBC
#define QK_SLASH	0xBF
#define QK_SEMICOLON	0xBA
#define QK_APOSTROPHE	0xDE	// quoteright
#define QK_BRACKETLEFT	0xDB
#define QK_BRACKETRIGHT	0xDD
#define QK_BACKSLASH	0xDC
#define QK_MINUS	0xBD
#define QK_EQUALS	0xBB
#define QK_GRAVE	0xC0	// quoteleft
#else
#define QK_PERIOD	XK_period
#define QK_COMMA	XK_comma
#define QK_SLASH	XK_slash
#define QK_SEMICOLON	XK_semicolon
#define QK_APOSTROPHE	XK_apostrophe	// quoteright
#define QK_BRACKETLEFT	XK_bracketleft
#define QK_BRACKETRIGHT	XK_bracketright
#define QK_BACKSLASH	XK_backslash
#define QK_MINUS	XK_minus
#define QK_EQUALS	XK_equal
#define QK_GRAVE	XK_grave	// quoteleft
#endif

// Function keys
#ifdef WIN32
#define QK_F1		VK_F1
#define QK_F2		VK_F2
#define QK_F3		VK_F3
#define QK_F4		VK_F4
#define QK_F5		VK_F5
#define QK_F6		VK_F6
#define QK_F7		VK_F7
#define QK_F8		VK_F8
#define QK_F9		VK_F9
#define QK_F10		VK_F10
#define QK_F11		VK_F11
#define QK_F12		VK_F12
#else
#define QK_F1		XK_F1
#define QK_F2		XK_F2
#define QK_F3		XK_F3
#define QK_F4		XK_F4
#define QK_F5		XK_F5
#define QK_F6		XK_F6
#define QK_F7		XK_F7
#define QK_F8		XK_F8
#define QK_F9		XK_F9
#define QK_F10		XK_F10
#define QK_F11		XK_F11
#define QK_F12		XK_F12
#endif

// Global key functions
char QKeyToASCII(qkey key);

// Utility functions
int QKeyToNumber(qkey key,int flags=0);

#endif

#endif
