/*
 * acq400_PM.cpp
 *
 *  Created on: 6 Mar 2026
 *      Author: pgm
 */

#include "acq400_asyn_common.h"
#include "acq400_PM.h"
#include <fcntl.h>                // open()
#include <unistd.h>
#include <string.h>
static const char *driverName="acq400_PM";

using namespace std;
#include "Buffer.h"

#define DN	driverName
#define FN	__FUNCTION__

int acq400_PM::nice = ::getenv_default("acq400_PM_NICE", 0);
int acq400_PM::verbose = ::getenv_default("acq400_PM_VERBOSE", 0);

acq400_PM::acq400_PM(const char* portName):
	acq400_asynPortDriver(portName,
	/* maxAddr */		MAX_PM_BUFFERS,    /* nbuffers from 0 (most recent) */
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
	fprintf(stderr, "%s R1000 \n", FN);
	memset(&pm_cols, 0, sizeof(pm_cols));

	eventId = epicsEventCreate(epicsEventEmpty);

	for (epicsInt8 row = 0; row < MAX_PM_BUFFERS; ++ row){
		pm_cols.c_rownum[row] = row;
	}

	createParam(PS_RUNSTOP,		asynParamInt32,		&P_RUNSTOP);
	createParam(PS_UPDATES,  	asynParamInt32,		&P_UPDATES);
	createParam(PS_TS_USEC,  	asynParamInt64,		&P_TS_USEC);
	createParam(PS_NBUF,  		asynParamInt32,		&P_NBUF);
	createParam(PS_PM_COL_ROWNUM,	asynParamInt8Array,	&P_COL_ROWNUM);
	createParam(PS_PM_COL_IBLIVE,	asynParamInt16Array,	&P_COL_IBLIVE);
	createParam(PS_PM_COL_IBSTORE,	asynParamInt16Array,	&P_COL_IBSTORE);
	createParam(PS_PM_COL_TS,	asynParamInt64Array,	&P_COL_TS);


	createParam(PS_PM_COL_SP0,	asynParamInt32, &P_COL_SP0);
	createParam(PS_PM_COL_SP1,	asynParamInt32, &P_COL_SP1);
	createParam(PS_PM_COL_SP2,	asynParamInt32, &P_COL_SP2);
	createParam(PS_PM_COL_WRVS,	asynParamInt32, &P_COL_WRVS);
	createParam(PS_PM_COL_WRVT,	asynParamInt32, &P_COL_WRVT);
	createParam(PS_PM_COL_WRUS,	asynParamInt32, &P_COL_WRUS);

	createParam(PS_RAWBUF,		asynParamInt32Array, &P_RAWBUF);

	/* Create the thread that computes the waveforms in the background */
	status = (asynStatus)(epicsThreadCreate("PM_task",
			epicsThreadPriorityHigh - nice,
			epicsThreadGetStackSize(epicsThreadStackMedium),
			(EPICSTHREADFUNC)task_runner,
			this) == NULL);
	if (status) {
		printf("%s:%s: epicsThreadCreate failure\n", DN, FN);
		return;
	}
}

void acq400_PM::task_runner(void *drvPvt)
{
	((acq400_PM *)drvPvt)->task();
}

int FIRST=2;   // @@todo SWAG. Make official.

void acq400_PM::init_buffers(const unsigned nbuf)
{
	if (verbose) fprintf(stderr, "%s 01\n", FN);
	filled.clear();
	empties.clear();
	for (short ii = FIRST; ii <= (short)nbuf; ++ii){
		if (verbose>1) fprintf(stderr, "%s 50\n", FN);
		empties.push_back({-1, ii});
	}
}

void acq400_PM::stash_buffer(int ib_live, const unsigned nbuf)
{
	BufferPair bp;

	if (verbose > 1) fprintf(stderr, "%s 01\n", FN);

	const char* fill_from = "";
	if (empties.empty()){
		assert(!filled.empty());
		bp = filled.back(); filled.pop_back();
		fill_from = "filled";
	}else{
		bp = empties.front(); empties.pop_front();
		fill_from = "empties";
	}
	bp.ib_live = ib_live;
	if (verbose > 1) fprintf(stderr, "%s fill_from:%s push %d.%d\n",
			FN, fill_from, bp.ib_store, bp.ib_live);

	filled.push_front(bp);

	// @@todo copy DRAM using ioctl
}

