/*
 * acq400_SOE.cpp
 *
 *  Created on: 23 Feb 2026
 *      Author: pgm
 */

#include "acq400_asyn_common.h"
#include "acq400_SOE.h"
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

#define MARK	fprintf(stderr, "%s %d\n", FN, __LINE__)
#define MARKI(p) fprintf(stderr, "%s %d P_ %s:%d\n", FN, __LINE__, #p, p)

int acq400_SOE::nice = ::getenv_default("acq400_SOE_NICE", 0);

acq400_SOE::acq400_SOE(const char* portName, acq400_SOE_Strategy* _strategy):
	acq400_asynPortDriver(portName,
	/* maxAddr */		SOE_HLD_ROWS,    /* nchan from 0 */
	/* Interface mask */    asynEnumMask|asynOctetMask|asynInt32Mask|asynInt64Mask|asynFloat64Mask|
				asynInt8ArrayMask|asynInt16ArrayMask|asynInt32ArrayMask|
				asynFloat32ArrayMask|asynInt64ArrayMask|asynDrvUserMask,
	/* Interrupt mask */	asynEnumMask|asynOctetMask|asynInt32Mask|asynInt64Mask|asynFloat64Mask|
				asynInt8ArrayMask|asynInt16ArrayMask|asynInt32ArrayMask|
				asynFloat32ArrayMask|asynInt64ArrayMask,
	/* asynFlags no block*/ 0,
	/* Autoconnect */       1,
	/* Default priority */  0,
	/* Default stack size*/ 0),
	strategy(_strategy),
	update(0),
	fmt_rx_timeouts(0), fmt_rx_success(0),
	hold_row_limit(SOE_HLD_ROWS)
{
	fprintf(stderr, "%s R1032\n", FN);
	asynStatus status = asynSuccess;
	memset(soe_lut, 0, sizeof(soe_lut));

	assert(SOE_LUT_ROWS < 0xffU);
	for (epicsInt8 row = 0; row < SOE_LUT_ROWS; ++row){
		cols.c_rownum[row] = row;
		soe_lut[row].event = 1000+row;
		soe_lut[row].pv_id = 2000+row;
		soe_lut[row].offset_us = row*2;
	}

	assert(SOE_HLD_ROWS < 0xffU);
	for (epicsInt8 row = 0; row < SOE_HLD_ROWS; ++row){
		hold_cols.c_rownum[row] = row;
		/* .. temp will get overwritten from raw TABLE copy */
		hold_cols.c_DI1[row] = 0xd1;
		hold_cols.c_DI2[row] = 0xd2;
		hold_cols.c_SP0[row] = row;
		hold_cols.c_SP1[row] = 0x12345678;
	}


	eventId = epicsEventCreate(epicsEventEmpty);

	createParam(PS_RUNSTOP,  asynParamInt32,        &P_RUNSTOP);
	createParam(PS_UPDATES,  asynParamInt32,        &P_UPDATES);
	createParam(PS_TS_USEC,  asynParamInt64,	&P_TS_USEC);

	createParam(PS_SOE_STRATEGY, asynParamInt32,  &P_SOE_STRATEGY);

	createParam(PS_SOE_AGG_SITES,		asynParamOctet,      &P_SOE_AGG_SITES);
	createParam(PS_SOE_SITE_SSB,		asynParamInt32,      &P_SOE_SITE_SSB);
	createParam(PS_SOE_SITE_IS_ADC,		asynParamInt32,      &P_SOE_SITE_IS_ADC);
	createParam(PS_SOE_SMPL_SS_U32,		asynParamInt32,      &P_SOE_SMPL_SS_U32);
	createParam(PS_SOE_SMPL_NSAM,		asynParamInt32,      &P_SOE_SMPL_NSAM);
	createParam(PS_SOE_SMPL_AI_COUNT,	asynParamInt32,	     &P_SOE_SMPL_AI_COUNT);
	createParam(PS_SOE_SMPL_DI_COUNT,	asynParamInt32,	     &P_SOE_SMPL_DI_COUNT);
	createParam(PS_SOE_SMPL_SP_COUNT,	asynParamInt32,	     &P_SOE_SMPL_SP_COUNT);
	createParam(PS_SOE_SMPL_DI_INDEX, 	asynParamInt32,	     &P_SOE_SMPL_DI_INDEX);
	createParam(PS_SOE_SMPL_SP_INDEX, 	asynParamInt32,	     &P_SOE_SMPL_SP_INDEX);

	createParam(PS_SOE_LUT_COL_ROWNUM,	asynParamInt8Array,  &P_SOE_LUT_COL_ROWNUM);
	createParam(PS_SOE_LUT_COL_EVENT,	asynParamInt16Array, &P_SOE_LUT_COL_EVENT);
	createParam(PS_SOE_LUT_COL_PAD,		asynParamInt16Array, &P_SOE_LUT_COL_PAD);
	createParam(PS_SOE_LUT_COL_PV_ID,	asynParamInt32Array, &P_SOE_LUT_COL_PV_ID);
	createParam(PS_SOE_LUT_COL_OFFSET_US,	asynParamInt32Array, &P_SOE_LUT_COL_OFFSET_US);

	createParam(PS_SOE_KBUF_INDEX,		asynParamInt32,	     &P_SOE_KBUF_INDEX);
	createParam(PS_SOE_KBUF_WRT0,    	asynParamInt64,      &P_SOE_KBUF_WRT0);
	createParam(PS_SOE_KBUF_WRT1,    	asynParamInt64,      &P_SOE_KBUF_WRT1);

	createParam(PS_SOE_HLD_COL_ROWNUM,	asynParamInt8Array,  &P_SOE_HLD_COL_ROWNUM);
	createParam(PS_SOE_HLD_COL_PV_ID,    	asynParamInt32Array, &P_SOE_HLD_COL_PV_ID);
	createParam(PS_SOE_HLD_COL_CLIDAT,    	asynParamInt32Array, &P_SOE_HLD_COL_CLIDAT);
	createParam(PS_SOE_HLD_COL_TS,    	asynParamInt64Array, &P_SOE_HLD_COL_TS);
	createParam(PS_SOE_HLD_COL_AI1,	asynParamFloat32Array, &P_SOE_HLD_COL_AI1);
	createParam(PS_SOE_HLD_COL_AI2,	asynParamFloat32Array, &P_SOE_HLD_COL_AI2);

	createParam(PS_SOE_HLD_COL_DI1,	asynParamInt32Array, &P_SOE_HLD_COL_DI1);
	createParam(PS_SOE_HLD_COL_DI2,	asynParamInt32Array, &P_SOE_HLD_COL_DI2);
/* SP0 : SPAD0 aka Sample Number
 * SP1 : SPAD1 aka local clock us
 * SP2 : SPAD2 aka WR_VERNIER
 */
	createParam(PS_SOE_HLD_COL_SP0,	asynParamInt32Array, &P_SOE_HLD_COL_SP0);
	createParam(PS_SOE_HLD_COL_SP1,	asynParamInt32Array, &P_SOE_HLD_COL_SP1);
	createParam(PS_SOE_HLD_COL_SP2, asynParamInt32Array, &P_SOE_HLD_COL_SP2);
	createParam(PS_SOE_HLD_COL_SP3, asynParamInt32Array, &P_SOE_HLD_COL_SP3);
	createParam(PS_SOE_HLD_COL_WRVS, asynParamInt8Array, &P_SOE_HLD_COL_WRVS);
	createParam(PS_SOE_HLD_COL_WRVT, asynParamInt32Array, &P_SOE_HLD_COL_WRVT);
	createParam(PS_SOE_HLD_COL_WRUS, asynParamInt64Array, &P_SOE_HLD_COL_WRUS);


	createParam(PS_SOE_HLD_COL_DATA_OFFSET, asynParamInt32,      &P_SOE_HLD_COL_DATA_OFFSET);

	createParam(PS_SOE_LUT_REDIT_ROW, 	asynParamInt32,      &P_SOE_LUT_REDIT_ROW);


	createParam(PS_SOE_LUT_REDIT_ROWCOUNT, 	asynParamInt32, &P_SOE_LUT_REDIT_ROWCOUNT);
	createParam(PS_SOE_LUT_REDIT_EVENT,	asynParamInt32, &P_SOE_LUT_REDIT_EVENT);
	createParam(PS_SOE_LUT_REDIT_EVENT_STEP,asynParamInt32, &P_SOE_LUT_REDIT_EVENT_STEP);
	createParam(PS_SOE_LUT_REDIT_PV_ID, 	asynParamInt32, &P_SOE_LUT_REDIT_PV_ID);
	createParam(PS_SOE_LUT_REDIT_PV_ID_STEP,asynParamInt32, &P_SOE_LUT_REDIT_PV_ID_STEP);
	createParam(PS_SOE_LUT_REDIT_OFFSET_US, asynParamInt32, &P_SOE_LUT_REDIT_OFFSET_US);
	createParam(PS_SOE_LUT_REDIT_OFFSET_US_STEP,asynParamInt32, &P_SOE_LUT_REDIT_OFFSET_US_STEP);
	createParam(PS_SOE_LUT_REDIT_COMMIT,	asynParamInt32, &P_SOE_LUT_REDIT_COMMIT);

	createParam(PS_SOE_FMT_RX_TIMEOUTS,	asynParamInt32, &P_SOE_FMT_RX_TIMEOUTS);
	createParam(PS_SOE_FMT_RX_TIMEOUT_REASON, asynParamInt32, &P_SOE_FMT_RX_TIMEOUT_REASON);
	createParam(PS_SOE_FMT_DELTA_TS,	asynParamInt64, &P_SOE_FMT_DELTA_TS);
	createParam(PS_SOE_FMT_RX_SUCCESS,	asynParamInt32, &P_SOE_FMT_RX_SUCCESS);
	createParam(PS_SOE_FMT_EV_MATCHES,      asynParamInt32, &P_SOE_FMT_EV_MATCHES);
	createParam(PS_SOE_FMT_EV_NIB,		asynParamInt32, &P_SOE_FMT_EV_NIB);
	createParam(PS_SOE_HLD_TABLE_WF,	asynParamInt32Array, &P_SOE_HLD_TABLE_WF);

	/* Create the thread that computes the waveforms in the background */
	status = (asynStatus)(epicsThreadCreate("SOE_task",
			epicsThreadPriorityHigh - nice,
			epicsThreadGetStackSize(epicsThreadStackMedium),
			(EPICSTHREADFUNC)task_runner,
			this) == NULL);
	if (status) {
		printf("%s:%s: epicsThreadCreate failure\n", DN, FN);
		return;
	}
}


