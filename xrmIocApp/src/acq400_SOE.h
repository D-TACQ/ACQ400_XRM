/*
 * acq400_SOE.h
 *
 *  Created on: 23 Feb 2026
 *      Author: pgm
 */

/* provide an interface to the SOE_LUT and SOE_HOLD_TABLE
 * block on "new buffer"
 * copy new buffer to backing store (for PM)
 * wait for FMT
 * act on FMT.
 */
#ifndef XRMIOCAPP_SRC_ACQ400_SOE_H_
#define XRMIOCAPP_SRC_ACQ400_SOE_H_

#include "asynPortDriver.h"
#include "xrm_structs.h"
#include "acq400_FMT.h"


#define PS_SOE_LUT_COL_ROWNUM	 "SOE_LUT_COL_ROWNUM"  /* cosmetic for display, NOT part of FMT */
#define PS_SOE_LUT_COL_EVENT	 "SOE_LUT_COL_EVENT"  	/* asynInt16Array, ro */
#define PS_SOE_LUT_COL_PAD	 "SOE_LUT_COL_PAD"	/* asynInt16Array, ro */
#define PS_SOE_LUT_COL_PV_ID	 "SOE_LUT_COL_PV_ID"	/* asynInt32Array, ro */
#define PS_SOE_LUT_COL_OFFSET_US "SOE_LUT_COL_OFFSET_US" /* asynInt32Array, ro */

#define PS_SOE_HHR_COL_ROWNUM	 "SOE_HHR_COL_ROWNUM"
#define PS_SOE_HHR_COL_CLIDAT    "SOE_HHR_COL_CLIDAT"
#define PS_SOE_HHR_COL_TS        "SOE_HHR_COL_TS"
#define PS_SOE_HHR_COL_DATA_OFFSET "SOE_HHT_COL_DATA_OFFSET"

#define PS_SOE_HHR_COL_SS_U32	"SOE_HHR_COL_SS_U32"
#define PS_SOE_HHR_COL_AI_COUNT "SOE_HHR_COL_AI_COUNT"
#define PS_SOE_HHR_COL_DI_COUNT "SOE_HHR_COL_DI_COUNT"
#define PS_SOE_HHR_COL_SP_COUNT "SOE_HHR_COL_SP_COUNT"

/* define a column for each data type.
 * There will be up to 64 of these by asyn "address"
 * BUT: this is NOT going to play well with a table
 * Really, one row per event again, 32 AI cols, 2DI, 2SP ..
 * @@todo !Think! Also, suck it and see..
 */
#define PS_SOE_HHR_DATA_COL_AI	"SOE_HHR_DATA_COL_AI"  // 64 records
#define PS_SOE_HHR_DATA_COL_DI  "SOE_HHR_DATA_COL_DI"
#define PS_SOE_HHR_DATA_COL_SP  "SOE_HHR_DATA_COL_SP"

class acq400_SOE: public asynPortDriver {
protected:
	SOE_LUT soe_lut;

	/* TABLE representation for Phoebus */
	struct COLUMNS {
		epicsInt8  c_rownum[SOE_LUT_ROWS];
		epicsInt16 c_event[SOE_LUT_ROWS];
		epicsInt16 c_pad[SOE_LUT_ROWS];
		epicsInt32 c_pv_id[SOE_LUT_ROWS];
		epicsInt64 c_offset_us[SOE_LUT_ROWS];
	}cols;

	unsigned update;

	virtual void update_soe_lut(bool first_time = false);
	virtual void update_soe_lut_columns(void);
	virtual void update_soe_lut_callbacks(void);

	epicsEventId eventId;

	virtual void task();

	static void task_runner(void *drvPvt);

	int P_RUNSTOP;
	int P_UPDATES;
	int P_TS_USEC;


	int P_SOE_LUT_COL_ROWNUM;
	int P_SOE_LUT_COL_EVENT;
	int P_SOE_LUT_COL_PAD;
	int P_SOE_LUT_COL_PV_ID;
	int P_SOE_LUT_COL_OFFSET_US;

	int P_SOE_HHR_COL_ROWNUM;
	int P_SOE_HHR_COL_CLIDAT;
	int P_SOE_HHR_COL_TS;
	int P_SOE_HHR_COL_DATA_OFFSET;

	int P_SOE_HHR_COL_SS_U32;
	int P_SOE_HHR_COL_AI_COUNT;
	int P_SOE_HHR_COL_DI_COUNT;
	int P_SOE_HHR_COL_SP_COUNT;

public:
	acq400_SOE(const char *portName);
	virtual ~acq400_SOE() {}

};

#endif /* XRMIOCAPP_SRC_ACQ400_SOE_H_ */
