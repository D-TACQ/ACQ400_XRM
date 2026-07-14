/** @file acq400_SOE_Strategy.cpp
 *  @brief acq400_SOE_Strategy implementation.
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

//static const char *driverName="acq400_SOE";

#define DN	driverName
#define FN	__FUNCTION__


#define MARK	fprintf(stderr, "%s %d\n", FN, __LINE__)

/* copy first 64 samples to output, together with first 64 SOE_LUT entries, no FMT matchup */
class NullStrategy : public acq400_SOE_Strategy
{
	const int stride;

	virtual acq400_SOE_Strategy::RC operator() (
			const SOE_DIMS& soe,
			SOE_HOLD_TABLE ht);
	NullStrategy(int _stride = 1):
		acq400_SOE_Strategy(),
		stride(_stride)
	{}
friend class acq400_SOE_Strategy;
};

acq400_SOE_Strategy::RC NullStrategy::operator() (
		const SOE_DIMS& soe,
		SOE_HOLD_TABLE ht)
{
	const int SSB = soe.samplePrams.SSB;
	const int SSL = SSB/sizeof(long);
	const char* raw = soe.kbuf.raw;
	const unsigned* sp_raw =
		(const unsigned*)soe.kbuf.raw + soe.samplePrams.SP_INDEX;

	assert(SOE_LUT_ROWS >= SOE_HLD_ROWS);


	/* first 10 rows client_data becomes ib history for diags.. */
	for (int ii = 10; ii; --ii){
		ht[ii].client_data = ht[ii-1].client_data;
	}
	ht[0].client_data = soe.kbuf.ib;

	unsigned short ht_data_offset = HOLD_DATA_OFF()/sizeof(U32);

	for (int row = 0; row < SOE_HLD_ROWS; ++row,
			ht_data_offset += SSL*stride, sp_raw += SSL*stride, raw += SSB*stride){
		unsigned wrs, wrv;

		ht[row].pv_id = soe.lut[row].pv_id;
		// From FMT! ht->entries[row].client_data = soe_lut[row].client_data;

		wrv = sp_raw[SP2];
		wrs = sp_raw[SP3];

		ht[row].timestamp = getWrTs(wrs, wrv);
		ht[row].data_offset = ht_data_offset;
		memcpy((U32*)ht+ht_data_offset, raw, SSB);
	}
	ht_data_offset += SSL*stride;

	acq400_SOE_Strategy::RC rc = {
			SOE_SUCCESS, 0LL, ht_data_offset, SOE_HLD_ROWS, 0,
	};
	fprintf(stderr, "%s returning %d, %d, %lld, %d\n", FN,
				rc.status, rc.events_accepted, rc.delta_us, rc.ht_size32);

	return rc;
}


class LutFmtStrategy1 : public acq400_SOE_Strategy
{
protected:
	acq400_FMT_rx* FMT_rx;

	RC soe_lut_lookup (
			const SOE_DIMS& soe,
			const FMT& latest,
			SOE_HOLD_TABLE ht,
			acq400_SOE_Strategy::RC rc);

	RC timestamp_in_buf(
			const SOE_DIMS& soe, const epicsInt64 fmt_ts);

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
			SOE_HOLD_TABLE ht,
			int ihold);
public:
	/**
	 * allow FMT up to 80% out of range
	 * (event offset could pick).
	 * NEVER 100% because then we'd pick the adjacent (wrong) buffer every time..
	 * */
	int deadBand_us() {
		return acq400_SOE::CYCLE_MS*1000*4/5;
	}
	LutFmtStrategy1() :
		FMT_rx(0)
	{}
	virtual acq400_SOE_Strategy::RC operator() (
			const SOE_DIMS& soe,
			SOE_HOLD_TABLE ht);
friend class acq400_SOE_Strategy;
};

int  LutFmtStrategy1::build_hold_entry(
		const KBUF& kbuf,
		const SamplePrams& samplePrams,
		const FMT_ROW& fmt_row,
		const SOE_LUT_ROW& lut_row,
		int bsi,			// buffer sample index in samples
		SOE_HOLD_TABLE ht,
		int ihold)
