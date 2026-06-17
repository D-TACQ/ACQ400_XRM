/** @file acq400_FMT_sim.cpp
 *  @brief acq400_FMT_sim implementation.
 *
 *  Created on: 9 Feb 2026
 *      Author: pgm
 */

#include "acq400_asyn_common.h"
#include "acq400_FMT_sim.h"
#include "Multicast.h"



static const char *driverName="acq400_FMT_sim";

#define DN	driverName
#define FN	__FUNCTION__

class TimeProviderLocaltime: public TimeProvider {
	epicsInt64 _time_now();
public:
	virtual epicsInt64 time_now();
};


epicsInt64 TimeProviderLocaltime::_time_now()
/* non-Blocking */
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

epicsInt64 TimeProviderLocaltime::time_now()
/* "Blocking" */
{
	usleep(50000);
	return _time_now();
}

#define WR_TS 	"/dev/acq400.0.wr_ts"
#define WR_TAI	"/dev/acq400.0.knobs/wr_tai_cur_raw"
#define TICKS_PER_US	40	/* @@todo ASSUME 40Mhz clock */


class TimeProviderWrts: public TimeProvider {
	FILE* fp_wrts;

	/* BLOCKING */
	epicsInt64 _time_now() {
		unsigned wr_ts;
		int rc = fread(&wr_ts, sizeof(unsigned), 1, fp_wrts);
		if (rc != 1){
			fprintf(stderr, "ERROR %s:%s %s fread()\n", DN, FN, WR_TS);
			exit(1);
			return 0;
		}

		FILE* fp_tai_raw = fopen(WR_TAI, "r");
		if (fp_tai_raw == 0){
			fprintf(stderr, "ERROR %s:%s %s\n", DN, FN, WR_TAI);
			exit(1);
		}
		char txt_line[80];
		if (fgets(txt_line, 80, fp_tai_raw) == 0){
			fprintf(stderr, "ERROR %s:%s %s fgets()\n", DN, FN, WR_TAI);
			exit(1);
		}
		fclose(fp_tai_raw);

		unsigned seconds = strtol(txt_line, 0, 16);

		if ((seconds&0x7) != ((wr_ts>>28)&0x7)){
			fprintf(stderr, "WARNING %s:%s %s seconds mismatch %08x %08x DISCARD\n",
					DN, FN, WR_TAI, seconds, wr_ts);
		}
		epicsInt64 usec = ((epicsInt64)seconds) * 1000000;
		usec += (wr_ts&0x0fffffff) / TICKS_PER_US;
		return usec;
	}
public:
	TimeProviderWrts() {
		fp_wrts = fopen(WR_TS, "r");
		if (fp_wrts == 0){
			fprintf(stderr, "ERROR %s:%s %s\n", DN, FN, WR_TS);
			exit(1);
		}
	}


	virtual epicsInt64 time_now() {
		epicsInt64 tc;
		do {
			tc = _time_now();
		} while(tc == 0);

		return tc;
	}
};


void acq400_FMT_Sim::update_fmt(bool first_time)
/* first_time: set now_us; else first use previous now_us then set now_us; */
{
	if (first_time){
		assert(FMT_ROWS < 0xffU);
		for (epicsInt8 row = 0; row < FMT_ROWS; ++row){
			fmt[row].event = 0x100 + row;
			fmt[row].client_data = row;
		}
	}else{
		for (int row = 0; row < FMT_ROWS; ++row){
			fmt[row].timestamp = now_us + row*10;
		}

		fmt[0].pad = update >> 16;
		fmt[1].pad = update;
	}
	now_us = timeProvider.time_now();

}

acq400_FMT_Sim::acq400_FMT_Sim(
		const char* portName, TimeProvider& _timeProvider):
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
	timeProvider(_timeProvider)
{
	asynStatus status = asynSuccess;


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

	epicsEventWait(eventId);

	MultiCast& multicast = acq400_FMT_abc::mc_factory(MultiCast::MC_SENDER);
	MonitorRateLimit rateLimit;

	for (int runstop, runstop0 = 0; ; runstop0 = runstop){
		lock();
		status = getIntegerParam(P_RUNSTOP, &runstop);
		if (status){
			fprintf(stderr, "%s:%s getIntegerParam P_FMT_MC_PORT fail\n", DN, FN);
			return;
		}
		unlock();
		if (runstop == 1){
			if (runstop0 == 0){
				update_fmt(true);
			}else{
				update_fmt(false);
				multicast.sendto(fmt, sizeof(fmt));
				rateLimit.newData(mrl_param);
				if (rateLimit.goAhead()){
					update_fmt_columns();
				}
				lock();
				updateTimeStamp();
				update_fmt_callbacks(rateLimit.goAhead());
				unlock();
			}

		}else{
			usleep(50000);
		}
	}
}


void acq400_FMT_Sim::redit()
{
	int row, row_count, event, event_step, clidat, clidat_step;

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
	    }else if (function == P_MON_RL){
		    mrl_param = value;
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
	int acq400_FMT_SimConfigure(const char *portName, const char* timeProviderDef)
	{
		printf("%s:%s R1001 %s, %s\n", DN, FN, portName, timeProviderDef);
		TimeProvider* tp;

		if (strcmp(timeProviderDef, "WRTS") == 0){
			tp = new TimeProviderWrts();
		}else{
			tp = new TimeProviderLocaltime();
		}
		new acq400_FMT_Sim(portName, *tp);
		return 0;
	}

	/* EPICS iocsh shell commands */

	static const iocshArg initArg0 = { "port", iocshArgString };
	static const iocshArg initArg1 = { "time_provider", iocshArgString };
	static const iocshArg * const initArgs[] = { &initArg0, &initArg1 };
	static const iocshFuncDef initFuncDef = { "acq400_FMT_SimConfigure", 2, initArgs };
	static void initCallFunc(const iocshArgBuf *args)
	{
		acq400_FMT_SimConfigure(args[0].sval, args[1].sval);
	}

	void acq400_FMT_simRegister(void)
	{
	    iocshRegister(&initFuncDef, initCallFunc);
	}

	epicsExportRegistrar(acq400_FMT_simRegister);
}




