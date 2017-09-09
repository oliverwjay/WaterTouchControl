#ifndef WOOL_FEED
#define WOOL_FEED
#include <Arduino.h>

enum WF_STATE:uint8_t{
	WFS_OFF,
	WFS_IDLE,
	WFS_REVERSING,
	WFS_FEEDING,
	WFS_MAN_REVERSING,
	WFS_MAN_FEEDING
};

void initWoolFeed();
void updateWoolFeed();
void setWoolFeedState(WF_STATE newState);

#endif
