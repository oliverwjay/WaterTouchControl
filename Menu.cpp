#include <Key.h>
#include <Keypad.h>

#include <EEPROM.h>
#include <Arduino.h>
#include <math.h>
#include <Adafruit_GFX.h>    // Core graphics library
//#include <Adafruit_TFTLCD.h> // Hardware-specific library
//#include <pin_magic.h>
//#include <registers.h>
#include "TouchScreen.h"
#include "RTClib.h"
#include "Adafruit_HX8357.h"

#include  "Menu.h"
#include "Schedule.h"
#include "Monitoring.h"

#define WASTED_MEMORY 0 //Set to zero for normal opperation set to higher (eg 300) to add memory pressure

// The control pins for the LCD can be assigned to any digital or
// analog pins...but we'll use the analog pins as this allows us to
// double up the pins with the touch screen (see the TFT paint example).
#define LCD_CS A8 // Chip Select goes to Analog 3
#define LCD_CD A9 // Command/Data goes to Analog 2
#define LCD_WR A10 // LCD Write goes to Analog 1
#define LCD_RD A11 // LCD Read goes to Analog 0

#define LCD_RESET -1 // Can alternately just connect to Arduino's reset pin

// These are the four touchscreen analog pins
#define YP A11  // must be an analog pin, use "An" notation!
#define XM A8  // must be an analog pin, use "An" notation!
#define YM A9   // can be a digital pin
#define XP A10   // can be a digital pin

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 110
#define TS_MINY 80
#define TS_MAXX 900
#define TS_MAXY 940

#define MINPRESSURE 30
#define MAXPRESSURE 1000

// The display uses hardware SPI, plus #9 & #10
#define TFT_RST 53  // dont use a reset pin, tie to arduino RST if you like
#define TFT_DC 49
#define TFT_CS 48

#define TAB_HEIGHT 40
#define TAB_WIDTH 130
#define ROW_OFFSET 48

#define KEYPAD_PIN 12

const MRect NULL_RECT = { -1, -1, -1, -1};

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC, TFT_RST);

//Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

// For better pressure precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// For the one we're using, its 300 ohms across the X plate
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

#define USE_ANALOG_KEYPAD false
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns

//define the symbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
  {1, 2, 3, 10},
  {4, 5, 6, 11},
  {7, 8, 9, 12},
  {14, 20, 15, 13}
};
byte rowPins[ROWS] = {45, 43, 41, 39}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {37, 35, 33, 31}; //connect to the column pinouts of the keypad

long memoryHog[WASTED_MEMORY];

//initialize an instance of class NewKeypad
Keypad keypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

TSPoint p;
bool wasTouched = false;

int digit = -1;
int lastDigit = -1;

MTabI selectedTab = MTI_STATUS;

MTab * tabs[N_TABS];
MLabel * labels[N_LABELS];
MVar * vars[N_VARS];
MTimeVar * timeVars[N_TIME_VARS];

bool isEditing = false;

int showIndex = 0;
int priorityIndex = 0;
MPriority priority;

const int N_VIEWS = (N_TABS + N_VARS + N_LABELS + N_TIME_VARS);
MView * views[N_VIEWS];

