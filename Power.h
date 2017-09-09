#ifndef POWER
#define POWER

#include <Arduino.h>

enum PowerState{
	PS_OFF,
	PS_MANUAL_OFF,
	PS_STARTUP,
	PS_ON
};

void initPower();
void updatePower();
PowerState getPowerState();
void setPowerState(PowerState state);
float getVoltage();
float getCurrent();

#endif
