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

class acq400_FMT_Sim: public asynPortDriver {
	FMT fmt;
	void update_fmt();
	static void task_runner(void *drvPvt);

	static int nice;

protected:
	virtual void task();
public:
	acq400_FMT_Sim(const char* portName);
};



#endif /* XRMIOCAPP_SRC_ACQ400_FMT_SIM_H_ */