void initMenu() {
  //  tft.reset();
  //  uint16_t identifier = tft.readID();
  //  tft.begin(identifier);
  for(int i = 0; i <WASTED_MEMORY;i++) memoryHog[i] = 294857L;

  tft.begin(HX8357D);

  tft.setTextSize(2);
  tft.setRotation(1);
  tft.fillScreen(BG_COLOR);
  tft.drawFastHLine(0, TAB_HEIGHT - 1, 480, LINE_COLOR);
  tft.drawFastHLine(0, TAB_HEIGHT - 2, 480, LINE_COLOR);
  tft.setTextColor(TEXT_COLOR);


  tabs[MTI_STATUS] = new MTab("Status", MTI_STATUS);

  tabs[MTI_FLOW] = new MTab("Flow", MTI_FLOW);
  tabs[MTI_POWER] = new MTab("Power", MTI_POWER);
  tabs[MTI_WOOL_FEED] = new MTab("Wool Feed", MTI_WOOL_FEED);
  tabs[MTI_DWELL_TANK] = new MTab("Dwell Tank", MTI_DWELL_TANK);
  tabs[MTI_MONITORING] = new MTab("Monitoring", MTI_MONITORING);
  tabs[MTI_SCHEDULE] = new MTab("Schedule", MTI_SCHEDULE);


  timeVars[MTVI_CURRENT_TIME] = new MTimeVar(MTVI_CURRENT_TIME, MTI_ALL, MTVT_FULL_TIME, false, 6, ROW_OFFSET - TAB_HEIGHT, MVS_EDITING);


  labels[MLI_STATUS_WARNING] = new MLabel(MLI_STATUS_WARNING, MTI_ALL, "No issues", 150,  ROW_OFFSET - TAB_HEIGHT);

  vars[MVI_STATUS_SWITCH] = new MSwitch(MVI_STATUS_SWITCH, MTI_STATUS, "Off", "On", 0, true, TAB_WIDTH + 10, ROW_OFFSET);

  labels[MLI_STATUS_STATE] = new MLabel(MLI_STATUS_STATE, MTI_STATUS, "Disabled", TAB_WIDTH + 95, ROW_OFFSET);
  timeVars[MTVI_STATE_TIME] = new MTimeVar(MTVI_STATE_TIME, MTI_STATUS, MTVT_SHORT_TIME, false, TAB_WIDTH + 275, ROW_OFFSET);

  vars[MVI_FLOW_RATE] = new MVar(MVI_FLOW_RATE, MTI_FLOW, "L/min", 1, false, TAB_WIDTH + 40, ROW_OFFSET, MVS_VISABLE, TAB_WIDTH + 10, TAB_HEIGHT + ROW_OFFSET);

  labels[MLI_P_MEASURED] = new MLabel(MLI_P_MEASURED, MTI_POWER, "Measured:", TAB_WIDTH + 10, ROW_OFFSET + TAB_HEIGHT);
  vars[MVI_P_M_VOLTAGE] = new MVar(MVI_P_M_VOLTAGE, MTI_POWER, "V", 3, false, TAB_WIDTH + 130, ROW_OFFSET + TAB_HEIGHT, MVS_VISABLE, TAB_WIDTH + 10, 2 * TAB_HEIGHT + ROW_OFFSET);
  vars[MVI_P_M_CURRENT] = new MVar(MVI_P_M_CURRENT, MTI_POWER, "A", 3, false, TAB_WIDTH + 230, ROW_OFFSET + TAB_HEIGHT, MVS_VISABLE, TAB_WIDTH + 160, 2 * TAB_HEIGHT + ROW_OFFSET);
  vars[MVI_POWER_SWITCH] = new MSwitch(MVI_POWER_SWITCH, MTI_POWER, "Off", "On", 0, false, TAB_WIDTH + 10, ROW_OFFSET);
  labels[MLI_P_REFERENCE] = new MLabel(MLI_P_REFERENCE, MTI_POWER, "Reference:", TAB_WIDTH + 10, ROW_OFFSET + 2 * TAB_HEIGHT);
  vars[MVI_P_REFERENCE_V] = new MVar(MVI_P_REFERENCE_V, MTI_POWER, "V", 3, true, TAB_WIDTH + 142, ROW_OFFSET + TAB_HEIGHT * 2);
  labels[MLI_P_SET] = new MLabel(MLI_P_SET, MTI_POWER, "Set:", TAB_WIDTH + 10, ROW_OFFSET + 3 * TAB_HEIGHT);
  vars[MVI_P_SET_VOLTAGE] = new MVar(MVI_P_SET_VOLTAGE, MTI_POWER, "V", 3, true, TAB_WIDTH + 70, ROW_OFFSET + TAB_HEIGHT * 3, MVS_EDITING);
  vars[MVI_P_SET_CURRENT] = new MVar(MVI_P_SET_CURRENT, MTI_POWER, "A", 3, true, TAB_WIDTH + 170, ROW_OFFSET + TAB_HEIGHT * 3, MVS_EDITING);

  vars[MVI_DT_MAX_VOL] = new MVar(MVI_DT_MAX_VOL, MTI_DWELL_TANK, "L", 1, true, TAB_WIDTH + 130, ROW_OFFSET + TAB_HEIGHT, MVS_EDITING, TAB_WIDTH + 130, ROW_OFFSET + 4 * TAB_HEIGHT);
  vars[MVI_DT_MIN_VOL] = new MVar(MVI_DT_MIN_VOL, MTI_DWELL_TANK, "L", 1, true, TAB_WIDTH + 238, ROW_OFFSET + TAB_HEIGHT, MVS_EDITING, TAB_WIDTH + 238, ROW_OFFSET + 4 * TAB_HEIGHT);
  vars[MVI_DT_ESTOP_VOL] = new MVar(MVI_DT_ESTOP_VOL, MTI_DWELL_TANK, "L", 1, true, TAB_WIDTH + 166, ROW_OFFSET + 2 * TAB_HEIGHT, MVS_EDITING);
  vars[MVI_DT_CURRENT_VOL] = new MVar(MVI_DT_CURRENT_VOL, MTI_DWELL_TANK, "L", 1, false, TAB_WIDTH + 190, ROW_OFFSET + 3 * TAB_HEIGHT);
  vars[MVI_DT_FILL_TIME] = new MVar(MVI_DT_FILL_TIME, MTI_DWELL_TANK, "s", 1, false, TAB_WIDTH + 70, ROW_OFFSET + 4 * TAB_HEIGHT);
  vars[MVI_DT_DRAIN_TIME] = new MVar(MVI_DT_DRAIN_TIME, MTI_DWELL_TANK, "s", 1, false, TAB_WIDTH + 82, ROW_OFFSET + 5 * TAB_HEIGHT);

  labels[MLI_DT_0] = new MLabel(MLI_DT_0, MTI_DWELL_TANK, "Pump from", TAB_WIDTH + 10, ROW_OFFSET + TAB_HEIGHT, MVS_VISABLE, TAB_WIDTH + 10, ROW_OFFSET + 4 * TAB_HEIGHT);
  labels[MLI_DT_1] = new MLabel(MLI_DT_1, MTI_DWELL_TANK, "to", TAB_WIDTH + 202, ROW_OFFSET + TAB_HEIGHT, MVS_VISABLE, TAB_WIDTH + 202, ROW_OFFSET + 4 * TAB_HEIGHT);
  labels[MLI_DT_3] = new MLabel(MLI_DT_3, MTI_DWELL_TANK, "EStop volume", TAB_WIDTH + 10, ROW_OFFSET + 2 * TAB_HEIGHT);
  labels[MLI_DT_2] = new MLabel(MLI_DT_2, MTI_DWELL_TANK, "Current volume", TAB_WIDTH + 10, ROW_OFFSET + 3 * TAB_HEIGHT);
  labels[MLI_DT_4] = new MLabel(MLI_DT_4, MTI_DWELL_TANK, "Fill", TAB_WIDTH + 10, ROW_OFFSET + 4 * TAB_HEIGHT);
  labels[MLI_DT_5] = new MLabel(MLI_DT_5, MTI_DWELL_TANK, "Drain", TAB_WIDTH + 10, ROW_OFFSET + 5 * TAB_HEIGHT);

  vars[MVI_WF_SWITCH] = new MSwitch(MVI_WF_SWITCH, MTI_WOOL_FEED, "Off", "On", 0, false, TAB_WIDTH + 10, ROW_OFFSET);
  labels[MLI_WF_FWD] = new MLabel(MLI_WF_FWD, MTI_WOOL_FEED, "Man. Forward", TAB_WIDTH + 10, ROW_OFFSET + TAB_HEIGHT);
  labels[MLI_WF_REV] = new MLabel(MLI_WF_REV, MTI_WOOL_FEED, "Man. Reverse", TAB_WIDTH + 160, ROW_OFFSET + TAB_HEIGHT);
  labels[MLI_WF_POWER] = new MLabel(MLI_WF_POWER, MTI_WOOL_FEED, "Power", TAB_WIDTH + 10, ROW_OFFSET + TAB_HEIGHT * 2);
  vars[MVI_WF_PWR] = new MVar(MVI_WF_PWR, MTI_WOOL_FEED, " (0-255)", 0, true, TAB_WIDTH + 82, ROW_OFFSET + TAB_HEIGHT * 2, MVS_EDITING);
  labels[MLI_WF_PERIOD] = new MLabel(MLI_WF_PERIOD, MTI_WOOL_FEED, "Adjust every", TAB_WIDTH + 10, ROW_OFFSET + TAB_HEIGHT * 3);
  vars[MVI_WF_PERIOD] = new MVar(MVI_WF_PERIOD, MTI_WOOL_FEED, "min", 1, true, TAB_WIDTH + 166, ROW_OFFSET + TAB_HEIGHT * 3, MVS_EDITING);
  labels[MLI_WF_REV_TIME] = new MLabel(MLI_WF_REV_TIME, MTI_WOOL_FEED, "Retract for", TAB_WIDTH + 10, ROW_OFFSET + TAB_HEIGHT * 4);
  vars[MVI_WF_REV_TIME] = new MVar(MVI_WF_REV_TIME, MTI_WOOL_FEED, "s", 0, true, TAB_WIDTH + 154, ROW_OFFSET + TAB_HEIGHT * 4, MVS_EDITING);
  vars[MVI_WF_ANALOG_IN] = new MVar(MVI_WF_ANALOG_IN, MTI_WOOL_FEED, "", 0, false, TAB_WIDTH + 10, ROW_OFFSET + TAB_HEIGHT * 5);
  vars[MVI_WF_THRESHOLD] = new MVar(MVI_WF_THRESHOLD, MTI_WOOL_FEED, "", 0, true, TAB_WIDTH + 190, ROW_OFFSET + TAB_HEIGHT * 5, MVS_EDITING);
  labels[MLI_WF_CAUSE_SHUTDOWN] = new MLabel(MLI_WF_CAUSE_SHUTDOWN, MTI_WOOL_FEED, "Trigger EStop", TAB_WIDTH + 10, ROW_OFFSET + TAB_HEIGHT * 6);
  vars[MVI_WF_CAUSE_SHUTDOWN] = new MSwitch(MVI_WF_CAUSE_SHUTDOWN, MTI_WOOL_FEED, "No", "Yes", 0, true, TAB_WIDTH + 178, ROW_OFFSET + TAB_HEIGHT * 6);

  vars[MVI_M_PH] = new MVar(MVI_M_PH, MTI_MONITORING, "", 2, false, TAB_WIDTH + 46, ROW_OFFSET + TAB_HEIGHT, MVS_VISABLE, TAB_WIDTH + 46, ROW_OFFSET + 5 * TAB_HEIGHT);

  labels[MLI_M_PH] = new MLabel(MLI_M_PH, MTI_MONITORING, "pH", TAB_WIDTH + 10, ROW_OFFSET + TAB_HEIGHT, MVS_VISABLE, TAB_WIDTH + 10, ROW_OFFSET + 5 * TAB_HEIGHT);
  labels[MLI_M_168] = new MLabel(MLI_M_168, MTI_MONITORING, "1.68", TAB_WIDTH + 10, ROW_OFFSET + 2 * TAB_HEIGHT);
  labels[MLI_M_4] = new MLabel(MLI_M_4, MTI_MONITORING, "4.0", TAB_WIDTH + 70, ROW_OFFSET + 2 * TAB_HEIGHT);
  labels[MLI_M_7] = new MLabel(MLI_M_7, MTI_MONITORING, "7.0", TAB_WIDTH + 130, ROW_OFFSET + 2 * TAB_HEIGHT);
  labels[MLI_M_10] = new MLabel(MLI_M_10, MTI_MONITORING, "10.0", TAB_WIDTH + 190, ROW_OFFSET + 2 * TAB_HEIGHT);
  labels[MLI_M_CLEAR] = new MLabel(MLI_M_CLEAR, MTI_MONITORING, "Clear", TAB_WIDTH + 250, ROW_OFFSET + 2 * TAB_HEIGHT);

  vars[MVI_M_LOGGING_SWITCH] = new MSwitch(MVI_M_LOGGING_SWITCH, MTI_MONITORING, "Off", "On", 0, false, TAB_WIDTH + 190, ROW_OFFSET + TAB_HEIGHT * 4);
  labels[MLI_M_LOG] = new MLabel(MLI_M_LOG, MTI_MONITORING, "Datalogging", TAB_WIDTH + 10, ROW_OFFSET + TAB_HEIGHT * 4);

  vars[MVI_FLOW_RATE]->value = .9;

  timeVars[MTVI_S_SHUTDOWN_TIME] = new MTimeVar(MTVI_S_SHUTDOWN_TIME, MTI_SCHEDULE, MTVT_DATE_TIME, true, TAB_WIDTH + 130, ROW_OFFSET + TAB_HEIGHT, MVS_EDITING);
  timeVars[MTVI_S_STARTUP_TIME] = new MTimeVar(MTVI_S_STARTUP_TIME, MTI_SCHEDULE, MTVT_DATE_TIME, true, TAB_WIDTH + 130, ROW_OFFSET + 2 * TAB_HEIGHT, MVS_EDITING);
  vars[MVI_WOOL_DELAY] = new MVar(MVI_WOOL_DELAY, MTI_SCHEDULE, "min", 0, true, TAB_WIDTH + 142, ROW_OFFSET + TAB_HEIGHT * 4, MVS_EDITING);

  labels[MLI_S_SHUTDOWN] = new MLabel(MLI_S_SHUTDOWN, MTI_SCHEDULE, "Shutdown:", TAB_WIDTH + 10, ROW_OFFSET + TAB_HEIGHT);
  labels[MLI_S_STARTUP] = new MLabel(MLI_S_STARTUP, MTI_SCHEDULE, "Startup:", TAB_WIDTH + 10, ROW_OFFSET + 2 * TAB_HEIGHT);
  labels[MLI_S_WOOL_DELAY] = new MLabel(MLI_S_WOOL_DELAY, MTI_SCHEDULE, "Iron delay", TAB_WIDTH + 10, ROW_OFFSET + TAB_HEIGHT * 4);

  labels[MLI_S_STATUS] = new MLabel(MLI_S_STATUS, MTI_STATUS, "Nothing Scheduled", TAB_WIDTH + 10, ROW_OFFSET + 6 * TAB_HEIGHT);

  vars[MVI_WARNING_PERIOD] = new MVar(MVI_WARNING_PERIOD, MTI_SCHEDULE, "min", 0, true, TAB_WIDTH + 154, ROW_OFFSET + TAB_HEIGHT * 5, MVS_EDITING);
  labels[MLI_WARNING_PERIOD] = new MLabel(MLI_WARNING_PERIOD, MTI_SCHEDULE, "EStop delay", TAB_WIDTH + 10, ROW_OFFSET + TAB_HEIGHT * 5);

  //  vars[MVI_TIME] = new MVar(MVI_TIME, MTI_SCHEDULE, "s", 0, false, 250, 75, MVS_VISABLE);
  //  vars[MVI_TEST] = new MVar(MVI_TEST, MTI_POWER, "V", 2, false, 150, 200, MVS_EDITING, 150, 150);;
  //  vars[MVI_TEST]->value = 82.19284;
  //  vars[MVI_TEST2] = new MSwitch(MVI_TEST2, MTI_FLOW, "On", "Off", 0, false, 200, 250);

  int index = 0;
  for (int i = 0; i < N_TABS; i++) {
    views[index] = tabs[i];
    index++;
  }
  for (int i = 0; i < N_VARS; i++) {
    views[index] = vars[i];
    index++;
  }
  for (int i = 0; i < N_LABELS; i++) {
    views[index] = labels[i];
    index++;
  }
  for (int i = 0; i < N_TIME_VARS; i++) {
    views[index] = timeVars[i];
    index++;
  }
}

