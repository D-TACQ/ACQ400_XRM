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

	PRMT_LUT_ROW lut_rows[MAX_ROWS]; // lazy allocation
	std::set<PRMT_LUT_ROW> event_set;

	static acq400_PermitLut* _instance;

	unsigned short last_load_event;
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
