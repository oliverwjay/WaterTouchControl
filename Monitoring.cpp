#include <SPI.h>
#include <SD.h>
#include <Arduino.h>
#include "Monitoring.h"
#include "Menu.h"
#include "Schedule.h"
#include "Status.h"
#include "Power.h"

#define PH_PROBE_PIN A7
#define N_PH_SAMPLES 20

#define SD_CHIP_SELECT 4

extern unsigned int __bss_end;
extern unsigned int __heap_start;
extern void *__brkval;

// Enable for logging to Serial Monitor
bool serialLogEnabled = true; //Note only one file can have Serial enabled at once

int phSamples[N_PH_SAMPLES];
uint8_t phIndex = 0;
float phValue;
float phCalibration[6][2] = {{ -1, -3.5}, { -2, -7}, {1.68, 0}, {4.0, 0}, {7.0, 0}, {10.0, 0}};
DateTime lastLog = DateTime(0);
long leastMemory = 5000;

void initMonitoring() {
  if (!SD.begin(SD_CHIP_SELECT)) {

  }
  lastLog = getTime();
  if (serialLogEnabled) {
    Serial.begin(9600);
  }
}

void updateMonitoring() {
  phSamples[phIndex] = analogRead(PH_PROBE_PIN);
  float phRawValue = centralAverage(phSamples, 6, 14) * 5.0 / 1024 / 6;
  phValue = interpolateValue(phRawValue, phCalibration);

  phIndex ++;
  phIndex %= N_PH_SAMPLES;

  bool loggingEnabled = getVar(MVI_M_LOGGING_SWITCH);
  if (getTime().secondstime() - 30 > lastLog.secondstime()) {
    String toPrint = "Memory free: ";
    toPrint += leastMemory;
    logText(toPrint);
    logText(String(getTime().secondstime()));
    if (!loggingEnabled) lastLog = getTime();
  }
  if (getTime().secondstime() - 30 > lastLog.secondstime() && loggingEnabled) {
    float save[3] = {getTime().secondstime(), getVoltage(), getCurrent()};
    bool a = saveArray(save, 3, String("datalog.csv"), String("Time, Voltage, Current"));
    lastLog = getTime();
  }
}

float centralAverage(int vals[], uint8_t lowIndex, uint8_t highIndex) {
  int len = sizeof(&vals) / sizeof(vals);
  int temp;
  for (int i = 0; i < len - 1; i++) {
    for (int j = i + 1; j < len; j++) {
      if (vals[i] > vals[j]) {
        temp = vals[i];
        vals[i] = vals[j];
        vals[j] = temp;
      }
    }
  }
  int sum = 0;
  if (vals[highIndex] == -1 || vals[lowIndex] == -1) {
    return -1;
  }
  for (int i = lowIndex; i <= highIndex; i++) {
    sum += vals[i];
  }
  return (float) sum / ((float) (highIndex - lowIndex + 1));
}

float interpolateValue(float rawValue, float c[6][2]) {
  int i1 = 0; // Closest calibration point
  int v1 = - 1000;
  int i2 = 1; // Second calibration point
  int v2 = - 2000;
  float v;

  for (int i = 0; i < 6; i ++) {
    v = abs(c[i][0] - rawValue);
    if (v < v2 && c[i][1] != 0) {
      if (v < v1) {
        v1 = v;
        i1 = i;
      }
      else {
        v2 = v;
        i2 = i;
      }
    }
  }
  return (rawValue - c[i1][0]) / (c[i2][0] - c[i1][0]) * (c[i2][1] - c[i1][1]) + c[i1][1];
}

bool saveArray(float ar [], int len, String filename, String header) {
  String toSave = "";

  for (int i = 0; i < len; i++) {
    toSave += String(ar[i]);
    if (i != len - 1) {
      toSave += ", ";
    }
  }
  File saveFile = SD.open(filename);
  bool needsHeader = true;
  if (saveFile) {
    saveFile.close();
    needsHeader = false;
  }

  saveFile = SD.open(filename, FILE_WRITE);

  if (saveFile) {
    if (needsHeader) saveFile.println(header);
    saveFile.println(toSave);
    saveFile.close();
    return true;
  }
  else {
    return false;
  }
}

bool logText(String text) {
  DateTime logTime = getTime();
  String filename = "";
  filename += String(logTime.month());
  filename += "_";
  filename += String(logTime.day());
  filename += "_";
  filename += String(logTime.year());
  filename += "_Run_Log.txt";

  String toSave = String(logTime.hour());
  toSave += ":";
  toSave += String(logTime.minute());
  toSave += ":";
  toSave += String(logTime.second());
  toSave += " ";
  toSave += text;

  if (serialLogEnabled) {
    Serial.println(toSave);
  }

  File saveFile = SD.open(filename, FILE_WRITE);

  if (saveFile) {
    saveFile.println(toSave);
    return true;
  }
  else {
    return false;
  }
}

int freeMemory() {
  int free_memory;

  if ((int)__brkval == 0)
    free_memory = ((int)&free_memory) - ((int)&__bss_end);
  else
    free_memory = ((int)&free_memory) - ((int)__brkval);

  return free_memory;
}

void checkMemory() {
  int free_memory = freeMemory();
  if (free_memory < leastMemory) {
    leastMemory = free_memory;
    String toPrint = "Min Memory: ";
    toPrint += leastMemory;
    logText(toPrint);
  }
}

