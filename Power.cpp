#include <Arduino.h>
#include "Power.h"
#include "Menu.h"
#include "Status.h"

#define POWER_RELAY_PIN 40

PowerState powerState = PS_OFF;
bool statusWasEnabled = false;
bool statusEnabled = false;
bool psEnabled = false;
float measuredCurrent = 0;
float measuredVoltage = 0;
float recievedCurrent = 0;
float recievedVoltage = 0;
float measuredRefrence = 0;

void initPower() {
  pinMode(POWER_RELAY_PIN, OUTPUT);
//  Serial.begin(9600);
  Serial1.begin(9600);
}

bool serialArray(float ar[], int len) {
  float temp;
  bool success = true;
  if (Serial1.available() && Serial1.read() == '(') {
    for (int i = 0; i < len; i++) {
      if (Serial1.available() && success) {
        temp = Serial1.parseFloat();
        ar[i] = temp;
      }
      else {
        success = false;
      }
    }
  }
  if (Serial1.available() && Serial1.read() == ')') {
    return success;
  }
  else {
    return false;
  }
}

void updatePower() {
  statusWasEnabled = statusEnabled;
  statusEnabled = getStatusState() >= SS_STARTUP_PS_ENABLE;
  if (statusEnabled != statusWasEnabled) {
    setPowerState(statusEnabled ? PS_STARTUP : PS_OFF);
  }
  else if (getVar(MVI_POWER_SWITCH) != (powerState > PS_MANUAL_OFF)) {
    setPowerState(1 + (powerState <= PS_MANUAL_OFF));
  }
  if (powerState == PS_STARTUP && measuredVoltage > 2.0) {
    setPowerState(PS_ON);
  }
  if (Serial1.available()) {
    if (Serial1.read() == 'U') {
      float inputs[7];
      bool wasChanged;
      if (serialArray(inputs, 7)) {
        psEnabled = (int) inputs[0];
        recievedCurrent = inputs[1];
        recievedVoltage = inputs[2];
        measuredCurrent = inputs[3];
        measuredVoltage = inputs[4];
        measuredRefrence = inputs[5];
        wasChanged = (int) inputs[6];
        if (wasChanged){
          setVar(MVI_P_SET_CURRENT, recievedCurrent);
          setVar(MVI_P_SET_VOLTAGE, recievedVoltage);
          if (getVar(MVI_POWER_SWITCH) != psEnabled){
            setPowerState(PS_MANUAL_OFF + psEnabled);
          }
        }
      }
    }
  }
  String toSend = "T(";
  toSend += String(powerState > PS_MANUAL_OFF);
  toSend += ",";
  toSend += String(getVar(MVI_P_SET_CURRENT));
  toSend += ",";
  toSend += String(getVar(MVI_P_SET_VOLTAGE));
  toSend += ")";
  Serial1.println(toSend);
  while (Serial1.available()) {
    Serial1.read();
  }

  setVar(MVI_P_M_VOLTAGE, measuredVoltage);
  setVar(MVI_P_M_CURRENT, measuredCurrent);
  setVar(MVI_P_REFERENCE_V, measuredRefrence);
}

void setPowerState(PowerState state) {
  if (state == powerState) {
    return;
  }
  powerState = state;
  setVar(MVI_POWER_SWITCH, state > PS_MANUAL_OFF);
  if (powerState >= PS_ON) {
    digitalWrite(POWER_RELAY_PIN, HIGH);
  }
  else {
    digitalWrite(POWER_RELAY_PIN, LOW);
  }
}

PowerState getPowerState() {
  return powerState;
}

float getVoltage() {
  return measuredVoltage;
}

float getCurrent() {
  return measuredCurrent;
}
