#!/bin/sh

SPORT=PM
# ACQ400_FMT_SIM_SOURCE=WRTS


echo acq400_PM_Configure\($SPORT,$SRC\)
PRAMS="UUT=${IOC_HOST},PORT=$SPORT,ADDR=0,TIMEOUT=0"
echo dbLoadRecords\(\"db/pm_base.db\",\"${PRAMS}\"\)

