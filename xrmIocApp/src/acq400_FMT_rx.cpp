/** @file acq400_FMT_rx.cpp
 *  @brief acq400_FMT_rx implementation
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

int acq400_FMT_rx::maxq = ::getenv_default("acq400_FMT_rx_maxq", 4);


acq400_FMT_rx::acq400_FMT_rx(const char* portName) :
		acq400_FMT_abc(portName,
		/* maxAddr */		FMT_ROWS,    /* nchan from 0 */
					maxq,
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
		ts(0),
		fmt_pm_trg_evt(NO_TRG_EVT)
{
	asynStatus status = asynSuccess;
	fmt_cache = new FMT[maxq];

	if (verbose)
		fprintf(stderr, "%s: sizeof(FMT) " FMTSZT " fmt_cache " FMTSZT " *fmt_cache " FMTSZT "\n",
			FN, sizeof(FMT), sizeof(fmt_cache), sizeof(*fmt_cache));



	for (int ii = 0; ii < maxq; ++ii){
		empties.push_front(ii);
	}

	for (auto ii: empties){
		printf("acq400_FMT_rx empties:%p\n", fmt_cache[ii]);
	}

	createParam(PS_FMT_PM_TRG_EVT,  asynParamInt32,	&P_FMT_PM_TRG_EVT);
	createParam(PS_FMT_PM_TRG_EVT_ACTION,  asynParamInt32,	&P_FMT_PM_TRG_EVT_ACTION);
	sip(0, P_FMT_PM_TRG_EVT_ACTION, 0);

	rx_event = epicsEventCreate(epicsEventEmpty);

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

acq400_FMT_rx::~acq400_FMT_rx() {
	fprintf(stderr, "%s SHOULD NOT HAPPEN\n", __FUNCTION__);
	assert(0);
}

void acq400_FMT_rx::update_fmt(FMT& fmt, bool first_time)
{
	++packet_count;
#if 0
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
#endif
	ts = fmt[0].timestamp;
}

void acq400_FMT_rx::process_fmt(FMT& fmt, bool first_time)
{
	bool triggered = false;
	epicsEventSignal(rx_event);

	if (fmt_pm_trg_evt){
		const epicsUInt16 trg_evt = fmt_pm_trg_evt;
		for (int ii = 0; ii < FMT_ROWS; ++ii){
			if (trg_evt == fmt[ii].event){
				triggered = true;
				onPM_trg_evt();
				break;
			}
		}
	}
	if (!triggered){
		sip(0, P_FMT_PM_TRG_EVT_ACTION, 0);
	}
}

int acq400_FMT_rx::get_empty() {
	const char* fill_from = "xxx";
	int ib;

	if (empties.empty()){
		assert(!filled.empty());
		ib = filled.back(); filled.pop_back();       // pull oldest
		fill_from = "filled";
	}else{
		ib = empties.back(); empties.pop_back();     // pull oldest (academic for empties)
		fill_from = "empties";
	}

	if (verbose > 1){
		fprintf(stderr, "%s fill_from:%s push %d\n",
			FN, fill_from, ib);
	}

	return ib;
}


FMT& acq400_FMT_rx::receive(MultiCast& multicast){
	const int ib = get_empty();
	if (verbose)
	    fprintf(stderr, "%s ib:%d\n", FN, ib);
	multicast.recvfrom(fmt_cache[ib], sizeof(FMT));
	filled.push_front(ib);                     // filled[0] is latest arrival
	return fmt_cache[ib];
}

const epicsUInt16 acq400_FMT_rx::getFMT_pm_trg_evt() {
	return fmt_pm_trg_evt;
}

void acq400_FMT_rx::task(void) {
	bool first_time = true;

	epicsEventWait(eventId);

	MultiCast& multicast = acq400_FMT_abc::mc_factory(MultiCast::MC_RECEIVER);
	MonitorRateLimit rateLimit;

	while(1){
		int runstop;
		lock();
		gip(P_RUNSTOP, &runstop);

		unlock();
		if (runstop == 1){
			FMT& fmt = receive(multicast);
			rateLimit.newData(mrl_param);
			update_fmt(fmt, first_time);
			process_fmt(fmt, first_time);
			if (rateLimit.goAhead()){
				update_fmt_columns(fmt);   // @@todo head
			}
			lock();
			updateTimeStamp();
			update_fmt_callbacks(rateLimit.goAhead());
			unlock();
			first_time = false;
		}else{
			usleep(50000);
		}
	}
}

void acq400_FMT_rx::onPM_trg_evt()
{
	sip(0, P_FMT_PM_TRG_EVT_ACTION, 1);
	callParamCallbacks();

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
	    }else if (function == P_MON_RL){
		    mrl_param = value;
	    }else if (function == P_FMT_PM_TRG_EVT){
		    fmt_pm_trg_evt = value;
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

int acq400_FMT_rx::waitFMT(unsigned timeout_ms)
{
	double timeout = (double)timeout_ms/1000;
	epicsEventStatus rx_wait_status =
			epicsEventWaitWithTimeout(rx_event, timeout);
	switch(rx_wait_status){
	case epicsEventOK:
		return 0;
	case epicsEventWaitTimeout:
		return -1;
	default:
		assert(rx_wait_status != epicsEventOK && rx_wait_status != epicsEventWaitTimeout);
		return -1;		// doesn't happen0
	}
}

const FMT& acq400_FMT_rx::get_fmt(unsigned icache)
{
	assert(icache < filled.size());
	return fmt_cache[filled[icache]];
}

acq400_FMT_rx* acq400_FMT_rx::instance(const char* portName)
{
	static acq400_FMT_rx* _instance;
	if (_instance == 0){
		assert (portName != 0);
		_instance = new acq400_FMT_rx(portName);
	}
	return _instance;
}



extern "C" {

	/** EPICS iocsh callable function to call constructor for the testAsynPortDriver class.
	  * \param[in] portName The name of the asyn port driver to be created.
	  */
	int acq400_FMT_rxConfigure(const char *portName)
	{
		acq400_FMT_rx::instance(portName);
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


