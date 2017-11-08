#include "NuOneEx.h"

#define SPI0_CLOCK	12000000 //SW 1020 copied from ConfigIP.h of NuOneExEnc

#define AUDIOROM_STORAGE_START_ADDR	(0)

extern S_ROM_AUDIO_CHUNK_INFO sAudioChunkInfo;
extern BOOL bPCMPlaying;
extern UINT8 u8TotalAudioNum;
extern BOOL 	bBufferEmpty;
extern UINT32 	PDMA1CallBackCount;

extern UINT32 SPIFlash_ReadDataCallback(void *pu8Buf, UINT32 u32StartAddr, UINT32 u32Count);
extern UINT32 UserEvent_ProcessCallback(UINT16 u16EventIndex, UINT16 u16EventSubIndex);

BOOL PlayNuOnebyIdx(UINT8 u8PlayIdx);
void PlayClose(void);
void PlayLoop(void);

BOOL PlaySPIFlash(uint32_t PlayStartAddr, uint32_t TotalPCMCount);
void Audio_Init(void);

