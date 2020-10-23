// menu.h

#ifndef __MENU_H
#define __MENU_H

void RMenuCreate();
void RMenuDestroy();
void RMenuRun();

// Export for menu components
void RMenuPaintLogo();
void RMenuPaintGlobal(int flags);
void RMenuSwap();
DTexFont *RMenuGetFont(int n);
void RMenuSetTitle(cstring s);

//void rrScaleRect(const QRect *rIn,QRect *rOut);
void rrPaintText(DTexFont *font,cstring s,int x,int y);

#endif

