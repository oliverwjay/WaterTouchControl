#include "Arduino.h"
#include "Menu.h"
#include "DwellTank.h"
#include "Status.h"
#include "Power.h"
#include "Schedule.h"
#include "Flow.h"
#include "Monitoring.h"
#include "WoolFeed.h"

void setup() {
  // put your setup code here, to run once:
  initMenu();
  initSchedule();
  initStatus();
  initDwellTank();
  initPower();
  initFlow();
  initMonitoring();
  initWoolFeed();
}

void loop() {
  // put your main code here, to run repeatedly:
  updateStatus();
  updateDwellTank();
  updatePower();
  updateSchedule();
  updateFlow();
  updateMonitoring();
  updateWoolFeed();
  updateMenu(50);
  while (millis() % 50 > 1) {
    delay(1);
  }
}
