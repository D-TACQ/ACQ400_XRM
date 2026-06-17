/** @file acq400_FMT.h
 * @brief Fermi Multicast Table Abstract Base Class
 *  Created on: 16 Feb 2026
 *      Author: pgm
 */

#ifndef XRMIOCAPP_SRC_ACQ400_FMT_H_
#define XRMIOCAPP_SRC_ACQ400_FMT_H_

#include "acq400_asynPortDriver.h"

#include "xrm_structs.h"
#include "Multicast.h"

#define PS_FMT_MC_GRP	"FMT_MC_GRP"    /* string, r/set on PINI */
#define PS_FMT_MC_PORT  "FMT_MC_PORT"   /* asynInt32, r/set on PINI */

#define PS_FMT_COL_ROWNUM	"FMT_COL_ROWNUM"  /* cosmetic for display, NOT part of FMT */
#define PS_FMT_COL_EVENT	"FMT_COL_EVENT"  	/* asynInt16Array, ro */
#define PS_FMT_COL_PAD		"FMT_COL_PAD"	/* asynInt16Array, ro */
#define PS_FMT_COL_CLIDAT	"FMT_COL_CLIDAT"	/* asynInt32Array, ro */
#define PS_FMT_COL_TS 		"FMT_COL_TS"	/* asynInt64Array, ro */

/** Fermi Multicast Table Abstract Base Class
 * - We represent the data as an NTTABLE for display purposes (usually subject to RATE_LIM)
 * - This is a column-major representation, unsuited for FMT in general.
 * - The FMT itself is published as a flat binary array of u32.
 */
class acq400_FMT_abc: public acq400_asynPortDriver {
public:
	FMT fmt;	/**< FMT binary instance */
protected:
	/** EPICS NTTABLE is a convenient display mechanism,
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

	char mc_group[80];
	int mc_port;

	virtual void update_fmt(bool first_time = false) = 0;
	virtual void update_fmt_columns(void);
	virtual void update_fmt_callbacks(bool call_array_callbacks);
	void init_mc_url(char* group, int maxgroup, int* port);
	MultiCast& mc_factory(MultiCast::MC txrx);

	static int nice;

	virtual void task() = 0;

	static void task_runner(void *drvPvt);

	unsigned update;
	epicsInt64 now_us;

	int P_FMT_MC_GRP;		/**< MultiCast Group eg 224.x.x.x */
	int P_FMT_MC_PORT;		/**< MultiCast Port */
	int P_FMT_COL_ROWNUM;		/**< NTTABLE column-major view, Row Number */
	int P_FMT_COL_EVENT;		/**< NTTABLE column-major view, Event */
	int P_FMT_COL_PAD;		/**< NTTABLE column-major view, PAD */
	int P_FMT_COL_CLIDAT;		/**< NTTABLE column-major view, CLIDAT client data */
	int P_FMT_COL_TS;		/**< NTTABLE column-major view, Timestamp */

	acq400_FMT_abc(const char *portName, int maxAddr, int interfaceMask, int interruptMask,
	                   int asynFlags, int autoConnect, int priority, int stackSize);
	virtual ~acq400_FMT_abc() {}

};



#endif /* XRMIOCAPP_SRC_ACQ400_FMT_H_ */
