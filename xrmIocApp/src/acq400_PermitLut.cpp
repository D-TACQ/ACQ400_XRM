/*
 * acq400_PermitLut.cpp
 *
 *  Created on: 19 Aug 2026
 *      Author: pgm
 */

#include "acq400_PermitLut.h"
#include "acq400_Proxy.h"
#include "acq400_Permit.h"

#define DN	__FILE__
#define FN	__FUNCTION__

int acq400_PermitLut::verbose = ::getenv_default("acq400_PERMIT_LUT_VERBOSE", 0);

acq400_PermitLut::acq400_PermitLut(const char *portName, int _max_addr):
	acq400_asynPortDriver(portName,
	/* maxAddr */		_max_addr,
	/* Interface mask */    asynEnumMask|asynOctetMask|asynInt32Mask|asynInt64Mask|asynFloat64Mask|asynUInt32DigitalMask|
				asynDrvUserMask|asynInt16ArrayMask|asynInt32ArrayMask,
	/* Interrupt mask */	asynEnumMask|asynOctetMask|asynInt32Mask|asynInt64Mask|asynFloat64Mask|asynUInt32DigitalMask,
	/* asynFlags no block*/ 0,
	/* Autoconnect */       1,
	/* Default priority */  0,
	/* Default stack size*/ 0),
	dsp(PermitDSP::instance())
{
	memset(lut_rows, 0, MAX_ROWS*sizeof(PRMT_LUT_ROW));

	createParam(PS_PRMT_LUT_SOFT_EVENT,  asynParamInt32, &P_PRMT_LUT_SOFT_EVENT);

	createParam(PS_PRMT_LUT_EVENT,  asynParamInt32,      &P_PRMT_LUT_EVENT);
	createParam(PS_PRMT_LUT_TH_AI,  asynParamInt16Array, &P_PRMT_LUT_TH_AI);
	createParam(PS_PRMT_LUT_TH_DI,  asynParamInt32Array, &P_PRMT_LUT_TH_DI);
	createParam(PS_PRMT_LUT_COMMIT, asynParamInt32,      &P_PRMT_LUT_COMMIT);
}

#define MARK fprintf(stderr, "%s %d %s\n", __FILE__, __LINE__, __FUNCTION__)

asynStatus acq400_PermitLut::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
	int function = pasynUser->reason;
	asynStatus status = asynSuccess;
	const char *paramName;
	int addr = 0;

	/* Fetch the parameter string name for possible use in debugging */
	getParamName(function, &paramName);

	if (maxAddr > 1){
		status = pasynManager->getAddr(pasynUser, &addr);
		if(status!=asynSuccess) return status;
	}

	/* Set the parameter in the parameter library. */
	status = (asynStatus) setIntegerParam(addr, function, value);

	if (function == P_RUNSTOP) {
		if (value == 1){
			epicsEventSignal(eventId);
		}
	}else if (function == P_PRMT_LUT_EVENT){
		assert(addr < MAX_ROWS);
//		if (lut_rows[addr].event != value){
//			lut_set.erase(&lut_rows[addr]);
//		}
		lut_rows[addr].event = value;
//		lut_set.insert(&lut_rows[addr]);

		if (verbose){
			fprintf(stderr, "%s [%d] P_PRMT_LUT_EVENT value:%d\n",
							FN, addr, value);
		}
	}else if (function == P_PRMT_LUT_SOFT_EVENT){
		for (int ii = 0; ii < MAX_ROWS; ii += 1){

			if (verbose){
				fprintf(stderr, "%s [%d] value:%d lut event:%d\n",
					FN, ii, value, lut_rows[ii].event);
			}

			if (value == lut_rows[ii].event){
				PermitDSP::instance().full_load(
					lut_rows[ii].ai_th_pairs,
					lut_rows[ii].di_pairs);
				break;
			}
		}
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

asynStatus acq400_PermitLut::writeInt16Array(
	asynUser *pasynUser, epicsInt16 *value,	size_t nElements)
{
	int function = pasynUser->reason;
	asynStatus status = asynSuccess;
	const char *paramName;
	int addr = 0;

	/* Fetch the parameter string name for possible use in debugging */
	getParamName(function, &paramName);


	if (maxAddr > 1){
		status = pasynManager->getAddr(pasynUser, &addr);
		if(status!=asynSuccess) return status;
	}

	assert(addr < MAX_ROWS);

	if (function == P_PRMT_LUT_TH_AI){
		const SamplePrams* sp = acq400_Proxy::getSamplePrams();
		const size_t expected_nchan = sp->AI_COUNT * NPAIR;

		if (nElements == expected_nchan){
			if (lut_rows[addr].ai_th_pairs == 0){
				lut_rows[addr].ai_th_pairs = new epicsInt16[nElements];
			}
			memcpy(lut_rows[addr].ai_th_pairs, value, nElements*sizeof(epicsInt16));
		}else{
			status = asynError;
		}
	}

	if (status)
		epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
				"%s:%s: status=%d, function=%d, name=%s, value=%p",
				DN, FN, status, function, paramName, value);
	else
		asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
				"%s:%s: function=%d, name=%s, value=%p\n",
				DN, FN, function, paramName, value);
	return status;
}
asynStatus acq400_PermitLut::doCallbacksInt16Array(
	epicsInt16 *value, size_t nElements, int reason, int addr)
{
	// no task() -> no callbacks (so far)
	return asynSuccess;
}

