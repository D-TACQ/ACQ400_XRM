/*
 * acq400_INST.cpp
 *
 *  Created on: 9 Apr 2026
 *      Author: pgm
 */

#include "acq400_asyn_common.h"
#include "acq400_INST.h"
#include "acq400_SOE.h"		//@@todo split .. static const SamplePrams *getSamplePrams()
#include "acq-util.h"
#include "split2.h"
#include <fcntl.h>                // open()
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <libgen.h>
#include <signal.h>
#include <sys/wait.h>
#include <asm/termbits.h>  /* Definition of constants */
#include <sys/ioctl.h>

using namespace std;
#include "Buffer.h"
#include "ES.h"

static const char *driverName="acq400_INST";

#define DN	driverName
#define FN	__FUNCTION__

#define MARK	fprintf(stderr, "%s %d\n", FN, __LINE__)
#define MARKI(p) fprintf(stderr, "%s %d P_ %s:%d\n", FN, __LINE__, #p, p)

int acq400_INST::nice = ::getenv_default("acq400_INST_NICE", 0);

const char* getenv_default(const char* key, const char* def = "echo undefined")
{
	const char *value = getenv(key);
	if (value == 0){
		value = def;
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
	/* Default stack size*/ 0),
	send_buffer_numbers(true)
{
	fprintf(stderr, "%s R1041\n", FN);

	createParam(PS_INST_STRATEGY, asynParamOctet,   &P_INST_STRATEGY);
	createParam(PS_REDIS_HOST,    asynParamOctet,   &P_REDIS_HOST);
	createParam(PS_REDIS_PORT,    asynParamOctet,   &P_REDIS_PORT);
	createParam(PS_REDIS_MKEY,    asynParamOctet,	&P_REDIS_MKEY);
	createParam(PS_REDIS_BCOUNT,  asynParamInt32,   &P_REDIS_BCOUNT);
	createParam(PS_REDIS_STATUS,  asynParamOctet,   &P_REDIS_STATUS);
	createParam(PS_REDIS_MMKEY,   asynParamOctet,   &P_REDIS_MMKEY);
	createParam(PS_ACQ_PORT,      asynParamOctet,   &P_ACQ_PORT);


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



extern char **environ;







/* builds a new environment. deletes all new entries on going out of scope .. */
class EnvBuilder {
	const int new_start;
	const int max_elements;


        int new_cursor;

public:
	char** env;

	EnvBuilder(int max_new, /* const */ char** copy_this = 0):
		new_start(env_count(copy_this)),
		max_elements(max_new+1 + new_start),
		new_cursor(new_start)
	{
		env = new char* [max_elements];
		if (copy_this != 0){
			copy_env(env, copy_this);
		}
	}
	void add(char* deleteable_kev){
		/* deleteable_key : key=value string allocated by new char [] */
		assert(new_cursor+1 < max_elements);
		env[new_cursor++] = deleteable_kev;
		env[new_cursor] = 0;
	}
	void print(const char* name) {
		fprintf(stderr, "EnvBuilder instance %s\n", name);
		for (int ii = 0; env[ii]; ++ii){
			fprintf(stderr, "[%2d] %c %p \"%s\"\n",
					ii, ii >= new_start? 'N':'c', env+ii, env[ii]);
		}
	}
	~EnvBuilder() {
		int cursor;
		// delete all the new stuff
		for (cursor = new_start; cursor < new_cursor; ++ cursor){
			delete [] env[cursor];
		}
		delete [] env;

		fprintf(stderr, "~EnvBuilder() deleted %d new, and env with %d entries\n",
				cursor-new_start, max_elements);
	}
	static int env_count(char** envp = environ);
	static int copy_env(char** new_env, char** old_env);
};


int EnvBuilder::env_count(char** envp)
/* returns number of environment entries, not including terminator */
{
	if (envp == 0){
		return 0;
	}else{
		int ii;
		for (ii = 0; envp[ii]; ++ii){
			;
		}
		return ii;
	}
}

int EnvBuilder::copy_env(char** new_env, char** old_env)
{
	int ii;
	for (ii = 0; old_env[ii]; ++ii){
		new_env[ii] = old_env[ii];
	}
	return ii;
}




// Source - https://stackoverflow.com/a/11461302/socketpair-in-c-unix
// Posted by Useless, modified by community. See post 'Timeline' for change history
// Retrieved 2026-04-11, License - CC BY-SA 3.0

child_process_info socket_fork_exec(const char *file, char *const argv[], char *const envp[])
/* create a bi-di socket. Fork child, exec cmd, output child pd and parent fd.
 * return 0 on success.
 */
{
	int fd[2];
	static const int parentsocket = 0;
	static const int childsocket = 1;
	pid_t pid;

	fprintf(stderr, "socketfork() hello\n");
	socketpair(PF_LOCAL, SOCK_STREAM, 0, fd);

	pid = fork();
	if (pid == 0){
		close(fd[parentsocket]);
		dup2(fd[childsocket], 0);
		dup2(fd[childsocket], 1);
		int rc = execvpe(file, argv, envp);
		assert(rc);
		return { 0, 0 };   // not going to happen
	}else{
		close(fd[childsocket]);
		return { pid, fd[parentsocket] };
	}
}

char* acq400_INST::make_kev_from_ip(const char* ps_name, int p_key){
	int value;
	gip(p_key, &value);
	const int kev_len = strlen(ps_name)+1+23+1; // math.log(2**32) = 22.2
	char* kev = new char[kev_len];
	assert(sprintf(kev, "%s=%d",ps_name, value) < kev_len);
	return kev;
}

char* acq400_INST::make_kev_from_sp(const char* ps_name, int p_key){
	char value_buf[128];
	gsp(p_key, 128, value_buf);
	char* kev = new char[strlen(ps_name)+1+strlen(value_buf)+1];
	sprintf(kev, "%s=%s",ps_name, value_buf);
	return kev;
}

char* make_kev_from_kvs(const char* key, const char* value)
{
	const int buffer_len = strlen(key)+1+strlen(value)+1;
	char* kev = new char[buffer_len];
	snprintf(kev, buffer_len, "%s=%s", key, value);
	return kev;

}

char* make_kev_from_kvi(const char* key, int value)
{
	const int buffer_len = strlen(key)+1+23+1;
	char* kev = new char[buffer_len];
	sprintf(kev, "%s=%d", key, value);
	return kev;

}
child_process_info acq400_INST::run_socket_fork_exec()
{
	char key[80];

	snprintf(key, 80, "%s_cmd", portName);
	cmd = getenv_default(key, FAKE_SPY);
	fprintf(stderr, "%s cmd: %s\n", FN, cmd);

	char* cmd_buf = new char[strlen(cmd)+1];
	strcpy(cmd_buf, cmd);

	EnvBuilder argv_builder(2);
	int argv0_len = strlen(basename(cmd_buf))+1;
	//fprintf(stderr, "%s argv0_len: %d\n", FN, argv0_len);
	char* pname = new char[argv0_len];
	strcpy(pname, basename(cmd_buf));
	//fprintf(stderr, "%s argv0: %s\n", FN, pname);
	argv_builder.add(pname);
	//argv_builder.print("argv_builder");

	EnvBuilder env_builder(10, environ);
	env_builder.add(make_kev_from_sp(PS_REDIS_HOST, P_REDIS_HOST));
	env_builder.add(make_kev_from_sp(PS_REDIS_PORT, P_REDIS_PORT));
	env_builder.add(make_kev_from_sp(PS_REDIS_MKEY, P_REDIS_MKEY));
	env_builder.add(make_kev_from_sp(PS_ACQ_PORT, P_ACQ_PORT));

	const SamplePrams* sp = acq400_SOE::getSamplePrams();

	env_builder.add(make_kev_from_kvi("REDIS_SSB", sp->SSB));
	env_builder.add(make_kev_from_kvi("REDIS_NSAM", sp->NSAM));
	env_builder.add(make_kev_from_kvi("REDIS_SPIX", sp->SP_INDEX));

	return socket_fork_exec(cmd, argv_builder.env, env_builder.env);
}

void acq400_INST::task()
{
	fprintf(stderr, "%s 01\n", FN);
	epicsEventWait(eventId);
	char ipc_buffer[128];
	int bcount = 0;
	int redis_bcount = 0;


	fprintf(stderr, "%s LET's go\n", FN);
	int fc = open("/dev/acq400.0.bq", O_RDONLY);
	assert(fc >= 0);

	MonitorRateLimit rateLimit;
	int ib;
	child_process_info cpi = {};

	if ((ib = getBufferId(fc)) < 0){
		fprintf(stderr, "ERROR: getBufferId() fail");
		return;
	}

	for (int runstop, runstop0 = 0; (ib = getBufferId(fc)) >= 0; runstop0 = runstop){
		lock();
		sip(0, P_UPDATES, bcount++);
		gip(P_RUNSTOP, &runstop);
		unlock();
		rateLimit.newData(mrl_param);
		if (runstop == 1 || runstop0 == 1){
			if (runstop0 == 0){
				memset(read_backlog, 0, sizeof(read_backlog));
				bcount = redis_bcount = 0;
				cpi = run_socket_fork_exec();
				lock();
				ssp(P_REDIS_STATUS, "RUN");
				unlock();
			}
			if (send_buffer_numbers){
				char tx_message[80];
				snprintf(tx_message, 80, "%d\n", ib);
				write(cpi.fd, tx_message, strlen(tx_message));
			}
		}
		if (cpi.fd != 0){
			int number_of_reads = 0;
			for (int nbytes = 0, retry = 0; retry < MAXREAD_BACKLOG; ++retry, nbytes=0){
				if (ioctl(cpi.fd, FIONREAD, &nbytes) == 0 && nbytes!= 0){
					int nread = read(cpi.fd, ipc_buffer, nbytes);
					if (nread > 0){
						if (ipc_buffer[nread-1] == '\n'){
							ipc_buffer[nread-1] = '\0';
						}else{
							ipc_buffer[nread] = '\0';
						}
						if (nread != nbytes){
							fprintf(stderr, "ipc:\"%s\" nread:%d/%d\n", ipc_buffer, nread, nbytes);
						}

						char *space = strchr(ipc_buffer, ' ');
						if (space){
							*space = '\0';
						}
						lock();
						ssp(P_REDIS_MMKEY, ipc_buffer);
						ssp(P_REDIS_STATUS, space+1);
						sip(0, P_REDIS_BCOUNT, ++redis_bcount);
						unlock();
						++number_of_reads;
					}else{
						fprintf(stderr, "ipc: read returned %d\n", nread);
					}
				}
			}
			read_backlog[number_of_reads] += 1;
		}

		if (rateLimit.goAhead()){
			lock();
			callParamCallbacks();
			unlock();
		}


		if (runstop == 0 && runstop0 == 1){
			// kill the spawned task
			fprintf(stderr, "shutdown: pid:%d fd:%d\n", cpi.pid, cpi.fd);
			shutdown(cpi.fd, SHUT_WR);
			sleep(1);
			assert(cpi.pid > 0);
			fprintf(stderr, "kill: pid:%d fd:%d\n", cpi.pid, cpi.fd);
			kill(cpi.pid, SIGKILL);
			int wstatus = 0;
			int rc = waitpid(cpi.pid, &wstatus, 0);
			if (rc == -1){
				perror("waitpid");
			}
			fprintf(stderr, "waitpid rc:%d status: %d\n", rc, wstatus);
			cpi = { 0, 0 };

			lock();
			ssp(P_REDIS_STATUS, "STOP");
			callParamCallbacks();
			unlock();
			fprintf(stderr, "readbacklog: %d,%d,%d\n",
					read_backlog[0], read_backlog[1], read_backlog[2]);
		}
	}

	close(fc);
}


asynStatus acq400_INST::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
	    int function = pasynUser->reason;
	    asynStatus status = asynSuccess;
	    const char *paramName;
	    int addr = 0;

	    /* Fetch the parameter string name for possible use in debugging */
	    getParamName(function, &paramName);

	    /* Set the parameter in the parameter library. */
	    status = (asynStatus) setIntegerParam(addr, function, value);

	    fprintf(stderr,
			"%s:%s: function=%d, addr=%d, name=%s, value=%d\n",
				DN, FN, function, addr, paramName, value);

	    if (function == P_RUNSTOP) {
	        if (value == 1){
	        	epicsEventSignal(eventId);
	        }
	    }else if (function == P_MON_RL){
		    mrl_param = value;
	    }

	    /* Do callbacks so higher layers see any changes */
	    status = (asynStatus) callParamCallbacks();

	    if (status)
	        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
	                  "%s:%s: status=%d, function=%d, name=%s, value=%d",
	                  DN, FN, status, function, paramName, value);
	    else
	        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
	              "%s:%s: function=%d, name=%s, value=%d\n",
	              DN, FN, function, paramName, value);
	    return status;
}

class acq400_INST_STR: public acq400_INST {
public:
	acq400_INST_STR(const char* portName, const char* _strategy):
		acq400_INST(portName, _strategy) {
		send_buffer_numbers = false;
	}
};

class acq400_INST_SPY: public acq400_INST {
public:
	acq400_INST_SPY(const char* portName): acq400_INST(portName, "SPY") {
		send_buffer_numbers = true;
	}
};
extern "C" {
	/** EPICS iocsh callable function to call constructor for the testAsynPortDriver class.
	  * \param[in] portName The name of the asyn port driver to be created.
	  */
	int acq400_INST_Configure(const char *portName, const char* stream_or_spy)
	{
		printf("%s:%s R1001 %s\n", DN, FN, portName);

		if (strncmp(stream_or_spy, "STR", 3) == 0){
			new acq400_INST_STR(portName, stream_or_spy);
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


