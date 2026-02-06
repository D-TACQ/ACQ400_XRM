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
	epicsUInt32 offset_us;
};

const int SOE_LUT_ROWS = 64;

typedef struct SOE_LUT_ROW  SOE_LUT[SOE_LUT_ROWS];




#endif /* XRMIOCAPP_SRC_XRM_STRUCTS_H_ */
