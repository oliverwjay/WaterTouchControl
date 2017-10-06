#include <Arduino.h>
#include "Schedule.h"
#include "Menu.h"
#include "Status.h"
#include "RTClib.h"

RTC_DS1307 rtc;

DateTime shutdownTime;
DateTime startupTime;
DateTime currentTime;

void initSchedule() {
  shutdownTime = DateTime(0);
  startupTime = DateTime(0);
  DateTime compileTime = DateTime(F(__DATE__), F(__TIME__));
  rtc.begin();
  if (! rtc.isrunning() || rtc.now().year() != compileTime.year()) {
    rtc.adjust(compileTime);
  }
  currentTime = rtc.now();
  setTimeVar(MTVI_CURRENT_TIME, currentTime);
}

bool isTime(DateTime _time) {
  bool ret = true;
  ret &= floor(_time.year() / 100.0) == 20;
  ret &= _time.month() <= 12;
  ret &= _time.day() <= 31;
  ret &= _time.hour() <= 24;
  ret &= _time.minute() < 60;
  ret &= _time.second() < 60;
  return ret;
}

void updateSchedule() {
  if (getTimeVar(MTVI_CURRENT_TIME).secondstime() != currentTime.secondstime()) {
    rtc.adjust(getTimeVar(MTVI_CURRENT_TIME));
  }
  DateTime tempTime = rtc.now();
  if (isTime(tempTime) && abs(tempTime.secondstime() - currentTime.secondstime()) < 30) currentTime = tempTime;

  setTimeVar(MTVI_CURRENT_TIME, currentTime);
  startupTime = getTimeVar(MTVI_S_STARTUP_TIME);
  shutdownTime = getTimeVar(MTVI_S_SHUTDOWN_TIME);

  String statusText = "";
  if (!isTime(startupTime) && !isTime(shutdownTime)) {
    statusText += "Nothing Scheduled";
  }
  else {
    DateTime nextChange;
    if (!isTime(shutdownTime) || (isTime(startupTime) && startupTime.secondstime() < shutdownTime.secondstime())) {
      statusText += "Startup in: ";
      nextChange = startupTime;
    }
    else {
      statusText += "Shutdown in: ";
      nextChange = shutdownTime;
    }
    TimeSpan timeToChange = TimeSpan(nextChange - currentTime);
    if (timeToChange.days() != 0) {
      statusText += timeToChange.days();
      statusText += " days";
    }
    else if (timeToChange.hours() != 0) {
      statusText += timeToChange.hours();
      statusText += " hours";
    }
    else if (timeToChange.minutes() != 0) {
      statusText += timeToChange.minutes();
      statusText += " minutes";
    }
    else {
      statusText += timeToChange.seconds();
      statusText += " seconds";
    }
  }
  setLabelText(MLI_S_STATUS, statusText);

  if (isTime(startupTime) && currentTime.secondstime() > startupTime.secondstime()) {
    setVar(MVI_STATUS_SWITCH, 1);
    setStartup(DateTime(0));
  }
  else if (isTime(shutdownTime) && currentTime.secondstime() > shutdownTime.secondstime()) {
    setVar(MVI_STATUS_SWITCH, 0);
    setShutdown(DateTime(0));
  }
}

DateTime getTime() {
  return currentTime;
}

void setShutdown(DateTime _shutdownTime) {
  shutdownTime = _shutdownTime;
  setTimeVar(MTVI_S_SHUTDOWN_TIME, DateTime(0));
}

void setStartup(DateTime _startupTime) {
  startupTime = _startupTime;
  setTimeVar(MTVI_S_STARTUP_TIME, DateTime(0));
}

int replaceDigit(int in, int d, int v) {
  int ret;
  if (d == 0) {
    ret = in - in % 100 + v * 10 + in % 10;
  }
  else {
    ret = in - in % 10 + v;
  }
  return ret;
}

DateTime replaceTimeDigit(DateTime dt, int d, int v) {
  uint8_t seg = floor(d / 2);
  uint8_t seg_d = d % 2;
  DateTime ret;
  switch (seg) {
    case 0:
      ret = DateTime(dt.year(), replaceDigit(dt.month(), seg_d, v), dt.day(), dt.hour(), dt.minute(), 0);
      break;
    case 1:
      ret = DateTime(dt.year(), dt.month(), replaceDigit(dt.day(), seg_d, v), dt.hour(), dt.minute(), 0);
      break;
    case 2:
      ret = DateTime(replaceDigit(dt.year(), seg_d, v), dt.month(), dt.day(), dt.hour(), dt.minute(), 0);
      break;
    case 3:
      ret = DateTime(dt.year(), dt.month(), dt.day(), replaceDigit(dt.hour(), seg_d, v), dt.minute(), 0);
      break;
    case 4:
      ret = DateTime(dt.year(), dt.month(), dt.day(), dt.hour(), replaceDigit(dt.minute(), seg_d, v), 0);
      break;
    case 5:
      ret = DateTime(dt.year(), dt.month(), dt.day(), dt.hour(), dt.minute(), replaceDigit(dt.second(), seg_d, v));
      break;
  }
  return ret;
}

