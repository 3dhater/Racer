// ai.h

#ifndef __AI_H
#define __AI_H

extern int curWPIndex;
extern RWayPoint *curWP;

void AISetMode();
void AIOp(int op);
bool AIKey(int key);
void AIStats();

#endif
