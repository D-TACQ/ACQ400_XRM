
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

export MultiCastVerbose=1


