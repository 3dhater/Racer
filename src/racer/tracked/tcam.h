// tcam.h

#ifndef __TCAM_H
#define __TCAM_H

void TCamSetMode();
void TCamOp(int op);
bool TCamKey(int key);
void TCamStats();

void TCamTarget();
void TCamPaintTarget();

extern RTrackCam *curCam;
extern int curCamIndex;             // Currently selected camera

#endif
