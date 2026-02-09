/*
 * acq400_FMC_sim.cpp
 *
 *  Created on: 9 Feb 2026
 *      Author: pgm
 */

#include "acq400_asyn_common.h"
#include "acq400_FMT_sim.h"
#include "acq-util.h"



static const char *driverName="acq400_FMT_sim";

int acq400_FMT_Sim::nice = ::getenv_default("acq400_FMT_Sim_NICE", 0);

epicsUInt64 time_now()
{
	struct timespec ts_now;
	epicsUInt64 us_now;
	int rc = clock_gettime(CLOCK_REALTIME, &ts_now);
	if (rc != 0){
		perror("clock_gettime");
	}
	us_now = ts_now.tv_sec*1000000 + ts_now.tv_nsec/1000;
	return us_now;
}

void acq400_FMT_Sim::update_fmt()
{
	epicsUInt64 now = time_now();

	for (int ii = 0; ii < FMT_ROWS; ++ii){
		fmt[ii].event = 0x1000 + ii;
		fmt[ii].client_data = ii;
		fmt[ii].timestamp = now + ii*10;
	}
}

void acq400_FMT_Sim::task_runner(void *drvPvt)
{
	acq400_FMT_Sim *pPvt = (acq400_FMT_Sim *)drvPvt;
	pPvt->task();
}

acq400_FMT_Sim::acq400_FMT_Sim(const char* portName):
		asynPortDriver(portName,
		/* maxAddr */		FMT_ROWS,    /* nchan from 0 */
		/* Interface mask */    asynEnumMask|asynInt32Mask|asynFloat64Mask|asynInt16ArrayMask|asynInt32ArrayMask|asynFloat32ArrayMask|asynDrvUserMask,
		/* Interrupt mask */	asynEnumMask|asynInt32Mask|asynFloat64Mask|asynInt16ArrayMask|asynInt32ArrayMask|asynFloat32ArrayMask,
		/* asynFlags no block*/ 0,
		/* Autoconnect */       1,
		/* Default priority */  0,
		/* Default stack size*/ 0)
{
	asynStatus status = asynSuccess;


	/* Create the thread that computes the waveforms in the background */
	status = (asynStatus)(epicsThreadCreate("FMT_simTask",
			epicsThreadPriorityHigh - nice,
			epicsThreadGetStackSize(epicsThreadStackMedium),
			(EPICSTHREADFUNC)task_runner,
			this) == NULL);
	if (status) {
		printf("%s:%s: epicsThreadCreate failure\n", driverName, __FUNCTION__);
		return;
	}
}

void acq400_FMT_Sim::task(void) {
	update_fmt();
}

extern "C" {

	/** EPICS iocsh callable function to call constructor for the testAsynPortDriver class.
	  * \param[in] portName The name of the asyn port driver to be created.
	  */
	int acq400_FMT_SimConfigure(const char *portName)
	{
		//return MultiChannelScope::factory(portName, nchan, maxPoints, data_size);
		printf("pgmwashere R1000\n");
		printf("%s: %s %s\n", __FUNCTION__, driverName, portName);

		new acq400_FMT_Sim(portName);
		return 0;
	}

	/* EPICS iocsh shell commands */

	static const iocshArg initArg0 = { "port", iocshArgString };
	static const iocshArg * const initArgs[] = { &initArg0 };
	static const iocshFuncDef initFuncDef = { "acq400_FMT_SimConfigure", 1, initArgs };
	static void initCallFunc(const iocshArgBuf *args)
	{
		acq400_FMT_SimConfigure(args[0].sval);
	}

	void acq400_FMT_simRegister(void)
	{
	    iocshRegister(&initFuncDef, initCallFunc);
	}

	epicsExportRegistrar(acq400_FMT_simRegister);
}