/* return sample index to data in buffer */
{
	const int SSB = samplePrams.SSB;
	const int SSL = SSB/sizeof(long);
	struct SOE_HOLD_HEADER& entry(ht[ihold]);

	entry.pv_id = lut_row.pv_id;
	//entry.client_data = fmt_row.client_data;
	entry.client_data = bsi;
	entry.timestamp = fmt_row.timestamp;
	// entry.data_offset set later
	entry.ss_u32 = SSL;
	entry.ai_count = samplePrams.AI_COUNT;
	entry.di_count = samplePrams.DI_COUNT;
	entry.sp_count = samplePrams.SP_COUNT;
	return 0;
}

int G_raw_dump = ::getenv_default("acq400_SOE_Strategy_dump", 0);

acq400_SOE_Strategy::RC LutFmtStrategy1::soe_lut_lookup(
		const SOE_DIMS& soe,
		const FMT& latest,
		SOE_HOLD_TABLE ht,
		acq400_SOE_Strategy::RC rc)
/* FMT, SOE_LUT assumed to be sorted by event */
{
	const int SSB = soe.samplePrams.SSB;
	const int SSL = SSB/sizeof(long);

	int bsi_entries[SOE_HLD_ROWS];
	int fmt_row = 0;
	int imatch = 0;

	for (; fmt_row < FMT_ROWS; ++fmt_row){
		const epicsUInt64 fmt_ts = latest[fmt_row].timestamp;
		const epicsUInt16 fmt_event = latest[fmt_row].event;
		if (fmt_event == EV99){
			break;
		}
		for (int soe_row = 0; soe_row < SOE_LUT_ROWS; ++soe_row){
			const epicsUInt16 soe_lut_event = soe.lut[soe_row].event;
			if (soe_lut_event == EV99){
				break;
			}else if (soe_lut_event > fmt_event){
				break;
			}else if (soe_lut_event != fmt_event){
				continue;                        // keep searching
			} // else MATCH!

			const int NSAM = soe.samplePrams.NSAM;
			const epicsUInt64 soe_ts = fmt_ts +
						soe.lut[soe_row].offset_us;

			int bsi = find_event_in_buf(soe.kbuf, NSAM, soe_ts);
			if (bsi >= 0){
				build_hold_entry(soe.kbuf, soe.samplePrams,
						latest[fmt_row],
						soe.lut[soe_row],
						bsi,
						ht,
						imatch);
				bsi_entries[imatch] = bsi;
				imatch += 1;
				rc.events_accepted++;
			}else{
				rc.events_not_in_buffer++;
			}
		}
	}

	int ht_data_offset = (imatch+1)*sizeof(SOE_HOLD_HEADER)/sizeof(U32);

	for (int ii = 0; ii < imatch; ++ii, ht_data_offset += SSL){
		const unsigned* sample_raw =
				(const U32*)soe.kbuf.raw + bsi_entries[ii]*SSL;
/*
		fprintf(stderr, "%s raw:%p from %p + %d\n",
				sample_raw, kbuf.raw, bsi_entries[ii]*SSL);
		fprintf(stderr, "%s memcpy( %p, %p, %d)\n",
				FN, (U32*)ht + ht_data_offset, sample_raw, SSB);
*/
		memcpy((U32*)ht + ht_data_offset, sample_raw, SSB);
		ht[ii].data_offset = ht_data_offset;
	}

	rc.ht_size32 = ht_data_offset;
	/*
	fprintf(stderr, "%s returning STATUS:%d, ACCEPT:%d, EV_NIB:%d DUS:%lld, SZ:%d\n",
			FN,
			rc.status, rc.events_accepted,
			rc.events_not_in_buffer, rc.delta_us, rc.ht_size32);
	*/
	if (G_raw_dump){
		FILE *fp = fopen("/tmp/ht.dat", "w");
		assert(fp != 0);
		fwrite(ht, sizeof(U32), rc.ht_size32, fp);
		fclose(fp);
	}
	return rc;
}

#define MARK	fprintf(stderr, "%s %d\n", FN, __LINE__)


