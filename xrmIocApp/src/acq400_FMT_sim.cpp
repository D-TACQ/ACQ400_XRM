/*
 * acq400_FMC_sim.cpp
 *
 *  Created on: 9 Feb 2026
 *      Author: pgm
 */

#include "acq400_asyn_common.h"
#include "acq400_FMT_sim.h"
#include "acq-util.h"
#include <unistd.h>

#include "Multicast.h"


static const char *driverName="acq400_FMT_sim";

#define DN	driverName
#define FN	__FUNCTION__

int acq400_FMT_Sim::nice = ::getenv_default("acq400_FMT_Sim_NICE", 0);

epicsInt64 time_now()
{
	struct timespec ts_now;
	static bool report_complete;
	epicsInt64 _now_us;
	int rc = clock_gettime(CLOCK_REALTIME, &ts_now);
	if (rc != 0){
		perror("clock_gettime");
	}
#ifndef OVERFLOW_NOW_SUCKER
	_now_us = ts_now.tv_sec;
	_now_us = _now_us*1000000 + ts_now.tv_nsec/1000;
	if (_now_us < 0){
		if (!report_complete){
			fprintf(stderr, "NO WAY, JOSE! %lld  %lu %lu\n", _now_us, ts_now.tv_sec, ts_now.tv_nsec);
			report_complete = true;
		}
	}else{
		report_complete = false;
	}
#else
	_now_us = ts_now.tv_sec*1000000 + ts_now.tv_nsec/1000;
#endif

	return _now_us;
}

void acq400_FMT_Sim::update_fmt()
{
	now_us = time_now();

	for (int ii = 0; ii < FMT_ROWS; ++ii){
		fmt[ii].event = 0x1000 + ii;
		fmt[ii].client_data = ii;
		fmt[ii].timestamp = now_us + ii*10;
	}
}

void acq400_FMT_Sim::task_runner(void *drvPvt)
{
	acq400_FMT_Sim *pPvt = (acq400_FMT_Sim *)drvPvt;
	pPvt->task();
}

namespace G {
	const char* fmt_mc_group = "1.2.3.4";
	const int fmt_mc_port = 6789;
}

acq400_FMT_Sim::acq400_FMT_Sim(const char* portName):
		asynPortDriver(portName,
		/* maxAddr */		FMT_ROWS,    /* nchan from 0 */
		/* Interface mask */    asynEnumMask|asynOctetMask|asynInt32Mask|asynFloat64Mask|asynInt16ArrayMask|asynInt32ArrayMask|asynFloat32ArrayMask|asynInt64Mask|asynDrvUserMask,
		/* Interrupt mask */	asynEnumMask|asynOctetMask|asynInt32Mask|asynFloat64Mask|asynInt16ArrayMask|asynInt32ArrayMask|asynFloat32ArrayMask|asynInt64Mask,
		/* asynFlags no block*/ 0,
		/* Autoconnect */       1,
		/* Default priority */  0,
		/* Default stack size*/ 0),
		update(0)
{
	asynStatus status = asynSuccess;

	createParam(PS_UPDATES,  asynParamInt32,        &P_UPDATES);
	createParam(PS_TS_USEC,  asynParamInt64,	&P_TS_USEC);
	createParam(PS_FMT_MC_GRP,  asynParamOctet,	&P_FMT_MC_GRP);
	createParam(PS_FMT_MC_PORT,  asynParamInt32,	&P_FMT_MC_PORT);

	setStringParam(P_FMT_MC_GRP, G::fmt_mc_group);
	setIntegerParam(P_FMT_MC_PORT, G::fmt_mc_port);

	/* Create the thread that computes the waveforms in the background */
	status = (asynStatus)(epicsThreadCreate("FMT_simTask",
			epicsThreadPriorityHigh - nice,
			epicsThreadGetStackSize(epicsThreadStackMedium),
			(EPICSTHREADFUNC)task_runner,
			this) == NULL);
	if (status) {
		printf("%s:%s: epicsThreadCreate failure\n", DN, FN);
		return;
	}

}

void acq400_FMT_Sim::task(void) {
	asynStatus status = asynSuccess;
	update_fmt();
	char mc_group[80];
	int mc_port;

	status = getStringParam(P_FMT_MC_GRP, 80, mc_group);
	if (status){
		fprintf(stderr, "%s:%s getStringParam P_FMT_MC_GRP fail\n", DN, FN);
		return;
	}
	status = getIntegerParam(P_FMT_MC_PORT, &mc_port);
	if (status){
		fprintf(stderr, "%s:%s getIntegerParam P_FMT_MC_PORT fail\n", DN, FN);
		return;
	}

	fprintf(stderr, "%s:%s mc_group \"%s\" mc_port %d\n", DN, FN, mc_group, mc_port);

	MultiCast& multicast = MultiCast::factory(
			mc_group, mc_port, MultiCast::MC_SENDER);

	/* fake event loop to start */
	while(1){
		update_fmt();
		//multicast.sendto(fmt, sizeof(fmt));
		updateTimeStamp();
		setIntegerParam(P_UPDATES, ++update);
		setInteger64Param(P_TS_USEC, now_us);
		callParamCallbacks();
		usleep(50000);

	}
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