asynStatus acq400_PermitLut::writeInt32Array(
	asynUser *pasynUser, epicsInt32 *value,	size_t nElements)
{
	int function = pasynUser->reason;
	asynStatus status = asynSuccess;
	const char *paramName;
	int addr = 0;

	/* Fetch the parameter string name for possible use in debugging */
	getParamName(function, &paramName);


	if (maxAddr > 1){
		status = pasynManager->getAddr(pasynUser, &addr);
		if(status!=asynSuccess) return status;
	}

	assert(addr < MAX_ROWS);

	if (function == P_PRMT_LUT_TH_DI){
		const SamplePrams* sp = acq400_Proxy::getSamplePrams();
		const size_t expected_nchan = sp->DI_COUNT * NPAIR;

		if (nElements == expected_nchan){
			if (lut_rows[addr].di_pairs == 0){
				lut_rows[addr].di_pairs = new epicsInt32[nElements];
			}
			memcpy(lut_rows[addr].di_pairs, value, nElements*sizeof(epicsInt32));
		}else{
			status = asynError;
		}
	}

	if (status)
		epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
				"%s:%s: status=%d, function=%d, name=%s, value=%p",
				DN, FN, status, function, paramName, value);
	else
		asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
				"%s:%s: function=%d, name=%s, value=%p\n",
				DN, FN, function, paramName, value);
	return status;
}
asynStatus acq400_PermitLut::doCallbacksInt32Array(
		epicsInt32 *value, size_t nElements, int reason, int addr)
{
	// no task() -> no callbacks (so far)
	return asynSuccess;
}

void acq400_PermitLut::check_if_fmt_event_trigger(const FMT& fmt)
/**< nested linear search for first event match.
 * max FMT_ROWS*MAX_ROWS iterations: 64*16 = 1024. That's quite small, so probably not a problem
 * @@todo would like to use a std::set() but it's difficult to set up.
 * "first make it work, then make it work fast"
 */
{
	lock();
	for (int fmt_row = 0; fmt_row < FMT_ROWS; ++fmt_row){
		const epicsUInt16 fmt_event = fmt[fmt_row].event;
		if (fmt_event == EV99){
			break;
		}
		for (int pl_row = 0; pl_row < MAX_ROWS; ++pl_row){
			const PRMT_LUT_ROW& plr = lut_rows[pl_row];
			if (fmt_event == plr.event){
				PermitDSP::instance().full_load(
						plr.ai_th_pairs, plr.di_pairs);
				break;
			}
		}
	}
	unlock();
}


acq400_PermitLut *acq400_PermitLut::_instance;

acq400_PermitLut& acq400_PermitLut::instance() {
	assert(acq400_PermitLut::_instance != 0);
	return *acq400_PermitLut::_instance;
}
void acq400_PermitLut::create_instance(const char *portName, int max_addr)
{
	assert(acq400_PermitLut::_instance == 0);
	_instance = new acq400_PermitLut(portName, max_addr);
}
extern "C" {
	/** EPICS iocsh callable function to call constructor for the testAsynPortDriver class.
	  * \param[in] portName The name of the asyn port driver to be created.
	  */
	int acq400_PermitLut_Configure(const char *portName, int max_addr)
	{
		printf("%s:%s R1002 %s %d\n", DN, FN, portName, max_addr);

		acq400_PermitLut::create_instance(portName, max_addr);
		return 0;
	}

	/* EPICS iocsh shell commands */

	static const iocshArg initArg0 = { "port", iocshArgString };
	static const iocshArg initArg1 = { "site", iocshArgInt };

	static const iocshArg * const initArgs[] = {
			&initArg0, &initArg1
	};
	static const iocshFuncDef initFuncDef = { "acq400_PermitLut_Configure", 2, initArgs };
	static void initCallFunc(const iocshArgBuf *args)
	{
		acq400_PermitLut_Configure(args[0].sval, args[1].ival);
	}

	void acq400_PermitLut_Register(void)
	{
	    iocshRegister(&initFuncDef, initCallFunc);
	}

	epicsExportRegistrar(acq400_PermitLut_Register);
}

