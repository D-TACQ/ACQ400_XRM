/*
 * acq400_SOE.h
 *
 *  Created on: 23 Feb 2026
 *      Author: pgm
 */

/* provide an interface to the SOE_LUT and SOE_HOLD_TABLE  SOE_HLD
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

/* REDIT : Row EDIT */
#define PS_SOE_LUT_REDIT_ROW		"SOE_LUT_REDIT_ROW" 	/* edit this row */
#define PS_SOE_LUT_REDIT_ROWCOUNT	"SOE_LUT_REDIT_ROWCOUNT"	/* for this many rows */
#define PS_SOE_LUT_REDIT_EVENT      	"SOE_LUT_REDIT_EVENT"       /* set this event */
#define PS_SOE_LUT_REDIT_EVENT_STEP 	"SOE_LUT_REDIT_EVENT_STEP"  /* adding to subsequent rows */
#define PS_SOE_LUT_REDIT_PV_ID      	"SOE_LUT_REDIT_PV_ID"       /* set this event */
#define PS_SOE_LUT_REDIT_PV_ID_STEP 	"SOE_LUT_REDIT_PV_ID_STEP"  /* adding to subsequent rows */
#define PS_SOE_LUT_REDIT_OFFSET_US	"SOE_LUT_REDIT_OFFSET_US"
#define PS_SOE_LUT_REDIT_OFFSET_US_STEP "SOE_LUT_REDIT_OFFSET_US_STEP"

#define PS_SOE_LUT_REDIT_COMMIT 	"SOE_LUT_REDIT_COMMIT"

#define PS_SOE_HLD_COL_ROWNUM	 "SOE_HLD_COL_ROWNUM"
#define PS_SOE_HLD_COL_PV_ID    "SOE_HLD_COL_PV_ID"
#define PS_SOE_HLD_COL_CLIDAT    "SOE_HLD_COL_CLIDAT"
#define PS_SOE_HLD_COL_TS        "SOE_HLD_COL_TS"
#define PS_SOE_HLD_COL_DATA_OFFSET "SOE_HLD_COL_DATA_OFFSET"

#define PS_SOE_SITE_SSB		"SOE_SITE_SSB" /* addr per site */
#define PS_SOE_SITE_IS_ADC	"SOE_SITE_IS_ADC"  /* addr per site */
#define PS_SOE_SMPL_SS_U32	"SOE_SMPL_SS_U32"
#define PS_SOE_SMPL_AI_COUNT 	"SOE_SMPL_AI_COUNT"
#define PS_SOE_SMPL_DI_COUNT 	"SOE_SMPL_DI_COUNT"
#define PS_SOE_SMPL_SP_COUNT 	"SOE_SMPL_SP_COUNT"

#define PS_SOE_SMPL_DI_INDEX 	"SOE_SMPL_DI_INDEX"
#define PS_SOE_SMPL_SP_INDEX 	"SOE_SMPL_SP_INDEX"


#define PS_SOE_HLD_COL_DI_COUNT "SOE_HLD_COL_DI_COUNT"
#define PS_SOE_HLD_COL_SP_COUNT "SOE_HLD_COL_SP_COUNT"


/* 2 columns each data for show */
#define PS_SOE_HLD_COL_AI1	"SOE_HLD_COL_AI1"
#define PS_SOE_HLD_COL_AI2	"SOE_HLD_COL_AI2"

#define PS_SOE_HLD_COL_DI1	"SOE_HLD_COL_DI1"
#define PS_SOE_HLD_COL_DI2	"SOE_HLD_COL_DI2"


/* SP0 : SPAD0 aka Sample Number
 * SP1 : SPAD1 aka WR_VERNIER
 */
#define PS_SOE_HLD_COL_SP0	"SOE_HLD_COL_SP0"
#define PS_SOE_HLD_COL_SP1	"SOE_HLD_COL_SP1"
#define PS_SOE_HLD_COL_SP2	"SOE_HLD_COL_SP2"
#define PS_SOE_HLD_COL_WRVS	"SOE_HLD_COL_WRVS"  // WR Vernier, seconds
#define PS_SOE_HLD_COL_WRVT     "SOE_HLD_COL_WRVT"  // WR Vernier, ticks
#define PS_SOE_HLD_COL_WRUS     "SOE_HLD_COL_WRUS"  // WR time, usec since epoch

