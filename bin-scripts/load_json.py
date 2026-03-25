#!/usr/bin/env python
# UPDATES=1 ./bin/linux-x86_64/xrm_ht_monitor  > xrm_ht_monitor.json
import json

with open("xrm_ht_monitor.json") as jdata:
    data = json.load(jdata)

#print(data)

ht = data['HT']

#print(ht)

for ix, hte in enumerate(ht):
#    print(f'ROW{ix}\n{hte}')
#    print(f"{ix} timestamp:{hte['HDR']['timestamp']} WRUS:")
    pvid = hte['HDR']['pv_id']
    ts = hte['HDR']['timestamp']
    wrus = hte['RAW']['WRUS']
    print(f"{ix} pv_id:{pvid} timestamp:{ts} WRUS:{wrus} delta:{wrus-ts}")


