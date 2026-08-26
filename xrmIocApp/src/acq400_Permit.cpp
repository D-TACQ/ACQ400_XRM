/** @file acq400_Permit.cpp
 *  @brief acq400_Permit implementation.
 *
 *  Strategy:
 + Permit Prams take up 0x300 BRAM space.
 + task will update Permit Prams at a suitable moment (right after DMA done)
 + updating the BRAM _must_ be as fast as possible. We propose a raw cache (R_cache) scheme so that
 + only dirty words are written.
 + We have up to 128*4 = 512 discrete AI logical prams. These are stored by in a backing store
 + L_cache, and must be converted to RAW data while the R_cache is not being flushed.
 The sequence is then:

 Records thread:
 + user Locks the LR update
 + user writes as many LPrams as required, filling the LCache, and marking dirty entries
 + user Unlocks LR
 + task: wakes on DMA done, flushes R-cache with highest priority.
  + then, if LR_UPDATE is unlocked, runs through the L_cache "cooking" L-values
  + (perhaps using Anatrg) to generate Raw ABCD values
 *  Created on: 14 Aug 2026
 *      Author: pgm
 */

#include "acq400_Permit.h"
#include "acq400_PermitLut.h"
#include "acq400_Proxy.h"
#include "anatrg.h"
#include "acq-util.h"
#include "split2.h"
#include <fcntl.h>                // open()
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

using namespace std;

#define DN	__FILE__
#define FN	__FUNCTION__

#define MARK	fprintf(stderr, "%s %d\n", FN, __LINE__)
#define MARKI(p) fprintf(stderr, "%s %d P_ %s:%d\n", FN, __LINE__, #p, p)

int PermitDSP::verbose = 	  ::getenv_default("acq400_PERMIT_DSP_VERBOSE", 0);

void PermitDSP::map_fpga()
{
	fpga.fd = open("/dev/mem", O_RDWR);
	if (fpga.fd <= 0){
		perror("failed to open /dev/mem");
		assert(fpga.fd <= 0);
	}
	fpga.map_base = (char*)mmap(0, MAP_LEN, PROT_READ|PROT_WRITE, MAP_SHARED, fpga.fd, DSP1_GLBASE);

	assert(fpga.map_base != MAP_FAILED);
}
void PermitDSP::map_shadow()
{
	shadow.fd = open("/tmp/permit_shadow", O_RDWR|O_CREAT);
	if (shadow.fd <= 0){
		perror("/tmp/permit_shadow");
		assert(shadow.fd <= 0);
	}
	char *k64 = new char[MAP_LEN];
	memset(k64, 0, MAP_LEN);
	write(shadow.fd, k64, MAP_LEN);
	delete [] k64;
	shadow.map_base = (char*)mmap(0, MAP_LEN, PROT_READ|PROT_WRITE, MAP_SHARED, shadow.fd, 0);
}

void PermitDSP::close_map(struct FILEMAP& fm)
{
	if (fm.map_base){
		munmap(fm.map_base, MAP_LEN);
	}
	if (fm.fd){
		close(fm.fd);
	}
}

PermitDSP::PermitDSP() {
	memset(sites, 0, sizeof(SITE_DEF)*(MAX_SITES+1));
	/* make the mapping */

	map_fpga();
	map_shadow();
}

PermitDSP::~PermitDSP() {
	close_map(fpga);
	close_map(shadow);
}
PermitDSP& PermitDSP::instance(){
	static PermitDSP* _instance;

	if (_instance == 0){
		_instance = new PermitDSP;
	}

	return *_instance;
}

bool PermitDSP::defineSite(int site, bool is_ai, int nelem)
{
	fprintf(stderr, "%s site:%d store:%d nelem:%d\n", FN, site, sites[site].site, nelem);
	assert(sites[site].site == 0);

	sites[site].site = site;         /** @@todo redundant .. struct not needed? */
	sites[site].is_ai = is_ai;
	sites[site].nelem = nelem;
	return true;
}
void PermitDSP::commit() {
	for (int site = 1; site < MAX_SITES; ++site){
		if (unsigned dirty = R_cache_map[site]){
			unsigned* pth = th_base(site);
			for (int ii = 0; dirty; ii++, dirty >>=1){
				if (dirty&1){
					pth[site] = R_cache[ii];
				}
			}
			R_cache_map[site] = 0;
		}
	}
}


