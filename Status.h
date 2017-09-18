#ifndef STATUS
#define STATUS
#include <Arduino.h>

enum StatusState{
	SS_OFF,
	SS_SHUTDOWN_DRAIN_ONLY,
	SS_SHUTDOWN_FLOW_ONLY,
	SS_STARTUP_FLOW_ENABLE,
	SS_STARTUP_PS_ENABLE,
	SS_STARTUP_PS_RELAY,
	SS_RUNNING
};

static const char *statusStateName[] = {"Disabled", "Stopped flow", "Cut power", "Started flow", "Starting Power", "Power on", "Running", "WARNING!"};

enum SErrorI{
	SE_DT_NO_FLOW,
	SE_DT_OVERFLOW,
	SE_DT_BAD_READING,
	SE_NO_WOOL,
	SE_N_ERRORS
};

static const char *errorText[] = {"No flow ", "Overflow ", "Fix sonar ", "No wool "};

enum SErrorState:uint8_t{
	SES_NORMAL,
	SES_ERROR,
  SES_TEXT_ONLY,
	SES_IGNORE
};

void initStatus();
void updateStatus();
void setStatusState(StatusState state);
StatusState getStatusState();
void updateError(SErrorI error, bool isError);
void updateError(SErrorI error, SErrorState isError);

#endif