void updateMenu(int endTime) {
  MTabI lastTab = selectedTab;
  updateTouch();
  if (lastTab != selectedTab) {
    priority = MP_HIGH;
    priorityIndex = showIndex;
  }

#if WASTED_MEMORY > 0
//  String strings[20];
//  for (int i = 0; i < WASTED_MEMORY; i++) {
//    strings[i] = "This string is long to test wasting memory to simulate crash";
//    checkMemory();
//  }
#endif

  isEditing = false;
  lastDigit = digit;
  digit = getButton();
  for (int i = 0; i < N_VARS + N_TIME_VARS; i++) {
    MVar * tempVar = i < N_VARS ? vars[i] : timeVars[i - N_VARS];
    if (tempVar != 0) {
      if (tempVar->viewState == MVS_EDITING) {
        isEditing = true;
        if (digit >= 0 && lastDigit < 0) {
          tempVar->updateTemp(digit);
        }
      }
    }
  }

  int endIndex = showIndex - 1;
  while (showIndex != endIndex && millis() % endTime < endTime - 20) {
    if (views[showIndex] != 0) {
      views[showIndex]->show(priority);
    }
    showIndex ++;
    showIndex %= N_VIEWS;
    if (priority > MP_LOW && showIndex == priorityIndex) {
      priority = MP_LOW;
    }
  }
  checkMemory();
}

