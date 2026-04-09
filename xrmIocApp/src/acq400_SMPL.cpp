/*
 * acq400_SMPL.cpp :: asyn driver for SMPL Sample Dims.
 *
 *  Created on: 6 Apr 2026
 *      Author: pgm
 */


#include "acq400_asyn_common.h"
#include "acq400_SMPL.h"
#include "acq-util.h"
#include "split2.h"
#include <fcntl.h>                // open()
#include <unistd.h>
#include <string.h>

static const char *driverName="acq400_SMPL";

#define DN	driverName
#define FN	__FUNCTION__


typedef std::vector<std::string> VS;

acq400_SMPL::acq400_SMPL(const char* portName):
	acq400_asynPortDriver(portName,
	/* maxAddr */		1,  /* SITE_SSB forces: MB + 6 sites */
	/* Interface mask */    asynEnumMask|asynOctetMask|asynInt32Mask|asynInt64Mask|asynFloat64Mask,
	/* Interrupt mask */	asynEnumMask|asynOctetMask|asynInt32Mask|asynInt64Mask|asynFloat64Mask,
	/* asynFlags no block*/ 0,
	/* Autoconnect */       1,
	/* Default priority */  0,
	/* Default stack size*/ 0)
{
	fprintf(stderr, "acq400_SMPL::acq400_SMPL v1001 %s PGMCOMOUT\n", portName);
	/*
	createParam(PS_SOE_SITE_SSB,		asynParamInt32,      &P_SITE_SSB);
	createParam(PS_SOE_SITE_IS_ADC,		asynParamInt32,      &P_SITE_IS_ADC);
	createParam(PS_SOE_SMPL_SS_U32,		asynParamInt32,      &P_SMPL_SS_U32);
	createParam(PS_SOE_SMPL_NSAM,		asynParamInt32,      &P_SMPL_NSAM);
	createParam(PS_SOE_SMPL_AI_COUNT,	asynParamInt32,	     &P_SMPL_AI_COUNT);
	createParam(PS_SOE_SMPL_DI_COUNT,	asynParamInt32,	     &P_SMPL_DI_COUNT);
	createParam(PS_SOE_SMPL_SP_COUNT,	asynParamInt32,	     &P_SMPL_SP_COUNT);
	createParam(PS_SOE_SMPL_DI_INDEX, 	asynParamInt32,	     &P_SMPL_DI_INDEX);
	createParam(PS_SOE_SMPL_SP_INDEX, 	asynParamInt32,	     &P_SMPL_SP_INDEX);
	createParam(PS_SOE_AGG_SITES,		asynParamOctet,      &P_AGG_SITES);
	setStringParam(P_AGG_SITES, "0000");
	*/
	/* Create the thread that computes the waveforms in the background */
	asynStatus status = (asynStatus)(epicsThreadCreate("SMPL_task",
			epicsThreadPriorityLow,
			epicsThreadGetStackSize(epicsThreadStackMedium),
			(EPICSTHREADFUNC)task_runner,
			this) == NULL);
	if (status) {
		printf("%s:%s: epicsThreadCreate failure\n", DN, FN);
		return;
	}
}

void acq400_SMPL::task_runner(void *drvPvt)
{
	((acq400_SMPL *)drvPvt)->task();
}

void acq400_SMPL::task()
{
	fprintf(stderr, "acq400_SMPL::task 01\n");
	epicsEventWait(eventId);
	fprintf(stderr, "acq400_SMPL::task 02\n");
	while(1){
		epicsEventWait(eventId);
		fprintf(stderr, "acq400_SMPL::task 03\n");
	}
}


