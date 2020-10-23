/*
 * QScreen.cpp - a screen on a display
 * 04-10-96: Created!
 * (C) MarketGraph/RVG
 */

#include <stdio.h>
#include <qlib/display.h>
#include <qlib/app.h>
#include <qlib/screen.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <qlib/debug.h>
DEBUG_ENABLE

QScreen::QScreen(int _n)
{
  n=_n;
}
QScreen::~QScreen()
{}

int QScreen::GetNumber()
{ return n;
}

int QScreen::GetWidth()
{
  return DisplayWidth(app->GetDisplay()->GetXDisplay(),n);
}

int QScreen::GetHeight()
{
  return DisplayHeight(app->GetDisplay()->GetXDisplay(),n);
}
