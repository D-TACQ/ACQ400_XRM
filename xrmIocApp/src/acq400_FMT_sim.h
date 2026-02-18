/*
 * acq400_FMT_sim.h
 *
 *  Created on: 9 Feb 2026
 *      Author: pgm
 */

#ifndef XRMIOCAPP_SRC_ACQ400_FMT_SIM_H_
#define XRMIOCAPP_SRC_ACQ400_FMT_SIM_H_

#include "acq400_FMT.h"

/* REDIT : Row EDIT */
#define PS_FMT_REDIT_ROW	"FMT_REDIT_ROW" 	/* edit this row */
#define PS_FMT_REDIT_ROWCOUNT	"FMT_REDIT_ROWCOUNT"	/* for this many rows */
#define PS_FMT_REDIT_EVENT      "FMT_REDIT_EVENT"       /* set this event */
#define PS_FMT_REDIT_EVENT_STEP "FMT_REDIT_EVENT_STEP"  /* adding to subsequent rows */
#define PS_FMT_REDIT_CLIDAT	"FMT_REDIT_CLIDAT"
#define PS_FMT_REDIT_CLIDAT_STEP "FMT_REDIT_CLIDAT_STEP"

#define PS_FMT_REDIT_COMMIT 	"FMT_REDIT_COMMIT"

struct TimeProvider {
	virtual epicsInt64 time_now() = 0; /* time in usec since epoch. Blocks until time available */
};


class acq400_FMT_Sim: public acq400_FMT_abc {
	virtual void update_fmt(bool first_time = false);

	asynStatus gip(int pnum, int* pram);
	void redit();  /* Row EDIT */

	TimeProvider& timeProvider;
protected:
	virtual void task();

	int P_FMT_REDIT_ROW;
	int P_FMT_REDIT_ROWCOUNT;
	int P_FMT_REDIT_EVENT;
	int P_FMT_REDIT_EVENT_STEP;
	int P_FMT_REDIT_CLIDAT;
	int P_FMT_REDIT_CLIDAT_STEP;
	int P_FMT_REDIT_COMMIT;

public:
	acq400_FMT_Sim(const char* portName, TimeProvider& _timeProvider);
	virtual ~acq400_FMT_Sim() {}

	asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

};


#endif /* XRMIOCAPP_SRC_ACQ400_FMT_SIM_H_ */