void acq400_SMPL::get_sample_dimensions()
{
	char site_list[80] = { "1,2"};
	//gsp(P_AGG_SITES, 80, site_list);
//	fprintf(stderr, "P_AGG_SITES \"%s\"\n", site_list);

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

		gip(site, P_SITE_IS_ADC, &is_adc);

		if (!is_adc && first_di){
			first_di_index = modules_ssb_total/sizeof(short);
			sip(0, P_SMPL_DI_INDEX, first_di_index);
			first_di = false;
		}


		gip(site, P_SITE_SSB, &ssb);
		modules_ssb_total += ssb;
		if (is_adc){
			modules_ai_ssb += ssb;
		}else{
			modules_di_ssb += ssb;
		}

		fprintf(stderr, "%s:%s %d ssb:%d is_adc?:%d first_di_index:%d modules_ssb_total:%d\n",
				DN, FN, site, ssb, is_adc, first_di_index, modules_ssb_total);
	}
	gip(0, P_SITE_SSB, &ssb);
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

	callParamCallbacks();
	SamplePrams::store(samplePrams);
}
#ifdef PGMCOMOUT
asynStatus acq400_SMPL::writeInt32(asynUser *pasynUser, epicsInt32 value)
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
	    if (value == 1){
		    fprintf(stderr, "STUB call to get_sample_dimensions()\n");
		    get_sample_dimensions();     // single thread
		    epicsEventSignal(eventId);   // if worker thread
	    }
	}

	sip(0, P_UPDATES, ++updates);

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

asynStatus acq400_SMPL::writeOctet(asynUser *pasynUser, const char *value,
                                    size_t nChars, size_t *nActual)
{
    int addr;
    int function;
    const char *paramName;
    asynStatus status = asynSuccess;
    static const char *functionName = "writeOctet";

    status = parseAsynUser(pasynUser, &function, &addr, &paramName);
    if (status != asynSuccess) return status;

    /* Set the parameter in the parameter library. */
    status = (asynStatus)setStringParam(addr, function, (char *)value);

     /* Do callbacks so higher layers see any changes */
    status = (asynStatus)callParamCallbacks(addr, addr);

    if (status)
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                  "%s:%s: status=%d, function=%d, name=%s, value=%s",
                  driverName, functionName, status, function, paramName, value);
    else
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s: function=%d, name=%s, value=%s\n",
              driverName, functionName, function, paramName, value);
    *nActual = nChars;

    fprintf(stderr,
                  "%s:%s: function=%d, name=%s, value=%s\n",
                  driverName, functionName, function, paramName, value);
    return status;
}
#endif

acq400_SMPL* acq400_SMPL::_instance;

acq400_SMPL* acq400_SMPL::instance()
{
	assert(_instance != 0);

	_instance->get_sample_dimensions();
	SamplePrams::store(_instance->samplePrams);
	return _instance;
}

void acq400_SMPL::create_instance(const char *portName)
{
	fprintf(stderr, "SamplePrams::create_instance() 01\n");
	assert(_instance == 0);
	_instance = new acq400_SMPL(portName);
}

SamplePrams& SamplePrams::get_instance() {
	fprintf(stderr, "SamplePrams::get_instance() 01\n");
	assert(acq400_SMPL::instance()->samplePrams.isValid());
	return acq400_SMPL::instance()->samplePrams;
}

extern "C" {
	/** EPICS iocsh callable function to call constructor for the testAsynPortDriver class.
	  * \param[in] portName The name of the asyn port driver to be created.
	  */
	int acq400_SMPL_Configure(const char *portName)
	{
		printf("%s:%s R1002 %s\n", DN, FN, portName);
		new acq400_SMPL(portName);
		//acq400_SMPL::create_instance(portName);
		return 0;
	}

	/* EPICS iocsh shell commands */

	static const iocshArg initArg0 = { "port", iocshArgString };
	static const iocshArg * const initArgs[] = { &initArg0 };
	static const iocshFuncDef initFuncDef = { "acq400_SMPL_Configure", 1, initArgs };
	static void initCallFunc(const iocshArgBuf *args)
	{
		acq400_SMPL_Configure(args[0].sval);
	}

	void acq400_SMPL_Register(void)
	{
	    iocshRegister(&initFuncDef, initCallFunc);
	}

	epicsExportRegistrar(acq400_SMPL_Register);
}
