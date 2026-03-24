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

#include <stdio.h>

int main(int argc, char* argv[]){
	printf("Hello World\n");
	char myline[512];
	int lno = 0;

	FILE *pp = popen("unbuffer pvxmonitor -r value  acq2206_588:SOE_HLD", "r");
	while (fgets(myline, 512, pp) != 0){
		lno += 1;
		printf("[%2d] %s", lno, myline);
	}
	return 0;
}

