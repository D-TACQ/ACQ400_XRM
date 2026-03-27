/*
 * pvxsWrapper.cpp
 *
 *  Created on: 27 Mar 2026
 *      Author: pgm
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <iostream>

#include "acq-util.h"
#include "split2.h"
#include "xrm_structs.h"
#include "pvxsWrapper.h"

#include "acq400_asyn_common.h"

extern int G_verbose;

template<class CONTAINER>
const CONTAINER* PVX_getter<CONTAINER>::parse(CONTAINER* values, char* txt)
{
	unsigned nelems;
	if (G_verbose > 1 ) fprintf(stderr, "parse: \"%s\"\n", txt);

	if (sscanf(txt, "{%u}", &nelems) != 1){
		fprintf(stderr, "scan fail\n");
		assert(false);
	}
	char* start = strchr(txt, '[');
	assert(start != 0);
	char* end = strchr(txt, ']');
	assert(end != 0);
	*end = '\0';

	split2(++start, *values, ',');
	if (G_verbose > 1 )
		fprintf(stderr, "parse: values.size %u nelems %u\n",
				(unsigned)(values->size()), nelems);

	assert(values->size() == nelems);

	return values;
}

template<class CONTAINER>
const CONTAINER* PVX_getter<CONTAINER>::pvx_get(
		CONTAINER* values,
		bool wait_first_change)
{
	static char myline[MAXSIZE];
	int lno = 0;
	char cmd[128];
	snprintf(cmd, 128, "unbuffer pvxmonitor -r value %s", pv_name);

	FILE *pp = popen(cmd, "r");
	while (fgets(myline, MAXSIZE, pp) != 0){
		if (lno++ == 0 && wait_first_change){
			fprintf(stderr, "skip initial value and wait_first change\n");
			continue;
		}

		if (G_verbose > 2 ) fprintf(stderr, "[%2d] %s\n", lno, myline);
		char* cursor = strstr(myline, needle);
		if (cursor){
			return parse(values, cursor+strlen(needle));
		}
	}
	assert(false);
	return 0;
}

template<class CONTAINER>
const char* PVX_getter<CONTAINER>::make_pv_name(
		const char* hostname, const char* pv_suffix)
{
	char* pvname = new char[strlen(hostname)+strlen(pv_suffix)];
	strcpy(pvname, hostname);
	return strcat(pvname, pv_suffix);
}


template<>
PVX_getter<VIS>::PVX_getter(
		const char* hostname,
		const char* pv_suffix,
		const char* NEEDLE):
	pv_name(make_pv_name(hostname, pv_suffix)),
	needle("value int32_t[] = ")
{

}

template<>
PVX_getter<VDS>::PVX_getter(
		const char* hostname,
		const char* pv_suffix,
		const char* NEEDLE):
	pv_name(make_pv_name(hostname, pv_suffix)),
	needle("value double[] = ")
{

}

template<class CONTAINER>
PVX_getter<CONTAINER>::PVX_getter(
		const char* hostname,
		const char* pv_suffix,
		const char* NEEDLE):
	pv_name(make_pv_name(hostname, pv_suffix)),
	needle(NEEDLE)
{
	assert(NEEDLE != 0);
}


template<class CONTAINER>
PVX_getter<CONTAINER>::~PVX_getter()
{
	delete [] pv_name;
}