void PermitDSP::load_site(unsigned *dst, unsigned** ppsrc, int nu)
/**< load data for site, updating src pointer */
{
	unsigned *psrc = *ppsrc;

	for (int cursor = nu; cursor; --cursor){
		*dst++ = *psrc++;
	}
	*ppsrc = psrc;
}

void PermitDSP::full_load(
		epicsInt16* ai_th_pairs,/**< agg NCHAN LOW, HI pairs   */
		epicsInt32* di_pairs)  /**< agg NDI32 VALUE, ACTIVE pairs */
{
	MARK;
	unsigned* ai_th_ch = (unsigned*)ai_th_pairs;
	unsigned* di_th_ch = (unsigned*)di_pairs;

	for (int site = 1; site <= MAX_SITES; ++site){
		if (sites[site].site){
			unsigned* pth = th_base(site);
			unsigned* src = sites[site].is_ai? ai_th_ch: di_th_ch; // for message only
			if (verbose){
				fprintf(stderr,
					"%s site:%d map:%p write %d %p -> %p @%04" FML "x\n",
					FN, site, fpga.map_base, sites[site].nelem,
					src, pth, (char*)pth-fpga.map_base);
			}

			if (sites[site].is_ai){
				load_site(pth, &ai_th_ch, sites[site].nelem);
			}else{
				load_site(pth, &di_th_ch, sites[site].nelem);
			}

		}
	}
}

int acq400_Permit::nice    = ::getenv_default("acq400_PERMIT_NICE", 10);
int acq400_Permit::verbose = ::getenv_default("acq400_PERMIT_VERBOSE", 0);


unsigned* PermitDSP::_get_regp(unsigned reg)
{
	return (unsigned*)(fpga.map_base + reg);
}
unsigned PermitDSP::get(unsigned reg)
{
	volatile unsigned *pr = _get_regp(reg);
	return *pr;
}
void PermitDSP::set(unsigned reg, unsigned value)
{
	volatile unsigned *pr = _get_regp(reg);
	*pr = value;
}
void PermitDSP::set_bits(unsigned reg, unsigned mask)
{
	volatile unsigned *pr = _get_regp(reg);
	unsigned rv = *pr;

	*pr = rv |= mask;
}
void PermitDSP::clr_bits(unsigned reg, unsigned mask)
{
	volatile unsigned *pr = _get_regp(reg);
	unsigned rv = *pr;

	*pr = rv &= ~mask;
}


void PermitDSP::enable(bool en)
{
	// @@todo
}
void PermitDSP::reset()
{
	// @@todo
}


#define DI_NELEM	2	/**< active, state */

std::map<int, acq400_Permit*> acq400_Permit::peers;

acq400_Permit::acq400_Permit(const char *portName, int _site, bool is_ai, int _nchan):
	acq400_asynPortDriver(portName,
	/* maxAddr */		_nchan+1,
	/* Interface mask */    asynEnumMask|asynOctetMask|asynInt32Mask|asynInt64Mask|asynFloat64Mask|asynUInt32DigitalMask|
				asynDrvUserMask,
	/* Interrupt mask */	asynEnumMask|asynOctetMask|asynInt32Mask|asynInt64Mask|asynFloat64Mask|asynUInt32DigitalMask,
	/* asynFlags no block*/ 0,
	/* Autoconnect */       1,
	/* Default priority */  0,
	/* Default stack size*/ 0),
	dsp(PermitDSP::instance()),
	site(_site),
	nchan(_nchan)
{
	asynStatus status = asynSuccess;

	createParam(PS_PERMIT_TRG_STAT,    	asynParamInt32,      &P_PERMIT_TRG_STAT);
	createParam(PS_PERMIT_TRG_LIVE,    	asynParamInt32,      &P_PERMIT_TRG_LIVE);


	createParam(PS_PERMIT_COMMIT,    	asynParamInt32,      &P_PERMIT_COMMIT);
	createParam(PS_PERMIT_LOCK,    		asynParamInt32,      &P_PERMIT_LOCK);
	fprintf(stderr, "%s P_PERMIT_LOCK:%d\n", FN, P_PERMIT_LOCK);

	if (is_ai){
		dsp.defineSite(site, true, nchan);
	}else{
		dsp.defineSite(site, false, DI_NELEM);
	}
	if (peers.size() == 0){
		/* first unit updates L_CACHE and controls the R_CACHE for all */
		status = (asynStatus)(epicsThreadCreate("PRMT_task",
				epicsThreadPriorityHigh - nice,
				epicsThreadGetStackSize(epicsThreadStackMedium),
				(EPICSTHREADFUNC)task_runner,
				this) == NULL);
		if (status) {
			printf("%s:%s: epicsThreadCreate failure\n", DN, FN);
			return;
		}
	}
}

