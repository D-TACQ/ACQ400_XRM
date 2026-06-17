/*
 * acq400_asyn_common.cpp
 *
 *  Created on: 17 Jun 2026
 *      Author: pgm
 */


/*
 * acq400_asyn_common.h
 *
 *  Created on: 21 Feb 2025
 *      Author: pgm
 */
#include "acq400_asyn_common.h"

std::vector<int> csv2int(const char* csv) {
	std::vector<std::string> strings;
	split2(csv, strings, ',');
	std::vector<int> vi;
	for (std::string s: strings){
		vi.push_back(stoi(s));
	}
	return vi;
}

bool epicsTimeDiffLessThan(epicsTimeStamp& t1, epicsTimeStamp& t0, double tgts)
{
	epicsTime et1 = t1;
	epicsTime et0 = t0;


	return (et1 - et0) < tgts;
}

bool epicsTimeDiffGreaterThan(epicsTimeStamp& t1, epicsTimeStamp& t0, double tgts)
{
	epicsTime et1 = t1;
	epicsTime et0 = t0;


	return (et1 - et0) > tgts;
}

epicsInt64 getWrTsUs(unsigned wrse, unsigned wrv)
/**< create full Timestamp from <wrse> White Rabbit Seconds from Epoch and
 * <wrv> : White Rabbit Vernier, the coded output from the WR firmware.
 */
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
	epicsInt64 ts = ((epicsInt64)sec)*USPS + usec;
	return ts;
}

epicsInt64 getWrTsNs(unsigned wrse, unsigned wrv)
/**< create full Timestamp from <wrse> White Rabbit Seconds from Epoch and
 * <wrv> : White Rabbit Vernier, the coded output from the WR firmware.
 * wrv max = 40e6. 40e6 * 25 = 0x369aca00 -> fits a u32
 * .. our 28 bit vernier is good to 250MHz.
 */
{
	unsigned sec = (wrv >> 28)&0x07;
	if ((wrse&7) == sec){
		sec = wrse;
	}else if (((++wrse)&7) == sec){
		sec = wrse;
	}else{
		// this is going to be REALLY obvious!
	}
	unsigned nsec = (wrv&0x0fffffff)*NSPERTICK;
	epicsInt64 ts = ((epicsInt64)sec)*NSPS + nsec;
	return ts;
}

epicsInt64 getWrTs(unsigned wrse, unsigned wrv)
{
	return getWrTsUs(wrse, wrv);
}

const char* getenv_default(const char* key, const char* def)
{
	const char *value = getenv(key);
	if (value == 0){
		value = def;
	}
	return value;
}

