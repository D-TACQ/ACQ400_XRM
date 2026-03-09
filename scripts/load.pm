#!/bin/sh

SPORT=PM
# ACQ400_FMT_SIM_SOURCE=WRTS


echo acq400_PM_Configure\($SPORT,$SRC\)
PRAMS="UUT=${IOC_HOST},PORT=$SPORT,ADDR=0,TIMEOUT=0"

for PMB in $(seq 0 19); do
        PMB02=$(printf %02d $PMB)
        echo dbLoadRecords\(\"db/pm_base.db\",\"${PRAMS},PMB=${PMB},PMB02=${PMB02}\"\)
done
echo dbLoadRecords\(\"db/pm_tab.db\",\"${PRAMS},PMB=${PMB}\"\)

