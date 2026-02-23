/*
 * acq400_FMT_abc.cpp  Abstract Base Class - common base for SIM and RX
 *
 *  Created on: 16 Feb 2026
 *      Author: pgm
 */

#include "acq400_asyn_common.h"
#include "acq400_FMT_sim.h"
#include "acq-util.h"
#include <unistd.h>
#include <string.h>

namespace G {
	const char* fmt_mc_group = "224.0.23.200";
	const int fmt_mc_port = 5055;
}

static const char *driverName="acq400_FMT_abc";

#define DN	driverName
#define FN	__FUNCTION__


int acq400_FMT_abc::nice = ::getenv_default("acq400_FMT_NICE", 0);

acq400_FMT_abc::acq400_FMT_abc(
	const char *portName, int maxAddr, int interfaceMask, int interruptMask,
	                   int asynFlags, int autoConnect, int priority, int stackSize) :
	asynPortDriver(portName, maxAddr, interfaceMask, interruptMask,
			   asynFlags, autoConnect, priority, stackSize),
	update(0)
{
	memset(fmt, 0, sizeof(fmt));

	assert(FMT_ROWS < 0xffU);
	for (epicsInt8 row = 0; row < FMT_ROWS; ++row){
		cols.c_rownum[row] = row;
	}
	//update_fmt(true);

	eventId = epicsEventCreate(epicsEventEmpty);
	createParam(PS_RUNSTOP,  asynParamInt32,        &P_RUNSTOP);
	createParam(PS_UPDATES,  asynParamInt32,        &P_UPDATES);
	createParam(PS_TS_USEC,  asynParamInt64,	&P_TS_USEC);
	createParam(PS_FMT_MC_GRP,  asynParamOctet,	&P_FMT_MC_GRP);
	createParam(PS_FMT_MC_PORT,  asynParamInt32,	&P_FMT_MC_PORT);

	createParam(PS_FMT_COL_ROWNUM, asynParamInt8Array, &P_FMT_COL_ROWNUM);
	createParam(PS_FMT_COL_EVENT, asynParamInt16Array, &P_FMT_COL_EVENT);
	createParam(PS_FMT_COL_PAD,  asynParamInt16Array,   &P_FMT_COL_PAD);
	createParam(PS_FMT_COL_CLIDAT, asynParamInt32Array, &P_FMT_COL_CLIDAT);
	createParam(PS_FMT_COL_TS, asynParamInt64Array,     &P_FMT_COL_TS);

	setStringParam(P_FMT_MC_GRP, G::fmt_mc_group);
	setIntegerParam(P_FMT_MC_PORT, G::fmt_mc_port);
}

void acq400_FMT_abc::init_mc_url(char* group, int maxgroup, int *port)
{
	asynStatus status = asynSuccess;

	status = getStringParam(P_FMT_MC_GRP, 80, group);
	if (status){
		fprintf(stderr, "%s:%s getStringParam P_FMT_MC_GRP fail\n",
				DN, FN);
		exit(1);
	}
	status = getIntegerParam(P_FMT_MC_PORT, port);
	if (status){
		fprintf(stderr, "%s:%s getIntegerParam P_FMT_MC_PORT fail\n",
				DN, FN);
		exit(1);

	}

	fprintf(stderr, "%s:%s mc_group \"%s\" mc_port %d\n",
			DN, FN, group, *port);
}

MultiCast& acq400_FMT_abc::mc_factory(MultiCast::MC txrx)
{
	char mc_group[80];
	int mc_port;

	init_mc_url(mc_group, 80, &mc_port);
	return MultiCast::factory(mc_group, mc_port, txrx);
}

void acq400_FMT_abc::update_fmt_columns()
{
	for (int row = 0; row < FMT_ROWS; ++row){
		cols.c_event[row]= fmt[row].event;
		cols.c_pad[row] = fmt[row].pad;
		cols.c_client_data[row] = fmt[row].client_data;
		cols.c_timestamp[row] = fmt[row].timestamp;
	}
}

void acq400_FMT_abc::update_fmt_callbacks()
{
	setIntegerParam(P_UPDATES, ++update);
	setInteger64Param(P_TS_USEC, now_us);
	callParamCallbacks();
	doCallbacksInt8Array(cols.c_rownum, FMT_ROWS, P_FMT_COL_ROWNUM, 0);
	doCallbacksInt16Array(cols.c_event, FMT_ROWS, P_FMT_COL_EVENT, 0);
	doCallbacksInt16Array(cols.c_pad, FMT_ROWS, P_FMT_COL_PAD, 0);
	doCallbacksInt32Array(cols.c_client_data, FMT_ROWS, P_FMT_COL_CLIDAT, 0);
	doCallbacksInt64Array(cols.c_timestamp, FMT_ROWS, P_FMT_COL_TS, 0);
}

void acq400_FMT_abc::task_runner(void *drvPvt)
{
	acq400_FMT_abc *pPvt = (acq400_FMT_abc *)drvPvt;
	pPvt->task();
}