void updateTouch() {
  p = getPress();
  if (p.z != 0 && !wasTouched) {
    for (int i = 0; i < N_VIEWS; i ++) {
      if (views[i] != 0) views[i]->touched(p);
    }
  }
  wasTouched = p.z > 0;
}

bool pointInRect(TSPoint point, MRect rect) {
  if (point.x > rect.x && point.y > rect.y && point.x < rect.x + rect.w && point.y < rect.y + rect.h) {
    return true;
  }
  else {
    return false;
  }
}

TSPoint getPress() {
  TSPoint _p = ts.getPoint();
  if (_p.z > MINPRESSURE && _p.z < MAXPRESSURE) {
    _p.x = map(_p.x, TS_MINX, TS_MAXX, tft.height(), 0);
    _p.y = map(_p.y, TS_MINY, TS_MAXY, 0, tft.width());
    return TSPoint(_p.y, _p.x, _p.z); //Switch x and y
  }
  else {
    return TSPoint(0, 0, 0);
  }
}

bool isNullRect(MRect rect) {
  return (rect.x < 0);
}

void clearRect(MRect rect) {
  tft.fillRect(rect.x, rect.y, rect.w, rect.h, BG_COLOR);
}

int getButton() {
#if USE_ANALOG_KEYPAD
  int a = analogRead(KEYPAD_PIN);
  if (a < 450) return -1;
  else if (a < 498) return 11;
  else if (a < 516) return 0;
  else if (a < 550) return 10;
  else if (a < 580) return 9;
  else if (a < 620) return 8;
  else if (a < 650) return 7;
  else if (a < 700) return 6;
  else if (a < 750) return 5;
  else if (a < 810) return 4;
  else if (a < 890) return 3;
  else if (a < 980) return 2;
  else return 1;
#else
  char key = keypad.getKey();
  if (key) {
    return ((int)key) % 20;
  }
  else return -1;
#endif
}

