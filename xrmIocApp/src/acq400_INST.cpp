/*
 * acq400_INST.cpp
 *
 *  Created on: 9 Apr 2026
 *      Author: pgm
 */

#include "acq400_asyn_common.h"
#include "acq400_INST.h"

#include "acq-util.h"
#include "split2.h"
#include <fcntl.h>                // open()
#include <unistd.h>
#include <string.h>

using namespace std;
#include "Buffer.h"
#include "ES.h"

static const char *driverName="acq400_INST";

#define DN	driverName
#define FN	__FUNCTION__

#define MARK	fprintf(stderr, "%s %d\n", FN, __LINE__)
#define MARKI(p) fprintf(stderr, "%s %d P_ %s:%d\n", FN, __LINE__, #p, p)

int acq400_INST::nice = ::getenv_default("acq400_INST_NICE", 0);


acq400_INST::acq400_INST(const char* portName):
	acq400_asynPortDriver(portName,
	/* maxAddr */		1,
	/* Interface mask */    asynEnumMask|asynOctetMask|asynInt32Mask|asynInt64Mask|asynFloat64Mask|
				asynDrvUserMask,
	/* Interrupt mask */	asynEnumMask|asynOctetMask|asynInt32Mask|asynInt64Mask|asynFloat64Mask,
	/* asynFlags no block*/ 0,
	/* Autoconnect */       1,
	/* Default priority */  0,
	/* Default stack size*/ 0)
{
	fprintf(stderr, "%s R1040\n", FN);

	createParam(PS_INST_STRATEGY, asynParamInt32,   &P_INST_STRATEGY);
	createParam(PS_REDIS_HOST,    asynParamOctet,   &P_REDIS_HOST);
	createParam(PS_REDIS_PORT,    asynParamOctet,   &P_REDIS_PORT);
	createParam(PS_REDIS_MKEY,    asynParamOctet,	&P_REDIS_MKEY);
	createParam(PS_REDIS_BCOUNT,  asynParamInt32,   &P_REDIS_BCOUNT);
	createParam(PS_REDIS_STATUS,  asynParamOctet,   &P_REDIS_STATUS);
	createParam(PS_REDIS_MMKEY,   asynParamOctet,   &P_REDIS_MMKEY);
}

class acq400_INST_MASTER: public acq400_INST {
public:
	acq400_INST_MASTER(const char* portName): acq400_INST(portName) {}
};

class acq400_INST_SPY: public acq400_INST {
public:
	acq400_INST_SPY(const char* portName): acq400_INST(portName) {}
};
extern "C" {
	/** EPICS iocsh callable function to call constructor for the testAsynPortDriver class.
	  * \param[in] portName The name of the asyn port driver to be created.
	  */
	int acq400_INST_Configure(const char *portName, const char* stream_or_spy)
	{
		printf("%s:%s R1001 %s\n", DN, FN, portName);

		if (strcmp(stream_or_spy, "STR") == 0){
			new acq400_INST_MASTER(portName);
		}else if (strcmp(stream_or_spy, "SPY") == 0){
			new acq400_INST_SPY(portName);
		}else{
			fprintf(stderr, "ERROR: arg \"%s\" not \"STR\" or \"SPY\"\n");
			return -1;
		}
		return 0;
	}

	/* EPICS iocsh shell commands */

	static const iocshArg initArg0 = { "port", iocshArgString };
	static const iocshArg initArg1 = { "STR", iocshArgString };
	static const iocshArg * const initArgs[] = { &initArg0, &initArg1 };
	static const iocshFuncDef initFuncDef = { "acq400_INST_Configure", 2, initArgs };
	static void initCallFunc(const iocshArgBuf *args)
	{

		acq400_INST_Configure(args[0].sval,args[1].sval);
	}

	void acq400_INST_Register(void)
	{
	    iocshRegister(&initFuncDef, initCallFunc);
	}

	epicsExportRegistrar(acq400_INST_Register);
}


