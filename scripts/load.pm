#!/bin/sh

SPORT=PM
# ACQ400_FMT_SIM_SOURCE=WRTS
ACQ400_PM_NBUF=${ACQ400_PM_NBUF:-20}
# default : 1024 * 128 / 4 = 32768
ACQ400_PM_NLW=${ACQ400_PM_NLW:-32768}

echo acq400_PM_Configure\($SPORT,$SRC\)
PRAMS="UUT=${IOC_HOST},PORT=$SPORT,ADDR=0,TIMEOUT=0"

echo dbLoadRecords\(\"db/pm_base.db\",\"${PRAMS},NBUF=${ACQ400_PM_NBUF}\"\)

NBM1=$((ACQ400_PM_NBUF-1))
# 00: most recent, 19: oldest of 20
for PMBUF in $(seq 0 $NBM1); do
        PMBUF02=$(printf %02d $PMBUF)
        echo dbLoadRecords\(\"db/pm_raw.db\",\"${PRAMS},NELM=${ACQ400_PM_NLW},PMBUF=${PMBUF},PMBUF02=${PMBUF02}\"\)
done

ACQ400_PM_NELM=${ACQ400_PM_NELM:-20}
echo dbLoadRecords\(\"db/pm_tab.db\",\"${PRAMS},ROWS=${ACQ400_PM_NELM}\"\)

