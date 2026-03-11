/*
 * acq400_asyn_common.h
 *
 *  Created on: 21 Feb 2025
 *      Author: pgm
 */

#ifndef ACQ400IOCAPP_SRC_ACQ400_ASYN_COMMON_H_
#define ACQ400IOCAPP_SRC_ACQ400_ASYN_COMMON_H_

#include <epicsExport.h>
#include <epicsTypes.h>
#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsString.h>
#include <epicsTimer.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <iocsh.h>

#include <vector>
#include <split2.h>

#include "asynPortDriver.h"
#include "acq400_asynPortDriver.h"
#include "acq-util.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define SP0	0
#define SP1	1
#define SP2	2
#define SP3	3

typedef short AI16_t;
typedef unsigned long DI32_t;
typedef unsigned long DO32_t;
typedef unsigned long SP32_t;

static inline
std::vector<int> csv2int(const char* csv) {
	std::vector<std::string> strings;
	split2(csv, strings, ',');
	std::vector<int> vi;
	for (std::string s: strings){
		vi.push_back(stoi(s));
	}
	return vi;
}

static inline
bool epicsTimeDiffLessThan(epicsTimeStamp& t1, epicsTimeStamp& t0, double tgts)
{
	epicsTime et1 = t1;
	epicsTime et0 = t0;


	return (et1 - et0) < tgts;
}

static inline
bool epicsTimeDiffGreaterThan(epicsTimeStamp& t1, epicsTimeStamp& t0, double tgts)
{
	epicsTime et1 = t1;
	epicsTime et0 = t0;


	return (et1 - et0) > tgts;
}

#define TICKSPERUS	40

/* @@todo .. specialize time provider */
static epicsInt64 getWrTs(unsigned wrse, unsigned wrv)
{
	unsigned sec = (wrv >> 28)&0x07;
	if ((wrse&7) == sec){
		sec = wrse;
	}else if (((++wrse)&7) == sec){
		sec = wrse;
	}else{
		// this is going to be REALLY obvious!
	}
	int usec = (wrv&0x0fffffff)/TICKSPERUS;
	epicsInt64 ts = ((epicsInt64)sec)*1000000 + usec;
	return ts;
}

#endif /* ACQ400IOCAPP_SRC_ACQ400_ASYN_COMMON_H_ */
