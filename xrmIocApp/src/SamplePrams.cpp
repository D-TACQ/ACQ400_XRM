/*
 * SamplePrams.cpp
 *
 *  Created on: 6 Apr 2026
 *      Author: pgm
 */


#include <stdio.h>
#include <string.h>

#include "SamplePrams.h"

// Binary pattern stored here for client apps to find

#define SAMPLE_PRAMS_FILE	"/dev/shm/SamplePrams"
#define SP_MAGIC	(('S' << 24)|('M' << 16)|('P' << 8)|'L')

SamplePrams::SamplePrams()
{
	memset(this, 0, sizeof(SamplePrams));
}
void SamplePrams::validate()
{
	if ((SSB * NSAM * (AI_COUNT+DI_COUNT+SP_COUNT)) != 0){
		MAGIC = SP_MAGIC;
	}else{
		fprintf(stderr, "%s:%s fail to validate\n", __FILE__, __FUNCTION__);
	}
}

bool SamplePrams::isValid() const
{
	return MAGIC == SP_MAGIC;
}
int SamplePrams::store(const SamplePrams& samplePrams)
{
	if (samplePrams.isValid()){
		FILE* fp = fopen(SAMPLE_PRAMS_FILE, "w");
		if (fp == 0){
			return -1;
		}else{
			int rc = fwrite(&samplePrams, sizeof(SamplePrams), 1, fp);
			fclose(fp);
			return rc;
		}
	}else{
		return -1;
	}
}

int SamplePrams::load(SamplePrams& samplePrams)
{
	FILE* fp = fopen(SAMPLE_PRAMS_FILE, "r");
	if (fp == 0){
		return -1;
	}else{
		int rc = fread(&samplePrams, sizeof(SamplePrams), 1, fp);
		fclose(fp);
		if (rc == 1){
			if (samplePrams.isValid()){
				return 1;
			}else{
				return -2;
			}
		}else{
			return rc;
		}
	}
}

