/** @file acq400_Permit.cpp
 *  @brief acq400_Permit implementation.
 *
 *  Strategy:
 + Permit Prams take up 0x300 BRAM space.
 + task will update Permit Prams at a suitable moment (right after DMA done)
 + updating the BRAM _must_ be as fast as possible. We propose a raw cache (R_cache) scheme so that
 + only dirty words are written.
 + We have up to 128*4 = 512 discrete AI logical prams. These are stored by in a backing store
 + L_cache, and must be converted to RAW data while the R_cache is not being flushed.
 The sequence is then:

 Records thread:
 + user Locks the LR update
 + user writes as many LPrams as required, filling the LCache, and marking dirty entries
 + user Unlocks LR
 + task: wakes on DMA done, flushes R-cache with highest priority.
  + then, if LR_UPDATE is unlocked, runs through the L_cache "cooking" L-values
  + (perhaps using Anatrg) to generate Raw ABCD values
 *  Created on: 14 Aug 2026
 *      Author: pgm
 */

#include "acq400_Permit.h"
#include "acq400_Proxy.h"
#include "anatrg.h"
#include "acq-util.h"
#include "split2.h"
#include <fcntl.h>                // open()
#include <unistd.h>
#include <string.h>

using namespace std;

#define DN	__FILE__
#define FN	__FUNCTION__

#define MARK	fprintf(stderr, "%s %d\n", FN, __LINE__)
#define MARKI(p) fprintf(stderr, "%s %d P_ %s:%d\n", FN, __LINE__, #p, p)



PermitDSP::PermitDSP() {
	/* make the mapping */
}
PermitDSP& PermitDSP::instance(){
	static PermitDSP* _instance;

	if (_instance == 0){
		_instance = new PermitDSP;
	}
	return *_instance;
}


bool PermitDSP::defineSite(int site, int nelem)
{
	assert(sites[site].site == 0);
	sites[site].site = site;         /** @@todo redundant .. struct not needed? */
	sites[site].nelem = nelem;
	return true;
}

int acq400_Permit::nice    = ::getenv_default("acq400_PERMIT_NICE", 10);
int acq400_Permit::verbose	= ::getenv_default("acq400_PERMIT_VERBOSE", 0);

#define DI_NELEM	2	/**< active, state */

std::map<int, acq400_Permit*> acq400_Permit::peers;

acq400_Permit::acq400_Permit(const char *portName, int _site, bool is_ai, int _nchan):
	acq400_asynPortDriver(portName,
	/* maxAddr */		1,
	/* Interface mask */    asynEnumMask|asynOctetMask|asynInt32Mask|asynInt64Mask|asynFloat64Mask|
				asynDrvUserMask,
	/* Interrupt mask */	asynEnumMask|asynOctetMask|asynInt32Mask|asynInt64Mask|asynFloat64Mask,
	/* asynFlags no block*/ 0,
	/* Autoconnect */       1,
	/* Default priority */  0,
	/* Default stack size*/ 0),
	dsp(PermitDSP::instance()),
	site(_site),
	nchan(_nchan)
{
	asynStatus status = asynSuccess;

	createParam(PS_PERMIT_TRG_STAT,    	asynParamInt32,      &P_PERMIT_TRG_STAT);
	createParam(PS_PERMIT_TRG_LIVE,    	asynParamInt32,      &P_PERMIT_TRG_LIVE);


	createParam(PS_PERMIT_COMMIT,    	asynParamInt32,      &P_PERMIT_COMMIT);
	createParam(PS_PERMIT_LOCK,    		asynParamInt32,      &P_PERMIT_LOCK);

	if (is_ai){
		dsp.defineSite(site, nchan);
	}else{
		dsp.defineSite(site, DI_NELEM);
	}
	if (peers.size() == 0){
		/* first unit updates L_CACHE and controls the R_CACHE for all */
		status = (asynStatus)(epicsThreadCreate("PRMT_task",
				epicsThreadPriorityHigh - nice,
				epicsThreadGetStackSize(epicsThreadStackMedium),
				(EPICSTHREADFUNC)task_runner,
				this) == NULL);
		if (status) {
			printf("%s:%s: epicsThreadCreate failure\n", DN, FN);
			return;
		}
	}
}

void acq400_Permit::task_runner(void *drvPvt)
{
	((acq400_Permit *)drvPvt)->task();
}

void acq400_Permit::task()
{
	fprintf(stderr, "%s 01\n", FN);
}


asynStatus acq400_Permit::writeInt32(asynUser *pasynUser, epicsInt32 value)
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

	    return status;
}



class acq400_Permit_AI: public acq400_Permit {
	int P_PERMIT_ATD_MODE;
	int P_PERMIT_ATD_L1;
	int P_PERMIT_ATD_L2;
	int P_PERMIT_ATD_HP;


