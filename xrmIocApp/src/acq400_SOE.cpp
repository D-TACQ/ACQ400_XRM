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
	/* Default stack size*/ 0)
{
	memset(soe_lut, 0, sizeof(soe_lut));

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
	;
}
void acq400_SOE::update_soe_lut_callbacks(void)
{
	;
}

void acq400_SOE::task()
{
	;
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


