/*
 * acq400_FMT_rx.h
 *
 *  Created on: 18 Feb 2026
 *      Author: pgm
 */

#ifndef XRMIOCAPP_SRC_ACQ400_FMT_RX_H_
#define XRMIOCAPP_SRC_ACQ400_FMT_RX_H_

#include "acq400_FMT.h"

class acq400_FMT_rx: public acq400_FMT_abc {
	virtual void update_fmt(bool first_time = false);

protected:
	virtual void task();

public:
	acq400_FMT_rx(const char* portName);
	virtual ~acq400_FMT_rx() {}

	/*
	asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
*/
};



#endif /* XRMIOCAPP_SRC_ACQ400_FMT_RX_H_ */
