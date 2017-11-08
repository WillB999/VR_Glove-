#include <stdio.h>
#include "ISD9100.h"
#include "dpwm.h"
#include "pdma.h"
#include "PdmaCtrl.h"//SW 0422 
#include "pdma_framework.h"
#include "NuOneEx.h"
#include "SPIFlash.h"
#include "AudioRom.h"
#include "PlaySpiNuOne.h"
#include "OverLapRam.h"

S_ROM_AUDIO_CHUNK_INFO sAudioChunkInfo;

extern S_AUDIO_CALLBACK g_asAppCallBack[];
extern S_SPIFLASH_HANDLER g_sSpiFlash;

BOOL	bPCMPlaying=FALSE;
BOOL 	bBufferEmpty=FALSE;
BOOL	bLastOneBuffer=FALSE;
UINT16 	u16SampleRate;
UINT32 	PDMA1CallBackCount;
UINT8 u8TotalAudioNum=0;

extern UINT8 SPIFlash_Initiate(void);

void Audio_Init(void)
{
		CLK_EnableLDO(CLK_LDOSEL_3_3V);
	if (! SPIFlash_Initiate())//SW 1125
		while(1);	
	
    /* Unlock protected registers */
    SYS_UnlockReg();
	
	/* Set ADC clock from HCLK */
    CLK_SetModuleClock(ADC_MODULE, MODULE_NoMsk, CLK_CLKDIV0_ADC(1));//ok	
	
	/* Set DPWM clock from HIRC2X */
	CLK_SetModuleClock(DPWM_MODULE, CLK_CLKSEL1_DPWMSEL_HIRC2X, MODULE_NoMsk);
	
	    SystemCoreClockUpdate();
	
	SYS_LockReg();
	
	u8TotalAudioNum= AudioRom_GetAudioNum( SPIFlash_ReadDataCallback, AUDIOROM_STORAGE_START_ADDR );
}


int32_t FrmWrk_PDMA_Open(E_DRVPDMA_CHANNEL_INDEX eChannel, STR_PDMA_T *sParam)
{						  
	volatile int32_t i = 10;
	PDMA_T *pdma = (PDMA_T *)((uint32_t) PDMA0_BASE + (0x100 * eChannel));
    //-----------------------------------------------------------------------------------------------------
    // Check PDMA channel                                                                                  
    //-----------------------------------------------------------------------------------------------------
    if (eChannel > eDRVPDMA_CHANNEL_3)
    {
		return E_DRVPDMA_ERR_PORT_INVALID;
    }

	//-----------------------------------------------------------------------------------------------------
    // Set PDMA settings                                                                                   
    //-----------------------------------------------------------------------------------------------------
    PDMA_Open( 1<<eChannel );//PDMA_GCR->GLOCTL |= (1 << eChannel) << PDMA_GLOCTL_CHCKEN_Pos; //PDMA Controller Channel Clock Enable//PDMA_GCR->GCR.HCLK_EN |= (1<<(eChannel));	              // Enable Channel Clock 
 	pdma->DSCT_CTL |= PDMA_DSCT_CTL_CHEN_Msk;//PDMA->channel[eChannel].CSR.PDMACEN = 1; 			  	  // Enable PDMA Channel 
	pdma->DSCT_CTL |= PDMA_DSCT_CTL_SWRST_Msk;//PDMA->channel[eChannel].CSR.SW_RST=1;					  // Reset PDMA Channel 
    while(i--);	  // Need a delay to allow reset 

	PDMA_SetTransferAddr(eChannel, sParam->sSrcAddr.u32Addr, (sParam->sSrcAddr.eAddrDirection)<<PDMA_DSCT_CTL_SASEL_Pos, sParam->sDestAddr.u32Addr, (sParam->sDestAddr.eAddrDirection)<<PDMA_DSCT_CTL_DASEL_Pos);    
		//PDMA->channel[eChannel].SAR = sParam->sSrcAddr.u32Addr;   // Set Source Address 
    	//PDMA->channel[eChannel].DAR = sParam->sDestAddr.u32Addr;  // Set Destination Address 
    	//PDMA->channel[eChannel].CSR.SAD_SEL = sParam->sSrcAddr.eAddrDirection;
    	//PDMA->channel[eChannel].CSR.DAD_SEL = sParam->sDestAddr.eAddrDirection;

	pdma->DSCT_CTL = (pdma->DSCT_CTL & ~PDMA_DSCT_CTL_TXWIDTH_Msk) | (sParam->u8TransWidth)<<PDMA_DSCT_CTL_TXWIDTH_Pos;//PDMA->channel[eChannel].CSR.APB_TWS = sParam->u8TransWidth;
  	PDMA_SetTransferDirection( eChannel, (sParam->u8Mode)<<PDMA_DSCT_CTL_MODESEL_Pos );//PDMA->channel[eChannel].CSR.MODE_SEL = sParam->u8Mode;
   	pdma->DSCT_CTL |= (sParam->u8WrapBcr)<<PDMA_DSCT_CTL_WAINTSEL_Pos;//PDMA->channel[eChannel].CSR.WRA_INT_SEL = sParam->u8WrapBcr;

    pdma->TXBCCH = sParam->i32ByteCnt;//PDMA->channel[eChannel].BCR = sParam->i32ByteCnt;	    // Set Byte Count Register 
    
	return E_SUCCESS;    

}


