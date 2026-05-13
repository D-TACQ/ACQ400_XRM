/*
 * acq400_Proxy.h : local proxy for UUT parameters, perhaps in a more accessible form
 * #1 SMPL: to be clawed back from SMPL so we don't HAVE to have SOE instantiated and running.
 * #2 CAL: $IOC_HOST:0:AI:CAL:ESLO/EOFF : view UUT calibration as a single vector of channels.
 *
 *  Created on: 12 May 2026
 *      Author: pgm
 */

#ifndef XRMIOCAPP_SRC_ACQ400_PROXY_H_
#define XRMIOCAPP_SRC_ACQ400_PROXY_H_

#include "acq400_asyn_common.h"
#include "xrm_structs.h"
#include "SamplePrams.h"

#define FIRST_SITE 1
#define LAST_SITE  6

#define MAX_SITES 6
#define MAX_ADDR  (MAX_SITES+1)    // site 0 and selection of 1..6


/* @@todo later .. */
#define PS_SMPL_AGG_SITES	"SMPL_AGG_SITES"
#define PS_SMPL_SITE_SSB	"SMPL_SITE_SSB" /* addr per site */
#define PS_SMPL_NSAM		"SMPL_NSAM"
#define PS_SMPL_SITE_IS_ADC	"SMPL_SITE_IS_ADC"  /* addr per site */
#define PS_SMPL_SS_U32		"SMPL_SS_U32"
#define PS_SMPL_AI_COUNT 	"SMPL_AI_COUNT"
#define PS_SMPL_DI_COUNT 	"SMPL_DI_COUNT"
#define PS_SMPL_SP_COUNT 	"SMPL_SP_COUNT"

#define PS_SMPL_DI_INDEX 	"SMPL_DI_INDEX"
#define PS_SMPL_SP_INDEX 	"SMPL_SP_INDEX"

#define PS_AI_CAL_ESLO		"AI_CAL_ESLO" /* addr: 0:summary, 1..6 : mirror UUT */
#define PS_AI_CAL_EOFF		"AI_CAL_EOFF" /* addr: 0:summary, 1..6 : mirror UUT */

class acq400_Proxy: public acq400_asynPortDriver {

	SamplePrams samplePrams;

	size_t ai_site_lengths[MAX_SITES+1];
	epicsFloat32** eslo_src;        // eslo_src[1..6][NCHAN+1] copy data from ACQ400IOC
	epicsFloat32** eoff_src;	// eoff_src[1..6][NCHAN+1] copy data from ACQ400IOC

	epicsFloat32* eslo_dst;         // eslo_dst[AGG_NCHAN+1] copy data from ACQ400IOC
	epicsFloat32* eoff_dst;	        // eoff_dst[AGG_NCHAN+1] copy data from ACQ400IOC

	static int verbose;

	void get_sample_dimensions();
	void get_cal();

	virtual void task();

	static void task_runner(void *drvPvt);

	int P_SMPL_AGG_SITES; // asynParamOctet
	int P_SMPL_SITE_SSB;
	int P_SMPL_NSAM;
	int P_SMPL_SITE_IS_ADC;
	int P_SMPL_SS_U32;
	int P_SMPL_AI_COUNT;
	int P_SMPL_DI_COUNT;
	int P_SMPL_SP_COUNT;
	int P_SMPL_DI_INDEX;
	int P_SMPL_SP_INDEX;

	int P_AI_CAL_ESLO;
	int P_AI_CAL_EOFF;

	acq400_Proxy(const char *portName);

	static acq400_Proxy* _instance;
public:
	static void create_instance(const char *portName);
	static const SamplePrams *getSamplePrams() {
		assert(_instance != 0);
		return &_instance->samplePrams;
	}
	virtual ~acq400_Proxy() {}

	virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

	virtual asynStatus readFloat32Array(asynUser *pasynUser, epicsFloat32 *value,
	                                       size_t nElements, size_t *nIn);
	virtual asynStatus writeFloat32Array(asynUser *pasynUser, epicsFloat32 *value,
	                                       size_t nElements);


};
#endif /* XRMIOCAPP_SRC_ACQ400_PROXY_H_ */
