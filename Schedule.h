#ifndef SCHEDULE
#define SCHEDULE

#include <Arduino.h>
#include "RTClib.h"

void initSchedule();
void updateSchedule();
DateTime getTime();
void setShutdown(DateTime _shutdownTime);
void setStartup(DateTime _startupTime);
DateTime replaceTimeDigit(DateTime dt, int d, int v);
bool isTime(DateTime _time);


#endif
