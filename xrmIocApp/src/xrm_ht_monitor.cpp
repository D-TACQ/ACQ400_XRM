/*
 * xrm_ht_monitor.cpp
 *
 *  Created on: 24 Mar 2026
 *      Author: pgm
 */


/* pvxs API is beyond my ken. PlanB: wrap, translate and output
 *
 * acq2206_088> pvxmonitor -r value  acq2206_588:SOE_HLD
acq2206_588:SOE_HLD Connected to 10.12.197.88:5075
acq2206_588:SOE_HLD
    value int32_t[] = {82}[2000, 0, -1338570257, 413124, 538968082, 1, 2001, 2, -1338570257, 413124, 538968114, 1, 0, 0, 0, 0, 0, 0, 1572888, 1638426, 1572889, 1703961, 1638425, 1638425, 1703962, 1572888, 1703962, 1638425, 1638425, 1703962, 1638425, 1572889, 1703960, 1507352, 0, 175311874, -29707575, 292025473, 1774357025, 1145324612, 1431655765, 1717986918, 2004318071, 0, 0, 0, 0, 0, 0, 0, 63833041, 63964116, 63898582, 64422864, 64095186, 64095188, 64553938, 63833039, 64095188, 64095183, 63767507, 64160726, 64160722, 64029651, 64291791, 63833041, 0, 175311876, -29707565, 292025873, 1774357025, 1145324612, 1431655765, 1717

    read them all into a binary array, output as HEADER + RAW pairs.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "acq-util.h"
#include "split2.h"
#include "xrm_structs.h"



int G_verbose = ::getenv_default("VERBOSE", 0);

#define MAXSIZE 65536    // if pvxmonitor output ever greater than this per line, we're in trouble



void print(SOE_HOLD_HEADER& header)
{
	printf("pv_id:		%u,\n", header.pv_id);
	printf("client_data:	%u,\n", header.client_data);
	printf("timestamp:	%llu,\n", header.timestamp);
	printf("data_offset:	%u,\n", header.data_offset);
	printf("ss_u32:		%u,\n", header.ss_u32);
	printf("ai_count:	%u,\n", header.ai_count);
	printf("di_count:	%u,\n", header.di_count);
	printf("sp_count:	%u\n", header.sp_count);

}
void print(SOE_HOLD_HEADER& header, int* ht_data)
{
	short* ai16 = (short*)ht_data;
	printf("ai16: [ ");
	for (int ic = 0; ic < header.ai_count; ++ic){
		printf("%d,", ai16[ic]);
	}
	printf(" ],\n");
	printf("di32: [ ");
	unsigned* di32 = (unsigned*)ht_data + header.ai_count/2;
	for (int id = 0; id < header.di_count; ++id){
		printf("0x%08x,", di32[id]);
	}
	printf(" ],\n");
	printf("sp32: [ ");
	unsigned* sp32 = di32 + header.di_count;
	for (int isp = 0; isp < header.sp_count; ++isp){
		printf("0x%08x,", sp32[isp]);
	}
	printf(" ]\n");
}
void parse(int* ht_data, unsigned nelems)
{
	if (G_verbose > 1 ) fprintf(stderr, "parse: %p, %u\n",
			ht_data, nelems);
	for (SOE_HOLD_HEADER* header = (SOE_HOLD_HEADER*)ht_data;
			header->pv_id != 0; ++header){
		print(*header);
		print(*header, ht_data+header->data_offset);
	}

}

#define NEEDLE "value int32_t[] = "

void parse(char* txt)
{
	unsigned nelems;
	if (G_verbose > 1 ) fprintf(stderr, "parse: \"%s\"\n", txt);

	if (sscanf(txt, "{%u}", &nelems) != 1){
		fprintf(stderr, "scan fail\n");
		return;
	}
	char* start = strchr(txt, '[');
	assert(start != 0);
	char* end = strchr(txt, ']');
	assert(end != 0);
	*end = '\0';

	VIS value_ints;
	split2(++start, value_ints, ',');
	if (G_verbose > 1 )
		fprintf(stderr, "parse: value_ints.size %u nelems %u\n",
				value_ints.size(), nelems);

	assert(value_ints.size() == nelems);

	int* ht_data = new int[nelems];
	int* cursor = ht_data;
	for (int ii: value_ints){
		if (G_verbose > 2 ){
			fprintf(stderr, "idx:%d ii:%d\n",
					cursor - ht_data, ii);
		}
		*cursor++ = ii;
		assert(cursor-ht_data <= nelems);
	}
	parse(ht_data, nelems);
	delete [] ht_data;
}
int main(int argc, char* argv[]){
	static char myline[MAXSIZE];
	int lno = 0;

	FILE *pp = popen("unbuffer pvxmonitor -r value  acq2206_588:SOE_HLD", "r");
	while (fgets(myline, MAXSIZE, pp) != 0){
		lno += 1;
		if (G_verbose > 2 ) fprintf(stderr, "[%2d] %s\n", lno, myline);
		char* cursor = strstr(myline, NEEDLE);
		if (cursor){
			parse(cursor+strlen(NEEDLE));
		}
	}
	return 0;
}

