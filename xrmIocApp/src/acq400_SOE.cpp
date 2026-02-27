/*
 * acq400_SOE.cpp
 *
 *  Created on: 23 Feb 2026
 *      Author: pgm
 */

#include "acq400_asyn_common.h"
#include "acq400_SOE.h"
#include "acq-util.h"
#include <fcntl.h>                // open()
#include <unistd.h>
#include <string.h>

using namespace std;
#include "Buffer.h"
#include "ES.h"

static const char *driverName="acq400_SOE";

#define DN	driverName
#define FN	__FUNCTION__


int acq400_SOE::nice = ::getenv_default("acq400_SOE_NICE", 0);

acq400_SOE::acq400_SOE(const char* portName):
	asynPortDriver(portName,
	/* maxAddr */		SOE_HLD_ROWS,    /* nchan from 0 */
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
	fprintf(stderr, "%s R1001 SP2\n", FN);
	asynStatus status = asynSuccess;
	memset(soe_lut, 0, sizeof(soe_lut));

	assert(SOE_LUT_ROWS < 0xffU);
	for (epicsInt8 row = 0; row < SOE_LUT_ROWS; ++row){
		cols.c_rownum[row] = row;
		soe_lut[row].event = 1000+row;
		soe_lut[row].pv_id = 2000+row;
		soe_lut[row].offset_us = row*2;
	}

	assert(SOE_HLD_ROWS < 0xffU);
	for (epicsInt8 row = 0; row < SOE_HLD_ROWS; ++row){
		hold_cols.c_rownum[row] = row;
		/* .. temp will get overwritten from raw TABLE copy */
		hold_cols.c_DI1[row] = 0xd1;
		hold_cols.c_DI2[row] = 0xd2;
		hold_cols.c_SP0[row] = row;
		hold_cols.c_SP1[row] = 0x12345678;
	}


	eventId = epicsEventCreate(epicsEventEmpty);

	createParam(PS_RUNSTOP,  asynParamInt32,        &P_RUNSTOP);
	createParam(PS_UPDATES,  asynParamInt32,        &P_UPDATES);
	createParam(PS_TS_USEC,  asynParamInt64,	&P_TS_USEC);

	createParam(PS_SOE_SMPL_SS_U32,		asynParamOctet,      &P_SOE_HLD_COL_SS_U32);
	createParam(PS_SOE_SMPL_AI_COUNT,	asynParamOctet,	     &P_SOE_HLD_COL_AI_COUNT);
	createParam(PS_SOE_SMPL_DI_COUNT,	asynParamOctet,	     &P_SOE_HLD_COL_DI_COUNT);
	createParam(PS_SOE_SMPL_SP_COUNT,	asynParamOctet,	     &P_SOE_HLD_COL_SP_COUNT);
	createParam(PS_SOE_SMPL_DI_INDEX, 	asynParamOctet,	     &P_SOE_SMPL_DI_INDEX);
	createParam(PS_SOE_SMPL_SP_INDEX, 	asynParamOctet,	     &P_SOE_SMPL_SP_INDEX);

	createParam(PS_SOE_LUT_COL_ROWNUM,	asynParamInt8Array,  &P_SOE_LUT_COL_ROWNUM);
	createParam(PS_SOE_LUT_COL_EVENT,	asynParamInt16Array, &P_SOE_LUT_COL_EVENT);
	createParam(PS_SOE_LUT_COL_PAD,		asynParamInt16Array, &P_SOE_LUT_COL_PAD);
	createParam(PS_SOE_LUT_COL_PV_ID,	asynParamInt32Array, &P_SOE_LUT_COL_PV_ID);
	createParam(PS_SOE_LUT_COL_OFFSET_US,	asynParamInt32Array, &P_SOE_LUT_COL_OFFSET_US);

	createParam(PS_SOE_HLD_COL_ROWNUM,	asynParamInt8Array,  &P_SOE_HLD_COL_ROWNUM);
	createParam(PS_SOE_HLD_COL_PV_ID,    	asynParamInt32Array, &P_SOE_HLD_COL_PV_ID);
	createParam(PS_SOE_HLD_COL_CLIDAT,    	asynParamInt32Array, &P_SOE_HLD_COL_CLIDAT);
	createParam(PS_SOE_HLD_COL_TS,    	asynParamInt64Array, &P_SOE_HLD_COL_TS);
	createParam(PS_SOE_HLD_COL_AI1,	asynParamFloat32Array, &P_SOE_HLD_COL_AI1);
	createParam(PS_SOE_HLD_COL_AI2,	asynParamFloat32Array, &P_SOE_HLD_COL_AI2);

	createParam(PS_SOE_HLD_COL_DI1,	asynParamInt32Array, &P_SOE_HLD_COL_DI1);
	createParam(PS_SOE_HLD_COL_DI2,	asynParamInt32Array, &P_SOE_HLD_COL_DI2);
/* SP0 : SPAD0 aka Sample Number
 * SP1 : SPAD1 aka local clock us
 * SP2 : SPAD2 aka WR_VERNIER
 */
	createParam(PS_SOE_HLD_COL_SP0,	asynParamInt32Array, &P_SOE_HLD_COL_SP0);
	createParam(PS_SOE_HLD_COL_SP1,	asynParamInt32Array, &P_SOE_HLD_COL_SP1);
	createParam(PS_SOE_HLD_COL_SP2, asynParamInt32Array, &P_SOE_HLD_COL_SP2);
	createParam(PS_SOE_HLD_COL_WRVS, asynParamInt8Array, &P_SOE_HLD_COL_WRVS);
	createParam(PS_SOE_HLD_COL_WRVT, asynParamInt32Array, &P_SOE_HLD_COL_WRVT);
	createParam(PS_SOE_HLD_COL_WRUS, asynParamInt64Array, &P_SOE_HLD_COL_WRUS);


	createParam(PS_SOE_HLD_COL_DATA_OFFSET, asynParamInt32,      &P_SOE_HLD_COL_DATA_OFFSET);



	createParam(PS_SOE_LUT_REDIT_ROW, 	asynParamInt32,      &P_SOE_LUT_REDIT_ROW);


	createParam(PS_SOE_LUT_REDIT_ROWCOUNT, 	asynParamInt32, &P_SOE_LUT_REDIT_ROWCOUNT);
	createParam(PS_SOE_LUT_REDIT_EVENT,	asynParamInt32, &P_SOE_LUT_REDIT_EVENT);
	createParam(PS_SOE_LUT_REDIT_EVENT_STEP,asynParamInt32, &P_SOE_LUT_REDIT_EVENT_STEP);
	createParam(PS_SOE_LUT_REDIT_PV_ID, 	asynParamInt32, &P_SOE_LUT_REDIT_PV_ID);
	createParam(PS_SOE_LUT_REDIT_PV_ID_STEP,asynParamInt32, &P_SOE_LUT_REDIT_PV_ID_STEP);
	createParam(PS_SOE_LUT_REDIT_OFFSET_US, asynParamInt32, &P_SOE_LUT_REDIT_OFFSET_US);
	createParam(PS_SOE_LUT_REDIT_OFFSET_US_STEP,asynParamInt32, &P_SOE_LUT_REDIT_OFFSET_US_STEP);
	createParam(PS_SOE_LUT_REDIT_COMMIT,	asynParamInt32, &P_SOE_LUT_REDIT_COMMIT);

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

asynStatus acq400_SOE::gip(int pnum, int* pram)
{
	asynStatus status = getIntegerParam(pnum, pram);
	if (status){
		fprintf(stderr, "%s:%s getIntegerParam %d fail\n",
				DN, FN, pnum);
	}
	return status;
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


void acq400_SOE::update_hld_tab(bool first_time)
{
	/*
	short* ai_raw = (short*)Buffer::the_buffers[ib];
	int * di_raw = (int*)Buffer::the_buffers[ib];
	*/
}

#define SKIP_ES 1
#define TRANSLEN	1024
#define TICKSPERUS	40

/* @@todo .. specialize time provider */
static epicsInt64 getWrTs(unsigned wrv)
{
	int sec = wrv >> 28;
	int usec = (wrv&0x0fffffff)/TICKSPERUS;
	epicsInt64 ts = sec*1000000 + usec;
	return ts;
}
static int SP1_SIM = 0;
void acq400_SOE::update_hld_tab_columns(void)
{

/* @@todo hardcoded make auto links to main ioc PV's
acq2206_088> get.site 0 run0_log
/usr/local/bin/run0 1,2 1,14,0 ssb=124
acq2206_088> get.site 2 nchan
ERROR:nchan" not found
acq2206_088> get.site 2 nchan_enabled
ERROR:nchan_enabled" not found
acq2206_088> get.site 2 NCHAN
32
acq2206_088> get.site 1 NCHAN
32
acq2206_088> get.site 1 MODEL
ACQ423ELF
acq2206_088> get.site 2 MODEL
DIO482ELF N=32 M=6B

 param P_SOE_SMPL_DI_INDEX 32/2 = 16
 param P_SOE_SMPL_SP_INDEX 16+4=20
 */
	const int SOE_SMPL_DI_INDEX = 16;   /* index in u32. There is only one DI32 here */
	const int SOE_SMPL_SP_INDEX = 17;
	const int SSB = 124;
	const int SSS = SSB/sizeof(short);
	const int SSL = SSB/sizeof(long);
	char* raw = Buffer::the_buffers[ib]->getBase() + SKIP_ES*SSB;
	short* ai_raw = (short*)raw;
	int * di_raw = (int*)raw + SOE_SMPL_DI_INDEX;
	int * sp_raw = (int*)raw + SOE_SMPL_SP_INDEX;


	/* first 10 rows c_client_data becomes ib history for diags.. */
	for (int ii = 10; ii; --ii){
		hold_cols.c_client_data[ii] = hold_cols.c_client_data[ii-1];
	}
	hold_cols.c_client_data[0] = ib;

	for (int row = 0; row < SOE_HLD_ROWS; ++row){
		const int srow = row*SSS;
		const int lrow = row*SSL;

		hold_cols.c_AI1[row] = ai_raw[srow+0]*10.0/32768;
		hold_cols.c_AI2[row] = ai_raw[srow+1]*10.0/32768;
		hold_cols.c_DI1[row] = di_raw[lrow+0];
		hold_cols.c_DI2[row] = ++SP1_SIM;
		hold_cols.c_SP0[row] = sp_raw[lrow+SP0];
		hold_cols.c_SP1[row] = sp_raw[lrow+SP1];
		hold_cols.c_SP2[row] = sp_raw[lrow+SP2];
		hold_cols.c_WRVS[row]= sp_raw[lrow+SP2] >> 28;
		hold_cols.c_WRVT[row]= sp_raw[lrow+SP2]&0x0fffffff;
		hold_cols.c_WRUS[row]= getWrTs(sp_raw[lrow+SP2]);
	}
}
void acq400_SOE::update_hld_tab_callbacks(void)
{
	doCallbacksInt8Array(hold_cols.c_rownum, 	SOE_HLD_ROWS, P_SOE_HLD_COL_ROWNUM, 0);
	doCallbacksInt32Array(hold_cols.c_pv_id, 	SOE_HLD_ROWS, P_SOE_HLD_COL_PV_ID, 0);
	doCallbacksInt32Array(hold_cols.c_client_data,  SOE_HLD_ROWS, P_SOE_HLD_COL_CLIDAT, 0);
	doCallbacksInt64Array(hold_cols.c_timestamp, 	SOE_HLD_ROWS, P_SOE_HLD_COL_TS, 0);
	doCallbacksFloat32Array(hold_cols.c_AI1, 	SOE_HLD_ROWS, P_SOE_HLD_COL_AI1, 0);
	doCallbacksFloat32Array(hold_cols.c_AI2, 	SOE_HLD_ROWS, P_SOE_HLD_COL_AI2, 0);
	doCallbacksInt32Array(hold_cols.c_DI1, 		SOE_HLD_ROWS, P_SOE_HLD_COL_DI1, 0);
	doCallbacksInt32Array(hold_cols.c_DI2, 		SOE_HLD_ROWS, P_SOE_HLD_COL_DI2, 0);
	doCallbacksInt32Array(hold_cols.c_SP0, 		SOE_HLD_ROWS, P_SOE_HLD_COL_SP0, 0);
	doCallbacksInt32Array(hold_cols.c_SP1, 		SOE_HLD_ROWS, P_SOE_HLD_COL_SP1, 0);
	doCallbacksInt32Array(hold_cols.c_SP2, 		SOE_HLD_ROWS, P_SOE_HLD_COL_SP2, 0);
	doCallbacksInt8Array( hold_cols.c_WRVS, 	SOE_HLD_ROWS, P_SOE_HLD_COL_WRVS, 0);
	doCallbacksInt32Array(hold_cols.c_WRVT, 	SOE_HLD_ROWS, P_SOE_HLD_COL_WRVT, 0);
	doCallbacksInt64Array(hold_cols.c_WRUS, 	SOE_HLD_ROWS, P_SOE_HLD_COL_WRUS, 0);
}

void acq400_SOE::task()
{
	asynStatus status = asynSuccess;
	epicsEventWait(eventId);

	int fc = open("/dev/acq400.0.bq", O_RDONLY);
	assert(fc >= 0);
	for (unsigned ii = 0; ii < Buffer::nbuffers; ++ii){
		Buffer::create(getRoot(0), Buffer::bufferlen);
	}

	if ((ib = getBufferId(fc)) < 0){
		fprintf(stderr, "ERROR: getBufferId() fail");
		return;
	}

	while((ib = getBufferId(fc)) >= 0){
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
			update_hld_tab_columns();
			lock();
			update_soe_lut_callbacks();
			update_hld_tab_callbacks();
			unlock();
		}
	}
}


void acq400_SOE::redit()
{
	int row, row_count, event, event_step, pv_id, pv_id_step, offset_us, offset_us_step;

	fprintf(stderr, "%d %d %d %d %d %d %d %d\n",
			P_SOE_LUT_REDIT_ROW,
			P_SOE_LUT_REDIT_ROWCOUNT,
			P_SOE_LUT_REDIT_EVENT,
			P_SOE_LUT_REDIT_EVENT_STEP,
			P_SOE_LUT_REDIT_PV_ID,
			P_SOE_LUT_REDIT_PV_ID_STEP,
			P_SOE_LUT_REDIT_OFFSET_US,
			P_SOE_LUT_REDIT_OFFSET_US_STEP);

	if (gip(P_SOE_LUT_REDIT_ROW, 	&row)			||
	    gip(P_SOE_LUT_REDIT_ROWCOUNT, 	&row_count)	||
	    gip(P_SOE_LUT_REDIT_EVENT,    	&event)		||
	    gip(P_SOE_LUT_REDIT_EVENT_STEP, &event_step)	||
	    gip(P_SOE_LUT_REDIT_PV_ID,     &pv_id)		||
	    gip(P_SOE_LUT_REDIT_PV_ID_STEP,&pv_id_step)		||
	    gip(P_SOE_LUT_REDIT_OFFSET_US,     &offset_us)	||
	    gip(P_SOE_LUT_REDIT_OFFSET_US_STEP,&offset_us_step)	){
		return;
	}

	fprintf(stderr, "%s:%s %d,%d %d,%d %d,%d, %d, %d\n", DN, FN,
			row, row_count, event, event_step, pv_id, pv_id_step, offset_us, offset_us_step);

	lock();
	for (int rn = 0; rn < row_count; ++rn){
		struct SOE_LUT_ROW& this_row = soe_lut[row+rn];
		this_row.event = event + rn*event_step;
		this_row.pv_id = pv_id + rn*pv_id_step;
		this_row.offset_us = offset_us +rn*offset_us_step;
	}
	unlock();
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
	    }else if (function == P_SOE_LUT_REDIT_COMMIT){
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