#define SP0	0
#define SP1	1
#define SP2	2
#define SP3	3

/* define a column for each data type.
 * There will be up to 64 of these by asyn "address"
 * BUT: this is NOT going to play well with a table
 * Really, one row per event again, 32 AI cols, 2DI, 2SP ..
 * @@todo !Think! Also, suck it and see..
 */
#define PS_SOE_HLD_DATA_COL_AI	"SOE_HLD_DATA_COL_AI"  // 64 records
#define PS_SOE_HLD_DATA_COL_DI  "SOE_HLD_DATA_COL_DI"
#define PS_SOE_HLD_DATA_COL_SP  "SOE_HLD_DATA_COL_SP"

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

	struct HOLD_COLS {
		epicsInt8    c_rownum[SOE_HLD_ROWS];
		epicsInt32  c_pv_id[SOE_HLD_ROWS];
		epicsInt32  c_client_data[SOE_HLD_ROWS];
		epicsInt64  c_timestamp[SOE_HLD_ROWS];		// really U64 but..
		epicsFloat32 c_AI1[SOE_HLD_ROWS];
		epicsFloat32 c_AI2[SOE_HLD_ROWS];
		epicsInt32  c_DI1[SOE_HLD_ROWS];       // really U32, but we only have doCallbacksInt32Array()
		epicsInt32  c_DI2[SOE_HLD_ROWS];
		epicsInt32  c_SP0[SOE_HLD_ROWS];
		epicsInt32  c_SP1[SOE_HLD_ROWS];
		epicsInt32  c_SP2[SOE_HLD_ROWS];
		epicsInt8   c_WRVS[SOE_HLD_ROWS];
		epicsInt32  c_WRVT[SOE_HLD_ROWS];
		epicsInt64  c_WRUS[SOE_HLD_ROWS];
	} hold_cols;
	unsigned update;
	static int nice;

	asynStatus gip(int pnum, int* pram);
	void redit();
	virtual void update_soe_lut(bool first_time = false);
	virtual void update_soe_lut_columns(void);
	virtual void update_soe_lut_callbacks(void);

	virtual void update_hld_tab(bool first_time = false);
	virtual void update_hld_tab_columns(void);
	virtual void update_hld_tab_callbacks(void);

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

	int P_SOE_LUT_REDIT_ROW;
	int P_SOE_LUT_REDIT_ROWCOUNT;
	int P_SOE_LUT_REDIT_EVENT;
	int P_SOE_LUT_REDIT_EVENT_STEP;
	int P_SOE_LUT_REDIT_PV_ID;
	int P_SOE_LUT_REDIT_PV_ID_STEP;
	int P_SOE_LUT_REDIT_OFFSET_US;
	int P_SOE_LUT_REDIT_OFFSET_US_STEP;

	int P_SOE_LUT_REDIT_COMMIT;


	int P_SOE_HLD_COL_ROWNUM;
	int P_SOE_HLD_COL_PV_ID;
	int P_SOE_HLD_COL_CLIDAT;
	int P_SOE_HLD_COL_TS;
	int P_SOE_HLD_COL_DATA_OFFSET;

	int P_SOE_SITE_SSB;
	int P_SOE_SITE_IS_ADC;
	int P_SOE_HLD_COL_SS_U32;
	int P_SOE_HLD_COL_AI_COUNT;
	int P_SOE_HLD_COL_DI_COUNT;
	int P_SOE_HLD_COL_SP_COUNT;
	int P_SOE_SMPL_DI_INDEX;
	int P_SOE_SMPL_SP_INDEX;

	int P_SOE_HLD_COL_AI1;
	int P_SOE_HLD_COL_AI2;

	int P_SOE_HLD_COL_DI1;
	int P_SOE_HLD_COL_DI2;

	int P_SOE_HLD_COL_SP0;
	int P_SOE_HLD_COL_SP1;
	int P_SOE_HLD_COL_SP2;
	int P_SOE_HLD_COL_WRVS;
	int P_SOE_HLD_COL_WRVT;
	int P_SOE_HLD_COL_WRUS;

	int ib;			/** ib is physical buffer contains bpb vpb's */
public:
	acq400_SOE(const char *portName);
	virtual ~acq400_SOE() {}

	asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

};

#endif /* XRMIOCAPP_SRC_ACQ400_SOE_H_ */