void InitialDPWM(uint32_t u32SampleRate)
{
	uint32_t u32clk;


	CLK_EnableModuleClock(DPWM_MODULE);
	CLK_SetModuleClock(DPWM_MODULE, CLK_CLKSEL1_DPWMSEL_HIRC2X, MODULE_NoMsk);
	SYS_ResetModule(DPWM_RST);
	DPWM_Open();
	u32clk = CLK_GetHCLKFreq();
	DPWM_SET_FSDIV(DPWM,(u32clk/64)/u32SampleRate);
	DPWM_START_PLAY(DPWM);
}

void PDMA1forDPWM(UINT32 u32BufRdyAddr)
{
	STR_PDMA_T sPDMA;  

	sPDMA.sSrcAddr.u32Addr 			= u32BufRdyAddr;
	sPDMA.sDestAddr.u32Addr 		= (uint32_t)&DPWM->DATA;
	sPDMA.u8Mode 					= eDRVPDMA_MODE_MEM2APB;
	sPDMA.u8TransWidth 				= eDRVPDMA_WIDTH_16BITS;
	sPDMA.sSrcAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_WRAPAROUND; 
	sPDMA.sDestAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_FIXED;  
	sPDMA.u8WrapBcr				 	= eDRVPDMA_WRA_WRAP_HALF_INT; 		//Interrupt condition set fro Half buffer & buffer end
    sPDMA.i32ByteCnt = NUONEEX_DECODE_SAMPLE_PER_FRAME * 4;	   	//Full MIC buffer length (byte)
    
	FrmWrk_PDMA_Open(eDRVPDMA_CHANNEL_1, &sPDMA);

	// PDMA Setting 
	PDMA_SetTransferMode( eDRVPDMA_CHANNEL_1, PDMA_DPWM, 0, 0 );

	// Enable INT 
	PDMA_CLR_CH_INT_FLAG(eDRVPDMA_CHANNEL_1, 0x00000503);
	//PDMA_DisableInt(eDRVPDMA_CHANNEL_1, eDRVPDMA_BLKD);
	PDMA_EnableInt(eDRVPDMA_CHANNEL_1, eDRVPDMA_WAR);
	
	// Enable DPWM DMA
	DPWM_ENABLE_PDMA(DPWM);
	 	
	PDMA_Trigger(eDRVPDMA_CHANNEL_1);
}

void PlayLoop(void)
{
	if(bBufferEmpty==TRUE)
	{
		if(!NuOneEx_DecodeIsEnd((UINT8*)overlapMem.NuOneMemPool.au32DecodeWorkBuf))
		{	
			if ((PDMA1CallBackCount&0x1)==1)
				NuOneEx_DecodeProcess((UINT8*)overlapMem.NuOneMemPool.au32DecodeWorkBuf, (UINT8*)overlapMem.NuOneMemPool.u32NuOneExTempBuf,&overlapMem.NuOneMemPool.i16OutBuf[0][0],SPIFlash_ReadDataCallback,UserEvent_ProcessCallback);		
			else
				NuOneEx_DecodeProcess((UINT8*)overlapMem.NuOneMemPool.au32DecodeWorkBuf, (UINT8*)overlapMem.NuOneMemPool.u32NuOneExTempBuf,&overlapMem.NuOneMemPool.i16OutBuf[1][0],SPIFlash_ReadDataCallback,UserEvent_ProcessCallback);			
			bBufferEmpty=FALSE;
		}
		else
		{
			if(bLastOneBuffer==TRUE)
				bPCMPlaying=FALSE;
			else
				bLastOneBuffer=TRUE;
		}		
	}
}

void PlayClose(void)
{
	PDMA_Close();
	DPWM_Close();
}

BOOL PlayNuOnebyIdx(UINT8 u8PlayIdx)
{
	AudioRom_GetAudioChunkInfo( SPIFlash_ReadDataCallback, AUDIOROM_STORAGE_START_ADDR, u8PlayIdx, &sAudioChunkInfo);

	if((u16SampleRate= NuOneEx_DecodeInitiate(	(UINT8*)overlapMem.NuOneMemPool.au32DecodeWorkBuf, (UINT8*)overlapMem.NuOneMemPool.u32NuOneExTempBuf, sAudioChunkInfo.u32AudioChunkAddr, SPIFlash_ReadDataCallback))==0)
	{	
		//to main printf("NuOne Initiation Failed\n");
		return FALSE;
	}

	bPCMPlaying=TRUE;
	PDMA1CallBackCount=0;
	bBufferEmpty=FALSE;

	if(!NuOneEx_DecodeIsEnd((UINT8*)overlapMem.NuOneMemPool.au32DecodeWorkBuf))
		NuOneEx_DecodeProcess((UINT8*)overlapMem.NuOneMemPool.au32DecodeWorkBuf, (UINT8*)overlapMem.NuOneMemPool.u32NuOneExTempBuf,&overlapMem.NuOneMemPool.i16OutBuf[0][0],SPIFlash_ReadDataCallback,UserEvent_ProcessCallback);

	PdmaCtrl_Init();//SW 0422	
	InitialDPWM(u16SampleRate);
	PDMA1forDPWM((UINT32)&overlapMem.NuOneMemPool.i16OutBuf[0][0]);

	if(!NuOneEx_DecodeIsEnd((UINT8*)overlapMem.NuOneMemPool.au32DecodeWorkBuf))
		NuOneEx_DecodeProcess((UINT8*)overlapMem.NuOneMemPool.au32DecodeWorkBuf, (UINT8*)overlapMem.NuOneMemPool.u32NuOneExTempBuf,&overlapMem.NuOneMemPool.i16OutBuf[1][0],SPIFlash_ReadDataCallback,UserEvent_ProcessCallback);

  while(bPCMPlaying==TRUE)
	{
		 PlayLoop();
	}
	
	PlayClose();
	
	return TRUE;
}





