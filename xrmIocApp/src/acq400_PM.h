/*
 * acq400_PM.h  : acq400_PM : Post Mortem class.
 *
 * ACQ400 has supplied PM data from the start "TRANSIENT".
 * however, TRANSIENT is a one shot, start, stop, offload
 * XRM calls for a "live PM" that allows the capture to continue.
 * We do this by copying EVERY capture buffer, maintining a ring-buffer
 * of the previous second.
 * The previous second is presented as :PM:00 latest .. PM:19: oldest
 * acq400_PM has a RUNSTOP paramter. On run, the ring-buffer is maintained as normal
 * on STOP, the
 *
 *  Created on: 6 Mar 2026
 *      Author: pgm
 */

#ifndef XRMIOCAPP_SRC_ACQ400_PM_H_
#define XRMIOCAPP_SRC_ACQ400_PM_H_

#include "acq400_asyn_common.h"

#define MAX_PM_BUFFERS	32

#define PS_RUNSTOP	"RUNSTOP"	/* asynInt32, r/w */
#define PS_UPDATES	"UPDATES"	/* asynInt32, r/c */
#define PS_NBUF		"NBUF"		/* asynInt32, r, number of buffers */
#define PS_RING		"RING"		/* show ringbuffer (diag) */

#define PS_RAWBUF	"RAWBUF"        /* addr 0..32 */

class acq400_PM: public acq400_asynPortDriver {

protected:

	epicsEventId eventId;

	int P_RUNSTOP;
	int P_UPDATES;
	int P_NBUF;
	int P_RING;
	int P_RAWBUF;

public:
	acq400_PM(const char *portName);
	virtual ~acq400_PM() {}

	virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
};
#endif /* XRMIOCAPP_SRC_ACQ400_PM_H_ */