float getVar(MVarI varIndex) {
  return vars[varIndex]->getValue();
}

void setVar(MVarI varIndex, float value) {
  vars[varIndex]->setValue(value);
}

DateTime getTimeVar(MTimeVarI timeVarIndex) {
  return timeVars[timeVarIndex]->getValue();
}

void setTimeVar(MTimeVarI timeVarIndex, DateTime _time) {
  timeVars[timeVarIndex]->setValue(_time);
}

bool labelTouched(MLabelI labelIndex) {
  return wasTouched && labels[labelIndex]->isTouched;
}

void setLabelViewState(MLabelI labelIndex, MViewState _viewState) {
  labels[labelIndex]->viewState = _viewState;
}

void setLabelText(MLabelI labelIndex, String _text) {
  labels[labelIndex]->setText(_text);

}

int addressOfVar(int varIndex) {
  return ((int) varIndex) * sizeof(float); // Find memory address of constant
}

String twoDigits(int number) {
  number %= 100;
  String prefix = (number < 10) ? String("0") : String("");
  return prefix + String(number);
}

validDigit(MTimeVarType type, uint8_t digit) {
  uint8_t minDigit = type <= MTVT_DATE_TIME ? 0 : 6;
  uint8_t maxDigit = type < MTVT_DATE_TIME ? 5 : 9;
  if (type == MTVT_FULL_TIME) maxDigit = 11;
  if (digit < minDigit || digit > maxDigit) {
    digit = minDigit;
  }
  return digit;
}

MView::MView() {

}

MRect MView::posForTab(MTabI _tab) {
  if (_tab == tab || tab == MTI_ALL) {
    return pos;
  }
  else if (_tab == MTI_STATUS && !isNullRect(statusPos)) {
    return statusPos;
  }
  else {
    return NULL_RECT;
  }
}

MViewState MView::stateForTab(MTabI _tab) {
  if (_tab == tab || tab == MTI_ALL) {
    if (viewState >= MVS_VISABLE) {
      return viewState;
    }
    else {
      return MVS_VISABLE;
    }
  }
  else if (_tab == MTI_STATUS && !isNullRect(statusPos)) {
    return MVS_VISABLE_STATUS;
  }
  else {
    return MVS_HIDDEN;
  }
}

