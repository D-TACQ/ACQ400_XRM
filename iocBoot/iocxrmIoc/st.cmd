#!../../bin/linux-arm/xrmIoc

#- You may have to change xrmIoc to something else
#- everywhere it appears in this file

< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/xrmIoc.dbd"
xrmIoc_registerRecordDeviceDriver pdbbase

## Load record instances
dbLoadTemplate "db/user.substitutions"
dbLoadRecords "db/xrmIocVersion.db", "user=pgm"
dbLoadRecords "db/dbSubExample.db", "user=pgm"

#- Set this to see messages from mySub
#-var mySubDebug 1

#- Run this to trace the stages of iocInit
#-traceIocInit

cd "${TOP}/iocBoot/${IOC}"
iocInit

## Start any sequence programs
#seq sncExample, "user=pgm"
