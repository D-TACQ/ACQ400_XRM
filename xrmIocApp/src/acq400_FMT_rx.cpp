/*
 * acq400_FMT_rx.cpp
 *
 *  Created on: 18 Feb 2026
 *      Author: pgm
 */

#include "acq400_asyn_common.h"
#include "acq400_FMT_rx.h"
#include "Multicast.h"

static const char *driverName="acq400_FMT_rx";

#define DN	driverName
#define FN	__FUNCTION__

acq400_FMT_rx::acq400_FMT_rx(const char* portName) :
		acq400_FMT_abc(portName,
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
		/* Default stack size*/ 0),
		packet_count(0),
		ts(0)
{
	asynStatus status = asynSuccess;

	/* Create the thread that computes the waveforms in the background */
	status = (asynStatus)(epicsThreadCreate("FMT_rxTask",
			epicsThreadPriorityHigh - nice,
			epicsThreadGetStackSize(epicsThreadStackMedium),
			(EPICSTHREADFUNC)task_runner,
			this) == NULL);
	if (status) {
		printf("%s:%s: epicsThreadCreate failure\n", DN, FN);
		return;
	}
}

void acq400_FMT_rx::update_fmt(bool first_time)
{
	++packet_count;
	fmt[4].pad = packet_count>>16;
	fmt[5].pad = packet_count&0x0ffff;
	if (!first_time){
		epicsInt64 dt = fmt[0].timestamp - ts;
		fmt[6].pad = dt>>48;
		fmt[7].pad = dt>>32;
		fmt[8].pad = dt>>16;
		fmt[9].pad = dt;            // BIG truncation, max 65536 us
		fmt[10].pad = dt/1000;	    // ms, 65s rollover
	}
}

void acq400_FMT_rx::process_fmt(bool first_time)
{

}

void acq400_FMT_rx::task(void) {
	asynStatus status = asynSuccess;
	bool first_time = true;

	epicsEventWait(eventId);

	MultiCast& multicast = acq400_FMT_abc::mc_factory(MultiCast::MC_RECEIVER);

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
			multicast.recvfrom(fmt, sizeof(fmt));
			update_fmt(first_time);
			process_fmt(first_time);
			update_fmt_columns();
			lock();
			updateTimeStamp();
			update_fmt_callbacks();
			unlock();
			first_time = false;
		}else{
			usleep(50000);
		}
	}
}

asynStatus acq400_FMT_rx::writeInt32(asynUser *pasynUser, epicsInt32 value)
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


extern "C" {

	/** EPICS iocsh callable function to call constructor for the testAsynPortDriver class.
	  * \param[in] portName The name of the asyn port driver to be created.
	  */
	int acq400_FMT_rxConfigure(const char *portName)
	{
		new acq400_FMT_rx(portName);
		return 0;
	}

	/* EPICS iocsh shell commands */

	static const iocshArg initArg0 = { "port", iocshArgString };
	static const iocshArg * const initArgs[] = { &initArg0 };
	static const iocshFuncDef initFuncDef = { "acq400_FMT_rxConfigure", 1, initArgs };
	static void initCallFunc(const iocshArgBuf *args)
	{
		acq400_FMT_rxConfigure(args[0].sval);
	}

	void acq400_FMT_rxRegister(void)
	{
	    iocshRegister(&initFuncDef, initCallFunc);
	}

	epicsExportRegistrar(acq400_FMT_rxRegister);
}


