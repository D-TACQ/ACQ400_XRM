/** @file SamplePrams.h
 * @brief defines SamplePrams, sample geometry definition struct.
 *
 *  Created on: 9 Apr 2026
 *      Author: pgm
 */

#ifndef XRMIOCAPP_SRC_SAMPLEPRAMS_H_
#define XRMIOCAPP_SRC_SAMPLEPRAMS_H_

struct SamplePrams {
	int MAGIC;		/**< set when valid 		*/
	int SSB;		/**< Sample Size Bytes 		*/
	int NSAM;		/**< Number of Samples in Burst */
	int AI_COUNT;		/**< Number of AI16 channels 	*/
	int AI_INDEX;		/**< Index of first AI16 	*/
	int DI_COUNT;		/**< Number of DI32 channels	*/
	int DI_INDEX;		/**< Index of first DI32	*/
	int SP_COUNT;		/**< Number of SP32 channels 	*/
	int SP_INDEX;		/**< Index of first SP32 channel*/
	SamplePrams();
	bool isValid() const;	/**< structure is valid */
	void validate();	/**< validate the structure */

	static int store(const SamplePrams& samplePrams);  /**< store to file */
	static int load(SamplePrams& samplePrams);         /**< load from file */
};

const SamplePrams* get_acq400_SamplePrams();



#endif /* XRMIOCAPP_SRC_SAMPLEPRAMS_H_ */
