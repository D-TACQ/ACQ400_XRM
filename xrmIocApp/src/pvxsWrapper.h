/*
 * pvxsWrapper.h
 *
 *  Created on: 27 Mar 2026
 *      Author: pgm
 */

#ifndef XRMIOCAPP_SRC_PVXSWRAPPER_H_
#define XRMIOCAPP_SRC_PVXSWRAPPER_H_

#define MAXSIZE 65536    // if pvxmonitor output ever greater than this per line, we're in trouble


template<class CONTAINER>
class PVX_getter {
	const CONTAINER* parse(CONTAINER* values, char* txt);

	const char* pv_name;
	const char* needle;

	static const char* make_pv_name(const char* hostname, const char* pv_suffix);
public:

	const CONTAINER*  pvx_get(
		CONTAINER* values,
		bool wait_first_change = false);


	PVX_getter(
		const char* hostname,
		const char* pv_suffix,
		const char* NEEDLE = 0);

	~PVX_getter();
};





#endif /* XRMIOCAPP_SRC_PVXSWRAPPER_H_ */