void acq400_SOE::task_runner(void *drvPvt)
{
	((acq400_SOE *)drvPvt)->task();
}

void acq400_SOE::update_soe_lut(bool first_time)
{
	;
}
void acq400_SOE::update_soe_lut_columns(void)
{
	for (int row = 0; row < SOE_LUT_ROWS; ++row){
		cols.c_event[row]= soe_lut[row].event;
		cols.c_pad[row] = soe_lut[row].pad;
		cols.c_pv_id[row] = soe_lut[row].pv_id;
		cols.c_offset_us[row] = soe_lut[row].offset_us;
	};
}
void acq400_SOE::update_soe_lut_callbacks(void)
{
	setIntegerParam(P_UPDATES, ++update);
	//setInteger64Param(P_TS_USEC, now_us);

	callParamCallbacks();
	doCallbacksInt8Array(cols.c_rownum, FMT_ROWS, P_SOE_LUT_COL_ROWNUM, 0);
	doCallbacksInt16Array(cols.c_event, FMT_ROWS, P_SOE_LUT_COL_EVENT, 0);
	doCallbacksInt16Array(cols.c_pad, FMT_ROWS, P_SOE_LUT_COL_PAD, 0);
	doCallbacksInt32Array(cols.c_pv_id, FMT_ROWS, P_SOE_LUT_COL_PV_ID, 0);
	doCallbacksInt64Array(cols.c_offset_us, FMT_ROWS, P_SOE_LUT_COL_OFFSET_US, 0);
}

