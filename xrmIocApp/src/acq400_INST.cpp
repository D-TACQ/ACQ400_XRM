/*
 * acq400_INST.cpp
 *
 *  Created on: 9 Apr 2026
 *      Author: pgm
 */

#include "acq400_asyn_common.h"
#include "acq400_INST.h"

#include "acq-util.h"
#include "split2.h"
#include <fcntl.h>                // open()
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>

using namespace std;
#include "Buffer.h"
#include "ES.h"

static const char *driverName="acq400_INST";

#define DN	driverName
#define FN	__FUNCTION__

#define MARK	fprintf(stderr, "%s %d\n", FN, __LINE__)
#define MARKI(p) fprintf(stderr, "%s %d P_ %s:%d\n", FN, __LINE__, #p, p)

int acq400_INST::nice = ::getenv_default("acq400_INST_NICE", 0);

const char* getenv_default(const char* key, const char* def = "undefined")
{
	const char *value = getenv(key);
	if (value == 0){
		value = getenv(def);
	}
	return value;
}

acq400_INST::acq400_INST(const char* portName, const char* _strategy):
	acq400_asynPortDriver(portName,
	/* maxAddr */		1,
	/* Interface mask */    asynEnumMask|asynOctetMask|asynInt32Mask|asynInt64Mask|asynFloat64Mask|
				asynDrvUserMask,
	/* Interrupt mask */	asynEnumMask|asynOctetMask|asynInt32Mask|asynInt64Mask|asynFloat64Mask,
	/* asynFlags no block*/ 0,
	/* Autoconnect */       1,
	/* Default priority */  0,
	/* Default stack size*/ 0)
{
	fprintf(stderr, "%s R1041\n", FN);

	createParam(PS_INST_STRATEGY, asynParamOctet,   &P_INST_STRATEGY);
	createParam(PS_REDIS_HOST,    asynParamOctet,   &P_REDIS_HOST);
	createParam(PS_REDIS_PORT,    asynParamOctet,   &P_REDIS_PORT);
	createParam(PS_REDIS_MKEY,    asynParamOctet,	&P_REDIS_MKEY);
	createParam(PS_REDIS_BCOUNT,  asynParamInt32,   &P_REDIS_BCOUNT);
	createParam(PS_REDIS_STATUS,  asynParamOctet,   &P_REDIS_STATUS);
	createParam(PS_REDIS_MMKEY,   asynParamOctet,   &P_REDIS_MMKEY);


	ssp(P_INST_STRATEGY, _strategy);

	fprintf(stderr, "call epicsThreadCreate(%s)\n", _strategy);

	asynStatus status = (asynStatus)(epicsThreadCreate(portName,
			epicsThreadPriorityLow - nice,
			epicsThreadGetStackSize(epicsThreadStackMedium),
			(EPICSTHREADFUNC)task_runner,
			this) == NULL);
	if (status) {
		printf("%s:%s: epicsThreadCreate failure\n", DN, FN);
		return;
	}
}

void acq400_INST::task_runner(void *drvPvt)
{
	((acq400_INST *)drvPvt)->task();
}

/* two way comms .. gotta be a socket. thanks
 *
 */

// Source - https://stackoverflow.com/a/11461302/socketpair-in-c-unix
// Posted by Useless, modified by community. See post 'Timeline' for change history
// Retrieved 2026-04-11, License - CC BY-SA 3.0

void child(int socket) {
    const char hello[] = "hello parent, I am child";
    write(socket, hello, sizeof(hello)); /* NB. this includes nul */
    /* go forth and do childish things with this end of the pipe */
}

void parent(int socket) {
	/* do parental things with this end, like reading the child's message */
	char buf[1024];
	int n = read(socket, buf, sizeof(buf));
	fprintf(stderr, "parent received '%.*s'\n", n, buf);
}

