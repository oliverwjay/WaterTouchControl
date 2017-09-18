#include <Arduino.h>
#include "DwellTank.h"
#include "Menu.h"
#include "Monitoring.h"
#include "Status.h"

//Change this to enable serial debugging of the dwell tank
#define DEBUG_DWELL_TANK false  //NOTE: serial monitor can only be enabled in one file at a time

#define ECHO_PIN 29 // Echo pin
#define TRIG_PIN 28 // Trigger pin
#define PUMP_RELAY_PIN 44 // Pin to relay for pump

#define DT_JUMP_PIN 22

#define TOTAL_VOL 12.0
#define VOL_PER_CM 0.5

bool debugMode = false;

int maximumRange = 200; // Maximum range needed
int minimumRange = 1; // Minimum range needed
long duration, distance; // Duration used to calculate distance
int dist; // Current distance
int dists[50]; //Array of last 50 distance readings
int oldDists[50]; //Array of old distance readings to observe change
uint8_t index = 0; //Current index in dists
uint8_t oldDistsIndex = 0;
uint8_t pauseIndex = -1;

int fillTime = 0;
int drainTime = 0;
long lastSwitch = 0;

#define TRIGGER_N 45 //Number that must meet condition to trigger

DT_STATE dwellTankState = DTS_INACTIVE;

float cmToVol(float cm) {
  return TOTAL_VOL - cm * VOL_PER_CM;
}

float volToCm(float vol) {
  return (TOTAL_VOL - vol) / VOL_PER_CM;
}

int getDist() { //Get current distance
  /* The following trigPin/echoPin cycle is used to determine the
    distance of the nearest object by bouncing soundwaves off of it.
    Function slightly modified from keyestudio code*/

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH); //Ping
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);
  duration = pulseIn(ECHO_PIN, HIGH, 30000); //Read pulse with 30ms timeout

  //Calculate the distance (in cm) based on the speed of sound.
  distance = duration / 58.2;
  if (debugMode) {
    distance = ((float)analogRead(A15)) / 40.0;
    if (digitalRead(DT_JUMP_PIN)) distance = -1;
  }

  if (distance >= maximumRange || distance <= minimumRange) {
    /* Send a negative number to computer and Turn LED ON
      to indicate "out of range" */
    dist = -1;
  }
  else {
    /* Send the distance to the computer using Serial protocol, and
      turn LED OFF to indicate successful reading. */
    dist = distance;
  }
  return dist;
}

bool meetsCondition(int val) { //Checks if a value is in range to trigger a state change
  if (dwellTankState == DTS_INACTIVE && val > -1 && val <= volToCm(getVar(MVI_DT_MAX_VOL))) {
    return true;
  }
  else if (dwellTankState == DTS_FLUSHING && val > -1 && val >= volToCm(getVar(MVI_DT_MIN_VOL))) {
    return true;
  }
  else {
    return false;
  }
}

void turnOnPump() {
  digitalWrite(PUMP_RELAY_PIN, HIGH);
  fillTime = ((int)((millis() - lastSwitch) / 1000));
  setVar(MVI_DT_FILL_TIME, fillTime);
  lastSwitch = millis();
}

void turnOffPump() {
  digitalWrite(PUMP_RELAY_PIN, LOW);
  drainTime = ((int)((millis() - lastSwitch) / 1000));
  setVar(MVI_DT_DRAIN_TIME, drainTime);
  lastSwitch = millis();
}


void setPumpState(DT_STATE state) { //Update pump state
  if (dwellTankState == DTS_FLUSHING && state != DTS_FLUSHING) {
    turnOffPump();
  }
  else if (dwellTankState != DTS_FLUSHING && state == DTS_FLUSHING) {
    turnOnPump();
  }
  dwellTankState = state;
}

void resetDists() {
  for (int i = 0; i < 50; i++) {
    dists[i] = -2; //Set array to out of range flags
  }
}

