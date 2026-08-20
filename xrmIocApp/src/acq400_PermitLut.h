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

/* default. nb: not used, settable from st.cmd */
#define MAX_ROWS	16   //* << LUT size from FNAL spec, one addr per row.


#define PS_PRMT_LUT_SOFT_EVENT	"PRMT_LUT_SOFT_EVENT" // int, address
#define PS_PRMT_LUT_EVENT	"PRMT_LUT_EVENT"  // int, scalar
#define PS_PRMT_LUT_TH_AI	"PRMT_LUT_TH_AI"  // u32, vector NCHAN
#define PS_PRMT_LUT_TH_DI       "PRMT_LUT_TH_DI"  // 2* UInt32Digital vector per site, max 2 sites
#define PS_PRMT_LUT_COMMIT      "PRMT_LUT_COMMIT" // atomic update

struct PRMT_LUT_ROW {
	unsigned short event;
	unsigned short commit;
	unsigned short ai_count;
	unsigned short di_count;
	unsigned* th_ai;
	unsigned* th_di;
};

class acq400_PermitLut: public acq400_asynPortDriver {
	static int verbose;
	PermitDSP& dsp;

	int P_PRMT_LUT_EVENT;
	int P_PRMT_LUT_TH_AI;
	int P_PRMT_LUT_TH_DI;
	int P_PRMT_LUT_COMMIT;

protected:
	acq400_PermitLut(const char *portName, int max_addr);

public:
	static void create_instance(const char *portName, int max_addr);
};

#endif /* XRMIOCAPP_SRC_ACQ400_PERMITLUT_H_ */
