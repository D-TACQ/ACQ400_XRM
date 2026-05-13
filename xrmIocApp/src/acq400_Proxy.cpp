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


epicsFloat32** init_ecal_src() {
	epicsFloat32** ecal_src = new epicsFloat32*[MAX_ADDR]; // site index from 1
	for (int site = 0; site < MAX_ADDR; ++site){
		ecal_src[site] = 0;  // prep for lazy init
	}
	return ecal_src;
}
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
	/* Default stack size*/ 0),
	eslo_src(init_ecal_src()),
	eoff_src(init_ecal_src()),
	eslo_dst(0),
	eoff_dst(0)
{
	asynStatus status = asynSuccess;

	memset(ai_site_lengths, 0, sizeof(ai_site_lengths));

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
	printf("INFO: %s:%s 01\n", DN, FN);

	size_t agglen = 0;

	for (int site = FIRST_SITE; site <= LAST_SITE; ++site){
		if (ai_site_lengths[site] > 1){
			agglen += ai_site_lengths[site] -1;
		}
	}
	if (!agglen){
		return;
	}
	agglen += 1;

	//fprintf(stderr, "%s:%s() computed agglen:%u\n", DN, FN, agglen);

	if (ai_site_lengths[0] == 0){
		assert(eslo_dst == 0);
		assert(eoff_dst == 0);

		//fprintf(stderr, "%s:%s() lazy init agglen:%u\n", DN, FN, agglen);

		eslo_dst = new epicsFloat32[agglen];
		eoff_dst = new epicsFloat32[agglen];
		ai_site_lengths[0] = agglen;

		eslo_dst[0] = agglen*1.0;                    // a good use for [0]
		eoff_dst[0] = agglen*1.0;
	}else if (ai_site_lengths[0] != agglen){
		fprintf(stderr, "WARNING: ai_site_lengths mismatch %u %u\n",
				ai_site_lengths[0], agglen);
		return;
	}

	const int max_dst = ai_site_lengths[0];
	int ito = 1;    // skip first element. CH index from 1..

	for (int site = FIRST_SITE; site <= LAST_SITE; ++site){
		if (ai_site_lengths[site] == 0){
			continue;
		}
		const int site_n = ai_site_lengths[site] - 1;
		const int site_bytes = site_n*sizeof(epicsFloat32);
		if (site_bytes == 0){
			continue;
		}
		fprintf(stderr, "%s:%s() site:%d ito:%d  site_n:%d max_dst:%d\n",
				DN, FN, site, ito, site_n, max_dst);

		if (!(ito+site_n <= max_dst)){
			fprintf(stderr, "%s:%s() ito:%d + site_n:%d <= max_dst:%d, avoid assert but skip\n",
					DN, FN, ito, site_n, max_dst);
			return;
		}
		assert(ito+site_n <= max_dst);

		memcpy(eslo_dst+ito, eslo_src[site]+1, site_bytes);
		memcpy(eoff_dst+ito, eoff_src[site]+1, site_bytes);
		ito += site_n;
	}
	if (ito > 1){
		doCallbacksFloat32Array(eslo_dst, ito, P_AI_CAL_ESLO, 0);
		doCallbacksFloat32Array(eoff_dst, ito, P_AI_CAL_EOFF, 0);
	}
	printf("INFO: %s:%s 99\n", DN, FN);
}
void acq400_Proxy::task()
{
	while(1){
		printf("INFO: %s:%s wait Event\n", DN, FN);
		epicsEventWait(eventId);
		printf("INFO: %s:%s wait done\n", DN, FN);

		lock();
		get_sample_dimensions();
		get_cal();
		unlock();

		printf("INFO: %s:%s cal done\n", DN, FN);
	}
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


asynStatus acq400_Proxy::readFloat32Array(asynUser *pasynUser, epicsFloat32 *value,
                                       size_t nElements, size_t *nIn)
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

	printf("INFO: %s:%s() function:%d addr:%d  nElements:%u\n",
			DN, FN, function, addr, nElements);

	if (addr == 0){
		lock();
		epicsFloat32** local_dst = 0;
		epicsFloat32 nominal;

		if (function == P_AI_CAL_ESLO){
			local_dst = &eslo_dst;
			nominal = 10/32768;
		}else if (function == P_AI_CAL_EOFF){
			local_dst = &eoff_dst;
			nominal = 0;
		}else{
			assert(function == P_AI_CAL_ESLO || function == P_AI_CAL_EOFF);
		}
		if (*local_dst == 0){
			// lazy init. first time will report zero
			epicsFloat32* pa = new epicsFloat32[nElements];
			for (size_t ii = 0; ii != nElements; ++ii){
				pa[ii] = nominal;
			}
			assert(ai_site_lengths[addr] == 0 || ai_site_lengths[addr] == nElements);
			ai_site_lengths[addr] = nElements;
			*local_dst = pa;

			printf("INFO: %s:%s() function:%d addr:%d lazy init: %p:%u nElements:%u\n",
						DN, FN, function, addr, pa, ai_site_lengths[addr], nElements);

		}
		//doCallbacksFloat32Array(*local_dst, nElements, function, addr);
		unlock();
		epicsEventSignal(eventId);
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
asynStatus acq400_Proxy::writeFloat32Array(asynUser *pasynUser, epicsFloat32 *value,
                                       size_t nElements)
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

	if (function == P_AI_CAL_ESLO || function == P_AI_CAL_EOFF){
		lock();
		assert(addr >= FIRST_SITE && addr <= LAST_SITE);
		epicsFloat32* copy_to;

		if (function == P_AI_CAL_ESLO){
			if (eslo_src[addr] == 0){
				eslo_src[addr] = new epicsFloat32[nElements];
				assert(ai_site_lengths[addr] == 0 || ai_site_lengths[addr] == nElements);
				ai_site_lengths[addr] = nElements;

				printf("INFO: %s:%s() function:%d addr:%d lazy init: %p:%u nElements:%u\n",
					DN, FN, function, addr, eslo_src[addr], ai_site_lengths[addr], nElements);
			}
			copy_to = eslo_src[addr];
		}else{
			if (eoff_src[addr] == 0){
				eoff_src[addr] = new epicsFloat32[nElements];
				assert(ai_site_lengths[addr] == 0 || ai_site_lengths[addr] == nElements);
				ai_site_lengths[addr] = nElements;

				printf("INFO: %s:%s() function:%d addr:%d lazy init: %p:%u nElements:%u\n",
					DN, FN, function, addr, eoff_src[addr], ai_site_lengths[addr], nElements);
			}
			copy_to = eoff_src[addr];
		}
		memcpy(copy_to, value, nElements*sizeof(epicsFloat32));
		unlock();
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