void acq400_SOE::init_the_hold_table()
{
	const int SSB = samplePrams.SSB;
	the_hold_table = (SOE_HOLD_TABLE)(new unsigned[HOLD_MAX_NELM(SSB)]);

	memset(the_hold_table, 0, HOLD_MAXSIZE(SSB));

	fprintf(stderr, "%s SOE_HLD_ROWS %d ssb:%d maxb:%d\n",
			FN, SOE_HLD_ROWS, SSB, HOLD_MAXSIZE(SSB));
}

void acq400_SOE::clearHold() {
	for (int row = 0; row < SOE_HLD_ROWS; ++row){
		the_hold_table[row].pv_id = 0;
	}
}

// @@todo replace with HAS_ES PV!
#define SKIP_ES 1

static int SP1_SIM = 0;


void acq400_SOE::update_hld_tab_columns()
{
	int row;

	for (row = 0; row < SOE_HLD_ROWS; ++row){
		if (the_hold_table[row].pv_id == 0){
			break;
		}
		U32* raw = (U32*)the_hold_table + the_hold_table[row].data_offset;
		short* ai_raw = (short*)raw;
		int * di_raw = (int*)raw + samplePrams.DI_INDEX;
		int * sp_raw = (int*)raw + samplePrams.SP_INDEX;

		hold_cols.c_pv_id[row] = the_hold_table[row].pv_id;
		hold_cols.c_client_data[row] = the_hold_table[row].client_data;
		hold_cols.c_timestamp[row] = the_hold_table[row].timestamp;

		hold_cols.c_AI1[row] = ai_raw[0]*10.0/32768;
		hold_cols.c_AI2[row] = ai_raw[1]*10.0/32768;
		hold_cols.c_DI1[row] = di_raw[0];
		if (samplePrams.DI_COUNT > 1){
			hold_cols.c_DI2[row] = ++SP1_SIM;    // fake data
		}else{
			hold_cols.c_DI2[row] = di_raw[1];
		}
		hold_cols.c_SP0[row] = sp_raw[SP0];
		hold_cols.c_SP1[row] = sp_raw[SP1];

		unsigned wrs, wrv;

		hold_cols.c_SP2[row] = wrv = sp_raw[SP2];
		hold_cols.c_SP3[row] = wrs = sp_raw[SP3];
		hold_cols.c_WRVS[row]= (sp_raw[SP2] >> 28)&0x07;
		hold_cols.c_WRVT[row]= sp_raw[SP2]&0x0fffffff;
		hold_cols.c_WRUS[row]= getWrTs(wrs, wrv);
	}
	hold_row_limit = row;
}