bool MView::needsUpdate() {
  viewState = stateForTab(selectedTab);
  return (viewState != lastViewState || (viewState > MVS_HIDDEN && valueChanged));
}

bool MView::touched(TSPoint _p) {
  isTouched = (( tab == selectedTab || tab == MTI_ALL ) && pointInRect(_p, pos));
  if (!wasTouched && ( tab == selectedTab || tab == MTI_ALL ) && isTouched) {
    if (viewState == MVS_VISABLE) {
      if (touchedViewState != MVS_EDITING || !isEditing) {
        viewState = touchedViewState;
      }
    }
    else if (viewState > MVS_VISABLE) {
      viewState = MVS_VISABLE;
    }
    return true;
  }
  else {
    return false;
  }
}

MTab::MTab() {

}

MTab::MTab(String _tabName, MTabI _tabIndex) {
  valueChanged = true;
  pos.x = 0;
  pos.y = TAB_HEIGHT * (_tabIndex + 1);
  pos.w = TAB_WIDTH;
  pos.h = TAB_HEIGHT;

  if (_tabIndex == selectedTab) {
    viewState = MVS_VISABLE;
    selected = true;
  }
  else {
    viewState = MVS_HIDDEN;
    selected = false;
  }
  viewState = MVS_VISABLE + selected;
  lastViewState = MVS_HIDDEN;
  tabIndex = _tabIndex;
  tabName = _tabName;
  color = BLACK;
  tab = MTI_ALL;
}

void MTab::show(MPriority priority) {
  if (!needsUpdate()) {
    return;
  }
  if (tabIndex == selectedTab) {
    tft.fillRect(pos.x, pos.y, pos.w, pos.h, BG_COLOR);
    //    tft.drawFastVLine(pos.x + pos.w, pos.y, pos.h, BG_COLOR);
    selected = true;
  }
  else {
    tft.fillRect(pos.x, pos.y, pos.w, pos.h, LINE_COLOR);
    //    tft.drawFastVLine(pos.x + pos.w, pos.y, pos.h, LINE_COLOR);
    selected = false;
  }
  if (lastViewState == MVS_HIDDEN || true) {
    tft.setCursor(6, 14 + pos.y);
    tft.println(tabName);
  }
  valueChanged = false;
  lastViewState = viewState;
}

bool MTab::needsUpdate() {
  if (!valueChanged && (selected == (selectedTab == tabIndex))) {
    return false;
  }
  else {
    valueChanged = true;
    selected = (selectedTab == tabIndex);
    viewState = selected + MVS_VISABLE;
    return true;
  }
}

bool MTab::touched(TSPoint _p) {
  if (pointInRect(_p, pos)) {
    selectedTab = tabIndex;
    return true;
  }
  return false;
}


MVar::MVar() {

}

MVar::MVar(MVarI _varIndex, MTabI _tab, String _unitString, int _decimals, bool _store, int x, int y, MViewState _touchedViewState = MVS_VISABLE, int statusX = -1, int statusY = -2) {
  index = _varIndex;
  tab = _tab;
  unitString = _unitString;
  decimals = _decimals;
  store = _store;
  uint8_t len = 3 + unitString.length() + decimals;
  pos.x = x;
  pos.y = y;
  pos.h = 24;
  pos.w = len * 10 + 8;
  if (statusX != -1) {
    statusPos.x = statusX;
    statusPos.y = statusY;
    statusPos.h = 24;
    statusPos.w = len * 10 + 8;
  } else {
    statusPos = NULL_RECT;
  }
  valueChanged = true;
  viewState = stateForTab(selectedTab);
  lastViewState = MVS_HIDDEN;
  lastTab = MTI_NONE;
  touchedViewState = _touchedViewState;
  if (store) {
    EEPROM.get(addressOfVar(index), value);
  }
  else {
    value = 0;
  }
  tempValue = 0;
}

void MVar::show(MPriority _priority) {
  if (!needsUpdate()) {
    return;
  }
  MRect _p = posForTab(selectedTab);
  int offset = _p.h / 2 - 8;
  float val;
  if (viewState == MVS_EDITING) {
    val = tempValue;
  }
  else {
    val = value;
  }
  if (lastViewState == MVS_VISABLE_STATUS) {
    clearRect(statusPos);
    lastViewState = MVS_HIDDEN;
  }
  else if (lastViewState >= MVS_VISABLE) {
    clearRect(pos);
    lastViewState = MVS_HIDDEN;
  }
  if (viewState > MVS_HIDDEN && _priority < MP_HIGH) {
    tft.setCursor(_p.x + offset, _p.y + offset);
    tft.print(val, decimals);
    tft.print(unitString);
    pos.w = (String(val, decimals).length() + unitString.length()) * 12 - 2 + offset * 2;
    statusPos.w = pos.w;
    if (viewState == MVS_SELECTED) {
      tft.drawRect(pos.x, pos.y, pos.w, pos.h, TEXT_COLOR);
    }
    else if (viewState == MVS_EDITING) {
      tft.drawRect(pos.x, pos.y, pos.w, pos.h, EDIT_COLOR);
    }
    lastViewState = viewState;
    valueChanged = false;
  }
  checkMemory();
}

