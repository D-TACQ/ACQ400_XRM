/*
 * acq400_FMT_rx.h
 *
 *  Created on: 18 Feb 2026
 *      Author: pgm
 */

#ifndef XRMIOCAPP_SRC_ACQ400_FMT_RX_H_
#define XRMIOCAPP_SRC_ACQ400_FMT_RX_H_

#include "acq400_FMT.h"

/* singleton */
class acq400_FMT_rx: public acq400_FMT_abc {

	virtual void update_fmt(bool first_time = false);
	/**< basic housekeeping and instrumentation */
	int packet_count;
	epicsInt64 ts;

protected:
	virtual void task();
	virtual void process_fmt(bool first_time);
	/**< main processing here (subclass?) */

	acq400_FMT_rx(const char* portName);

	epicsEventId rx_event;
public:

	virtual ~acq400_FMT_rx();


	int waitFMT(unsigned timeout_ms);

	asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

	static acq400_FMT_rx* instance(const char* portName = 0);
	/* first caller MUST set portName */
};



#endif /* XRMIOCAPP_SRC_ACQ400_FMT_RX_H_ */
