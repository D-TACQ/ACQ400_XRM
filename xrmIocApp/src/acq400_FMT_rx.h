/** @file acq400_FMT_rx.h
 * @brief define FMT receiver class
 *
 *  Created on: 18 Feb 2026
 *      Author: pgm
 */

#ifndef XRMIOCAPP_SRC_ACQ400_FMT_RX_H_
#define XRMIOCAPP_SRC_ACQ400_FMT_RX_H_

#include <deque>
#include "acq400_FMT.h"

#define PS_FMT_PM_TRG_EVT 		"FMT_PM_TRG_EVT"	/* asynInt32, rw */
#define PS_FMT_PM_TRG_EVT_ACTION	"FMT_PM_TRG_EVT_ACTION" /* asynInt32, ro */

#define PS_NULL_FMT_COUNT		"NULL_FMT_COUNT"	/* asynInt32, rw */

#define NO_TRG_EVT	0

/** FMT receiver
 * - singleton - must bind to port.
 */
class acq400_FMT_rx: public acq400_FMT_abc {

	virtual void update_fmt(FMT& fmt, bool first_time = false);
	/**< basic housekeeping and instrumentation */
	int packet_count;
	epicsInt64 ts;
	int fmt_pm_trg_evt;

	int get_empty();
	FMT& receive(MultiCast& multicast);
protected:
	static int maxq;
	FMT* fmt_cache;			/* fmt_cache[maxq] possible store for short list of FMT instances */
	std::deque<int> empties;        /* indexes fmt_cache .. avoid copy */
	std::deque<int> filled;		/* indexes fmt_cache .. avoid copy; push from front, [0] is latest */

	virtual void task();
	virtual void process_fmt(FMT& fmt, bool first_time);
	/**< main processing here */

	acq400_FMT_rx(const char* portName);

	epicsEventId rx_event;

	int P_FMT_PM_TRG_EVT;
	int P_FMT_PM_TRG_EVT_ACTION;
	int P_NULL_FMT_COUNT;

	int null_fmt_count;

	static FMT null_fmt;
public:

	virtual ~acq400_FMT_rx();

	const FMT& get_fmt(unsigned icache);
	const epicsUInt16 getFMT_pm_trg_evt();
	/**< returns true it pm_trg_evt is selected */

	int waitFMT(unsigned timeout_ms);
	/**< clients block for FMT arrival. */

	asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

	static acq400_FMT_rx* instance(const char* portName = 0);
	/**< singleton factory: first caller MUST set portName */

	void onPM_trg_evt();
	/**< called if PM_trg_evt detected */

	inline static bool isNull(const FMT& fmt){
		const FMT_ROW row = fmt[0];
		return row.event == 0 && row.timestamp == 0;
	}
	inline static bool hasEvent(const FMT& fmt){
		const FMT_ROW row = fmt[0];
		return row.event !=0 && row.timestamp != 0;
	}
};



#endif /* XRMIOCAPP_SRC_ACQ400_FMT_RX_H_ */