	ATD* atd;
	AnaTrg anatrg;

public:
	acq400_Permit_AI(const char *portName, int site, int nchan):
		acq400_Permit(portName, site, true, nchan),
		anatrg(nchan, 0, 0)
	{
		createParam(PS_PERMIT_ATD_MODE,    	asynParamInt32,      &P_PERMIT_ATD_MODE);
		createParam(PS_PERMIT_ATD_L1,		asynParamFloat64,    &P_PERMIT_ATD_L1);
		createParam(PS_PERMIT_ATD_L2,		asynParamFloat64,    &P_PERMIT_ATD_L2);
		createParam(PS_PERMIT_ATD_HP,    	asynParamInt32,      &P_PERMIT_ATD_HP);

		atd = new ATD[nchan+1];			/**< index 0 = ALL */
		memset(atd, 0, (nchan+1)*sizeof(ATD));
	}

	virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
	virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);

	virtual void flush_L_CACHE() {

		const epicsFloat32* eslo = acq400_Proxy::instance().getEslo(site);
		const epicsFloat32* eoff = acq400_Proxy::instance().getEslo(site);

		if (dirty_L_CACHE){
			unsigned l_cache = dirty_L_CACHE;
			for (int ic = 0; l_cache; ++ic, l_cache>>=1){
				if (l_cache&1){
					const int ch = ic;
					char l1 = static_cast<int>((atd[ic].atd_v1 - eoff[ch])/eslo[ch])>>8;
					char l2 = static_cast<int>((atd[ic].atd_v2 - eoff[ch])/eslo[ch])>>8;
					int hyst = 1; /**< @@todo */

					dsp.setElement(site, ic, (*anatrg.commands[atd[ic].atd_mode])(l1, l2, hyst));
				}
			}
			fprintf(stderr, "%s STUB\n", FN);
		}
	}
};


class acq400_Permit_DI: public acq400_Permit {
	int P_PERMIT_DI_CH_ACTIVE;
	int P_PERMIT_DI_CH_VALUE;
	DTD dtd;
public:
	acq400_Permit_DI(const char *portName, int site):
		acq400_Permit(portName, site, false, 2)
	{
		createParam(PS_PERMIT_DI_CH_ACTIVE,    	asynParamInt32,      &P_PERMIT_DI_CH_ACTIVE);
		createParam(PS_PERMIT_DI_CH_VALUE,    	asynParamInt32,      &P_PERMIT_DI_CH_VALUE);
	}

	virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

	virtual void flush_L_CACHE() {
		if (dirty_L_CACHE){
			dsp.setElement(site, 0, dtd.dtd_active);
			dsp.setElement(site, 1, dtd.dtd_value);
		}
	}
};



asynStatus acq400_Permit_DI::writeInt32(asynUser *pasynUser, epicsInt32 value)
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

	    return status;
}

asynStatus acq400_Permit_AI::writeInt32(asynUser *pasynUser, epicsInt32 value)
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

	    return status;
}

asynStatus acq400_Permit_AI::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
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
	    	              "%s:%s: function=%d, addr=%d, name=%s, value=%.2f\n",
	    	              DN, FN, function, addr, paramName, value);

	    return status;
}

void acq400_Permit::create_instance(const char *portName, int site, bool is_ai, int nchan)
{
	if (peers[site] == 0){
		if (is_ai){
			peers[site] = new acq400_Permit_AI(portName, site, nchan);
		}else{
			peers[site] = new acq400_Permit_DI(portName, site);
		}
	}
}

extern "C" {
	/** EPICS iocsh callable function to call constructor for the testAsynPortDriver class.
	  * \param[in] portName The name of the asyn port driver to be created.
	  */
	int acq400_Permit_Configure(const char *portName, int site, int is_ai, int nchan)
	{
		printf("%s:%s R1002 %s\n", DN, FN, portName);

		acq400_Permit::create_instance(portName, site, is_ai, nchan);
		return 0;
	}

	/* EPICS iocsh shell commands */

	static const iocshArg initArg0 = { "port", iocshArgString };
	static const iocshArg initArg1 = { "site", iocshArgInt };
	static const iocshArg initArg2 = { "is_ai", iocshArgInt };
	static const iocshArg initArg3 = { "nchan", iocshArgInt };
	static const iocshArg * const initArgs[] = {
			&initArg0, &initArg1, &initArg2, &initArg3 };
	static const iocshFuncDef initFuncDef = { "acq400_Permit_Configure", 4, initArgs };
	static void initCallFunc(const iocshArgBuf *args)
	{
		acq400_Permit_Configure(args[0].sval, args[1].ival, args[2].ival, args[3].ival);
	}

	void acq400_Permit_Register(void)
	{
	    iocshRegister(&initFuncDef, initCallFunc);
	}

	epicsExportRegistrar(acq400_Permit_Register);
}




