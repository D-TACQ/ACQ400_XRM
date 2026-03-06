/*
 * acq400_common.cpp
 * BufferInitCommon: map all the buffers ONCE. Shared by multiple threads
 *
 *  Created on: 6 Mar 2026
 *      Author: pgm
 */

#include <stdio.h>
#include <vector>
#include <unistd.h>
using namespace std;
#include "acq-util.h"
#include "Buffer.h"

class BufferInitCommon {
public:
	BufferInitCommon() {
		for (unsigned ii = 0; ii < Buffer::nbuffers; ++ii){
			Buffer::create(getRoot(0), Buffer::bufferlen);
		}
	}
};

static BufferInitCommon init_buffers_early;