void MVar::setValue(float _value) {
  if (_value != value) {
    valueChanged = true;
    value = _value;
    if (store) {
      EEPROM.put(addressOfVar(index), value); // Store in memory
    }
  }
  checkMemory();
}

void MVar::updateTemp(int nextDigit) {
  if (nextDigit < 10) {
    tempValue *= 10;
    tempValue += nextDigit / pow(10, decimals);
  }
  else if (nextDigit == 11) {
    setValue(tempValue);
    viewState = MVS_VISABLE;
    tempValue = 0;
  }
  else if (nextDigit == 10) {
    tempValue = 0;
  }
  valueChanged = true;
}

float MVar::getValue() {
  return value;
}

MLabel::MLabel() {

}

MLabel::MLabel(MLabelI _textIndex, MTabI _tab, String _text, int x, int y, MViewState _touchedViewState = MVS_VISABLE, int statusX = -1, int statusY = -2) {
  index = _textIndex;
  text = _text;
  tab = _tab;
  pos.x = x;
  pos.y = y;
  pos.h = 24;
  int len = text.length();
  pos.w = len * 10 + 8;
  if (statusX != -1) {
    statusPos.x = statusX;
    statusPos.y = statusY;
    statusPos.h = 24;
    statusPos.w = len * 12 - 2 + 8;
  } else {
    statusPos = NULL_RECT;
  }
  valueChanged = true;
  viewState = stateForTab(selectedTab);
  lastViewState = MVS_HIDDEN;
  touchedViewState = _touchedViewState;
}

void MLabel::show(MPriority _priority) {
  if (!needsUpdate()) {
    return;
  }
  MRect _p = posForTab(selectedTab);
  int offset = _p.h / 2 - 8;
  if (lastViewState == MVS_VISABLE_STATUS) {
    clearRect(statusPos);
    lastViewState = MVS_HIDDEN;
  }
  else if (lastViewState >= MVS_VISABLE) {
    clearRect(pos);
    lastViewState = MVS_HIDDEN;
  }
  if (viewState > MVS_HIDDEN && _priority < MP_HIGH) {
    tft.setCursor(_p.x + offset, _p.y + offset);
    tft.print(text);
    pos.w = (text.length()) * 12 - 2 + offset * 2;
    statusPos.w = pos.w;
    if (viewState == MVS_SELECTED) {
      tft.drawRect(pos.x, pos.y, pos.w, pos.h, SELECTED_COLOR);
    }
    lastViewState = viewState;
    valueChanged = false;
  }
}

void MLabel::setText(String _text) {
  if (!text.equals(_text)) {
    text = _text;
    valueChanged = true;
  }
}

MSwitch::MSwitch() {

}

MSwitch::MSwitch(MVarI _varIndex, MTabI _tab, String _option0, String _option1, int defaultOption, bool _store, int x, int y) {
  index = _varIndex;
  tab = _tab;
  unitString = "";
  decimals = 0;
  store = _store;
  uint8_t len = _option0.length() + _option1.length();
  option0 = MLabel(MLI_NONE, tab, _option0, x, y, MVS_SELECTED);
  option1 = MLabel(MLI_NONE, tab, _option1, x + option0.pos.w + 4, y, MVS_SELECTED);
  pos.x = x;
  pos.y = y;
  pos.h = 24;
  pos.w = option0.pos.w + option1.pos.w;
  statusPos = NULL_RECT;
  valueChanged = true;
  viewState = stateForTab(selectedTab);
  lastViewState = MVS_HIDDEN;
  lastTab = MTI_NONE;
  touchedViewState = MVS_SELECTED;
  if (store) {
    float storedValue;
    EEPROM.get(addressOfVar(index), storedValue);
    defaultOption = (bool) storedValue;
  }
  value = defaultOption;
  tempValue = 0;
}

void MSwitch::show(MPriority _priority) {
  if (!needsUpdate()) {
    return;
  }
  if (value == 0) {
    option0.viewState = MVS_SELECTED;
    option1.viewState = MVS_VISABLE;
  }
  else {
    option0.viewState = MVS_VISABLE;
    option1.viewState = MVS_SELECTED;
  }
  option0.show(_priority);
  option1.show(_priority);
  pos.w = option0.pos.w + option1.pos.w;
  lastViewState = option0.lastViewState;
  if (_priority < MP_HIGH) {
    valueChanged = false;
  }
}

bool MSwitch::touched(TSPoint _p) {
  if (value == 1 && option0.touched(_p)) {
    option0.viewState = MVS_SELECTED;
    option1.viewState = MVS_VISABLE;
    setValue(0);
  }
  else if (value == 0 && option1.touched(_p)) {
    option0.viewState = MVS_VISABLE;
    option1.viewState = MVS_SELECTED;
    setValue(1);
  }
  return pointInRect(_p, pos);
}

MTimeVar::MTimeVar() {

}

