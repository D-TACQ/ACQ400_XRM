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
#include <unistd.h>    // getopt(3)

#include <iostream>

#include "acq-util.h"
#include "split2.h"
#include "xrm_structs.h"

#include "acq400_asyn_common.h"

int G_verbose = ::getenv_default("VERBOSE", 0);
int G_updates = ::getenv_default("UPDATES", 0);

const char* G_host;

#define MAXSIZE 65536    // if pvxmonitor output ever greater than this per line, we're in trouble

char* indent(int delta)
{
	static char str[80];
	static size_t level;

	while(strlen(str) < level){
		strcat(str, "\t");   // post increment from previous delta==1
	}

	switch(delta){
	case 1:
		++level; /* incr level for next time, no change to str this time */
		break;
	case 0:
		break;	/* no change */
	case -1:	/* reduce before */
		if (level){
			--level;
			if (strlen(str) >= 1){
				str[strlen(str)-1] = '\0';
			}
		}
		break;
	}
	return str;
}

class Formatter {

protected:
	Formatter() {}
public:
	virtual void print(SOE_HOLD_HEADER* header, int* ht_data) = 0;
	virtual void start() {}
	virtual void finish() {}

	static Formatter* instance(const char* mode = 0);
	/* call first time with mode != 0 to create, call with mode==0 to re-use */
};

class JSON_Formatter: public Formatter {
	void print_intro()
	{
		printf("%s{ \"HT\": [\n", indent(1));
	}
	void print_outro()
	{
		printf("%s] }\n", indent(-1));
	}

	void print_header_intro()
	{
		printf("%s " "{\n", indent(1));
	}
	void print_header_outro(bool not_final)
	{
		printf("%s " "}%c\n", indent(-1), not_final? ',': ' ');
	}
	void print(SOE_HOLD_HEADER& header);
	void print_wrus(unsigned* sp32);
	void print_raw(SOE_HOLD_HEADER& header, int* ht_data);

protected:
	JSON_Formatter() : Formatter() {}
public:
	virtual void start() {
		print_intro();
	}
	virtual void finish() {
		print_outro();
	}
	virtual void print(SOE_HOLD_HEADER* header,int* ht_data);

	friend class Formatter;
};

void JSON_Formatter::print(SOE_HOLD_HEADER& header)
{
	printf("%s" "\"HDR\": {\n", indent(1));
	printf("%s" "\"pv_id\":		%u,\n", indent(0), header.pv_id);
	printf("%s" "\"client_data\":	%u,\n", indent(0), header.client_data);
	printf("%s" "\"timestamp\":	%llu,\n", 	indent(0), header.timestamp);
	printf("%s" "\"data_offset\":	%u,\n", indent(0), header.data_offset);
	printf("%s" "\"ss_u32\":		%u,\n", indent(0), header.ss_u32);
	printf("%s" "\"ai_count\":	%u,\n", indent(0), header.ai_count);
	printf("%s" "\"di_count\":	%u,\n", indent(0), header.di_count);
	printf("%s" "\"sp_count\":	%u\n", indent(0), header.sp_count);
	printf("%s" "},\n", indent(-1));
}

void JSON_Formatter::print_wrus(unsigned* sp32) {
	unsigned wrv = sp32[SP2];
	unsigned wrs = sp32[SP3];

	printf("%s " "\"WRVS\":		%u,\n", indent(0), (wrv>>28)&0x07);
	printf("%s " "\"WRVT\":         %u,\n", indent(0), wrv&0x0fffffff);
	printf("%s " "\"WRUS\":         %llu\n", indent(0), getWrTs(wrs, wrv));
}