bool isFlowing() {
  int oldDist = 0;
  int newDist = 0;
  int i = oldDistsIndex;
  int j = (oldDistsIndex + 49) % 50;
  while (oldDist < 0 && i != j) { //Finds a valid refrence distance
    oldDist = oldDists[i];
    i = (i + 1) % 50;
  }
  while (oldDist < 0 && j != i) {//Finds a valid recent distance
    newDist = oldDists[i];
    i = (i + 49) % 50;
  }
  if (index == 0 && DEBUG_DWELL_TANK) {
    Serial.print("ms since last switch: "); Serial.println(millis() - lastSwitch);
    Serial.print("Reference distance: "); Serial.println(oldDists[oldDistsIndex]);
    Serial.print("Current distance: ");Serial.println(oldDists[(oldDistsIndex - 1) % 50]);
  }
  if (millis() - lastSwitch < 120000 && lastSwitch > 5000) {
    return true;
  }
  else if (dwellTankState == DTS_INACTIVE && oldDists[oldDistsIndex] != 0 && oldDists[oldDistsIndex] - oldDists[(oldDistsIndex + 49) % 50] > 2) {
    return true;
  }
  else if (dwellTankState == DTS_FLUSHING) {
    return true;
  }
  else {
    return false;
  }
}

void initDwellTank() {
  //Initialize ports
  pinMode(PUMP_RELAY_PIN, OUTPUT); // Update pump
  pinMode(ECHO_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(DT_JUMP_PIN, INPUT_PULLUP);
  debugMode = !digitalRead(DT_JUMP_PIN);

#if DEBUG_DWELL_TANK==true
  Serial.begin(9600);
#endif

  turnOffPump(); // Default off
  resetDists(); //Clear dists
}

void updateDwellTank() { //Loop function to update state and pump
  dists[index] = getDist(); //Check distance
  index = (index + 1) % 50; //Update index
  if (index == 0) {
    oldDists[oldDistsIndex] = dists[index];
    oldDistsIndex = (oldDistsIndex + 1) % 50;
  }
  updateError(SE_DT_NO_FLOW, !isFlowing());

  float level = centralAverage(dists, 3, 46);

  updateError(SE_DT_BAD_READING, level == -1);

  setVar(MVI_DT_CURRENT_VOL, cmToVol(level)); //Update user interface

  if (level == -1) {
    return;
  }

  updateError(SE_DT_OVERFLOW, level < volToCm(getVar(MVI_DT_ESTOP_VOL)));

  if (level < volToCm(getVar(MVI_DT_MAX_VOL)) && dwellTankState == DTS_INACTIVE && millis() - lastSwitch > 2000) {
    setPumpState(DTS_FLUSHING);
  }
  else if (level > volToCm(getVar(MVI_DT_MIN_VOL)) && dwellTankState == DTS_FLUSHING && millis() - lastSwitch > 2000) {
    setPumpState(DTS_INACTIVE);
  }

  //  int nMeetsCondition = 0; //Number of data points that meet state change conditions
  //  int nMeetsEmergency = 0; //Number of data points that meet estop conditions
  //  for (int i=0; i<50; i++){ //Check how many data points meet state change and estop conditions
  //    if(meetsCondition(dists[i])){
  //      nMeetsCondition ++;
  //    }
  //    if(dists[i]>0&&dists[i]<volToCm(getVar(MVI_DT_ESTOP_VOL))){
  //      nMeetsEmergency ++;
  //    }
  //  }
  //  if (nMeetsEmergency > TRIGGER_N){ //Check if estop should be triggered
  //    //disable();
  //  }
  //  if (nMeetsCondition > TRIGGER_N){ //Check if state change should be triggered
  //    if(dwellTankState==DTS_INACTIVE){
  //      setPumpState(DTS_FLUSHING);
  //    }
  //    else if(dwellTankState==DTS_FLUSHING){
  //      setPumpState(DTS_INACTIVE);
  //    }
  //    resetDists(); //Clear distances
  //  }
}