void acq400_SOE::update_hld_tab_callbacks(int n_u32)
{
	fprintf(stderr, "%s: NORD:%d\n", FN, n_u32);
	doCallbacksInt32Array((epicsInt32*)the_hold_table, n_u32, P_SOE_HLD_TABLE_WF, 0);
}
void acq400_SOE::update_hld_tab_columns_callbacks(void)
{
	doCallbacksInt8Array(hold_cols.c_rownum, 	hold_row_limit, P_SOE_HLD_COL_ROWNUM, 0);
	doCallbacksInt32Array(hold_cols.c_pv_id, 	hold_row_limit, P_SOE_HLD_COL_PV_ID, 0);
	doCallbacksInt32Array(hold_cols.c_client_data,  hold_row_limit, P_SOE_HLD_COL_CLIDAT, 0);
	doCallbacksInt64Array(hold_cols.c_timestamp, 	hold_row_limit, P_SOE_HLD_COL_TS, 0);
	doCallbacksFloat32Array(hold_cols.c_AI1, 	hold_row_limit, P_SOE_HLD_COL_AI1, 0);
	doCallbacksFloat32Array(hold_cols.c_AI2, 	hold_row_limit, P_SOE_HLD_COL_AI2, 0);
	doCallbacksInt32Array(hold_cols.c_DI1, 		hold_row_limit, P_SOE_HLD_COL_DI1, 0);
	doCallbacksInt32Array(hold_cols.c_DI2, 		hold_row_limit, P_SOE_HLD_COL_DI2, 0);
	doCallbacksInt32Array(hold_cols.c_SP0, 		hold_row_limit, P_SOE_HLD_COL_SP0, 0);
	doCallbacksInt32Array(hold_cols.c_SP1, 		hold_row_limit, P_SOE_HLD_COL_SP1, 0);
	doCallbacksInt32Array(hold_cols.c_SP2, 		hold_row_limit, P_SOE_HLD_COL_SP2, 0);
	doCallbacksInt32Array(hold_cols.c_SP3, 		hold_row_limit, P_SOE_HLD_COL_SP3, 0);
	doCallbacksInt8Array( hold_cols.c_WRVS, 	hold_row_limit, P_SOE_HLD_COL_WRVS, 0);
	doCallbacksInt32Array(hold_cols.c_WRVT, 	hold_row_limit, P_SOE_HLD_COL_WRVT, 0);
	doCallbacksInt64Array(hold_cols.c_WRUS, 	hold_row_limit, P_SOE_HLD_COL_WRUS, 0);
}