void JSON_Formatter::print_raw(SOE_HOLD_HEADER& header, int* ht_data)
{
	printf("%s" "\"RAW\": {\n", indent(1));
	short* ai16 = (short*)ht_data;
	printf("%s" "\"ai16\": [ ", indent(0));
	for (int ic = 0; ic < header.ai_count; ++ic){
		printf("%d%c", ai16[ic], ic+1 < header.ai_count? ',': ' ');
	}
	printf("],\n");
	printf("%s" "\"di32\": [ ", indent(0));
	unsigned* di32 = (unsigned*)ht_data + header.ai_count/2;
	for (int id = 0; id < header.di_count; ++id){
		printf("\"0x%08x\"%c", di32[id],  id+1 < header.di_count? ',': ' ');
	}
	printf("],\n");
	printf("%s" "\"sp32\": [ ", indent(0));
	unsigned* sp32 = di32 + header.di_count;
	for (int isp = 0; isp < header.sp_count; isp++){
		printf("\"0x%08x\"%c", sp32[isp], isp+1 < header.sp_count? ',': ' ');
	}
	printf("],\n");
	print_wrus(sp32);
	printf("%s" "}\n", indent(-1));
}
void JSON_Formatter::print(SOE_HOLD_HEADER* header, int* ht_data)
{
	print_header_intro();
	print(*header);
	print_raw(*header, ht_data);
	print_header_outro((header+1)->pv_id != 0);
}


void parse(int* ht_data, unsigned nelems)
{
	if (G_verbose > 1 ) fprintf(stderr, "parse: %p, %u\n",
			ht_data, nelems);

	Formatter::instance()->start();

	for (SOE_HOLD_HEADER* header = (SOE_HOLD_HEADER*)ht_data;
			header->pv_id != 0; ++header){
		Formatter::instance()->print(header, ht_data+header->data_offset);
	}
	Formatter::instance()->finish();

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

void usage(const char* argv0)
{
	std::cerr << "Usage: " << argv0 << " <opts> HOST\n"
		"\n"
		"monitors <HOST>:SOE_HLD and outputs selected format"
		"-h 		Show this message\n"
		"-f <file>      output to file\n"
		"-F <format>	brief|json|bin|hexdump|hint\n"
		"-U <updates>   update <updates> times, quit, default: forever\n"
		"-# <max>       set maximum number of array elements to output"
		"-e		AI output in egu\n"
		;

}


Formatter* Formatter::instance(const char* mode)
{
	static Formatter* _instance;

	if (mode == 0){
		assert(_instance != 0);
		return(_instance);
	}else if (strcmp(mode, "json") == 0){
		return _instance = new JSON_Formatter;
	}else{
		std::cerr << "ERROR format " << mode << " unknown" << "\n";
		exit(1);
	}
}

void ui(int argc, char* argv[])
{
	int opt;
	const char* format = "json";

	while((opt = getopt(argc, argv, "heU:#:F:f:")) != -1){
		switch(opt){
		case 'h':
			usage(argv[0]);
			exit(0);
		case 'U':
			G_updates = atol(optarg);
			break;
		case 'F':
			format = optarg;
			break;
		default:
			usage(argv[0]);
			std::cerr << "\nUnknown Argument: " <<char(opt)<<"\n";
			exit(1);
		}
	}
	if (optind < argc){
		G_host = argv[optind];
	}else{
		std::cerr << "ERROR HOST not defined\n";
		exit(1);
	}
	Formatter::instance(format);
}
int main(int argc, char* argv[]){

	ui(argc, argv);

	static char myline[MAXSIZE];
	int lno = 0;
	int update = 0;
	char cmd[128];
	snprintf(cmd, 128, "unbuffer pvxmonitor -r value  %s:SOE_HLD", G_host);

	FILE *pp = popen(cmd, "r");
	while (fgets(myline, MAXSIZE, pp) != 0){
		lno += 1;
		if (G_verbose > 2 ) fprintf(stderr, "[%2d] %s\n", lno, myline);
		char* cursor = strstr(myline, NEEDLE);
		if (cursor){
			parse(cursor+strlen(NEEDLE));
			if (++update >= G_updates && G_updates != 0){
				break;
			}
		}
	}
	return 0;
}

