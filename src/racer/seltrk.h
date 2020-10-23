// seltrk.h

#ifndef __SELTRK_H
#define __SELTRK_H

#include <raceru/button.h>
#include <raceru/listbox.h>

// Standard set of buttons; Prev/Next/Sel/Cancel
extern RButton *bCmd[4];
extern RListBox *lbSel;
void SetupButtons();
void DestroyButtons();

void rrSelectTrkIdle();
void rrSelectTrk();

#endif
