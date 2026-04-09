/*
 * acq400_SMPL.h  :: asyn driver for SMPL Sample Dims.
 *
 *  Created on: 6 Apr 2026
 *      Author: pgm
 */

#ifndef XRMIOCAPP_SRC_ACQ400_SMPL_H_
#define XRMIOCAPP_SRC_ACQ400_SMPL_H_

#include "acq400_asyn_common.h"
#include "SamplePrams.h"


#define PS_SOE_AGG_SITES	"AGG_SITES"
#define PS_SOE_SITE_SSB		"SITE_SSB" /* addr per site */
#define PS_SOE_SMPL_NSAM	"SMPL_NSAM"
#define PS_SOE_SITE_IS_ADC	"SITE_IS_ADC"  /* addr per site */
#define PS_SOE_SMPL_SS_U32	"SMPL_SS_U32"
#define PS_SOE_SMPL_AI_COUNT 	"SMPL_AI_COUNT"
#define PS_SOE_SMPL_DI_COUNT 	"SMPL_DI_COUNT"
#define PS_SOE_SMPL_SP_COUNT 	"SMPL_SP_COUNT"

#define PS_SOE_SMPL_DI_INDEX 	"SMPL_DI_INDEX"
#define PS_SOE_SMPL_SP_INDEX 	"SMPL_SP_INDEX"

/* singleton */
class acq400_SMPL: public acq400_asynPortDriver {

protected:
	virtual void task();

	static void task_runner(void *drvPvt);

	void get_sample_dimensions();

	int P_AGG_SITES; // asynParamOctet
	int P_SITE_SSB;
	int P_SMPL_NSAM;
	int P_SITE_IS_ADC;
	int P_SMPL_SS_U32;
	int P_SMPL_AI_COUNT;
	int P_SMPL_DI_COUNT;
	int P_SMPL_SP_COUNT;
	int P_SMPL_DI_INDEX;
	int P_SMPL_SP_INDEX;



	static acq400_SMPL* _instance;
public:
	acq400_SMPL(const char *portName);
	SamplePrams samplePrams;
#ifdef PGMCOMOUT
	asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
	asynStatus writeOctet(asynUser *pasynUser, const char *value,
	                                    size_t nChars, size_t *nActual);
#endif
	static void create_instance(const char *portName);
	static acq400_SMPL* instance();
};



#endif /* XRMIOCAPP_SRC_ACQ400_SMPL_H_ */
