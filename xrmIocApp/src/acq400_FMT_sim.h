/*
 * acq400_FMT_sim.h
 *
 *  Created on: 9 Feb 2026
 *      Author: pgm
 */

#ifndef XRMIOCAPP_SRC_ACQ400_FMT_SIM_H_
#define XRMIOCAPP_SRC_ACQ400_FMT_SIM_H_


#include "asynPortDriver.h"
#include "xrm_structs.h"

#define PS_UPDATES	"UPDATES"	/* asynInt32, r/c */
#define PS_TS_USEC	"TS_USEC"       /* asynInt32, ro  */

class acq400_FMT_Sim: public asynPortDriver {
	FMT fmt;
	void update_fmt();
	static void task_runner(void *drvPvt);

	static int nice;

protected:
	virtual void task();

	unsigned update;
	epicsInt64 now_us;

	int P_UPDATES;
	int P_TS_USEC;
public:
	acq400_FMT_Sim(const char* portName);

};



#endif /* XRMIOCAPP_SRC_ACQ400_FMT_SIM_H_ */
