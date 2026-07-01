/** @file acq400_INST.h
 *  @brief class provides launch framework for instrumentation streaming.
 *
 *  Created on: 9 Apr 2026
 *      Author: pgm
 */

#ifndef XRMIOCAPP_SRC_ACQ400_INST_H_
#define XRMIOCAPP_SRC_ACQ400_INST_H_

#include "acq400_asyn_common.h"
#include "xrm_structs.h"
#include "acq400_FMT.h"
#include "SamplePrams.h"

#define PS_INST_STRATEGY "INST_STRATEGY"  // master or spy

#define PS_REDIS_HOST	 "REDIS_HOST"       // HOST ip or DNS name
#define PS_REDIS_PORT	 "REDIS_PORT"       // HOST port (default:xxx)
#define PS_REDIS_MKEY	 "REDIS_MKEY"       // Major key (OUTPUT)

#define PS_REDIS_BCOUNT	 "REDIS_BCOUNT"     // buffers (cycles) processed
#define PS_REDIS_STATUS	 "REDIS_STATUS"     // status good/bad
#define PS_REDIS_MMKEY   "REDIS_MMKEY"      // Major+Minor key (INPUT)
#define PS_ACQ_PORT	 "ACQ_PORT"         // source port for spawned app (if used)
#define PS_STREAM_SUBSET_MASK "STREAM_SUBSET_MASK"  // to allow a SPY to handle subset mask

#define FAKE_SPY	"/usr/local/xrm/epics/scripts/inst-spy-fake"

struct child_process_info {
	pid_t pid;
	int fd;
};

#define MAXREAD_BACKLOG 2

/** provides launch framework for instrumentation streaming.
 *
 */
class acq400_INST: public acq400_asynPortDriver {
protected:
	static int nice;
	const char* cmd;
	bool send_buffer_numbers;
	int read_backlog[MAXREAD_BACKLOG+1];

	virtual void task();
	static void task_runner(void *drvPvt);

	child_process_info run_socket_fork_exec();

	int P_INST_STRATEGY;	/**< Instrumentation strategy "STReam" or "SPY" (output).
				  - SPY is non-intrusive and recommended.
				*/
	int P_REDIS_HOST;	/**< REDIS server DNS-name or IP. (input)	*/
	int P_REDIS_PORT;	/**< REDIS port to send to. (input)		*/
	int P_REDIS_MKEY;	/**< REDIS major key.	(input)			*/
	int P_REDIS_BCOUNT;	/**< REDIS byte count (output)			*/
	int P_REDIS_STATUS;	/**< REDIS status (output)			*/
	int P_REDIS_MMKEY;	/**< REDIS current major.minor key (output) 	*/
	int P_ACQ_PORT;		/**< Port used on acq400 (4210=STReam, 53667=SPY) */
	int P_STREAM_SUBSET_MASK; /**< SPY client would need to know this */

	char* make_kev_from_ip(const char* ps_name, int p_key);
	/**< returns new char[] key-equals-value string from INTEGER param.
	 * conventionally ps_name is the PS_xxx def from above, but it doesn't _have_ to be.
	 * client to delete
	 */

	char* make_kev_from_sp(const char* ps_name, int p_key);
	/**< returns new char[] key-equals-value string from STRING param.
	 * conventionally ps_name is the PS_xxx def from above, but it doesn't _have_ to be.
	 * client to delete
	 */
public:
	acq400_INST(const char* portName, const char* _strategy);

	asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
};


#endif /* XRMIOCAPP_SRC_ACQ400_INST_H_ */