typedef std::vector<std::string> VS;


void acq400_SOE::get_sample_dimensions()
{
	char site_list[80] = {};
	gsp(P_SOE_AGG_SITES, 80, site_list);
	fprintf(stderr, "SOE_AGG_SITES \"%s\"\n", site_list);

	int modules_ssb_total = 0;
	int first_di_index = 0;
	int ssb;
	bool first_di = true;
	int modules_ai_ssb = 0;
	int modules_di_ssb = 0;

	VIS agg_sites;
	split2(site_list, agg_sites, ',');

	for (int site: agg_sites){
		int is_adc;

		gip(site, P_SOE_SITE_IS_ADC, &is_adc);

		if (!is_adc && first_di){
			first_di_index = modules_ssb_total/sizeof(short);
			sip(0, P_SOE_SMPL_DI_INDEX, first_di_index);
			first_di = false;
		}


		gip(site, P_SOE_SITE_SSB, &ssb);
		modules_ssb_total += ssb;
		if (is_adc){
			modules_ai_ssb += ssb;
		}else{
			modules_di_ssb += ssb;
		}

		fprintf(stderr, "%s:%s %d ssb:%d is_adc?:%d first_di_index:%d modules_ssb_total:%d\n",
				DN, FN, site, ssb, is_adc, first_di_index, modules_ssb_total);
	}
	gip(0, P_SOE_SITE_SSB, &ssb);
	samplePrams.SSB = ssb;
	gip(0, P_SOE_SMPL_NSAM, &samplePrams.NSAM);


	int modules_ssl = modules_ssb_total/sizeof(long);
	int agg_ssl = ssb/sizeof(long);
	assert(agg_ssl >= modules_ssl);


	samplePrams.AI_INDEX = 0;
	sip(0, P_SOE_SMPL_AI_COUNT, samplePrams.AI_COUNT = modules_ai_ssb/sizeof(AI16_t));

	sip(0, P_SOE_SMPL_DI_INDEX, samplePrams.DI_INDEX = modules_ai_ssb/sizeof(DI32_t));
	sip(0, P_SOE_SMPL_DI_COUNT, samplePrams.DI_COUNT = modules_di_ssb/sizeof(DI32_t));

	sip(0, P_SOE_SMPL_SP_INDEX, samplePrams.SP_INDEX = modules_ssl);
	sip(0, P_SOE_SMPL_SP_COUNT, agg_ssl-modules_ssl);

	callParamCallbacks();
}

