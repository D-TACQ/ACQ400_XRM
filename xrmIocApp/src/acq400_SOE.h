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

#include "acq400_asyn_common.h"
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

#define PS_SOE_AGG_SITES	"SOE_AGG_SITES"
#define PS_SOE_SITE_SSB		"SOE_SITE_SSB" /* addr per site */
#define PS_SOE_SMPL_NSAM	"SOE_SMPL_NSAM"
#define PS_SOE_SITE_IS_ADC	"SOE_SITE_IS_ADC"  /* addr per site */
#define PS_SOE_SMPL_SS_U32	"SOE_SMPL_SS_U32"
#define PS_SOE_SMPL_AI_COUNT 	"SOE_SMPL_AI_COUNT"
#define PS_SOE_SMPL_DI_COUNT 	"SOE_SMPL_DI_COUNT"
#define PS_SOE_SMPL_SP_COUNT 	"SOE_SMPL_SP_COUNT"

#define PS_SOE_SMPL_DI_INDEX 	"SOE_SMPL_DI_INDEX"
#define PS_SOE_SMPL_SP_INDEX 	"SOE_SMPL_SP_INDEX"

#define PS_SOE_KBUF_INDEX		"SOE_KBUF_INDEX"
#define PS_SOE_KBUF_WRT0		"SOE_KBUF_WRT0"
#define PS_SOE_KBUF_WRT1		"SOE_KBUF_WRT1"

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
#define PS_SOE_HLD_COL_SP2	"SOE_HLD_COL_SP2"   // WRV
#define PS_SOE_HLD_COL_SP3	"SOE_HLD_COL_SP3"   	    // WRS Seconds Since EPOCH, SP3
#define PS_SOE_HLD_COL_WRVS	"SOE_HLD_COL_WRVS"  // WR Vernier, seconds
#define PS_SOE_HLD_COL_WRVT     "SOE_HLD_COL_WRVT"  // WR Vernier, ticks
#define PS_SOE_HLD_COL_WRUS     "SOE_HLD_COL_WRUS"  // WR time, usec since epoch



/* define a column for each data type.git
 * There will be up to 64 of these by asyn "address"
 * BUT: this is NOT going to play well with a table
 * Really, one row per event again, 32 AI cols, 2DI, 2SP ..
 * @@todo !Think! Also, suck it and see..
 */
#define PS_SOE_HLD_DATA_COL_AI	"SOE_HLD_DATA_COL_AI"  // 64 records
#define PS_SOE_HLD_DATA_COL_DI  "SOE_HLD_DATA_COL_DI"
#define PS_SOE_HLD_DATA_COL_SP  "SOE_HLD_DATA_COL_SP"

#define PS_SOE_FMT_RX_TIMEOUTS	"SOE_FMT_RX_TIMEOUTS"
#define PS_SOE_FMT_RX_TIMEOUT_REASON	\
				"SOE_FMT_RX_TIMEOUT_REASON"
#define PS_SOE_FMT_RX_SUCCESS	"SOE_FMT_RX_SUCCESS"

struct SamplePrams {
	int SSB;
	int NSAM;
	int AI_COUNT;
	int AI_INDEX;
	int DI_COUNT;
	int DI_INDEX;
	int SP_COUNT;
	int SP_INDEX;
};


struct KBUF {
	unsigned ib;
	epicsInt64 wrt0;
	epicsInt64 wrt1;
	const char* raw;
};

/** singleton */
class acq400_SOE_Strategy {
protected:
	int last_error_code;
public:
	/** implements strategy, .. waitFMT, compare LUT, look up data in raw and build output <ht> */
	virtual int operator() (const KBUF& kbuf, const SamplePrams& sp, const SOE_LUT& soe_lut, SOE_HOLD_TABLE* ht) = 0;

	static acq400_SOE_Strategy& factory();

	int getLastErrCode() const {
		return last_error_code;
	}

	enum ERR_CODES {
		E_TIMEOUT = 1,
		E_FMT_TS_TOO_EARLY = 2,
		E_FMT_TS_TOO_LATE = 3,
	};
};

class acq400_SOE: public acq400_asynPortDriver {
protected:
	SOE_LUT soe_lut;
	SOE_HOLD_TABLE* the_hold_table;   // preallocate the max possible size

	acq400_SOE_Strategy& strategy;

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
		epicsInt32  c_SP3[SOE_HLD_ROWS];
		epicsInt8   c_WRVS[SOE_HLD_ROWS];
		epicsInt32  c_WRVT[SOE_HLD_ROWS];
		epicsInt64  c_WRUS[SOE_HLD_ROWS];
	} hold_cols;
	unsigned update;
	unsigned fmt_rx_timeouts;
	unsigned fmt_rx_success;
	static int nice;

	SamplePrams samplePrams;
	void get_sample_dimensions();

	void init_the_hold_table();

	void redit();
	virtual void update_soe_lut(bool first_time = false);
	virtual void update_soe_lut_columns(void);
	virtual void update_soe_lut_callbacks(void);

	virtual void update_hld_tab_columns(void);
	virtual void update_hld_tab_callbacks(void);

	struct KBUF current_kb;
	void update_kbuf_info(char* raw);


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

	int P_SOE_AGG_SITES; // asynParamOctet
	int P_SOE_SITE_SSB;
	int P_SOE_SMPL_NSAM;
	int P_SOE_SITE_IS_ADC;
	int P_SOE_SMPL_SS_U32;
	int P_SOE_SMPL_AI_COUNT;
	int P_SOE_SMPL_DI_COUNT;
	int P_SOE_SMPL_SP_COUNT;
	int P_SOE_SMPL_DI_INDEX;
	int P_SOE_SMPL_SP_INDEX;

	int P_SOE_HLD_COL_AI1;
	int P_SOE_HLD_COL_AI2;

	int P_SOE_HLD_COL_DI1;
	int P_SOE_HLD_COL_DI2;

	int P_SOE_HLD_COL_SP0;
	int P_SOE_HLD_COL_SP1;
	int P_SOE_HLD_COL_SP2;
	int P_SOE_HLD_COL_SP3;
	int P_SOE_HLD_COL_WRVS;
	int P_SOE_HLD_COL_WRVT;
	int P_SOE_HLD_COL_WRUS;

	int P_SOE_KBUF_INDEX;
	int P_SOE_KBUF_WRT0;
	int P_SOE_KBUF_WRT1;

	int P_SOE_FMT_RX_TIMEOUTS;
	int P_SOE_FMT_RX_TIMEOUT_REASON;
	int P_SOE_FMT_RX_SUCCESS;

	int ib;			/** ib is physical buffer contains bpb vpb's */
public:
	acq400_SOE(const char *portName, acq400_SOE_Strategy& strategy);
	virtual ~acq400_SOE() {}

	asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

};

#endif /* XRMIOCAPP_SRC_ACQ400_SOE_H_ */