void acq400_Permit::task_runner(void *drvPvt)
{
	((acq400_Permit *)drvPvt)->task();
}

void acq400_Permit::flush()
{
	if (dsp.L_CACHE_lock.tryLock()){
		dsp.commit();
		for (int site = 1; site <= MAX_SITES; ++site){
			if (peers[site] != 0){
				peers[site]->flush_L_CACHE();
			}
		}
		dsp.L_CACHE_lock.unlock();
	}
}


void acq400_Permit::task()
{
	fprintf(stderr, "%s 01\n", FN);

	epicsEventWait(eventId);

	int fc = open("/dev/acq400.0.bq", O_RDONLY);
	assert(fc >= 0);
	int ib;

	if ((ib = getBufferId(fc)) < 0){
		fprintf(stderr, "ERROR: getBufferId() fail");
		return;
	}
	MonitorRateLimit rateLimit;

	for (int runstop, runstop0 = 0; (ib = getBufferId(fc)) >= 0;
							runstop0 = runstop){
		lock();
		gip(P_RUNSTOP, &runstop);
		unlock();
		rateLimit.newData(mrl_param);

		if (FMT_rx == 0){
			FMT_rx = acq400_FMT_rx::instance();
		}

		if (runstop == 1){
			if (runstop != runstop0){
				dsp.enable(true);
			}
			flush();

			lock();
			acq400_PermitLut::instance().check_if_fmt_event_trigger(FMT_rx->get_fmt(0));
			update_callbacks(rateLimit.goAhead());
			unlock();
		}else{
			if (runstop != runstop0){
				dsp.enable(false);
			}
		}
	}
}


asynStatus acq400_Permit::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
	    int function = pasynUser->reason;
	    asynStatus status = asynSuccess;
	    const char *paramName;
	    int addr = 0;

	    /* Fetch the parameter string name for possible use in debugging */
	    getParamName(function, &paramName);

	    if (maxAddr > 1){
		    status = pasynManager->getAddr(pasynUser, &addr);
		    if(status!=asynSuccess) return status;
	    }

	    if (function == P_RUNSTOP){
		    if (value == 1){
			    epicsEventSignal(eventId);
		    }
	    }else if (function == P_MON_RL){
		    mrl_param = value;
	    }else if (function == P_PERMIT_LOCK){
		    if (value){
			    dsp.L_CACHE_lock.lock();
		    }else{
			    dsp.L_CACHE_lock.unlock();
		    }
	    }else if (function == P_PERMIT_COMMIT){
		    /* force commit in own time */
		    dsp.L_CACHE_lock.lock();
		    flush();
		    dsp.L_CACHE_lock.unlock();
	    }
	    /* Set the parameter in the parameter library. */
	    status = (asynStatus) setIntegerParam(addr, function, value);

	    if (verbose){
		    fprintf(stderr,
	    	              "%s:%s: function=%d, addr=%d, name=%s, value=%d\n",
	    	              DN, FN, function, addr, paramName, value);
	    }

	    return status;
}



class acq400_Permit_AI: public acq400_Permit {
	int P_PERMIT_ATD_MODE;
	int P_PERMIT_ATD_L1;
	int P_PERMIT_ATD_L2;

