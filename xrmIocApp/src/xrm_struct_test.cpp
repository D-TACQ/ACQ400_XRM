/*
 * xrm_struct_test.cpp
 *
 *  Created on: 6 Feb 2026
 *      Author: pgm
 */



#include "xrm_structs.h"


/* can't have main in IOC build system .. */
int _main(int argc, char* argv[])
{
	volatile FMT the_fmt = {};
	volatile SOE_LUT the_lut = {};
	volatile SOE_HOLD_TABLE hold_table = {};

	return 0;
}

