/*
 * xrm_structs.h
 *
 *  Created on: 6 Feb 2026
 *      Author: pgm
 */

#include <epicsTypes.h>

#ifndef XRMIOCAPP_SRC_XRM_STRUCTS_H_
#define XRMIOCAPP_SRC_XRM_STRUCTS_H_

/* FMT : FNAL Multicast Table
 * input from plant: 20Hz
 */
struct FMT_ROW {
	epicsUInt16 event;           // FNAL Event number
	epicsUInt16 pad;             // 32 bit alignment is best, available for future
	epicsUInt32 client_data;     // opaque value to pass back
	epicsUInt64 timestamp;       // WR usec from EPOCH
};

const int FMT_ROWS = 64;
//#define FMT_ROWS 64

typedef struct FMT_ROW  FMT[FMT_ROWS];

/* SOE_LUT : Sample On Event Lookup Table
 * input from plant: as required (EPICS PV)
 */
struct SOE_LUT_ROW {
	epicsUInt16 event;           // FNAL Event number
	epicsUInt16 pad;             // 32 bit alignment is best, available for future
	epicsUInt32 pv_id;
	epicsInt32 offset_us;        // Before or After Event, to the limit of data this cycle
};

const int SOE_LUT_ROWS = 64;

typedef struct SOE_LUT_ROW  SOE_LUT[SOE_LUT_ROWS];

/* SOE HOLD TABLE
 * This is the OUTPUT from each CYCLE
 * First, a max 64 element array of headers, then N fixed size rows of channel data.
 */

struct SOE_HOLD_HEADER {
	epicsUInt32 pv_id;		// links Event and Offset
	epicsUInt32 client_data;	// copied from FMT (if required)
	epicsUInt64 timestamp;		// cross check: which FMT update this derives from.
	epicsUInt16 data_offset;	// offset in u32 in data array
	/* description of raw sample from hardware
	 * it's not totally raw because all AI are presented as calibrated V.
	 * but after that a series of U32 representing DI, SPAD
	 * this is not in the spec, but will be useful for validation.
	 */
	epicsUInt8  ss_u32;		// sample size  (u32)
	epicsUInt8  ai_count;           // number of AI (floats) in data
	epicsUInt8  di_count;           // number of DI (u32) in data
	epicsUInt8  sp_count;           // number of SP (u32) in data
};

const int SPAD0_SC = 0;                   // SPAD[0] is sample count (u32)
const int SPAD1_TS = 1;                   // SPAD[1] is WR TS 3 bit seconds, 28 bit ticks

const int SOE_HOLD_HEADER_ROWS = 64;




typedef struct {
	struct SOE_HOLD_HEADER entries[SOE_HOLD_HEADER_ROWS];
	epicsUInt32 data[1];             // first data word. Many more to follow
} SOE_HOLD_TABLE;

/* actual sample data:
 */

const int XRM_MAGPS_AI32 = 32;
const int XRM_MAGPS_DI32 =  2;
const int XRM_MAGPS_SP32 =  6;   // pad to round number

struct XRM_MAGPS_SAMPLE {
	epicsFloat32 ai[XRM_MAGPS_AI32];
	epicsUInt32  di[XRM_MAGPS_DI32];
	epicsUInt32  sp[XRM_MAGPS_SP32];
};


const int XRM_QPMS_AI32 = 128;
const int XRM_QPMS_DI32 =   0;
const int XRM_QPMS_SP32 =   8;

struct XRM_QPMS_SAMPLE{
	epicsFloat32 ai[XRM_MAGPS_AI32];
	epicsUInt32  di[XRM_MAGPS_DI32];
	epicsUInt32  sp[XRM_MAGPS_SP32];
};

const int XRM_INST_A_AI32 = 32;
const int XRM_INST_A_DI32 =  0;
const int XRM_INST_A_SP32 =  4;

struct XRM_INST_A_SAMPLE {
	epicsFloat32 ai[XRM_INST_A_AI32];
	epicsUInt32  di[XRM_INST_A_DI32];
	epicsUInt32  sp[XRM_INST_A_SP32];
};

const int XRM_INST_B_AI32 = 32;
const int XRM_INST_B_DI32 =  1;
const int XRM_INST_B_SP32 =  3;

struct XRM_INST_B_SAMPLE {
	epicsFloat32 ai[XRM_INST_A_AI32];
	epicsUInt32  di[XRM_INST_A_DI32];
	epicsUInt32  sp[XRM_INST_A_SP32];
};


void print(FMT& fmt, bool verbose = false);
void print(SOE_LUT& lut, bool verbose = false);
void print(SOE_HOLD_TABLE& hold, bool verbose = false);




#endif /* XRMIOCAPP_SRC_XRM_STRUCTS_H_ */