acq400_SOE_Strategy::RC LutFmtStrategy1::timestamp_in_buf(
		const SOE_DIMS& soe, const epicsInt64 fmt_ts)
{
	if (fmt_ts < soe.kbuf.wrt0-deadBand_us()){
		return { E_FMT_TS_TOO_LATE_FOR_KB, soe.kbuf.wrt0-fmt_ts, };
	}else if (fmt_ts > soe.kbuf.wrt1+deadBand_us()){
		fprintf(stderr, "FMT TOO EARLY %llu > %llu by %llu\n",
				fmt_ts, soe.kbuf.wrt1, fmt_ts-soe.kbuf.wrt1);
		return { E_FMT_TS_TOO_EARLY_FOR_KB, fmt_ts-soe.kbuf.wrt1, };
	}else{
		return { SOE_SUCCESS, fmt_ts - soe.kbuf.wrt0};
	}
}
acq400_SOE_Strategy::RC LutFmtStrategy1::operator() (
		const SOE_DIMS& soe,
		SOE_HOLD_TABLE ht)
{
	if (FMT_rx == 0){
		FMT_rx = acq400_FMT_rx::instance();
	}

	if (FMT_rx->waitFMT(acq400_SOE::CYCLE_MS) == 0){
		const FMT& latest = FMT_rx->get_fmt(0);
		const epicsInt64 fmt_ts = latest[0].timestamp;

		acq400_SOE_Strategy::RC rc = timestamp_in_buf(soe, fmt_ts);
		if (rc.status == SOE_SUCCESS){
			rc = soe_lut_lookup(soe, latest, ht, rc);
		}
		return rc;
	} else {
		return { E_TIMEOUT, };
	}
}

class LutFmtStrategy2 : public LutFmtStrategy1
{
protected:

public:
	LutFmtStrategy2(): LutFmtStrategy1()
	{}
	virtual acq400_SOE_Strategy::RC operator() (
			const SOE_DIMS& soe,
			SOE_HOLD_TABLE ht);
friend class acq400_SOE_Strategy;
};

acq400_SOE_Strategy::RC LutFmtStrategy2::operator() (
			const SOE_DIMS& soe,
			SOE_HOLD_TABLE ht)
{
	if (FMT_rx == 0){
		FMT_rx = acq400_FMT_rx::instance();
	}

	const FMT& fmt = FMT_rx->get_fmt(0);

	acq400_SOE_Strategy::RC rc = timestamp_in_buf(soe, fmt[0].timestamp);
	switch(rc.status){
	case SOE_SUCCESS:
		rc.fmt_num = FMT_CUR;
		return soe_lut_lookup(soe, fmt, ht, rc);
	case E_FMT_TS_TOO_EARLY_FOR_KB: {
		const FMT& fmt_m1 = FMT_rx->get_fmt(1);

		rc = timestamp_in_buf(soe, fmt_m1[0].timestamp);
		rc.fmt_num = FMT_PRE;
		switch(rc.status){
		case SOE_SUCCESS:
			return soe_lut_lookup(soe, fmt_m1, ht, rc);
		default:
			return rc;
		}
	} case E_FMT_TS_TOO_LATE_FOR_KB:
		for (unsigned retry = 0, delay = acq400_SOE::CYCLE_MS/5; retry < 5; ++retry){
			if (FMT_rx->waitFMT(delay) == 0){
				const FMT& latest = FMT_rx->get_fmt(0);
				const epicsInt64 fmt_ts = latest[0].timestamp;
				if (fmt_ts == fmt[0].timestamp){
					continue;              // bogus:event was already set from a previous run
				}else{
					acq400_SOE_Strategy::RC rc = timestamp_in_buf(soe, fmt_ts);

					rc = soe_lut_lookup(soe, latest, ht, rc);
					rc.fmt_num = FMT_WAIT;
					rc.next_retry = retry;
					return rc;
				}
			}
		}
		rc.status = E_TIMEOUT;
		rc.fmt_num = FMT_WAIT;
		return rc;
	default:
		assert(rc.status == E_FMT_UNDEFINED);
	}
	return { E_FMT_UNDEFINED, };   // fake return for compiler, "never called"
}


acq400_SOE_Strategy** acq400_SOE_Strategy::factory()
{
	static acq400_SOE_Strategy* strategies[10] = { 0, };
	if (strategies[0] == 0){
		int is = 0;
		strategies[is++] = new LutFmtStrategy1();
		strategies[is++] = new LutFmtStrategy2();
		strategies[is++] = new NullStrategy(1);
		strategies[is++] = new NullStrategy(2);
		strategies[is++] = new NullStrategy(5);
		strategies[is++] = new NullStrategy(10);
		strategies[is++] = new NullStrategy(20);
	}
	return strategies;
}

