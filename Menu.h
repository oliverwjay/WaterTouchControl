#ifndef MENU
#define MENU

#include <Arduino.h>
#include <Adafruit_GFX.h>    // Core graphics library
//#include <Adafruit_TFTLCD.h> // Hardware-specific library
//#include <pin_magic.h>
//#include <registers.h>
#include "TouchScreen.h"
#include "RTClib.h"

#define N_TABS 8
enum MTabI:uint8_t {
  MTI_STATUS,
  MTI_FLOW,
  MTI_POWER,
  MTI_WOOL_FEED,
  MTI_DWELL_TANK,
  MTI_MONITORING,
  MTI_SCHEDULE,
  MTI_ALL,
  MTI_NONE
};

#define N_VARS 25
enum MVarI:uint8_t {
  MVI_STATUS_SWITCH,
  MVI_FLOW_RATE,
  MVI_POWER_SWITCH,
  MVI_P_SET_VOLTAGE,
  MVI_P_SET_CURRENT,
  MVI_P_REFERENCE_V,
  MVI_P_M_VOLTAGE,
  MVI_P_M_CURRENT,
  MVI_DT_MAX_VOL,
  MVI_DT_MIN_VOL,
  MVI_DT_ESTOP_VOL,
  MVI_DT_CURRENT_VOL,
  MVI_DT_FILL_TIME,
  MVI_DT_DRAIN_TIME,
  MVI_WF_PERIOD,
  MVI_WF_PWR,
  MVI_WF_REV_TIME,
  MVI_WF_SWITCH,
  MVI_WF_ANALOG_IN,
  MVI_WF_THRESHOLD,
  MVI_M_PH,
  MVI_M_LOGGING_SWITCH,
  MVI_WOOL_DELAY,
  MVI_WARNING_PERIOD,
  MVI_WF_CAUSE_SHUTDOWN,
  MVI_NONE
};

#define N_LABELS 30
enum MLabelI:uint8_t {
  MLI_STATUS_WARNING,
  MLI_STATUS_STATE,
  MLI_P_MEASURED,
  MLI_P_REFERENCE,
  MLI_P_SET,
  MLI_DT_0,
  MLI_DT_1,
  MLI_DT_2,
  MLI_DT_3,
  MLI_DT_4,
  MLI_DT_5,
  MLI_M_PH,
  MLI_M_168,
  MLI_M_4,
  MLI_M_7,
  MLI_M_10,
  MLI_M_CLEAR,
  MLI_M_LOG,
  MLI_WF_FWD,
  MLI_WF_REV,
  MLI_WF_POWER,
  MLI_WF_PERIOD,
  MLI_WF_REV_TIME,
  MLI_WF_SWITCH,
  MLI_WF_CAUSE_SHUTDOWN,
  MLI_S_SHUTDOWN,
  MLI_S_STARTUP,
  MLI_S_STATUS,
  MLI_S_WOOL_DELAY,
  MLI_WARNING_PERIOD,
  MLI_NONE
};

#define N_TIME_VARS 4
enum MTimeVarI:uint8_t {
  MTVI_CURRENT_TIME,
  MTVI_STATE_TIME,
  MTVI_S_SHUTDOWN_TIME,
  MTVI_S_STARTUP_TIME,
  MTVI_NONE
};

enum MColor { // Assign human-readable names to some common 16-bit color values:
  BLACK   = 0x0001,
  BLUE    = 0x001F,
  RED     = 0xF800,
  GREEN   = 0x07E0,
  CYAN    = 0x07FF,
  MAGENTA = 0xF81F,
  YELLOW  = 0xFFE0,
  WHITE   = 0xFFFF,
  GRAY    = 0x7BEF,
  ORANGE  = 0b1111110001000000,
  LIGHT_GRAY = 0b1100011000011000
};

/* Dark Theme */
//#define BG_COLOR BLACK
//#define TEXT_COLOR WHITE
//#define SELECTED_COLOR WHITE
//#define EDIT_COLOR BLUE
//#define LINE_COLOR GRAY

/* Gray Theme */
//#define BG_COLOR GRAY
//#define TEXT_COLOR WHITE
//#define SELECTED_COLOR WHITE
//#define EDIT_COLOR BLUE
//#define LINE_COLOR BLACK

