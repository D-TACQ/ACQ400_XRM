/*
 * acq400_PM.h  : acq400_PM : Post Mortem class.
 *
 * ACQ400 has supplied PM data from the start "TRANSIENT".
 * however, TRANSIENT is a one shot, start, stop, offload
 * XRM calls for a "live PM" that allows the capture to continue.
 * We do this by copying EVERY capture buffer, maintining a ring-buffer
 * of the previous second.
 * The previous second is presented as :PM:00 latest .. PM:19: oldest
 * acq400_PM has a RUNSTOP paramter. On run, the ring-buffer is maintained as normal
 * on STOP, the
 *
 *  Created on: 6 Mar 2026
 *      Author: pgm
 */

#ifndef XRMIOCAPP_SRC_ACQ400_PM_H_
#define XRMIOCAPP_SRC_ACQ400_PM_H_

#include "acq400_asyn_common.h"
#include "epicsRingBytes.h"

#include <algorithm>
#include <iostream>
#include <numeric>
#include <string>
#include <deque>

struct BufferPair {
	short ib_live;	// index of live buffer
	short ib_store;	// index of buffer in store
};

#define MAX_PM_BUFFERS	32

#define PS_RUNSTOP	"RUNSTOP"	/* asynInt32, r/w */
#define PS_UPDATES	"UPDATES"	/* asynInt32, r/c */
#define PS_TS_USEC	"TS_USEC"
#define PS_NBUF		"NBUF"		/* asynInt32, r, number of buffers */

#define PS_PM_COL_ROWNUM 	"PM_COL_ROWNUM"
#define PS_PM_COL_IBLIVE 	"PM_COL_IBLIVE"
#define PS_PM_COL_IBSTORE 	"PM_COL_IBSTORE"
#define PS_PM_COL_TS		"PM_COL_TS"

#define PS_PM_COL_SP0		"PM_COL_SP0"
#define PS_PM_COL_SP1		"PM_COL_SP1"
#define PS_PM_COL_SP2		"PM_COL_SP2"
#define PS_PM_COL_WRVS		"PM_COL_WRVS"  // WR Vernier, seconds
#define PS_PM_COL_WRVT  	"PM_COL_WRVT"  // WR Vernier, ticks
#define PS_PM_COL_WRUS  	"PM_COL_WRUS"  // WR time, usec since epoch


#define PS_RAWBUF	"RAWBUF"        /* addr 0..32 */

class acq400_PM: public acq400_asynPortDriver {


protected:
	std::deque<BufferPair> empties;
	std::deque<BufferPair> filled;

	void init_buffers(const unsigned nbuf);
	void stash_buffer(int ib_live, const unsigned nbuf);

	struct PM_COLS {
		epicsInt8   c_rownum[MAX_PM_BUFFERS];
		epicsInt16   c_ib_live[MAX_PM_BUFFERS];
		epicsInt16   c_ib_store[MAX_PM_BUFFERS];
		epicsInt64  c_timestamp[MAX_PM_BUFFERS];		// really U64 but..
		epicsInt32  c_SP0[MAX_PM_BUFFERS];
		epicsInt32  c_SP1[MAX_PM_BUFFERS];
		epicsInt32  c_SP2[MAX_PM_BUFFERS];
		epicsInt8   c_WRVS[MAX_PM_BUFFERS];
		epicsInt32  c_WRVT[MAX_PM_BUFFERS];
		epicsInt64  c_WRUS[MAX_PM_BUFFERS];
	} pm_cols;

	epicsEventId eventId;
	unsigned update;

	virtual void update_pm_tab_row(int row, int ib);
	virtual void update_pm_callbacks(void);

	virtual void task();

	static void task_runner(void *drvPvt);
	static int nice;
	static int verbose;
	static int spX_from_live;

	int P_RUNSTOP;
	int P_UPDATES;
	int P_TS_USEC;
	int P_NBUF;
	int P_RING;
	int P_RAWBUF;

	int P_COL_ROWNUM;
	int P_COL_IBLIVE;
	int P_COL_IBSTORE;
	int P_COL_TS;

	int P_COL_SP0;
	int P_COL_SP1;
	int P_COL_SP2;
	int P_COL_WRVS;
	int P_COL_WRVT;
	int P_COL_WRUS;

	int ib;			/** ib is physical buffer contains bpb vpb's */

public:
	acq400_PM(const char *portName);
	virtual ~acq400_PM() {}

	virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
};
#endif /* XRMIOCAPP_SRC_ACQ400_PM_H_ */
