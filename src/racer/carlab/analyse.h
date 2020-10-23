// analyse.h

#ifndef __ANALYSE_H
#define __ANALYSE_H

// Export from analyse.cpp (a bit rough like this)
extern rfloat weightRatio[RCar::MAX_WHEEL]; // Ratio of full weight (sum=1)
extern rfloat weight[RCar::MAX_WHEEL];      // Tire weight (sum=mass)
extern rfloat zf,zr,L;                      // Static load

void Analyse(bool display=TRUE);

#endif
