/*
 * acq400_INST.h
 *
 *  Created on: 9 Apr 2026
 *      Author: pgm
 */

#ifndef XRMIOCAPP_SRC_ACQ400_INST_H_
#define XRMIOCAPP_SRC_ACQ400_INST_H_

#include "acq400_asyn_common.h"
#include "xrm_structs.h"
#include "acq400_FMT.h"
#include "SamplePrams.h"

#define PS_INST_STRATEGY "INST_STRATEGY"  // master or spy

#define PS_REDIS_HOST	 "REDIS_HOST"       // HOST ip or DNS name
#define PS_REDIS_PORT	 "REDIS_PORT"       // HOST port (default:xxx)
#define PS_REDIS_MKEY	 "REDIS_MKEY"       // Major key (OUTPUT)

#define PS_REDIS_BCOUNT	 "REDIS_BCOUNT"     // buffers (cycles) processed
#define PS_REDIS_STATUS	 "REDIS_STATUS"     // status good/bad
#define PS_REDIS_MMKEY   "REDIS_MMKEY"      // Major+Minor key (INPUT)

class acq400_INST: public acq400_asynPortDriver {
protected:
	static int nice;

	int P_INST_STRATEGY;
	int P_REDIS_HOST;
	int P_REDIS_PORT;
	int P_REDIS_MKEY;
	int P_REDIS_BCOUNT;
	int P_REDIS_STATUS;
	int P_REDIS_MMKEY;
public:
	acq400_INST(const char* portName);
};


#endif /* XRMIOCAPP_SRC_ACQ400_INST_H_ */
