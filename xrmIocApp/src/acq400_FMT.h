/*
 * ACQ400_FMT.h
 *
 *  Created on: 16 Feb 2026
 *      Author: pgm
 */

#ifndef XRMIOCAPP_SRC_ACQ400_FMT_H_
#define XRMIOCAPP_SRC_ACQ400_FMT_H_

#include "asynPortDriver.h"
#include "xrm_structs.h"

#define PS_RUNSTOP	"RUNSTOP"	/* asynInt32, r/w */
#define PS_UPDATES	"UPDATES"	/* asynInt32, r/c */
#define PS_TS_USEC	"TS_USEC"       /* asynInt32, ro  */
#define PS_FMT_MC_GRP	"FMT_MC_GRP"    /* string, r/set on PINI */
#define PS_FMT_MC_PORT  "FMT_MC_PORT"   /* asynInt32, r/set on PINI */

/*
epicsUInt16 event;           // FNAL Event number
epicsUInt16 pad;             // 32 bit alignment is best, available for future
epicsUInt32 client_data;     // opaque value to pass back
epicsUInt64 timestamp;
*/
/* arrays should be writable on sim... but a user would _really_ want to write rows */
/* writeable FMT interface should be by rows. so a row should be a GROUP of scalars,
 * and the TABLE should be an array of the groups.. @@todo LATER!
 * or we just have a name explosion of scalars that happen to be grouped for atomicity..
 */
#define PS_FMT_COL_ROWNUM	"FMT_COL_ROWNUM"  /* cosmetic for display, NOT part of FMT */
#define PS_FMT_COL_EVENT	"FMT_COL_EVENT"  	/* asynInt16Array, ro */
#define PS_FMT_COL_PAD		"FMT_COL_PAD"	/* asynInt16Array, ro */
#define PS_FMT_COL_CLIDAT	"FMT_COL_CLIDAT"	/* asynInt32Array, ro */
#define PS_FMT_COL_TS 		"FMT_COL_TS"	/* asynInt64Array, ro */

class acq400_FMT_abstract: public asynPortDriver {

protected:
	FMT fmt;
	/* EPICS NTTABLE is a convenient display mechanism,
	 * but unfortunately it needs the data in columns.
	 *
	 * @@todo : for dynamic update, we want a NTGROUP per row,
	 * ie 64 groups of 4 scalar pvs, that's a lot of PV names.. rather than one array..
	 */
	struct COLUMNS {
		epicsInt8  c_rownum[FMT_ROWS];
		epicsInt16 c_event[FMT_ROWS];
		epicsInt16 c_pad[FMT_ROWS];
		epicsInt32 c_client_data[FMT_ROWS];
		epicsInt64 c_timestamp[FMT_ROWS];
	} cols;

	virtual void update_fmt(bool first_time = false) = 0;
	virtual void update_fmt_columns(void);

	static int nice;

	epicsEventId eventId;

	virtual void task() = 0;

	unsigned update;
	epicsInt64 now_us;

	int P_RUNSTOP;
	int P_UPDATES;
	int P_TS_USEC;
	int P_FMT_MC_GRP;
	int P_FMT_MC_PORT;
	int P_FMT_COL_ROWNUM;
	int P_FMT_COL_EVENT;
	int P_FMT_COL_PAD;
	int P_FMT_COL_CLIDAT;
	int P_FMT_COL_TS;

	acq400_FMT_abstract(const char *portName, int maxAddr, int interfaceMask, int interruptMask,
	                   int asynFlags, int autoConnect, int priority, int stackSize);
	virtual ~acq400_FMT_abstract() {}

};



#endif /* XRMIOCAPP_SRC_ACQ400_FMT_H_ */
