
Customer agreed to settle on a single rep of time: NSEC since EPOCH (TAI).

It would be very helpful to hold this in a single int64  (epicsInt64).   We have the headroom:
```
>>> 2**63
9223372036854775808
>>> # max nsec
>>> 2**63/1e9
9223372036.854776
>>> # years till rollover
>>> 2**63/1e9/3600/24/365
292.471208677536
>>> # rollover date:
>>> 1970+292
2262
>>> 
```

It's been noted that common formats for TAI include:
```
TAI64 (8 bytes): Represents the number of seconds since 1970-01-01 00:00:00 TAI. The first 8 bits are '40' (hex), followed by 64 bits (8 bytes) of big-endian data.
Example: 40 00 00 00 00 00 00 00 (1970-01-01 00:00:00 TAI)
TAI64N (12 bytes): TAI64 label + a 4-byte big-endian nanosecond counter.
```

However, that's not really helpful because it's not possible to rep the information in a single number, where an int64 is clearly sufficient (and if EPICS were to come out with a epicsUInt64 in the next 200y, we could use that). 

Also, our machine speaks LittleEndian, why waste cycles?.

Next, we have to convert from our internal "White Rabbit Vernier time " to the external number  WRV in the hardware is a 32 bit reg, with 4 bits of seconds and 28 bits of tick. Where tick is defined as 25ns.  Software lines up the 4 bit seconds value with a 32 bit "White Rabbit Seconds since EPOCH" reg "WRSE". 
On the XRM system, SPAD[3] is being updated with WRSE at 100Hz, so that  both WRSE and WRV are present in the data set 
The function to convert WRSE+WRV to "microseconds since EPOCH" is as follow;  

```c++
#define TICKSPERUS 40
#define USPS 1000000

static inline
epicsInt64 getWrTsUs(unsigned wrse, unsigned wrv)

/**< create full Timestamp from <wrse> White Rabbit Seconds from Epoch and
* <wrv> : White Rabbit Vernier, the coded output from the WR firmware.
*/

{
	unsigned sec = (wrv >> 28)&0x07;
	if ((wrse&7) == sec){
		sec = wrse;
	}else if (((++wrse)&7) == sec){
		sec = wrse;
	}else{
		// this is going to be REALLY obvious!
	}

	unsigned usec = (wrv&0x0fffffff)/TICKSPERUS;
	epicsInt64 ts = ((epicsInt64)sec)*USPS + usec;
	return ts;
}

```

A function to convert WRV to "nanoseconds since EPOCH" will be very very similar.

```c++
#define NSPERTICK 25
#define NSPS 1000000000

static inline
epicsInt64 getWrTsNs(unsigned wrse, unsigned wrv)
/**< create full Timestamp from <wrse> White Rabbit Seconds from Epoch and
* <wrv> : White Rabbit Vernier, the coded output from the WR firmware.
* wrv max = 40e6. 40e6 * 25 = 0x369aca00 -> fits a u32
* our 28 bit vernier is good to 250MHz.
*/
{
	unsigned sec = (wrv >> 28)&0x07;

	if ((wrse&7) == sec){
		sec = wrse;
	}else if (((++wrse)&7) == sec){
		sec = wrse;
	}else{
		// this is going to be REALLY obvious!
	}

	unsigned nsec = (wrv&0x0fffffff)*NSPERTICK;
	epicsInt64 ts = ((epicsInt64)sec)*NSPS + nsec;
	return ts;
}
```

IF the customer REALLY REALLY insists on TAI64N, well they can have it. But let's not make life difficult for ourselves shall we?
If the REDIS key is in fact a string, and Derek really really wants WRSE + nsec, then we _could_ easily do that processing when generating the key.  Quite nice really, then endian-ness doesn't come into it, probably ends up as a very similar number of chars anyway.

If you can see an error in the above routines, let me know. 

Since wrse is being updated rapidly, and embedded in the data at point of sampling, when it comes to be processed down the line, it's highly unlikely that the seconds fields don't align as above; we don't have an error output from this function, but a step of -56 years in the data is going to be really obvious.


