/*
 * acq400_SOE.cpp
 *
 *  Created on: 23 Feb 2026
 *      Author: pgm
 */

#include "acq400_asyn_common.h"
#include "acq400_SOE.h"
#include "acq-util.h"
#include <unistd.h>
#include <string.h>

static const char *driverName="acq400_SOE";

#define DN	driverName
#define FN	__FUNCTION__


int acq400_SOE::nice = ::getenv_default("acq400_SOE_NICE", 0);

acq400_SOE::acq400_SOE(const char* portName):
	asynPortDriver(portName,
	/* maxAddr */		SOE_HOLD_HEADER_ROWS,    /* nchan from 0 */
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
	update(0)
{
	asynStatus status = asynSuccess;
	memset(soe_lut, 0, sizeof(soe_lut));

	assert(SOE_LUT_ROWS < 0xffU);
	for (epicsInt8 row = 0; row < SOE_LUT_ROWS; ++row){
		cols.c_rownum[row] = row;

		cols.c_event[row] = 1000+row;
		cols.c_pv_id[row] = 2000+row;
		cols.c_offset_us[row] = row*2;
	}


	eventId = epicsEventCreate(epicsEventEmpty);

	createParam(PS_RUNSTOP,  asynParamInt32,        &P_RUNSTOP);
	createParam(PS_UPDATES,  asynParamInt32,        &P_UPDATES);
	createParam(PS_TS_USEC,  asynParamInt64,	&P_TS_USEC);

	createParam(PS_SOE_LUT_COL_ROWNUM,	asynParamInt8Array,  &P_SOE_LUT_COL_ROWNUM);
	createParam(PS_SOE_LUT_COL_EVENT,	asynParamInt16Array, &P_SOE_LUT_COL_EVENT);
	createParam(PS_SOE_LUT_COL_PAD,		asynParamInt16Array, &P_SOE_LUT_COL_PAD);
	createParam(PS_SOE_LUT_COL_PV_ID,	asynParamInt32Array, &P_SOE_LUT_COL_PV_ID);
	createParam(PS_SOE_LUT_COL_OFFSET_US,	asynParamInt32Array, &P_SOE_LUT_COL_OFFSET_US);

	createParam(PS_SOE_HHR_COL_ROWNUM,	asynParamInt8Array,  &P_SOE_HHR_COL_ROWNUM);
	createParam(PS_SOE_HHR_COL_CLIDAT,    	asynParamInt16Array, &P_SOE_HHR_COL_CLIDAT);
	createParam(PS_SOE_HHR_COL_TS,    	asynParamInt64Array, &P_SOE_HHR_COL_TS);
	createParam(PS_SOE_HHR_COL_DATA_OFFSET, asynParamInt32,      &P_SOE_HHR_COL_DATA_OFFSET);
	createParam(PS_SOE_HHR_COL_SS_U32,	asynParamOctet,      &P_SOE_HHR_COL_SS_U32);
	createParam(PS_SOE_HHR_COL_AI_COUNT,	asynParamOctet,	     &P_SOE_HHR_COL_AI_COUNT);
	createParam(PS_SOE_HHR_COL_DI_COUNT,	asynParamOctet,	     &P_SOE_HHR_COL_DI_COUNT);
	createParam(PS_SOE_HHR_COL_SP_COUNT,	asynParamOctet,	     &P_SOE_HHR_COL_SP_COUNT);

	/* Create the thread that computes the waveforms in the background */
	status = (asynStatus)(epicsThreadCreate("SOE_task",
			epicsThreadPriorityHigh - nice,
			epicsThreadGetStackSize(epicsThreadStackMedium),
			(EPICSTHREADFUNC)task_runner,
			this) == NULL);
	if (status) {
		printf("%s:%s: epicsThreadCreate failure\n", DN, FN);
		return;
	}
}

void acq400_SOE::task_runner(void *drvPvt)
{
	acq400_SOE *pPvt = (acq400_SOE *)drvPvt;
	pPvt->task();
}

void acq400_SOE::update_soe_lut(bool first_time)
{
	;
}
void acq400_SOE::update_soe_lut_columns(void)
{
	for (int row = 0; row < SOE_LUT_ROWS; ++row){
		cols.c_event[row]= soe_lut[row].event;
		cols.c_pad[row] = soe_lut[row].pad;
		cols.c_pv_id[row] = soe_lut[row].pv_id;
		cols.c_offset_us[row] = soe_lut[row].offset_us;
	};
}
void acq400_SOE::update_soe_lut_callbacks(void)
{
	setIntegerParam(P_UPDATES, ++update);
	//setInteger64Param(P_TS_USEC, now_us);

	callParamCallbacks();
	doCallbacksInt8Array(cols.c_rownum, FMT_ROWS, P_SOE_LUT_COL_ROWNUM, 0);
	doCallbacksInt16Array(cols.c_event, FMT_ROWS, P_SOE_LUT_COL_EVENT, 0);
	doCallbacksInt16Array(cols.c_pad, FMT_ROWS, P_SOE_LUT_COL_PAD, 0);
	doCallbacksInt32Array(cols.c_pv_id, FMT_ROWS, P_SOE_LUT_COL_PV_ID, 0);
	doCallbacksInt64Array(cols.c_offset_us, FMT_ROWS, P_SOE_LUT_COL_OFFSET_US, 0);
}

void acq400_SOE::task()
{
	asynStatus status = asynSuccess;
	epicsEventWait(eventId);

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
			/** @@todo SOE_LUT doesn't really update periodically, make it PASV, for now, period is good */
			update_soe_lut_columns();
			lock();
			update_soe_lut_callbacks();
			unlock();
		}
		usleep(50000);
	}
}

asynStatus acq400_SOE::writeInt32(asynUser *pasynUser, epicsInt32 value)
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
	int acq400_SOE_Configure(const char *portName)
	{
		printf("%s:%s R1001 %s\n", DN, FN, portName);

		new acq400_SOE(portName);
		return 0;
	}

	/* EPICS iocsh shell commands */

	static const iocshArg initArg0 = { "port", iocshArgString };
	static const iocshArg * const initArgs[] = { &initArg0 };
	static const iocshFuncDef initFuncDef = { "acq400_SOE_Configure", 1, initArgs };
	static void initCallFunc(const iocshArgBuf *args)
	{
		acq400_SOE_Configure(args[0].sval);
	}

	void acq400_SOE_Register(void)
	{
	    iocshRegister(&initFuncDef, initCallFunc);
	}

	epicsExportRegistrar(acq400_SOE_Register);
}


