/*
 * acq400_FMT_abstract.cpp
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


acq400_FMT_abstract::acq400_FMT_abstract(
	const char *portName, int maxAddr, int interfaceMask, int interruptMask,
	                   int asynFlags, int autoConnect, int priority, int stackSize) :
	asynPortDriver(portName, maxAddr, interfaceMask, interruptMask,
			   asynFlags, autoConnect, priority, stackSize),
	update(0)
{
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


void acq400_FMT_abstract::update_fmt_columns()
{
	for (int row = 0; row < FMT_ROWS; ++row){
		cols.c_event[row]= fmt[row].event;
		cols.c_pad[row] = fmt[row].pad;
		cols.c_client_data[row] = fmt[row].client_data;
		cols.c_timestamp[row] = fmt[row].timestamp;
	}
}