epicsInt64 getWrTsFromRaw(unsigned* sp_raw)
{
	unsigned wrv = sp_raw[SP2];
	unsigned wrs = sp_raw[SP3];
	return getWrTs(wrs, wrv);
}



void acq400_SOE::update_kbuf_info(char* raw)
{
	unsigned * sp_raw = (unsigned*)raw + samplePrams.SP_INDEX;
	current_kb.wrt0 = getWrTsFromRaw(sp_raw);

	const int SSL = samplePrams.SSB/sizeof(long);
	sp_raw += SSL*(samplePrams.NSAM-1-SKIP_ES);
	current_kb.wrt1 = getWrTsFromRaw(sp_raw);

	sip(0, P_SOE_KBUF_INDEX, current_kb.ib = ib);
	sip(0, P_SOE_KBUF_WRT0,  current_kb.wrt0);
	sip(0, P_SOE_KBUF_WRT1,  current_kb.wrt1);

	current_kb.raw = raw;
}

void acq400_SOE::task()
{
	epicsEventWait(eventId);

	int fc = open("/dev/acq400.0.bq", O_RDONLY);
	assert(fc >= 0);


	if ((ib = getBufferId(fc)) < 0){
		fprintf(stderr, "ERROR: getBufferId() fail");
		return;
	}


	get_sample_dimensions();
	init_the_hold_table();
	callParamCallbacks();

	for (int runstop, runstop0 = 0; (ib = getBufferId(fc)) >= 0;
							runstop0 = runstop){
		lock();
		gip(P_RUNSTOP, &runstop);
		unlock();

		if (runstop == 1){
			if (runstop0 == 0){
				;   // onStart actions
			}
			clearHold();
			update_soe_lut_columns(); // cosmetic stuff in slack time.
			char* raw = Buffer::the_buffers[ib]->getBase() +
					SKIP_ES*samplePrams.SSB;

			update_kbuf_info(raw);

			const acq400_SOE_Strategy::RC rc =
					(*strategy)(current_kb, samplePrams,
						 soe_lut, the_hold_table);

			sip(0, P_SOE_FMT_RX_TIMEOUT_REASON, rc.status);
			sip(0, P_SOE_FMT_DELTA_TS, rc.delta_us);
			sip(0, P_SOE_FMT_EV_MATCHES, rc.events_accepted);
			sip(0, P_SOE_FMT_EV_NIB, rc.events_not_in_buffer);

			if (rc.status != 0){
				sip(0, P_SOE_FMT_RX_TIMEOUTS, ++fmt_rx_timeouts);
				sip(0, P_SOE_FMT_RX_TIMEOUT_REASON, rc.status);

				lock();
				callParamCallbacks();
				unlock();
			}else{
				sip(0, P_SOE_FMT_RX_SUCCESS, ++fmt_rx_success);
				update_hld_tab_columns();
				lock();
				callParamCallbacks();
				update_hld_tab_callbacks(rc.ht_size32);
				update_soe_lut_callbacks();
				unlock();
				/* now lower priority .. maybe at subrate? */
				lock();
				update_hld_tab_columns_callbacks();
				unlock();
			}
		}
	}

	close(fc);
}


