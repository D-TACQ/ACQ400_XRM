/*
 * acq400_PermitLut.cpp
 *
 *  Created on: 19 Aug 2026
 *      Author: pgm
 */

#include "acq400_PermitLut.h"

#define DN	__FILE__
#define FN	__FUNCTION__

acq400_PermitLut::acq400_PermitLut(const char *portName, int _max_addr):
	acq400_asynPortDriver(portName,
	/* maxAddr */		_max_addr,
	/* Interface mask */    asynEnumMask|asynOctetMask|asynInt32Mask|asynInt64Mask|asynFloat64Mask|asynUInt32DigitalMask|
				asynDrvUserMask,
	/* Interrupt mask */	asynEnumMask|asynOctetMask|asynInt32Mask|asynInt64Mask|asynFloat64Mask|asynUInt32DigitalMask,
	/* asynFlags no block*/ 0,
	/* Autoconnect */       1,
	/* Default priority */  0,
	/* Default stack size*/ 0),
	dsp(PermitDSP::instance())
{

}
void acq400_PermitLut::create_instance(const char *portName, int max_addr)
{
	new acq400_PermitLut(portName, max_addr);
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