/* Dark GRAY Theme */
//#define BG_COLOR GRAY
//#define TEXT_COLOR WHITE
//#define SELECTED_COLOR WHITE
//#define EDIT_COLOR BLUE
//#define LINE_COLOR BLACK

/* Hacker Theme */
#define BG_COLOR BLACK
#define TEXT_COLOR GREEN
#define SELECTED_COLOR WHITE
#define EDIT_COLOR RED
#define LINE_COLOR GRAY

/* Light Theme */
//#define BG_COLOR WHITE
//#define TEXT_COLOR BLACK
//#define SELECTED_COLOR GRAY
//#define EDIT_COLOR RED
//#define LINE_COLOR LIGHT_GRAY


enum MViewState:uint8_t {
  MVS_HIDDEN,
  MVS_VISABLE_STATUS,
  MVS_VISABLE,
  MVS_SELECTED,
  MVS_EDITING
};

enum MPriority:uint8_t {
  MP_LOW,
  MP_HIGH,
  MP_EXTREME
};

enum MTimeVarType:uint8_t {
  MTVT_DATE,
  MTVT_DATE_TIME,
  MTVT_SHORT_TIME,
  MTVT_FULL_TIME
};

struct MRect {
  uint16_t x;
  uint16_t y;
  uint16_t w;
  uint16_t h;
};

void initMenu();
void updateMenu(int endTime);
void updateTouch();
bool pointInRect(TSPoint point, MRect rect);
bool isNullRect(MRect rect);
void clearRect(MRect rect);
float getVar(MVarI varIndex);
void setVar(MVarI varIndex, float value);
DateTime getTimeVar(MTimeVarI timeVarIndex);
void setTimeVar(MTimeVarI timeVarIndex, DateTime _time);
bool labelTouched(MLabelI labelIndex);
void setLabelViewState(MLabelI labelIndex, MViewState _viewState);
void setLabelText(MLabelI labelIndex, String _text);
MTabI currentTab();
int getButton();
TSPoint getPress();

class MView {
  public:
    MView();
    bool needsUpdate();
    virtual void show(MPriority _priority);
    virtual bool touched(TSPoint _p);
    MRect posForTab(MTabI _tab);
    MViewState stateForTab(MTabI _tab);
    uint8_t index;
    MTabI tab;
    bool valueChanged;
    bool isTouched;
    MRect pos;
    MRect statusPos;
    MViewState viewState;
    MViewState touchedViewState;
    MViewState lastViewState;
};

class MTab: public MView {
  public:
    MTab();
    MTab(String _tabName, MTabI _tabIndex);
    bool needsUpdate();
    void show(MPriority _priority);
    bool touched(TSPoint _p);
    MTabI tabIndex;
    String tabName;
    bool selected;
    MColor color;
};

class MVar: public MView {
  public:
    MVar();
    MVar(MVarI _varIndex, MTabI _tab, String _unitString, int _decimals, bool _store, int x, int y, MViewState _touchedViewState = MVS_VISABLE, int statusX=-1, int statusY = -2);
    virtual void show(MPriority _priority);
    void setValue(float _value);
    float getValue();
    virtual void updateTemp(int nextDigit);
    String unitString;
    uint8_t decimals;
    bool store;
    MTabI lastTab;
    float value;
    float tempValue;
};

class MLabel: public MView {
public:
  MLabel();
  MLabel(MLabelI _textIndex, MTabI _tab, String _text, int x, int y, MViewState _touchedViewState = MVS_VISABLE, int statusX=-1, int statusY = -2);
  void show(MPriority _priority);
  void setText(String _text);
  String text;
};

class MSwitch: public MVar{
public:
  MSwitch();
  MSwitch(MVarI _varIndex, MTabI _tab, String _option1, String _option2, int defaultOption, bool _store, int x, int y);
  void show(MPriority _pritority);
  bool touched(TSPoint _p);
  MLabel option0;
  MLabel option1;
};

class MTimeVar: public MVar{
public:
  MTimeVar();
  MTimeVar(MTimeVarI _timeVarIndex, MTabI _tab, MTimeVarType _type, bool _store, int x, int y, MViewState _touchedViewState = MVS_VISABLE, int statusX = -1, int statusY = -2);
  void show(MPriority _priority);
  DateTime getValue();
  void setValue(DateTime _time);
  void updateTemp(int nextDigit);
  DateTime value;
  DateTime tempValue;
  MTimeVarType type;
  uint8_t activeDigit;
};

#endif


