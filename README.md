```
 1071  makeBaseApp.pl -t example xrmIoc
 1072  makeBaseApp.pl -t example -i xrmIoc
enter: linux-arm
 1073  make

cat /mnt/local/sysconfig/xrm_epics.sh
#!/bin/sh

echo 1>&2 xrm_epics.sh
#export IOC_HOST=xrmmagps_123
export IOC_HOST=acq2206_588
export ACQ400IOC=acq2206_088
# connects to custom NIC on eigg
#ifconfig eth0:0 192.168.1.88 up
# pick an unused address that's visible on Peter's vpn
ETH00_IP=10.12.197.88
ifconfig eth0:0 $ETH00_IP  up
ETH0_IP=$(/usr/local/CARE/ip_addr_show eth0)
# bind server to ETH00
export EPICS_CAS_INTF_ADDR_LIST=$ETH00_IP
export EPICS_PVAS_INTF_ADDR_LIST=$ETH00_IP
#export EPICS_CAS_BEACON_ADDR_LIST=192.168.1.88

# bind client (including in-server client) to both

export EPICS_CA_ADDR_LIST="$ETH0_IP $ETH00_IP"
export EPICS_PVA_ADDR_LIST="$ETH0_IP $ETH00_IP"

#export XRM_FMT_SIM=1
export XRM_FMT_RX=1
export XRM_SOE=1

# only ONE can be STR. Both could be SPY ..
# STRSPY also supported : SPY, but through a socket rather than memory
# SPY: IOC sends buffer numbers, spy-cmd is assumed to have a Buffers mapping
# STR: IOC sends NOTHING, str-cmd is assumed to use a socket
# ALL: IOC reads ONE line of input from stdin, and shows this as status.
export XRM_INST1="STRATEGY={STR|SPY};REDIS_HOST={a.b.c.d};REDIS_PORT=nnn;REDIS_MKEY=string"
export XRM_INST2="STRATEGY={STR|SPY};REDIS_HOST={a.b.c.d};REDIS_PORT=nnn;REDIS_MKEY=string"
IN1STR_cmd=command for IN1STR
IN2STR_cmd=command for IN2STR

export MultiCastVerbose=1


'# 20260409: SamplePrams available as a BLOB and an PV Table

acq2206_088> hexdump  /dev/shm/SamplePrams 
0000000 504c 534d 0080 0000 0400 0000 0020 0000
0000010 0000 0000 0001 0000 0010 0000 000f 0000
0000020 0011 0000                              
0000024
acq2206_088> pvget_value acq2206_588:SMPL
    epics:nt/NTScalar:1.0 AI_COUNT        int value 32
    epics:nt/NTScalar:1.0 AI_INDEX        int value 0
    epics:nt/NTScalar:1.0 DI_COUNT        int value 1
    epics:nt/NTScalar:1.0 DI_INDEX        int value 16
    epics:nt/NTScalar:1.0 NSAM        int value 1024
    epics:nt/NTScalar:1.0 SP_COUNT        int value 15
    epics:nt/NTScalar:1.0 SP_INDEX        int value 17
    epics:nt/NTScalar:1.0 SSB        int value 128

```
