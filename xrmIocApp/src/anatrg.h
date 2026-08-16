/*
 * anatrg.h
 *
 *  Created on: 16 Aug 2026
 *      Author: pgm
 */

#ifndef XRMIOCAPP_SRC_ANATRG_H_
#define XRMIOCAPP_SRC_ANATRG_H_

#include <vector>

struct Command {
	const char* key;
	Command(const char* _key);
	bool match(const char *testkey);

	virtual unsigned operator() (int p1, int p2, int hyst) const = 0;
};

class AnaTrg {

	const int nchan;
	const unsigned offset;
	void* const mapping;

	void setThreshold(int chan, unsigned abcd);
	void setThreshold(unsigned abcd);
public:
	AnaTrg(int _nchan, void* _mapping, unsigned _offset);
	int operator() (const char* key, int p1, int p2, int hyst);

	static std::vector<Command*> commands;
};


#endif /* XRMIOCAPP_SRC_ANATRG_H_ */
