/*
 * acq400_Permit.h: one acq400_Permit per site, first site master
 *
 *  Created on: 14 Aug 2026
 *      Author: pgm
 */

#ifndef XRMIOCAPP_SRC_ACQ400_PERMIT_H_
#define XRMIOCAPP_SRC_ACQ400_PERMIT_H_

#include "acq400_asyn_common.h"
#include "acq400_FMT_rx.h"
#include "xrm_structs.h"
#include "SamplePrams.h"
#include <map>
#include <bitset>

#include <epicsGuard.h>
#include <epicsMutex.h>

#define FIRST_SITE 1
#define LAST_SITE  6

#define MAX_SITES 6
#define MAX_ADDR  (MAX_SITES+1)    // site 0 and selection of 1..6


#define PS_PERMIT_TRG_STAT	"PERMIT_TRG_STAT" /**<< latched status addr 1..6, perhaps 0: vector? 	*/
#define PS_PERMIT_TRG_LIVE	"PERMIT_TRG_LIVE" /**<< live status addr 1..6, perhaps 0: ALL?		*/
#define PS_PERMIT_ATD_MODE	"PERMIT_ATD_MODE"
#define PS_PERMIT_ATD_L1	"PERMIT_ATD_L1"
#define PS_PERMIT_ATD_L2	"PERMIT_ATD_L2"

#define PS_PERMIT_DI_CH_ACTIVE	"PERMIT_DI_CH_ACTIVE"
#define PS_PERMIT_DI_CH_VALUE	"PERMIT_DI_CH_VALUE"

/* commit knob is really for testing */
#define PS_PERMIT_COMMIT	"PERMIT_COMMIT"  /**<< set values for next cycle 1: L_CACHE, 2:R_CACHE, 3:BOTH */
#define PS_PERMIT_LOCK		"PERMIT_LOCK"    /**<< update L_CACHE without touching R_CACHE (non-atomic params), also, untimed */

struct ATD {
	enum ATD_MODE { ATD_OFF, ATD_RISING, ATD_FALLING, ATD_INSIDE, ATD_OUTSIDE } atd_mode;
	float atd_v1;   /**<< start voltage, all modes */
	float atd_v2;   /**<< second level, ATD_INSIDE, ATD_OUTSIDE */
	unsigned char atd_hysteresis_pc; /**<<hysteresis percentage FS */
};

/** Permit if (dtd_active&value) == (dtd_value&value) */
struct DTD {
	unsigned dtd_active;  /**<< bitmask of active channels CH01=1<<0, CH32=1<<31 */
	unsigned dtd_value;   /**<< bitmask of permit levels                         */

	DTD() {
		dtd_active = dtd_value = 0;
	}
};

#define MAP_LEN		0x10000
#define DSP1_GLBASE	0x80000000
#define DSP_TH_BASE	0xE000
#define DSP_TH_SIZE	0x0300

#define DSP_TH_ELEMENTS	(DSP_TH_SIZE/sizeof(unsigned))

#define L_CACHE_ELEMENTS (6*32)


class PermitDSP {
/**< Singleton */

private:

	 struct REGS {
		static const unsigned MODID  = 0x0000;
		static const unsigned PCR    = 0x0004;
		static const unsigned PICR   = 0x0018;
		static const unsigned PTSITE = 0x0070;

		static const unsigned PTLAT(const int site) { return 0x0100 + (site-1)*0x10; }
		static const unsigned PTLIV(const int site) { return 0x0104 + (site-1)*0x10; }
	} REGS;

	unsigned* _get_regp(unsigned reg);
	unsigned get(unsigned reg);
	void set(unsigned reg, unsigned value);
	void set_bits(unsigned reg, unsigned mask);
	void clr_bits(unsigned reg, unsigned mask);

	static int verbose;
	static int slug_mmap_writes;
	int fd;
	FILE* fp;			// mmap file test hack
	char* map_base;


	unsigned element_offset(int site){
		return (site-1)*0x080;
	}
	unsigned* th_base(int site) {
		return (unsigned*)(map_base + DSP_TH_BASE + element_offset(site));
	}
	unsigned R_cache[DSP_TH_ELEMENTS];
	unsigned R_cache_map[MAX_SITES];

	PermitDSP();
	~PermitDSP();

	void load_site(unsigned *dst, unsigned** ppsrc, int nu);
	/**< load data for site, updating src pointer */


	struct SITE_DEF {
		bool is_ai;
		unsigned char site;  /**<< 1..6 */
		unsigned char nelem; /**<< DI:2, AI16:16, AI32:32 */
	};
	SITE_DEF sites[MAX_SITES+1];  /**< address from 1 */
public:
	epicsMutex L_CACHE_lock;

	void enable(bool en);
	void reset();
	unsigned active_mask();       /**<< set bits show active site 0..5 -> 1..6 	*/
	unsigned status(int addr);    /**<< latched status site 1..6 0:ALL? */
	unsigned live(int addr);      /**<< live status site 1..6 0:ALL? */

	void setElement(int site, int ch_off, unsigned element)
	{
		R_cache[element_offset(site)+ch_off];
		R_cache_map[site] |= 1<<ch_off;
	}
	/**<< set element u32 return true on Success */

	unsigned get_permit_latch(int site) const;
	unsigned get_permit_live(int site) const;

	bool defineSite(int site, bool is_ai, int nelem);
	/**< as each site is enumerated, declare it */
	void commit();
	/**< commit all values to TH RAM */

	void full_load(
		epicsInt16* ai_th_pairs,/**< agg NCHAN LOW, HI pairs   */
		epicsInt32* di_pairs);  /**< agg NDI32 VALUE, ACTIVE pairs */


	static PermitDSP& instance();
};

class acq400_Permit: public acq400_asynPortDriver {


	static int nice;

	static std::map<int, acq400_Permit*> peers;

	virtual void task();
	static void task_runner(void *drvPvt);

	int P_PERMIT_TRG_STAT; /**<< latched status addr 1..6, perhaps 0: vector? 	*/
	int P_PERMIT_TRG_LIVE; /**<< live status addr 1..6, perhaps 0: ALL?		*/
	int P_PERMIT_COMMIT;  /**<< set values for next cycle */
	int P_PERMIT_LOCK;


protected:
	static int verbose;
	unsigned dirty_L_CACHE;
	PermitDSP& dsp;
	acq400_Permit(const char *portName, int site, bool is_ai, int nchan);

	acq400_FMT_rx* FMT_rx;
	void check_if_fmt_event_trigger(const FMT& fmt);

	void flush();

	virtual void update_callbacks(bool call_array_callbacks);
public:
	virtual void flush_L_CACHE(void) = 0;

	const int site;
	const int nchan;

	virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
	static void create_instance(const char *portName, int site, bool is_ai, int nchan);
};


#endif /* XRMIOCAPP_SRC_ACQ400_PERMIT_H_ */
