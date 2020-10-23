/*
 * Glue between QLib apps and Win32
 * NOTES:
 * - Add this to your Win32 project and supply main()
 * - Uses basic Win32 startup, no MFC or such fun stuff
 * (C) 2000 MarketGraph/Ruud van Gaal
 */

#ifdef WIN32

#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>

// Global variable
HINSTANCE qHInstance;

void main();

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,
				   LPSTR lpCmdLine,int iCmdShow )
{
  qHInstance=hInstance;
  main();
  return 0;
}

#endif
