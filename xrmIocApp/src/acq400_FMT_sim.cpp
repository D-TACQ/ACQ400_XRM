/*
 * acq400_FMT_sim.cpp
 *
 *  Created on: 9 Feb 2026
 *      Author: pgm
 */

#include "acq400_asyn_common.h"
#include "acq400_FMT_sim.h"
#include "acq-util.h"
#include <unistd.h>
#include <string.h>

#include "Multicast.h"



static const char *driverName="acq400_FMT_sim";

#define DN	driverName
#define FN	__FUNCTION__

int acq400_FMT_abstract::nice = ::getenv_default("acq400_FMT_NICE", 0);

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

void acq400_FMT_Sim::update_fmt(bool first_time)
{
	if (first_time){
		assert(FMT_ROWS < 0xffU);
		for (epicsInt8 row = 0; row < FMT_ROWS; ++row){
			cols.c_rownum[row] = row;
			fmt[row].event = 0x100 + row;
			fmt[row].client_data = row;
		}
	}
	now_us = time_now();

	for (int row = 0; row < FMT_ROWS; ++row){
		fmt[row].timestamp = now_us + row*10;
	}
}

acq400_FMT_Sim::acq400_FMT_Sim(const char* portName):
	acq400_FMT_abstract(portName,
	/* maxAddr */		FMT_ROWS,    /* nchan from 0 */
	/* Interface mask */    asynEnumMask|asynOctetMask|asynInt32Mask|asynInt64Mask|asynFloat64Mask|
					asynInt8ArrayMask|asynInt16ArrayMask|asynInt32ArrayMask|
					asynFloat32ArrayMask|asynInt64ArrayMask|asynDrvUserMask,
	/* Interrupt mask */	asynEnumMask|asynOctetMask|asynInt32Mask|asynInt64Mask|asynFloat64Mask|
					asynInt8ArrayMask|asynInt16ArrayMask|asynInt32ArrayMask|
					asynFloat32ArrayMask|asynInt64ArrayMask,
	/* asynFlags no block*/ 0,
	/* Autoconnect */       1,
	/* Default priority */  0,
	/* Default stack size*/ 0)
{
	asynStatus status = asynSuccess;
	memset(fmt, 0, sizeof(fmt));
	update_fmt(true);

	createParam(PS_FMT_REDIT_ROW, 		asynParamInt32, &P_FMT_REDIT_ROW);
	createParam(PS_FMT_REDIT_ROWCOUNT, 	asynParamInt32, &P_FMT_REDIT_ROWCOUNT);
	createParam(PS_FMT_REDIT_EVENT,		asynParamInt32, &P_FMT_REDIT_EVENT);
	createParam(PS_FMT_REDIT_EVENT_STEP, 	asynParamInt32, &P_FMT_REDIT_EVENT_STEP);
	createParam(PS_FMT_REDIT_CLIDAT, 	asynParamInt32, &P_FMT_REDIT_CLIDAT);
	createParam(PS_FMT_REDIT_CLIDAT_STEP, 	asynParamInt32, &P_FMT_REDIT_CLIDAT_STEP);
	createParam(PS_FMT_REDIT_COMMIT,	asynParamInt32, &P_FMT_REDIT_COMMIT);


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

	epicsEventWait(eventId);

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
		int runstop;
		lock();
		status = getIntegerParam(P_RUNSTOP, &runstop);
		if (status){
			fprintf(stderr, "%s:%s getIntegerParam P_FMT_MC_PORT fail\n", DN, FN);
			return;
		}
		unlock();
		if (runstop == 1){
			update_fmt();
			multicast.sendto(fmt, sizeof(fmt));
			update_fmt_columns();
			lock();
			updateTimeStamp();
			setIntegerParam(P_UPDATES, ++update);
			setInteger64Param(P_TS_USEC, now_us);
			callParamCallbacks();
			doCallbacksInt8Array(cols.c_rownum, FMT_ROWS, P_FMT_COL_ROWNUM, 0);
			doCallbacksInt16Array(cols.c_event, FMT_ROWS, P_FMT_COL_EVENT, 0);
			doCallbacksInt16Array(cols.c_pad, FMT_ROWS, P_FMT_COL_PAD, 0);
			doCallbacksInt32Array(cols.c_client_data, FMT_ROWS, P_FMT_COL_CLIDAT, 0);
			doCallbacksInt64Array(cols.c_timestamp, FMT_ROWS, P_FMT_COL_TS, 0);
			unlock();
		}
		usleep(50000);
	}
}

asynStatus acq400_FMT_Sim::gip(int pnum, int* pram)
{
	asynStatus status = getIntegerParam(pnum, pram);
	if (status){
		fprintf(stderr, "%s:%s getIntegerParam %d fail\n",
				DN, FN, pnum);
	}
	return status;
}

void acq400_FMT_Sim::redit()
{
	int row, row_count, event, event_step, clidat, clidat_step;
	asynStatus status;

	fprintf(stderr, "%d %d %d %d %d %d\n",
			P_FMT_REDIT_ROW,
			P_FMT_REDIT_ROWCOUNT,
			P_FMT_REDIT_EVENT,
			P_FMT_REDIT_EVENT_STEP,
			P_FMT_REDIT_CLIDAT,
			P_FMT_REDIT_CLIDAT_STEP);

	if (gip(P_FMT_REDIT_ROW, 	&row)		||
	    gip(P_FMT_REDIT_ROWCOUNT, 	&row_count)	||
	    gip(P_FMT_REDIT_EVENT,    	&event)		||
	    gip(P_FMT_REDIT_EVENT_STEP, &event_step)	||
	    gip(P_FMT_REDIT_CLIDAT,     &clidat)	||
	    gip(P_FMT_REDIT_CLIDAT_STEP,&clidat_step)		){
		return;
	}

	fprintf(stderr, "%s:%s %d,%d %d,%d %d,%d\n", DN, FN,
			row, row_count, event, event_step, clidat, clidat_step);

	lock();
	for (int rn = 0; rn < row_count; ++rn){
		struct FMT_ROW& this_row = fmt[row+rn];
		this_row.event = event + rn*event_step;
		this_row.client_data = clidat + rn*clidat_step;
		this_row.timestamp = 0;
	}
	unlock();
}
asynStatus acq400_FMT_Sim::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
	    int function = pasynUser->reason;
	    asynStatus status = asynSuccess;
	    const char *paramName;

	    /* Set the parameter in the parameter library. */
	    status = (asynStatus) setIntegerParam(function, value);

	    /* Fetch the parameter string name for possible use in debugging */
	    getParamName(function, &paramName);

	    if (function == P_RUNSTOP) {
	        if (value) epicsEventSignal(eventId);
	    }else if (function == P_FMT_REDIT_COMMIT){
		    redit();
	    }

	    /* Do callbacks so higher layers see any changes */
	    status = (asynStatus) callParamCallbacks();

	    if (status)
	        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
	                  "%s:%s: status=%d, function=%d, name=%s, value=%d",
	                  DN, FN, status, function, paramName, value);
	    else
	        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
	              "%s:%s: function=%d, name=%s, value=%d\n",
	              DN, FN, function, paramName, value);
	    return status;
}

void acq400_FMT_Sim::task_runner(void *drvPvt)
{
	acq400_FMT_Sim *pPvt = (acq400_FMT_Sim *)drvPvt;
	pPvt->task();
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