void socketfork() {
	int fd[2];
	static const int parentsocket = 0;
	static const int childsocket = 1;
	pid_t pid;

	fprintf(stderr, "socketfork() hello\n");
	socketpair(PF_LOCAL, SOCK_STREAM, 0, fd);

	pid = fork();
	if (pid == 0){
		close(fd[parentsocket]);
		fprintf(stderr, "call child\n");
		child(fd[childsocket]);
		fprintf(stderr, "child done\n");
	}else{
		close(fd[childsocket]);
		fprintf(stderr, "call parent\n");
		parent(fd[parentsocket]);
		fprintf(stderr, "parent done\n");
	}
}


void acq400_INST::task()
{
#ifdef PGMCOMOUT
	fprintf(stderr, "%s 01\n", FN);
	epicsEventWait(eventId);

	fprintf(stderr, "%s LET's go\n", FN);
	int fc = open("/dev/acq400.0.bq", O_RDONLY);
	assert(fc >= 0);

	MonitorRateLimit rateLimit;
	int ib;

	if ((ib = getBufferId(fc)) < 0){
		fprintf(stderr, "ERROR: getBufferId() fail");
		return;
	}

	for (int runstop, runstop0 = 0; (ib = getBufferId(fc)) >= 0; runstop0 = runstop){
		lock();
		gip(P_RUNSTOP, &runstop);
		unlock();
		rateLimit.newData(mrl_param);
		if (runstop == 1 || runstop0 == 1){
			if (runstop0 == 0){
				char key[80];
				snprintf(key, 80, "%s_cmd", portName);
				cmd = getenv_default(key, FAKE_SPY);
			}
			fprintf(stderr, "let\'s go socketfork() \"%s\"\n", cmd);
			//socketfork();
			lock();
			//update_pm_callbacks(rateLimit.goAhead());
			unlock();
		}
		if (runstop == 0 && runstop0 == 1){
			// kill the spawned task
			lock();
			// do callbacks
			unlock();

		}
	}

	close(fc);
#else
	int bcount = 0;

	while (1) {
		sip(0, P_REDIS_BCOUNT, ++bcount);
		lock();
		callParamCallbacks();
		unlock();
		sleep(1);
	}
#endif
}

class acq400_INST_STR: public acq400_INST {
public:
	acq400_INST_STR(const char* portName): acq400_INST(portName, "STR") {

	}
};

class acq400_INST_SPY: public acq400_INST {
public:
	acq400_INST_SPY(const char* portName): acq400_INST(portName, "SPY") {

	}
};
extern "C" {
	/** EPICS iocsh callable function to call constructor for the testAsynPortDriver class.
	  * \param[in] portName The name of the asyn port driver to be created.
	  */
	int acq400_INST_Configure(const char *portName, const char* stream_or_spy)
	{
		printf("%s:%s R1001 %s\n", DN, FN, portName);

		if (strcmp(stream_or_spy, "STR") == 0){
			new acq400_INST_STR(portName);
		}else if (strcmp(stream_or_spy, "SPY") == 0){
			new acq400_INST_SPY(portName);
		}else{
			fprintf(stderr, "ERROR: arg \"%s\" not \"STR\" or \"SPY\"\n",
					stream_or_spy);
			assert(0);
		}
		return 0;
	}

	/* EPICS iocsh shell commands */

	static const iocshArg initArg0 = { "port", iocshArgString };
	static const iocshArg initArg1 = { "STR", iocshArgString };
	static const iocshArg * const initArgs[] = { &initArg0, &initArg1 };
	static const iocshFuncDef initFuncDef = { "acq400_INST_Configure", 2, initArgs };
	static void initCallFunc(const iocshArgBuf *args)
	{

		acq400_INST_Configure(args[0].sval, args[1].sval);
	}

	void acq400_INST_Register(void)
	{
	    iocshRegister(&initFuncDef, initCallFunc);
	}

	epicsExportRegistrar(acq400_INST_Register);
}


