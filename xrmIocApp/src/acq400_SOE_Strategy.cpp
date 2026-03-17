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
	virtual acq400_SOE_Strategy::RC operator() (
			const KBUF& kbuf,
			const SamplePrams& samplePrams,
			const SOE_LUT& soe_lut,
			SOE_HOLD_TABLE* ht);
};

acq400_SOE_Strategy::RC NullStrategy::operator() (
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
	return { 0, 0, 0 };
}


class LutFmtStrategy1 : public acq400_SOE_Strategy
{
	acq400_FMT_rx* FMT_rx;
	unsigned short ht_data_offset;

	acq400_SOE_Strategy::RC soe_lut_lookup (
			const KBUF& kbuf,
			const SamplePrams& sp, const SOE_LUT& soe_lut,
			SOE_HOLD_TABLE* ht);

	int find_event_in_buf(
			const KBUF& kbuf,
			const int ndata,
			epicsInt64 ev_ts)
	/* return sample index to data in buffer */
	{
		if (ev_ts >= kbuf.wrt0 && ev_ts <= kbuf.wrt1){
			return ndata*(ev_ts - kbuf.wrt0)/
					(kbuf.wrt1 - kbuf.wrt0);
		}
		return -1;
	}

	int build_hold_entry(
			const KBUF& kbuf,
			const SamplePrams& samplePrams,
			const FMT_ROW& fmt_row,
			const SOE_LUT_ROW& lut_row,
			int bsi,		// buffer sample index in samples
			SOE_HOLD_TABLE* ht,
			int ihold);
public:
	LutFmtStrategy1() :
		FMT_rx(0)
	{}
	virtual acq400_SOE_Strategy::RC operator() (
			const KBUF& kbuf,
			const SamplePrams& samplePrams, const SOE_LUT& soe_lut,
			SOE_HOLD_TABLE* ht);
};

#define CYCLE_MS	50		// @@todo make me programmable


int  LutFmtStrategy1::build_hold_entry(
		const KBUF& kbuf,
		const SamplePrams& samplePrams,
		const FMT_ROW& fmt_row,
		const SOE_LUT_ROW& lut_row,
		int bsi,			// buffer sample index in samples
		SOE_HOLD_TABLE* ht,
		int ihold)
/* return sample index to data in buffer */
{
	const int SSB = samplePrams.SSB;
	const int SSL = SSB/sizeof(long);
	const unsigned* sample_raw = (const unsigned*)kbuf.raw + bsi*SSL;
	const unsigned* sample_sp0 = sample_raw + samplePrams.SP_INDEX;
	struct SOE_HOLD_HEADER& entry(ht->entries[ihold]);

	entry.pv_id = lut_row.pv_id;
	//entry.client_data = fmt_row.client_data;
	entry.client_data = bsi;
	entry.timestamp = fmt_row.timestamp;
	entry.data_offset = ht_data_offset;
	entry.ss_u32 = SSL;
	entry.ai_count = samplePrams.AI_COUNT;
	entry.di_count = samplePrams.DI_COUNT;
	entry.sp_count = samplePrams.SP_COUNT;

	memcpy(ht->data+ihold*SSL, sample_raw, SSB);

	ht_data_offset += ihold*SSL;
	return 0;
}
acq400_SOE_Strategy::RC LutFmtStrategy1::soe_lut_lookup(
		const KBUF& kbuf,
		const SamplePrams& sp, const SOE_LUT& soe_lut,
		SOE_HOLD_TABLE* ht)
/* FMT, SOE_LUT assumed to be sorted by event */
{
	/* always "SOE_SUCCESS" because the FMT and KBUF TS matched */
	acq400_SOE_Strategy::RC rc = { SOE_SUCCESS, 0, FMT_rx->fmt[0].timestamp-kbuf.wrt0 };


	for (int fmt_row = 0; fmt_row < FMT_ROWS; ++fmt_row){
		const epicsUInt64 fmt_ts = FMT_rx->fmt[fmt_row].timestamp;
		const epicsUInt16 fmt_event = FMT_rx->fmt[fmt_row].event;
		if (fmt_event == EV99){
			break;
		}
		for (int soe_row = 0; soe_row < SOE_LUT_ROWS; ++soe_row){
			const epicsUInt16 soe_lut_event = soe_lut[soe_row].event;
			if (soe_lut_event == EV99){
				break;
			}else if (soe_lut_event > fmt_event){
				break;
			}else if (soe_lut_event != fmt_event){
				continue;                        // keep searching
			} // else MATCH!

			const int NSAM = sp.NSAM;
			const epicsUInt64 soe_ts = fmt_ts +
						soe_lut[soe_row].offset_us;

			int bsi = find_event_in_buf(kbuf, NSAM, soe_ts);
			if (bsi >= 0){
				build_hold_entry(kbuf, sp,
						FMT_rx->fmt[fmt_row],
						soe_lut[soe_row],
						bsi,
						ht,
						rc.events_accepted++);
			}else{
				;
			}
		}
	}

	return rc;
}

#define MARK	fprintf(stderr, "%s %d\n", FN, __LINE__)

acq400_SOE_Strategy::RC LutFmtStrategy1::operator() (
		const KBUF& kbuf,
		const SamplePrams& samplePrams, const SOE_LUT& soe_lut,
		SOE_HOLD_TABLE* ht)
{
	if (FMT_rx == 0){
		FMT_rx = acq400_FMT_rx::instance();
	}

	if (FMT_rx->waitFMT(CYCLE_MS) == 0){
		const epicsInt64 fmt_ts = FMT_rx->fmt[0].timestamp;
		ht_data_offset = offsetof(SOE_HOLD_TABLE, data)/sizeof(long);

		if (fmt_ts < kbuf.wrt0-CYCLE_MS*1000){
			return { E_FMT_TS_TOO_LATE, 0, kbuf.wrt0-fmt_ts };
		}else if (fmt_ts > kbuf.wrt1+CYCLE_MS*1000){
			fprintf(stderr, "FMT TOO EARLY %llu > %llu by %llu\n",
					fmt_ts, kbuf.wrt1, fmt_ts-kbuf.wrt1);
			return { E_FMT_TS_TOO_EARLY, 0, fmt_ts-kbuf.wrt1};
		}else{
			return soe_lut_lookup(kbuf, samplePrams, soe_lut, ht);
		}
	} else {
		return { -E_TIMEOUT, 0, 0 };
	}
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