	ATD* atd;
	AnaTrg anatrg;

public:
	acq400_Permit_AI(const char *portName, int site, int nchan):
		acq400_Permit(portName, site, true, nchan),
		anatrg(nchan, 0, 0)
	{
		createParam(PS_PERMIT_ATD_MODE,    	asynParamInt32,      &P_PERMIT_ATD_MODE);
		createParam(PS_PERMIT_ATD_L1,		asynParamFloat64,    &P_PERMIT_ATD_L1);
		createParam(PS_PERMIT_ATD_L2,		asynParamFloat64,    &P_PERMIT_ATD_L2);

		atd = new ATD[nchan+1];			/**< index 0 = ALL */
		memset(atd, 0, (nchan+1)*sizeof(ATD));

		for (int ic = 0; ic < nchan+1; ++ic){
			sip(ic, P_PERMIT_ATD_MODE, 0);
			sip(ic, P_PERMIT_ATD_L1, 0.0);
			sip(ic, P_PERMIT_ATD_L2, 0.0);
		}
	}

	virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
	virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);

	virtual void flush_L_CACHE() {

		const epicsFloat32* eslo = acq400_Proxy::instance().getEslo(site);
		const epicsFloat32* eoff = acq400_Proxy::instance().getEslo(site);

		if (dirty_L_CACHE){
			unsigned l_cache = dirty_L_CACHE;
			for (int ic = 0; l_cache; ++ic, l_cache>>=1){
				if (l_cache&1){
					const int ch = ic;
					const ATD& atdx = atd[ic];
					/* float volts to raw char ..
					 * cal esp eoff is a bit superfluous,
					 * but importantly eslo gives range setting result.
					 */
					const char l1 = static_cast<int>((atdx.atd_v1 - eoff[ch])/eslo[ch])>>8;
					const char l2 = static_cast<int>((atdx.atd_v2 - eoff[ch])/eslo[ch])>>8;
					const int hyst = atdx.atd_hysteresis_pc*256/100;
					const Command& command = *anatrg.commands[atd[ic].atd_mode];
					dsp.setElement(site, ic, command(l1, l2, hyst));
				}
			}
			dirty_L_CACHE = 0;
		}
	}
};


class acq400_Permit_DI: public acq400_Permit {
	int P_PERMIT_DI_CH_ACTIVE;
	int P_PERMIT_DI_CH_VALUE;
	DTD dtd;
public:
	acq400_Permit_DI(const char *portName, int site):
		acq400_Permit(portName, site, false, 2)
	{
		createParam(PS_PERMIT_DI_CH_ACTIVE, asynParamUInt32Digital, &P_PERMIT_DI_CH_ACTIVE);
		createParam(PS_PERMIT_DI_CH_VALUE,  asynParamUInt32Digital, &P_PERMIT_DI_CH_VALUE);
	}

	virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

	virtual void flush_L_CACHE() {
		if (dirty_L_CACHE){
			dsp.setElement(site, 0, dtd.dtd_active);
			dsp.setElement(site, 1, dtd.dtd_value);
			dirty_L_CACHE = 0;
		}
	}
};



asynStatus acq400_Permit_DI::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
	    int function = pasynUser->reason;
	    asynStatus status = asynSuccess;
	    const char *paramName;
	    int addr = 0;

	    /* Fetch the parameter string name for possible use in debugging */
	    getParamName(function, &paramName);

	    if (maxAddr > 1){
		    status = pasynManager->getAddr(pasynUser, &addr);
		    if(status!=asynSuccess) return status;
	    }

	    if (function == P_PERMIT_DI_CH_ACTIVE ||
	        function == P_PERMIT_DI_CH_VALUE     ){
		    dsp.L_CACHE_lock.lock();

		    if (function == P_PERMIT_DI_CH_ACTIVE){
			    dtd.dtd_active = value;
		    }else if (function == P_PERMIT_DI_CH_VALUE){
			    dtd.dtd_value = value;
		    }else{
			    assert(function == P_PERMIT_DI_CH_ACTIVE ||
			           function == P_PERMIT_DI_CH_VALUE);
		    }
		    dsp.L_CACHE_lock.unlock();
	    }


	    return acq400_Permit::writeInt32(pasynUser, value);
}

