/*
 * acq400_SOE_Strategy.cpp
 *
 *  Created on: 13 Mar 2026
 *      Author: pgm
 */

#include "acq400_asyn_common.h"
#include "acq400_SOE.h"
#include "acq400_FMT_rx.h"
#include "acq-util.h"
#include "split2.h"
#include <fcntl.h>                // open()
#include <unistd.h>
#include <string.h>

using namespace std;
#include "Buffer.h"
#include "ES.h"

static const char *driverName="acq400_SOE";

#define DN	driverName
#define FN	__FUNCTION__

/* copy first 64 samples to output, together with first 64 SOE_LUT entries, no FMT matchup */
class NullStrategy : public acq400_SOE_Strategy
{
	virtual int operator() (
			const KBUF& kbuf,
			const SamplePrams& samplePrams,
			const SOE_LUT& soe_lut,
			SOE_HOLD_TABLE* ht);
};

int NullStrategy::operator() (
		const KBUF& kbuf,
		const SamplePrams& samplePrams,
		const SOE_LUT& soe_lut,
		SOE_HOLD_TABLE* ht)
{
	const int SSB = samplePrams.SSB;
	const int SSL = SSB/sizeof(long);
	const char* raw = kbuf.raw;
	const unsigned* sp_raw = (const unsigned*)kbuf.raw + samplePrams.SP_INDEX;;

	assert(SOE_LUT_ROWS >= SOE_HLD_ROWS);


	/* first 10 rows client_data becomes ib history for diags.. */
	for (int ii = 10; ii; --ii){
		ht->entries[ii].client_data = ht->entries[ii].client_data;
	}
	ht->entries[0].client_data = kbuf.ib;

	unsigned short ht_data_offset = offsetof(SOE_HOLD_TABLE, data)/sizeof(long);

	for (int row = 0; row < SOE_HLD_ROWS; ++row,
			ht_data_offset += SSL, sp_raw += SSL, raw += SSB){
		unsigned wrs, wrv;

		ht->entries[row].pv_id = soe_lut[row].pv_id;
		// From FMT! ht->entries[row].client_data = soe_lut[row].client_data;

		wrv = sp_raw[SP2];
		wrs = sp_raw[SP3];

		ht->entries[row].timestamp = getWrTs(wrs, wrv);
		ht->entries[row].data_offset = ht_data_offset;
		memcpy(ht->data+row*SSL, raw, SSB);
	}
	return 0;
}


class LutFmtStrategy1 : public acq400_SOE_Strategy
{
	int soe_lut_lookup (
			const KBUF& kbuf,
			const SamplePrams& samplePrams, const SOE_LUT& soe_lut,
			SOE_HOLD_TABLE* ht);
	virtual int operator() (
			const KBUF& kbuf,
			const SamplePrams& samplePrams, const SOE_LUT& soe_lut,
			SOE_HOLD_TABLE* ht);
};

#define CYCLE_MS	50		// @@todo make me programmable

int LutFmtStrategy1::soe_lut_lookup(
		const KBUF& kbuf,
		const SamplePrams& samplePrams, const SOE_LUT& soe_lut,
		SOE_HOLD_TABLE* ht)
{
	return -1;
}

int LutFmtStrategy1::operator() (
		const KBUF& kbuf,
		const SamplePrams& samplePrams, const SOE_LUT& soe_lut,
		SOE_HOLD_TABLE* ht)
{
	acq400_FMT_rx FMT_rx(acq400_FMT_rx::instance());
	if (FMT_rx.waitFMT(CYCLE_MS) == 0){
		epicsInt64 fmt_ts = FMT_rx.fmt[0].timestamp;
		if (fmt_ts < kbuf.wrt0-CYCLE_MS*1000){
			last_error_code = E_FMT_TS_TOO_EARLY;
		}else if (fmt_ts > kbuf.wrt1+CYCLE_MS*1000){
			last_error_code = E_FMT_TS_TOO_LATE;
		}else{
			return soe_lut_lookup(kbuf, samplePrams, soe_lut, ht);
		}
		return -1;
	} else {
		last_error_code = E_TIMEOUT;
		return -1;
	}
	/*
	const int SSS = samplePrams.SSB/sizeof(short);
	const int SSL = samplePrams.SSB/sizeof(long);
	short* ai_raw = (short*)raw;
	int * di_raw = (int*)raw + samplePrams.DI_INDEX;
	int * sp_raw = (int*)raw + samplePrams.SP_INDEX;
	*/

	// @@todo fill the blanks
	// acq400_FMT_RX::instance().waitFMT(10);
	return 0;
}



acq400_SOE_Strategy& acq400_SOE_Strategy::factory()
{
	const char* sel = ::getenv("acq400_SOE_Strategy");

	if (sel != 0){
		if (strcmp(sel, "LUT_FMT1") == 0){
			return *new LutFmtStrategy1();
		}
		// else could be others ..
	}
	return *new NullStrategy();
}

