#include "SPIFlash.h"

#define ADC_SAMPLE_RATE	16000//8000

#define RECORD_START_ADDR		0x18000//0x10000		//0x10000 = 64KB, need 4K aligned for first address of a sector
#define MAX_RECORD_COUNT		8* 16 * 1024	//maximum sampling points in each recording, should be multiple of 1024

void Record2SPIFlash(uint32_t RecordStartAddr, uint32_t TotalPCMCount);

extern S_SPIFLASH_HANDLER g_sSpiFlash;
