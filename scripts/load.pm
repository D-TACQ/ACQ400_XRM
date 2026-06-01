#!/bin/sh

SPORT=PM
# ACQ400_FMT_SIM_SOURCE=WRTS
ACQ400_PM_NBUF=${ACQ400_PM_NBUF:-20}
# default : 4096 * 128 / 4 = 131072


#retry=0

#while [ $retry -lt 5  ]; do
#	ssb=$(get.site 0 ssb)
#	nsam=$(get.site 1 RTM_TRANSLEN | awk '{ print $2 }')
#	nlw=$((ssb*nsam/4))
#	if [ $nlw -gt 0 ]; then
#		echo "NLW $nlw, PROCEED after $retry attempts"
#		break
#	fi

#	echo "retry $retry ssb:$ssb nsam:$nsam, take 5"
#	sleep 5
#	retry=$((retry+1))
#done

#if [ $nlw -gt 0 ] && [ -z $ACQ400_PM_NLW ]; then
#	ACQ400_PM_NLW=$nlw
#fi

ACQ400_PM_NLW=${ACQ400_PM_NLW:-131072}

echo acq400_PM_Configure\($SPORT,$SRC\)

ARP="P=${IOC_HOST}:,R=${SPORT},PORT=${SPORT},ADDR=0,IMAX=100,OMAX=100,TB3=0,TIB0=0"
echo dbLoadRecords\(\"db/asynRecord.db\",\"${ARP}\"\)

PRAMS="UUT=${IOC_HOST},PORT=$SPORT,ADDR=0,TIMEOUT=0"
echo dbLoadRecords\(\"db/acq400_asyn_base.db\",\"${PRAMS},FUN=PM\"\)
echo dbLoadRecords\(\"db/pm_base.db\",\"${PRAMS},NBUF=${ACQ400_PM_NBUF}\"\)

NBM1=$((ACQ400_PM_NBUF-1))
# 00: most recent, 19: oldest of 20
for PMBUF in $(seq 0 $NBM1); do
        PMBUF02=$(printf %02d $PMBUF)
        echo dbLoadRecords\(\"db/pm_raw.db\",\"${PRAMS},NELM=${ACQ400_PM_NLW},PMBUF=${PMBUF},PMBUF02=${PMBUF02}\"\)
done

ACQ400_PM_NELM=${ACQ400_PM_NELM:-20}
echo dbLoadRecords\(\"db/pm_tab.db\",\"${PRAMS},ROWS=${ACQ400_PM_NELM}\"\)