void acq400_PM::update_pm_callbacks(void)
{
	doCallbacksInt8Array(pm_cols.c_rownum, 		MAX_PM_BUFFERS, P_COL_ROWNUM, 0);
	doCallbacksInt16Array(pm_cols.c_ib_live, 	MAX_PM_BUFFERS, P_COL_IBLIVE, 0);
	doCallbacksInt16Array(pm_cols.c_ib_store, 	MAX_PM_BUFFERS, P_COL_IBSTORE, 0);
	// ...
	doCallbacksInt64Array(pm_cols.c_WRUS,		MAX_PM_BUFFERS, P_COL_WRUS, 0);

	setIntegerParam(P_UPDATES, ++update);
	callParamCallbacks();
}

#define SSB		128
#define TRANSLEN	1024
#define BURST_LW 	(SSB*TRANSLEN/sizeof(int))

void acq400_PM::task()
{
	asynStatus status = asynSuccess;

	fprintf(stderr, "%s 01\n", FN);
	epicsEventWait(eventId);

	fprintf(stderr, "%s LET's go\n", FN);
	int fc = open("/dev/acq400.0.bq", O_RDONLY);
	assert(fc >= 0);

	int nbuf;
	gip(P_NBUF, &nbuf);
	const unsigned NBUF = (unsigned)nbuf;

	if ((ib = getBufferId(fc)) < 0){
		fprintf(stderr, "ERROR: getBufferId() fail");
		return;
	}

	for (int runstop, runstop0 = 0; (ib = getBufferId(fc)) >= 0; runstop0 = runstop){
		lock();
		status = getIntegerParam(P_RUNSTOP, &runstop);
		if (status){
			fprintf(stderr, "%s:%s getIntegerParam P_FMT_MC_PORT fail\n", DN, FN);
			return;
		}
		unlock();
		if (runstop == 1 || runstop0 == 1){
			if (runstop0 == 0){
				init_buffers(NBUF);
			}
			stash_buffer(ib, NBUF);

			int icol = 0;
			for (auto&& bpi: filled){
				pm_cols.c_ib_live[icol] = bpi.ib_live;
				pm_cols.c_ib_store[icol] = bpi.ib_store;
				++icol;
			}
			lock();
			update_pm_callbacks();
			unlock();
		}
		if (runstop == 0 && runstop0 == 1){
			/* call P_RAWBUF callbacks on stop. This is HEAVY, but infrequent */
			int baddr = 0;
			lock();
			for (auto&& bpi: filled){
				int* bp = (int*)Buffer::the_buffers[bpi.ib_store]->getBase();
				doCallbacksInt32Array(bp, BURST_LW, P_RAWBUF, baddr++);
			}
			unlock();

		}
	}

	close(fc);
}

asynStatus acq400_PM::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    const char *paramName;
    int addr;

    /* Fetch the parameter string name for possible use in debugging */
    getParamName(function, &paramName);

    status = pasynManager->getAddr(pasynUser, &addr);
    if(status!=asynSuccess) return status;

    /* Set the parameter in the parameter library. */
    status = (asynStatus) setIntegerParam(addr, function, value);

    fprintf(stderr,
		      "%s:%s: function=%d, addr=%d, name=%s, value=%d\n",
		      DN, FN, function, addr, paramName, value);

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
	int acq400_PM_Configure(const char *portName)
	{
		printf("%s:%s R1001 %s\n", DN, FN, portName);

		new acq400_PM(portName);
		return 0;
	}

	/* EPICS iocsh shell commands */

	static const iocshArg initArg0 = { "port", iocshArgString };
	static const iocshArg * const initArgs[] = { &initArg0 };
	static const iocshFuncDef initFuncDef = { "acq400_PM_Configure", 1, initArgs };
	static void initCallFunc(const iocshArgBuf *args)
	{
		acq400_PM_Configure(args[0].sval);
	}

	void acq400_PM_Register(void)
	{
	    iocshRegister(&initFuncDef, initCallFunc);
	}

	epicsExportRegistrar(acq400_PM_Register);
}

