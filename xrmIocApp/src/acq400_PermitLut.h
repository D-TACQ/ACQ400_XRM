/*
 * acq400_PermitLut.h: one acq400_PermitLut per LUT row
 *
 *  Created on: 19 Aug 2026
 *      Author: pgm
 */

#ifndef XRMIOCAPP_SRC_ACQ400_PERMITLUT_H_
#define XRMIOCAPP_SRC_ACQ400_PERMITLUT_H_

#include "acq400_asyn_common.h"
#include "acq400_Permit.h"
#include <set>

/* default. nb: not used, settable from st.cmd */
#define MAX_ROWS	16   //* << LUT size from FNAL spec, one addr per row.


#define PS_PRMT_LUT_SOFT_EVENT	"PRMT_LUT_SOFT_EVENT" // int, address
#define PS_PRMT_LUT_EVENT	"PRMT_LUT_EVENT"  // int, scalar
#define PS_PRMT_LUT_TH_AI	"PRMT_LUT_TH_AI"  // u32, vector NCHAN
#define PS_PRMT_LUT_TH_DI       "PRMT_LUT_TH_DI"  // 2* UInt32Digital vector per site, max 2 sites
#define PS_PRMT_LUT_COMMIT      "PRMT_LUT_COMMIT" // atomic update

#define PS_PRMT_LUT_LAST_EVENT	"PRMT_LUT_LAST_EVENT"   // last event accepted

/**< NTTable view of the permit lut - show Event and Active selection for LUT row
 *  "Active" : 32 bit mask showing participating channels - AI, DI, same treatment.
 *  empty site? Active=0
 *  Phoebus displays decimal integer.
 *   - a bit mask is indecipherable in decimal
 *
 *  Sx COL encoding : ACTIVE*10000, LAST*100, FIRST*1
 *   - eg  323201   : 32 active channels, LAST=32 FIRST=1
 *   - eg   23201   :  2 active channels, LAST=32, FIRST=1
 *   - eg   40401   :  4 active channels, LAST=4, FIRST=1
 *   - eg   43229   :  4 active channels, LAST=32, FIRST=29
 */

#define PS_PRMT_LUT_EV_COL	"PRMT_LUT_EV_COL"
#define PS_PRMT_LUT_S1_COL	"PRMT_LUT_S1_COL"
#define PS_PRMT_LUT_S2_COL	"PRMT_LUT_S2_COL"
#define PS_PRMT_LUT_S3_COL	"PRMT_LUT_S3_COL"
#define PS_PRMT_LUT_S4_COL	"PRMT_LUT_S4_COL"
#define PS_PRMT_LUT_S5_COL	"PRMT_LUT_S5_COL"
#define PS_PRMT_LUT_S6_COL	"PRMT_LUT_S6_COL"
#define PS_PRMT_LUT_COL_ROW_NUM "PRMT_LUT_COL_ROW_NUM"

/** @@todo may not be needed */
struct _PRMT_LUT_ROW {
	unsigned short event;
	unsigned short commit;
	unsigned short ai_count;
	unsigned short di_count;
	unsigned* th_ai;
	unsigned* th_di;
};

#define NPAIR	2

struct PRMT_LUT_ROW {
	unsigned short event;	/**< event value to select row */
	epicsInt16* ai_th_pairs;/**< agg NCHAN LOW, HI pairs   */
	epicsInt32* di_pairs;   /**< agg NDI32 VALUE, ACTIVE pairs */

	bool operator<(const PRMT_LUT_ROW& other) const {
		return event < other.event;
	}
};

struct LutRowComparator {
	bool operator()(PRMT_LUT_ROW& a, PRMT_LUT_ROW &b) const {
		return a.event < b.event;
	}
};

class acq400_PermitLut: public acq400_asynPortDriver {
/**< singleton */
	static int verbose;
	PermitDSP& dsp;

	int P_PRMT_LUT_SOFT_EVENT;
	int P_PRMT_LUT_EVENT;
	int P_PRMT_LUT_TH_AI;
	int P_PRMT_LUT_TH_DI;
	int P_PRMT_LUT_COMMIT;
	int P_PRMT_LUT_LAST_EVENT;

	int P_PRMT_LUT_EV_COL;
	int P_PRMT_LUT_00_COL;
	int P_PRMT_LUT_S1_COL;
	int P_PRMT_LUT_S2_COL;
	int P_PRMT_LUT_S3_COL;
	int P_PRMT_LUT_S4_COL;
	int P_PRMT_LUT_S5_COL;
	int P_PRMT_LUT_S6_COL;
	int P_PRMT_LUT_COL_ROW_NUM;

	PRMT_LUT_ROW lut_rows[MAX_ROWS]; // lazy allocation
	std::set<PRMT_LUT_ROW> event_set;

	static acq400_PermitLut* _instance;

	unsigned cols[MAX_SITES+1][MAX_ROWS];    /** [0]: event, [123456]: Sn Active mask */

	unsigned short last_load_event;


	void set_EV_col(int addr, unsigned event);
	void set_AI_selection_col(int addr, short ai_th_pairs[], int npairs);
	void set_DI_selection_col(int addr, epicsInt32 di_pairs[], int npairs);

protected:
	acq400_PermitLut(const char *portName, int max_addr);

	virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

	virtual asynStatus writeInt16Array(asynUser *pasynUser, epicsInt16 *value,
					size_t nElements);
	virtual asynStatus doCallbacksInt16Array(epicsInt16 *value,
					size_t nElements, int reason, int addr);

	virtual asynStatus writeInt32Array(asynUser *pasynUser, epicsInt32 *value,
					size_t nElements);
	virtual asynStatus doCallbacksInt32Array(epicsInt32 *value,
	                                        size_t nElements, int reason, int addr);
public:
	void check_if_fmt_event_trigger(const FMT& fmt);

	static acq400_PermitLut& instance();
	static void create_instance(const char *portName, int max_addr);
};

#endif /* XRMIOCAPP_SRC_ACQ400_PERMITLUT_H_ */
