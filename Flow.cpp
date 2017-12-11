#include <Arduino.h>
#include "Flow.h"
#include "Status.h"
#include "Menu.h"

#define INFLUENT_PUMP_PIN 36
#define PEROXIDE_PUMP_PIN 25
#define BRINE_PUMP_PIN 34
#define EFFLUENT_VALVE_PIN 30

bool isFlowEnabled = false;
bool isDrainEnabled = false;

bool shouldEnableFlow() {
  return getStatusState() >= SS_SHUTDOWN_FLOW_ONLY;
}

bool shouldEnableDrain() {
  return getStatusState() >= SS_SHUTDOWN_DRAIN_ONLY;
}

void initFlow() {
  pinMode(INFLUENT_PUMP_PIN, OUTPUT);
  pinMode(PEROXIDE_PUMP_PIN, OUTPUT);
  pinMode(BRINE_PUMP_PIN, OUTPUT);
  pinMode(EFFLUENT_VALVE_PIN, OUTPUT);
}

void updateFlow() {
  if (isFlowEnabled != shouldEnableFlow()) {
    isFlowEnabled = shouldEnableFlow();
    digitalWrite(BRINE_PUMP_PIN, isFlowEnabled);
    digitalWrite(PEROXIDE_PUMP_PIN, isFlowEnabled);
    digitalWrite(INFLUENT_PUMP_PIN, isFlowEnabled);
  }
  if (isDrainEnabled != shouldEnableDrain()) {
    isDrainEnabled = shouldEnableDrain();
    digitalWrite(EFFLUENT_VALVE_PIN, isDrainEnabled);
  }
}

