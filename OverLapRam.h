#ifndef __OVERLAPRAM_H__
#define __OVERLAPRAM_H__

//#include "base_types.h"
#include "NVTTypes.h"
#include "NuOneEx.h"

#ifdef __cplusplus
extern "C"{
#endif

#define AUDIOBUFFERSIZE		160		   //duplicated defined
#define k_nMemSize				9600

union _overlapMem {
	struct {
		UINT8 lpbyMemPool[k_nMemSize];					//VR.c Use
		INT16 MicBuffer[2][AUDIOBUFFERSIZE];	//WaveRecord.c Use	使用錄音會用到
	} vrUseMemPool;													//使用VR辨識時會用到
	/*
	有需要在其他時後使用到的大容量記憶體，可以利用這一塊使用，注意不要和VR同時就可。
	*/
	
	struct{
		UINT32 au32DecodeWorkBuf[(NUONEEX_DECODE_WORK_BUF_SIZE+3)/4];
		UINT32 u32NuOneExTempBuf[(NUONEEX_DECODE_TEMP_BUF_SIZE+3)/4];
		INT16 i16OutBuf[2][NUONEEX_DECODE_SAMPLE_PER_FRAME];
	} NuOneMemPool;
};

extern union _overlapMem overlapMem;

#ifdef __cplusplus
}
#endif

#endif // __OVERLAPRAM_H__