MTimeVar::MTimeVar(MTimeVarI _timeVarIndex, MTabI _tab, MTimeVarType _type, bool _store, int x, int y, MViewState _touchedViewState = MVS_VISABLE, int statusX = -1, int statusY = -2) {
  index = _timeVarIndex;
  tab = _tab;
  store = _store;
  type = _type;
  uint8_t len = 8;
  pos.x = x;
  pos.y = y;
  pos.h = 24;
  pos.w = len * 10 + 8;
  if (statusX != -1) {
    statusPos.x = statusX;
    statusPos.y = statusY;
    statusPos.h = 24;
    statusPos.w = len * 10 + 8;
  } else {
    statusPos = NULL_RECT;
  }
  valueChanged = true;
  viewState = stateForTab(selectedTab);
  lastViewState = MVS_HIDDEN;
  lastTab = MTI_NONE;
  touchedViewState = _touchedViewState;
  if (store) {
    long temp;
    EEPROM.get(1000 + addressOfVar(index) * 8, temp);
    value = DateTime(temp);
  }
  else {
    value = DateTime(0);
  }
  tempValue = DateTime(0);
  activeDigit = validDigit(type, 6);
}

void MTimeVar::show(MPriority _priority) {
  if (!needsUpdate()) {
    return;
  }
  if (!isTime(tempValue) || tempValue.secondstime() < getTime().secondstime()) tempValue = getTime();
  MRect _p = posForTab(selectedTab);
  int offset = _p.h / 2 - 8;
  DateTime val;
  if (viewState == MVS_EDITING) {
    val = tempValue;
  }
  else {
    val = value;
  }
  if (lastViewState == MVS_VISABLE_STATUS) {
    clearRect(statusPos);
    lastViewState = MVS_HIDDEN;
  }
  else if (lastViewState >= MVS_VISABLE) {
    clearRect(pos);
    lastViewState = MVS_HIDDEN;
  }
  if (viewState > MVS_HIDDEN && _priority < MP_HIGH) {
    tft.setCursor(_p.x + offset, _p.y + offset);
    String toPrint = "";
    if (floor(val.year() / 100.0) == 20) {
      if (type <= MTVT_DATE_TIME) {
        toPrint += twoDigits(val.month());
        toPrint += "/";
        toPrint += twoDigits(val.day());
        toPrint += "/";
        toPrint += twoDigits(val.year());
      }
      if (type == MTVT_DATE_TIME) {
        toPrint += " ";
      }
      if (type >= MTVT_DATE_TIME) {
        toPrint += twoDigits(val.hour());
        toPrint += ":";
        toPrint += twoDigits(val.minute());
        if (type == MTVT_FULL_TIME) {
          toPrint += ":";
          toPrint += twoDigits(val.second());
        }
      }
    }
    else {
      toPrint = "Set";
    }
    tft.print(toPrint);
    pos.w = toPrint.length() * 12 - 2 + offset * 2;
    statusPos.w = pos.w;
    if (viewState == MVS_SELECTED) {
      tft.drawRect(pos.x, pos.y, pos.w, pos.h, SELECTED_COLOR);
    }
    else if (viewState == MVS_EDITING) {
      tft.drawFastHLine(pos.x + offset, pos.y + pos.h - 3, pos.w - offset * 2, BG_COLOR);
      int underlineX;
      if (type <= MTVT_DATE_TIME) underlineX = pos.x + offset + (activeDigit + floor((float)activeDigit / 2.0)) * 12;
      else {
        underlineX = pos.x + offset + (activeDigit - 6 + floor((float)(activeDigit - 6) / 2.0)) * 12;
      }
      tft.drawFastHLine(underlineX, pos.y + pos.h - 3, 10, SELECTED_COLOR);
      tft.drawFastHLine(underlineX, pos.y + pos.h - 4, 10, SELECTED_COLOR);
      tft.drawRect(pos.x, pos.y, pos.w, pos.h, EDIT_COLOR);
    }
    lastViewState = viewState;
    valueChanged = false;
  }
  checkMemory();
}

DateTime MTimeVar::getValue() {
  return value;
}

void MTimeVar::setValue(DateTime _time) {
  if (floor(value.secondstime() / 60) != floor(_time.secondstime() / 60) || type == MTVT_FULL_TIME && value.secondstime() != _time.secondstime()) {
    value = _time;
    if (store) {
      EEPROM.put(1000 + addressOfVar(index) * 8, value.unixtime());
    }
    if (viewState != MVS_EDITING) {
      if (isTime(_time)) {
        tempValue = _time;
      }
      else {
        tempValue = getTime();
      }
      valueChanged = true;
    }
  }
  checkMemory();
}

void MTimeVar::updateTemp(int nextDigit) {
  if (!isTime(tempValue) || tempValue.secondstime() < getTime().secondstime()) tempValue = getTime();
  if (nextDigit < 10) {
    tempValue = replaceTimeDigit(tempValue, activeDigit, nextDigit);
    activeDigit = validDigit(type, activeDigit + 1);
  }
  else if (nextDigit == 11) {
    setValue(tempValue);
    viewState = MVS_VISABLE;
    tempValue = value;
    activeDigit = validDigit(type, 6);
  }
  else if (nextDigit == 10) {
    tempValue = value;
    activeDigit = validDigit(type, 6);
  }
  valueChanged = true;
}


