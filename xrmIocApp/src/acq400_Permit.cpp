/*
 * acq400_Permit.cpp
 *
 *  Created on: 14 Aug 2026
 *      Author: pgm
 */

#include "acq400_Permit.h"
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

int acq400_Permit::nice    = ::getenv_default("acq400_PERMIT_NICE", 10);
int acq400_Permit::verbose	= ::getenv_default("acq400_PERMIT_VERBOSE", 0);


std::map<int, acq400_Permit*> acq400_Permit::peers;

acq400_Permit::acq400_Permit(const char *portName, int site, bool is_ai, int nchan):
	acq400_asynPortDriver(portName,
	/* maxAddr */		1,
	/* Interface mask */    asynEnumMask|asynOctetMask|asynInt32Mask|asynInt64Mask|asynFloat64Mask|
				asynDrvUserMask,
	/* Interrupt mask */	asynEnumMask|asynOctetMask|asynInt32Mask|asynInt64Mask|asynFloat64Mask,
	/* asynFlags no block*/ 0,
	/* Autoconnect */       1,
	/* Default priority */  0,
	/* Default stack size*/ 0)
{}

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

asynStatus acq400_Permit::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
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
		peers[site] = new acq400_Permit(portName, site, is_ai, nchan);
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




