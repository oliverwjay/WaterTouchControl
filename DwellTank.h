#ifndef DWELL_TANK
#define DWELL_TANK

#include <Arduino.h>

enum DT_STATE{
  DTS_INACTIVE,
  DTS_FLUSHING
};

void setDwellTankState(DT_STATE state);
void initDwellTank();
void updateDwellTank();
bool isFlowing();

#endif
