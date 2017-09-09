#include <Arduino.h>
#include "WoolFeed.h"
#include "Menu.h"
#include "Status.h"

#define DRIVE_CHANNEL 12
#define DRIVE_BRAKE 9
#define DRIVE_PWM 3
#define SERVO_PORT 5
#define CIRCULATION_PUMP_RELAY 42
#define WOOL_SENSE_PIN A15

//Servo servo;

WF_STATE woolFeedState = WFS_OFF;
unsigned long lastWFSwitch;
bool statusWFEnabled = false;
bool statusWasWFEnabled = false;

void initWoolFeed() {
  pinMode(DRIVE_CHANNEL, OUTPUT); // set up motor drive
  pinMode(DRIVE_BRAKE, OUTPUT);
  pinMode(CIRCULATION_PUMP_RELAY, OUTPUT);

  //  servo.attach(SERVO_PORT);
  //  servo.write(0);

//  Serial.begin(9600);

  lastWFSwitch = millis();
}

void updateWoolFeed() {
  bool isEnabled = getVar(MVI_WF_SWITCH);
  if (woolFeedState < WFS_MAN_REVERSING && (woolFeedState > WFS_OFF != isEnabled)) {
    setWoolFeedState((uint8_t) isEnabled);
  }

  statusWFEnabled = getStatusState() == SS_RUNNING;
  if (statusWFEnabled != statusWasWFEnabled && statusWFEnabled != isEnabled) {
    setWoolFeedState((uint8_t)statusWFEnabled);
  }
  statusWasWFEnabled = statusWFEnabled;

  if (labelTouched(MLI_WF_FWD)) {
    setWoolFeedState(WFS_MAN_FEEDING);
  }
  else if (labelTouched(MLI_WF_REV)) {
    setWoolFeedState(WFS_MAN_REVERSING);
  }
  else if (woolFeedState >= WFS_MAN_REVERSING) {
    setWoolFeedState((uint8_t) isEnabled);
  }
  setVar(MVI_WF_ANALOG_IN, analogRead(WOOL_SENSE_PIN));
  if (woolFeedState == WFS_OFF) updateError(SE_NO_WOOL, SES_NORMAL);
  
  switch (woolFeedState) {
    case WFS_IDLE:
      if (millis() - lastWFSwitch > getVar(MVI_WF_PERIOD) * 1000L * 60L) setWoolFeedState(WFS_REVERSING);
      break;
    case WFS_REVERSING:
      if (millis() - lastWFSwitch > getVar(MVI_WF_REV_TIME) * 1000L) setWoolFeedState(WFS_FEEDING);
      break;
    case WFS_FEEDING:
      if (analogRead(WOOL_SENSE_PIN) < getVar(MVI_WF_THRESHOLD)) {
        setWoolFeedState(WFS_IDLE);
        updateError(SE_NO_WOOL, SES_NORMAL);
      }
      if (millis() - lastWFSwitch > 30 * 1000L) {
        setWoolFeedState(WFS_IDLE);
        if (getVar(MVI_WF_CAUSE_SHUTDOWN) == 1) updateError(SE_NO_WOOL, SES_ERROR);
        else updateError(SE_NO_WOOL, SES_TEXT_ONLY);
      }
      break;
  }
}

void setWoolFeedState(WF_STATE newState) {
  if (newState == woolFeedState) {
    return ;
  }
  if (newState < WFS_MAN_REVERSING && (newState > WFS_OFF != getVar(MVI_WF_SWITCH))) {
    setVar(MVI_WF_SWITCH, newState > WFS_OFF);
  }
  if (newState == WFS_MAN_REVERSING || newState == WFS_REVERSING) {
    digitalWrite(DRIVE_CHANNEL, LOW);
    digitalWrite(DRIVE_BRAKE, LOW);
    analogWrite(DRIVE_PWM, getVar(MVI_WF_PWR));
  }
  else if (newState == WFS_MAN_FEEDING || newState == WFS_FEEDING) {
    digitalWrite(DRIVE_CHANNEL, HIGH);
    digitalWrite(DRIVE_BRAKE, LOW);
    analogWrite(DRIVE_PWM, getVar(MVI_WF_PWR));
  }
  else {
    digitalWrite(DRIVE_BRAKE, HIGH);
    analogWrite(DRIVE_PWM, 0);
  }
  if (newState == WFS_OFF) {
    digitalWrite(CIRCULATION_PUMP_RELAY, LOW);
  }
  else if (newState > WFS_OFF && newState < WFS_MAN_REVERSING) {
    digitalWrite(CIRCULATION_PUMP_RELAY, HIGH);
  }
  lastWFSwitch = millis();
  woolFeedState = newState;
}

