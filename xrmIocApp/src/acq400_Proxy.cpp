/*
 * acq400_Proxy.cpp
 *
 *  Created on: 12 May 2026
 *      Author: pgm
 */

#ifndef XRMIOCAPP_SRC_ACQ400_PROXY_CPP_
#define XRMIOCAPP_SRC_ACQ400_PROXY_CPP_

#include "acq400_Proxy.h"
#include "acq-util.h"
#include "split2.h"

#define DN	__FILE__
#define FN	__FUNCTION__

#define MARK	fprintf(stderr, "%s %d\n", FN, __LINE__)
#define MARKI(p) fprintf(stderr, "%s %d P_ %s:%d\n", FN, __LINE__, #p, p)

#define MAX_SITES 6
#define MAX_ADDR  (MAX_SITES+1)    // site 0 and selection of 1..6

acq400_Proxy::acq400_Proxy(const char* portName):
	acq400_asynPortDriver(portName,
	/* maxAddr */		MAX_ADDR,
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

	createParam(PS_SMPL_AGG_SITES,	asynParamOctet,      &P_SMPL_AGG_SITES);
	createParam(PS_SMPL_SITE_SSB,	asynParamInt32,      &P_SMPL_SITE_SSB);
	createParam(PS_SMPL_SITE_IS_ADC,asynParamInt32,      &P_SMPL_SITE_IS_ADC);
	createParam(PS_SMPL_SS_U32,	asynParamInt32,      &P_SMPL_SS_U32);
	createParam(PS_SMPL_NSAM,	asynParamInt32,      &P_SMPL_NSAM);
	createParam(PS_SMPL_AI_COUNT,	asynParamInt32,	     &P_SMPL_AI_COUNT);
	createParam(PS_SMPL_DI_COUNT,	asynParamInt32,	     &P_SMPL_DI_COUNT);
	createParam(PS_SMPL_SP_COUNT,	asynParamInt32,	     &P_SMPL_SP_COUNT);
	createParam(PS_SMPL_DI_INDEX, 	asynParamInt32,	     &P_SMPL_DI_INDEX);
	createParam(PS_SMPL_SP_INDEX, 	asynParamInt32,	     &P_SMPL_SP_INDEX);

	createParam(PS_AI_CAL_ESLO,     asynParamFloat32Array, &P_AI_CAL_ESLO);
	createParam(PS_AI_CAL_EOFF,     asynParamFloat32Array, &P_AI_CAL_EOFF);

	/* Create the thread that computes the waveforms in the background */
	status = (asynStatus)(epicsThreadCreate("acq400_Proxy_task",
			epicsThreadPriorityLow,
			epicsThreadGetStackSize(epicsThreadStackMedium),
			(EPICSTHREADFUNC)task_runner,
			this) == NULL);
	if (status) {
		printf("%s:%s: epicsThreadCreate failure\n", DN, FN);
		return;
	}
}

void acq400_Proxy::task_runner(void *drvPvt)
{
	((acq400_Proxy *)drvPvt)->task();
}

typedef std::vector<std::string> VS;


void acq400_Proxy::get_sample_dimensions()
{
	char site_list[80] = {};
	gsp(P_SMPL_AGG_SITES, 80, site_list);
	fprintf(stderr, "SMPL_AGG_SITES \"%s\"\n", site_list);

	int modules_ssb_total = 0;
	int first_di_index = 0;
	int ssb;
	bool first_di = true;
	int modules_ai_ssb = 0;
	int modules_di_ssb = 0;

	VIS agg_sites;
	split2(site_list, agg_sites, ',');

	for (int site: agg_sites){
		int is_adc;

		gip(site, P_SMPL_SITE_IS_ADC, &is_adc);

		if (!is_adc && first_di){
			first_di_index = modules_ssb_total/sizeof(short);
			sip(0, P_SMPL_DI_INDEX, first_di_index);
			first_di = false;
		}


		gip(site, P_SMPL_SITE_SSB, &ssb);
		modules_ssb_total += ssb;
		if (is_adc){
			modules_ai_ssb += ssb;
		}else{
			modules_di_ssb += ssb;
		}

		fprintf(stderr, "%s:%s %d ssb:%d is_adc?:%d first_di_index:%d modules_ssb_total:%d\n",
				DN, FN, site, ssb, is_adc, first_di_index, modules_ssb_total);
	}
	gip(0, P_SMPL_SITE_SSB, &ssb);
	samplePrams.SSB = ssb;
	gip(0, P_SMPL_NSAM, &samplePrams.NSAM);


	int modules_ssl = modules_ssb_total/sizeof(long);
	int agg_ssl = ssb/sizeof(long);
	assert(agg_ssl >= modules_ssl);


	samplePrams.AI_INDEX = 0;
	sip(0, P_SMPL_AI_COUNT, samplePrams.AI_COUNT = modules_ai_ssb/sizeof(AI16_t));

	sip(0, P_SMPL_DI_INDEX, samplePrams.DI_INDEX = modules_ai_ssb/sizeof(DI32_t));
	sip(0, P_SMPL_DI_COUNT, samplePrams.DI_COUNT = modules_di_ssb/sizeof(DI32_t));

	sip(0, P_SMPL_SP_INDEX, samplePrams.SP_INDEX = modules_ssl);
	sip(0, P_SMPL_SP_COUNT, samplePrams.SP_COUNT = agg_ssl-modules_ssl);

	samplePrams.validate();

	callParamCallbacks();
	SamplePrams::store(samplePrams);
}

void acq400_Proxy::get_cal()
{
	printf("INFO: %s:%s STUB\n", DN, FN);
}
void acq400_Proxy::task()
{
	printf("INFO: %s:%s wait Event\n", DN, FN);
	epicsEventWait(eventId);
	printf("INFO: %s:%s wait done\n", DN, FN);
	get_sample_dimensions();
	get_cal();
}

asynStatus acq400_Proxy::writeInt32(asynUser *pasynUser, epicsInt32 value)
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

	fprintf(stderr,
			"%s:%s: function=%d, addr=%d, name=%s, value=%d\n",
			DN, FN, function, addr, paramName, value);

	if (function == P_RUNSTOP) {
		if (value == 1){
			epicsEventSignal(eventId);
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


acq400_Proxy* acq400_Proxy::_instance;

void acq400_Proxy::create_instance(const char *portName)
{
	assert(_instance == 0);
	_instance = new acq400_Proxy(portName);
}

const SamplePrams* get_acq400_SamplePrams() {
	return acq400_Proxy::getSamplePrams();
}
extern "C" {
	/** EPICS iocsh callable function to call constructor for the testAsynPortDriver class.
	  * \param[in] portName The name of the asyn port driver to be created.
	  */
	int acq400_Proxy_Configure(const char *portName)
	{
		printf("%s:%s R1002 %s\n", DN, FN, portName);

		acq400_Proxy::create_instance(portName);
		return 0;
	}

	/* EPICS iocsh shell commands */

	static const iocshArg initArg0 = { "port", iocshArgString };
	static const iocshArg * const initArgs[] = { &initArg0 };
	static const iocshFuncDef initFuncDef = { "acq400_Proxy_Configure", 1, initArgs };
	static void initCallFunc(const iocshArgBuf *args)
	{
		acq400_Proxy_Configure(args[0].sval);
	}

	void acq400_Proxy_Register(void)
	{
	    iocshRegister(&initFuncDef, initCallFunc);
	}

	epicsExportRegistrar(acq400_Proxy_Register);
}


#endif /* XRMIOCAPP_SRC_ACQ400_PROXY_CPP_ */
