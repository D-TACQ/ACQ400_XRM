/** @file acq400_asynPortDriver.h
 * @brief common base class connects all acq400 PortDriver implementations
 *
 *  Created on: 6 Mar 2026
 *      Author: pgm
 */

#ifndef XRMIOCAPP_SRC_ACQ400_ASYNPORTDRIVER_H_
#define XRMIOCAPP_SRC_ACQ400_ASYNPORTDRIVER_H_

#include "acq400_asyn_common.h"

#define PS_RUNSTOP	"RUNSTOP"	/* asynInt32, r/w */
#define PS_UPDATES	"UPDATES"	/* asynInt32, r/c */
#define PS_TS_USEC	"TS_USEC"       /* asynInt32, ro  */
#define PS_MON_RL	"MON_RL"	/* asynInt32, r/w, "Monitor Rate Limit" */

/**
 * common base class connects all acq400 PortDriver implementations
 */
class acq400_asynPortDriver: public asynPortDriver {
protected:
	/**
	 * get integer parameter
	 * @param pnum: parameter number
	 * @param pram: outputs value
	 * @return: asynStatus
	 */
	asynStatus gip(int pnum, int* pram);
	/**
	 * get integer parameter
	 * @param addr: address number .. for objects with address dimension (typical: CH)
	 * @param pnum: parameter number
	 * @param pram: outputs value
	 * @return: asynStatus
	 */
	asynStatus gip(int addr, int pnum, int* pram);
	/**
	 * set integer parameter
	 * @param addr: address number .. for objects with address dimension (typical: CH)
	 * @param pnum: parameter number
	 * @param pram: outputs value
	 * @return: asynStatus
	 */
	asynStatus sip(int addr, int pnum, int pram);
	asynStatus sip(int addr, int pnum, unsigned pram);
	asynStatus sip(int addr, int pnum, epicsInt64 pram);

	/**
	 * get string parameter
	 * @param pnum: parameter number
	 * @param maxchar: maximum length of client buffer
	 * @param str: buffer holds client copy of string on success
	 * @return: asynStatus
	 */
	asynStatus gsp(int pnum, int maxchar, char* str);
	asynStatus ssp(int pnum, const char* str);

	epicsEventId eventId;  /**< synchronize main thread with task */

	int P_RUNSTOP;	/**< RUNSTOP button. Either UI or set on boot */
	int P_UPDATES;  /**< reports number of updates eg task loops */
	int P_TS_USEC;  /**< reports TimeStamp in USEC */
	int P_MON_RL;   /**< controls Monitor Rate Limit, mbbi with MRL values */

	/**
	 * controls update rate and hence cpu consumption per object.
	 */
	class MonitorRateLimit {
		epicsTimeStamp et0;
		bool go_ahead;
	public:
		enum MRL {
			DIS_MON,   	/**< DISable MONitor, ie no output */
			LIM_1Hz,	/**< LIMit output to 1Hz. */
			LIM_2Hz,	/**< LIMit output to 2Hz. */
			LIM_5Hz,	/**< LIMit output to 5Hz. */
			LIM_10Hz,	/**< LIMit output to 10Hz. */
			LIM_NOLIM	/**< NO LIMIT, update at incoming rate */
		};

		MonitorRateLimit();
		/** ratelimit allows client to proceed with update */
		bool goAhead() {
			return go_ahead;
		}
		/** client calls this to signal new data has arrived;
		 * whether the client should process it or not depends on goAhead()
		 */
		void newData(int mrl);
	};
	int mrl_param;                    // MUST be explicitly set by client ::writeInt32()
public:
	/** single public constructor */
	acq400_asynPortDriver(const char *portName, int maxAddr, int interfaceMask, int interruptMask,
			int asynFlags, int autoConnect, int priority, int stackSize);
	/** virtual destructor allows subclass override; default is a null function */
	virtual ~acq400_asynPortDriver() {}
};


#endif /* XRMIOCAPP_SRC_ACQ400_ASYNPORTDRIVER_H_ */
