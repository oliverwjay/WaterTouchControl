#ifndef MONITORING
#define MONITORING

#include <Arduino.h>


void initMonitoring();
void updateMonitoring();
float centralAverage(int vals[], uint8_t lowIndex, uint8_t highIndex);
float interpolateValue(float rawValue, float c[6][2]);
bool saveArray(float ar [], int len, String filename, String header);
bool logText(String text);

#endif
