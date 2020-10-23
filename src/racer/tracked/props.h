// props.h

#ifndef __PROPS_H
#define __PROPS_H

extern RTV_Node *nodeProp;
extern int nodePropIndex;

void PropsSetMode();
void PropsOp(int op);
bool PropsKey(int key);
void PropsStats();
void PropsPaint();

#endif
