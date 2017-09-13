#include <Arduino.h>
#include "Status.h"
#include "Menu.h"
#include "DwellTank.h"
#include "Monitoring.h"
#include "Power.h"
#include "Schedule.h"

StatusState statusState;
bool targetOn = false;
unsigned long stateSetTime;
unsigned long warningTime;
DateTime stateSetDateTime;

String warningString = "";

SErrorState errorStates[SE_N_ERRORS];

void initStatus() {
  statusState = SS_OFF;
  stateSetTime = millis();
  warningTime = millis();
  stateSetDateTime = getTime();
  Serial.begin(9600);
}

void updateStatus() {
  targetOn = getVar(MVI_STATUS_SWITCH) == 1.0;
  if (targetOn) {
    switch (statusState) {
      case SS_OFF:
        setStatusState(SS_STARTUP_FLOW_ENABLE);
        break;
      case SS_STARTUP_FLOW_ENABLE:
        if (isFlowing()) {
          setStatusState(SS_STARTUP_PS_ENABLE);
        }
        break;
      case SS_STARTUP_PS_ENABLE:
        if (getPowerState() == PS_ON) { //Wait for some voltage
          setStatusState(SS_STARTUP_PS_RELAY);
        }
        break;
      case SS_STARTUP_PS_RELAY:
        if (millis() - stateSetTime > 1000L * 60L *getVar(MVI_WOOL_DELAY)) { //One zero removed for testing
          setStatusState(SS_RUNNING);
        }
        break;
      case SS_RUNNING:
        break;
      case SS_SHUTDOWN_FLOW_ONLY:
        setStatusState(SS_STARTUP_FLOW_ENABLE);
        break;
      case SS_SHUTDOWN_DRAIN_ONLY:
        setStatusState(SS_STARTUP_FLOW_ENABLE);
        break;
    }
  }
  else {
    switch (statusState) {
      case SS_OFF:
        break;
      case SS_SHUTDOWN_FLOW_ONLY:
        setStatusState(SS_SHUTDOWN_DRAIN_ONLY);
        break;
      case SS_SHUTDOWN_DRAIN_ONLY:
        if (!isFlowing()) {
          setStatusState(SS_OFF);
        }
        break;
      default:
        setStatusState(SS_SHUTDOWN_FLOW_ONLY);
    }
  }
  warningString = "";
  bool startTimer = false;
  for (int i =0; i < SE_N_ERRORS; i++){
    if (errorStates[i] == SES_ERROR || errorStates[i] == SES_TEXT_ONLY){
      warningString += errorText[i];
    }
    if (errorStates[i] == SES_ERROR) {
      startTimer = true;
    }
  }
  if (warningString == ""){
    warningString = "No issues";
  }
  if (!startTimer) warningTime = millis();
  if (statusState == SS_OFF) warningTime = millis() + 1000L*60L*3;
  long warningDuration = millis() < warningTime ? 0 : millis() - warningTime;
  Serial.println(warningDuration);
  if(warningDuration > (long)1000L*60L*getVar(MVI_WARNING_PERIOD) && targetOn){
    setStatusState(SS_SHUTDOWN_FLOW_ONLY);
  }
  setLabelText(MLI_STATUS_WARNING, warningString);
  setTimeVar(MTVI_STATE_TIME, stateSetDateTime);
}

void setStatusState(StatusState state) {
  if (state == statusState) {
    return;
  }
  targetOn = state >= SS_STARTUP_FLOW_ENABLE;
  setVar(MVI_STATUS_SWITCH, targetOn);
  statusState = state;
  stateSetTime = millis();
  stateSetDateTime = getTime();
  setLabelText(MLI_STATUS_STATE, statusStateName[state]);
}

StatusState getStatusState() {
  return statusState;
}

void updateError(SErrorI error, bool isError) {
  if(errorStates[error] != SES_IGNORE){
  	errorStates[error] = isError ? SES_ERROR : SES_NORMAL;
  }
}