asynStatus acq400_Permit_AI::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
	    int function = pasynUser->reason;
	    asynStatus status = asynSuccess;
	    const char *paramName;
	    int addr = 0;

	    /* Fetch the parameter string name for possible use in debugging */
	    getParamName(function, &paramName);

	    if (maxAddr > 1){
		    status = pasynManager->getAddr(pasynUser, &addr);
		    if(status!=asynSuccess) return status;
	    }
	    if (function == P_PERMIT_ATD_MODE){
		    dsp.L_CACHE_lock.lock();

		    assert(value >= ATD::ATD_OFF && value <= ATD::ATD_OUTSIDE);
		    atd[addr].atd_mode = (ATD::ATD_MODE)value;
		    dsp.L_CACHE_lock.unlock();
	    }



	    return acq400_Permit::writeInt32(pasynUser, value);
}

asynStatus acq400_Permit_AI::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
	    int function = pasynUser->reason;
	    asynStatus status = asynSuccess;
	    const char *paramName;
	    int addr = 0;

	    /* Fetch the parameter string name for possible use in debugging */
	    getParamName(function, &paramName);

	    if (maxAddr > 1){
		    status = pasynManager->getAddr(pasynUser, &addr);
		    if(status!=asynSuccess) return status;
	    }

	    if (function == P_PERMIT_ATD_L1 || function == P_PERMIT_ATD_L2){
		    dsp.L_CACHE_lock.lock();
		    if (function == P_PERMIT_ATD_L1){
			    atd[addr].atd_v1 = value;
		    }else if (function == P_PERMIT_ATD_L2){
			    atd[addr].atd_v2 = value;
		    }else{
			    assert(function == P_PERMIT_ATD_MODE);
		    }
		    dsp.L_CACHE_lock.unlock();
	    }


	    /* Set the parameter in the parameter library. */
	    status = (asynStatus) setDoubleParam(addr, function, value);

	    if (verbose){
		    fprintf(stderr,
	    	              "%s:%s: function=%d, addr=%d, name=%s, value=%.2f\n",
	    	              DN, FN, function, addr, paramName, value);
	    }

	    return status;
}

void acq400_Permit::update_callbacks(bool call_array_callbacks)
{
	sip(0, P_UPDATES, ++update);
	callParamCallbacks();

	if (call_array_callbacks){
#if 0
		doCallbacksInt8Array(pm_cols.c_rownum, 		MAX_PM_BUFFERS, P_COL_ROWNUM, 0);
		doCallbacksInt16Array(pm_cols.c_ib_live, 	MAX_PM_BUFFERS, P_COL_IBLIVE, 0);
#endif
	}


}


void acq400_Permit::create_instance(const char *portName, int site, bool is_ai, int nchan)
{
//	if (auto search = peers.find(site); search == peers.end()){ C++17 only
	auto search = peers.find(site);
	if (search == peers.end()){
		if (is_ai){
			peers[site] = new acq400_Permit_AI(portName, site, nchan);
		}else{
			peers[site] = new acq400_Permit_DI(portName, site);
		}
	}
}

extern "C" {
	/** EPICS iocsh callable function to call constructor for the testAsynPortDriver class.
	  * \param[in] portName The name of the asyn port driver to be created.
	  */
	int acq400_Permit_Configure(const char *portName, int site, int is_ai, int nchan)
	{
		printf("%s:%s R1002 %s\n", DN, FN, portName);

		acq400_Permit::create_instance(portName, site, is_ai, nchan);
		return 0;
	}

	/* EPICS iocsh shell commands */

	static const iocshArg initArg0 = { "port", iocshArgString };
	static const iocshArg initArg1 = { "site", iocshArgInt };
	static const iocshArg initArg2 = { "is_ai", iocshArgInt };
	static const iocshArg initArg3 = { "nchan", iocshArgInt };
	static const iocshArg * const initArgs[] = {
			&initArg0, &initArg1, &initArg2, &initArg3 };
	static const iocshFuncDef initFuncDef = { "acq400_Permit_Configure", 4, initArgs };
	static void initCallFunc(const iocshArgBuf *args)
	{
		acq400_Permit_Configure(args[0].sval, args[1].ival, args[2].ival, args[3].ival);
	}

	void acq400_Permit_Register(void)
	{
	    iocshRegister(&initFuncDef, initCallFunc);
	}

	epicsExportRegistrar(acq400_Permit_Register);
}