void acq400_SOE::redit()
{
	int row, row_count, event, event_step, pv_id, pv_id_step, offset_us, offset_us_step;

	fprintf(stderr, "%d %d %d %d %d %d %d %d\n",
			P_SOE_LUT_REDIT_ROW,
			P_SOE_LUT_REDIT_ROWCOUNT,
			P_SOE_LUT_REDIT_EVENT,
			P_SOE_LUT_REDIT_EVENT_STEP,
			P_SOE_LUT_REDIT_PV_ID,
			P_SOE_LUT_REDIT_PV_ID_STEP,
			P_SOE_LUT_REDIT_OFFSET_US,
			P_SOE_LUT_REDIT_OFFSET_US_STEP);

	if (gip(P_SOE_LUT_REDIT_ROW, 	&row)			||
	    gip(P_SOE_LUT_REDIT_ROWCOUNT, 	&row_count)	||
	    gip(P_SOE_LUT_REDIT_EVENT,    	&event)		||
	    gip(P_SOE_LUT_REDIT_EVENT_STEP, &event_step)	||
	    gip(P_SOE_LUT_REDIT_PV_ID,     &pv_id)		||
	    gip(P_SOE_LUT_REDIT_PV_ID_STEP,&pv_id_step)		||
	    gip(P_SOE_LUT_REDIT_OFFSET_US,     &offset_us)	||
	    gip(P_SOE_LUT_REDIT_OFFSET_US_STEP,&offset_us_step)	){
		return;
	}

	fprintf(stderr, "%s:%s %d,%d %d,%d %d,%d, %d, %d\n", DN, FN,
			row, row_count, event, event_step, pv_id, pv_id_step, offset_us, offset_us_step);

	lock();
	for (int rn = 0; rn < row_count; ++rn){
		struct SOE_LUT_ROW& this_row = soe_lut[row+rn];
		this_row.event = event + rn*event_step;
		this_row.pv_id = pv_id + rn*pv_id_step;
		this_row.offset_us = offset_us +rn*offset_us_step;
	}
	unlock();
}
asynStatus acq400_SOE::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
	    int function = pasynUser->reason;
	    asynStatus status = asynSuccess;
	    const char *paramName;
	    int addr;

	    /* Fetch the parameter string name for possible use in debugging */
	    getParamName(function, &paramName);

	    status = pasynManager->getAddr(pasynUser, &addr);
	    if(status!=asynSuccess) return status;

	    /* Set the parameter in the parameter library. */
	    status = (asynStatus) setIntegerParam(addr, function, value);

	    fprintf(stderr,
	    	              "%s:%s: function=%d, addr=%d, name=%s, value=%d\n",
	    	              DN, FN, function, addr, paramName, value);

	    if (function == P_RUNSTOP) {
	        if (value) epicsEventSignal(eventId);
	    }else if (function == P_SOE_LUT_REDIT_COMMIT){
		    redit();
	    }else if (function == P_SOE_STRATEGY){
		   acq400_SOE_Strategy** strategies = acq400_SOE_Strategy::factory();
		   status = asynError;

		   for (int ii = 0; strategies[ii] != 0; ++ii){
			   if (value == ii){
				   strategy = strategies[ii];
				   status = asynSuccess;
				   break;
			   }
		   }
	    }

	    /* Do callbacks so higher layers see any changes */
	    status = (asynStatus) callParamCallbacks();

	    if (status)
	        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
	                  "%s:%s: status=%d, function=%d, name=%s, value=%d",
	                  DN, FN, status, function, paramName, value);
	    else
	        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
	              "%s:%s: function=%d, name=%s, value=%d\n",
	              DN, FN, function, paramName, value);
	    return status;
}

extern "C" {
	/** EPICS iocsh callable function to call constructor for the testAsynPortDriver class.
	  * \param[in] portName The name of the asyn port driver to be created.
	  */
	int acq400_SOE_Configure(const char *portName)
	{
		printf("%s:%s R1001 %s\n", DN, FN, portName);

		new acq400_SOE(portName, acq400_SOE_Strategy::factory()[0]);
		return 0;
	}

	/* EPICS iocsh shell commands */

	static const iocshArg initArg0 = { "port", iocshArgString };
	static const iocshArg * const initArgs[] = { &initArg0 };
	static const iocshFuncDef initFuncDef = { "acq400_SOE_Configure", 1, initArgs };
	static void initCallFunc(const iocshArgBuf *args)
	{
		acq400_SOE_Configure(args[0].sval);
	}

	void acq400_SOE_Register(void)
	{
	    iocshRegister(&initFuncDef, initCallFunc);
	}

	epicsExportRegistrar(acq400_SOE_Register);
}


