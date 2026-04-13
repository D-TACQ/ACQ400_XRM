
# PVA from HOST cmdline

```
cat ./set_pva_env
export EPICS_PVA_AUTO_ADDR_LIST=false
export EPICS_PVA_ADDR_LIST="acq2206_088 acq2206_588 acq1102_057 acq1102_557"
# gotta be numbers regardless of hosts setting..
export EPICS_PVA_ADDR_LIST="10.12.197.110 10.12.197.88 10.12.197.13 10.12.197.57"
export EPICS_PVA_NAME_SERVERS="$EPICS_PVA_ADDR_LIST"

```
Monitor: 1 update only for json:
```
source ./set_pva_env
./bin/linux-x86_64/xrm_ht_monitor -U 1 acq2206_588  >xrm_ht_monitor.json
```

Interpret:

```
(base) pgm@hoy6:~/PROJECTS/ACQ400/ACQ400_XRM$ ./bin-scripts/load_json.py 
0 pv_id:2001 timestamp:1774440118029773 WRUS:1774440118029780 delta:7
1 pv_id:2002 timestamp:1774440118029773 WRUS:1774440118029790 delta:17
2 pv_id:2003 timestamp:1774440118029773 WRUS:1774440118029800 delta:27
3 pv_id:2004 timestamp:1774440118029773 WRUS:1774440118029810 delta:37
4 pv_id:2005 timestamp:1774440118029773 WRUS:1774440118029820 delta:47
5 pv_id:2006 timestamp:1774440118029773 WRUS:1774440118029830 delta:57
6 pv_id:2007 timestamp:1774440118029773 WRUS:1774440118029840 delta:67
7 pv_id:2008 timestamp:1774440118029773 WRUS:1774440118029850 delta:77
8 pv_id:2009 timestamp:1774440118029773 WRUS:1774440118029860 delta:87
9 pv_id:2010 timestamp:1774440118029773 WRUS:1774440118029870 delta:97

```

HT row TS is the time of the cycle. The embdded WRUS shows that we picked the nearest (ish) sample and we have a constant OFFSET of 10us.

Here's Json:

```json
{ "HT": [
         {
                "HDR": {
                        "pv_id":                2001,
                        "client_data":  0,
                        "timestamp":    1774440118029773,
                        "data_offset":  66,
                        "ss_u32":               32,
                        "ai_count":     32,
                        "di_count":     1,
                        "sp_count":     15
                },
                "RAW": {
                        "ai16": [ 22,22,23,22,23,22,22,24,22,22,23,22,22,24,22,22,23,23,22,22,23,22,23,23,22,22,23,22,22,22,22,22 ],
                        "di32": [ "0x00000000" ],
                        "sp32": [ "0x47c37402","0xb0195efa","0x60122d31","0x69c3ceb6","0x44444444","0x55555555","0x66666666","0x77777777","0x00000000","0x00000000","0x00000000","0x00000000","0x00000000","0x00000000","0x00000000" ],
                         "WRVS":                6,
                         "WRVT":         1191217,
                         "WRUS":         1774440118029780
                }
         },
         {
                "HDR": {
                        "pv_id":                2002,
                        "client_data":  2,
                        "timestamp":    1774440118029773,
...
                        "WRVS":                6,
                         "WRVT":         1194817,
                         "WRUS":         1774440118029870
                }
         } 
] }

```

Strip selected JSON elements in stream:
```
pgm@hoy6:~/PROJECTS/ACQ400/ACQ400_XRM$ ./bin/linux-x86_64/xrm_ht_monitor -U 10 acq2206_588 | egrep -e pv_id -e WRUS | sed 'N;/\n.*WRUS/s/\n/\t/;P;D'
			"pv_id":       2001,				 "WRUS":         1774520596606635
			"pv_id":       2002,				 "WRUS":         1774520596606645
			"pv_id":       2003,				 "WRUS":         1774520596606655
			"pv_id":       2004,				 "WRUS":         1774520596606665
			"pv_id":       2005,				 "WRUS":         1774520596606675
			"pv_id":       2006,				 "WRUS":         1774520596606685
			"pv_id":       2007,				 "WRUS":         1774520596606695
			"pv_id":       2008,				 "WRUS":         1774520596606705
			"pv_id":       2009,				 "WRUS":         1774520596606715
			"pv_id":       2010,				 "WRUS":         1774520596606725

```

Nice! Now json only runs ONCE for valid json, maybe we should override

Time splits:
```
./bin/linux-x86_64/xrm_ht_monitor -U 0 acq2206_588 | egrep -e pv_id -e WRUS | sed 'N;/\n.*WRUS/s/\n/\t/;P;D' | grep 2001

			"pv_id":       2001,				 "WRUS":         1774520937758720
			"pv_id":       2001,				 "WRUS":         1774520937808720
			"pv_id":       2001,				 "WRUS":         1774520937858720
			"pv_id":       2001,				 "WRUS":         1774520937908720
			"pv_id":       2001,				 "WRUS":         1774520937958720
			"pv_id":       2001,				 "WRUS":         1774520938008725
			"pv_id":       2001,				 "WRUS":         1774520938058725

```

Step is 50000

