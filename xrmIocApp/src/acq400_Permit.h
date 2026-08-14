/*
 * acq400_Permit.h
 *
 *  Created on: 14 Aug 2026
 *      Author: pgm
 */

#ifndef XRMIOCAPP_SRC_ACQ400_PERMIT_H_
#define XRMIOCAPP_SRC_ACQ400_PERMIT_H_

#include "acq400_asyn_common.h"
#include "xrm_structs.h"
#include "SamplePrams.h"
#include <map>

#define FIRST_SITE 1
#define LAST_SITE  6

#define MAX_SITES 6
#define MAX_ADDR  (MAX_SITES+1)    // site 0 and selection of 1..6


#define PS_PERMIT_TRG_STAT	"PERMIT_TRG_STAT" /**<< latched status addr 1..6, perhaps 0: vector? 	*/
#define PS_PERMIT_TRG_LIVE	"PERMIT_TRG_LIVE" /**<< live status addr 1..6, perhaps 0: ALL?		*/
#define PS_PERMIT_ATD_MODE	"PERMIT_ATD_MODE"
#define PS_PERMIT_ATD_L1	"PERMIT_ATD_L1"
#define PS_PERMIT_ATD_L2	"PERMIT_ATD_L2"
#define PS_PERMIT_ATD_HP	"PERMIT_ATD_HP"
#define PS_PERMIT_COMMIT	"PERMIT_COMMIT"  /**<< set values for next cycle */

struct ATD {
	enum { ATD_OFF, ATD_RISING, ATD_FALLING, ATD_INSIDE, ATD_OUTSIDE } atd_mode;
	float atd_v1;   /**<< start voltage, all modes */
	float atd_v2;   /**<< second level, ATD_INSIDE, ATD_OUTSIDE */
	unsigned char atd_hysteresis_pc; /**<<hysteresis percentage FS */
};

/** Permit if (dtd_active&value) == (dtd_value&value) */
struct DTD {
	unsigned dtd_active;  /**<< bitmask of active channels CH01=1<<0, CH32=1<<31 */
	unsigned dtd_value;   /**<< bitmask of permit levels                         */
};

class PermitDSP {
/**< Singleton */

private:
	PermitDSP();

	struct SITE_DEF {
		unsigned char site;  /**<< 1..6 */
		bool          is_ai;
		unsigned char nchan; /**<< 16 or 32 typ */
		union {
			ATD* atd;    /**<< ->16 or ->32 items/ch */
			DTD* dtd;    /**<< ->1 item (32 ch)   */
		} settings;
	};
	SITE_DEF sites[MAX_SITES+1];  /**< address from 1 */
public:
	void enable(bool en);
	void reset();
	unsigned active_mask();       /**<< set bits show active site 0..5 -> 1..6 	*/
	unsigned status(int addr);    /**<< latched status site 1..6 0:ALL? */
	unsigned live(int addr);      /**<< live status site 1..6 0:ALL? */

	bool getATD(int site, int addr, ATD& atd) const;
	/**<< return true if valid (AI site, channel present) */
	bool setATD(int site, int addr, const ATD& atd);
	/**<< return true if valid (AI site, channel present) */

	bool getDTD(int site, int addr, DTD& dtd) const;
	/**<< return true if valid (DI site, channel present) */
	bool setDTD(int site, int addr, const DTD& dtd);
	/**<< return true if valid (DI site, channel present) */

	unsigned get_permit_latch(int site) const;
	unsigned get_permit_live(int site) const;

	bool defineSite(int site, bool is_ai, int nchan);
	/**< as each site is enumerated, declare it */
	void commit();
	/**< commit all values to TH RAM */
	static PermitDSP& instance();
	virtual void task();

	static void task_runner(void *drvPvt);
};

class acq400_Permit: public acq400_asynPortDriver {

	static int verbose;
	static int nice;

	static std::map<int, acq400_Permit*> peers;
	acq400_Permit(const char *portName, int site, bool is_ai, int nchan);

public:

	virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
	virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);

	static void create_instance(const char *portName, int site, bool is_ai, int nchan);
};


#endif /* XRMIOCAPP_SRC_ACQ400_PERMIT_H_ */
