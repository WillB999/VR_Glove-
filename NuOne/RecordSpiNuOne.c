#include <stdio.h>
#include "ISD9100.h"
#include "clk.h"
#include "adc.h"
#include "pdma.h"
#include "PdmaCtrl.h"//SW 0422 
#include "pdma_framework.h"
#include "NuOneEx.h"
#include "RecordSpiNuOne.h"
#include "SPIFlash.h"

// #define ALC_EN

BOOL	bMicBufferReady;
uint32_t PDMA0CallBackCount;
uint32_t	u32BufferAddr0, u32BufferAddr1;

extern BOOL 	bBufferEmpty;
extern UINT32 	PDMA1CallBackCount;

S_SPIFLASH_HANDLER g_sSpiFlash;

S_AUDIOCHUNK_HEADER sAudioChunkHeader;

__align(4) UINT32 au32WorkBuf[(NUONEEX_ENCODE_WORK_BUF_SIZE+3)/4];			  //encode buffer is bigger than decode buffer
__align(4) UINT32 u32NuOneEncodeTempBuf[(NUONEEX_ENCODE_TEMP_BUF_SIZE+3)/4];  //encode buffer is bigger than decode buffer
int16_t AudioBuffer[2][NUONEEX_DECODE_SAMPLE_PER_FRAME];
__align(4) int8_t i8EncodeDataBuf[E_NUONEEX_ENCODE_BPS_10/8];

extern S_SPIFLASH_HANDLER g_sSpiFlash;

extern int32_t FrmWrk_PDMA_Open(E_DRVPDMA_CHANNEL_INDEX eChannel, STR_PDMA_T *sParam);


void PDMA_IRQHandler(void)
{
/*for Playback*/
	if(PDMA_GET_CH_INT_STS(eDRVPDMA_CHANNEL_1)&PDMA_CHIF_WAIF_Msk)
	{
		PDMA_CLR_CH_INT_FLAG(eDRVPDMA_CHANNEL_1, PDMA_CHIF_WAIF_Msk);
		bBufferEmpty=TRUE;
		PDMA1CallBackCount++;
	}
/*for Record*/
	if(PDMA_GET_CH_INT_STS(eDRVPDMA_CHANNEL_0)&PDMA_CHIF_WAIF_Msk)
	{
		PDMA_CLR_CH_INT_FLAG(eDRVPDMA_CHANNEL_0, PDMA_CHIF_WAIF_Msk);

		PDMA0CallBackCount++;
		bMicBufferReady=TRUE;
	}
}


void PDMA0forMIC(uint32_t u32DestAddr)
{
	STR_PDMA_T sPDMA;  

	sPDMA.sSrcAddr.u32Addr 					= (uint32_t)&ADC->DAT; 
  sPDMA.sDestAddr.u32Addr 				= u32DestAddr;
	sPDMA.u8Mode 										= eDRVPDMA_MODE_APB2MEM;
	sPDMA.u8TransWidth 							= eDRVPDMA_WIDTH_16BITS;
	sPDMA.sSrcAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_FIXED; 
	sPDMA.sDestAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_WRAPAROUND;  
	sPDMA.u8WrapBcr				 					= eDRVPDMA_WRA_WRAP_HALF_INT; 		//Interrupt condition set fro Half buffer & buffer end
  sPDMA.i32ByteCnt 								= NUONEEX_ENCODE_SAMPLE_PER_FRAME * 4;	   	//Full MIC buffer length (byte)

	FrmWrk_PDMA_Open(eDRVPDMA_CHANNEL_0, &sPDMA);

	PDMA_SetTransferMode( eDRVPDMA_CHANNEL_0, PDMA_ADC, 0, 0 );
 
	PDMA_CLR_CH_INT_FLAG(eDRVPDMA_CHANNEL_0, 0x00000503);
	PDMA_EnableInt( eDRVPDMA_CHANNEL_0, eDRVPDMA_WAR);	//Enable INT

	ADC_ENABLE_PDMA(ADC);// Enable ADC PDMA and Trigger PDMA specified Channel 
	ADC_START_CONV(ADC);// Start A/D conversion 
	PDMA_Trigger(eDRVPDMA_CHANNEL_0);// start ADC PDMA transfer*/
}



void ADC_AnaOpen(void)
{
	/* Reset IP */
	CLK_EnableModuleClock(ADC_MODULE);
	CLK_EnableModuleClock(ANA_MODULE);
	SYS_ResetModule(EADC_RST);

}


void InitialADC(void)
{
	// Open Analog block 
	ADC_AnaOpen();	

	/* Enable Analog block power */
	ADC_ENABLE_SIGNALPOWER(ADC,
	                       ADC_SIGCTL_ADCMOD_POWER|
						   ADC_SIGCTL_IBGEN_POWER|
	                       ADC_SIGCTL_BUFADC_POWER|
	                       ADC_SIGCTL_BUFPGA_POWER|
						   ADC_SIGCTL_ZCD_POWER);

	/* PGA Setting */
	ADC_MUTEON_PGA(ADC, ADC_SIGCTL_MUTE_PGA);	
	ADC_MUTEOFF_PGA(ADC, ADC_SIGCTL_MUTE_IPBOOST);
#ifdef ALC_EN
	ADC_ENABLE_PGA(ADC, ADC_PGACTL_REFSEL_VMID, ADC_PGACTL_BOSST_GAIN_0DB);
	ADC_SetPGAGaindB(1000);
#else
	ADC_ENABLE_PGA(ADC, ADC_PGACTL_REFSEL_VMID, ADC_PGACTL_BOSST_GAIN_26DB);//ADC_PGACTL_BOSST_GAIN_0DB);
	ADC_SetPGAGaindB(2000);  
#endif

	/*ALC setting*/	  //Diabled
	ADC_SetALCTargetLevel(-1650);
	ADC_SET_ALCDECAYTIME(ADC, 3);
	ADC_SET_ALCATTACKTIME(ADC, 2);
	ADC_ENABLE_NOISEGATE(ADC, 6);
#ifdef ALC_EN
	CLK->APBCLK0 |= CLK_APBCLK0_BFALCKEN_Msk;
	ADC_ENABLE_ALC(ADC, ADC_ALCCTL_NORMAL_MODE, ADC_ALCCTL_ABS_PEAK, ADC_ALCCTL_FASTDEC_ON);
#else
	ADC_DISABLE_ALC(ADC);
#endif
	
	/* MIC circuit configuration */
	ADC_ENABLE_VMID(ADC, ADC_VMID_HIRES_DISCONNECT, ADC_VMID_LORES_CONNECT);
	ADC_EnableMICBias(ADC_MICBSEL_65_VCCA);
	ADC_SetAMUX(ADC_MUXCTL_MIC_PATH, ADC_MUXCTL_POSINSEL_NONE, ADC_MUXCTL_NEGINSEL_NONE);
	
	/* Open ADC block */
	ADC_Open();

	ADC_SET_OSRATION(ADC, ADC_OSR_RATION_192);	 
#if (ADC_SAMPLE_RATE==16000)
ADC_SET_SDCLKDIV(ADC, 0x10); //Sample rate=16000
#endif
#if (ADC_SAMPLE_RATE==8000)
ADC_SET_SDCLKDIV(ADC, 0x20); //Sample rate=8000
#endif

	ADC_SET_FIFOINTLEVEL(ADC, 7);
	ADC_MUTEOFF_PGA(ADC, ADC_SIGCTL_MUTE_PGA);

	//BIQFilter_Setting();

	ADC_STOP_CONV(ADC);


}


void Record2SPIFlash(uint32_t RecordStartAddr, uint32_t TotalPCMCount)//RecordStart(void)
{
	uint32_t FlashRecordAddr, u32DataLength, u32EncodeDataSize;

	NuOneEx_EncodeInitiate((UINT8*)au32WorkBuf, (UINT8*)u32NuOneEncodeTempBuf, &sAudioChunkHeader, E_NUONEEX_ENCODE_BPS_10, ADC_SAMPLE_RATE);
	
	SPIFlash_Erase4K(&g_sSpiFlash,(RecordStartAddr/4096) , ((TotalPCMCount/8)/4096+1));//SW 1207 need to check//SW 1111 如果整除的話會多1sector	

	InitialADC();	  		//ADC initialization

	u32EncodeDataSize=0;
	PDMA0CallBackCount = 0;
	bMicBufferReady=FALSE;
	FlashRecordAddr = RecordStartAddr+0xC;//

	PdmaCtrl_Init();//SW 0422
	PDMA0forMIC((uint32_t)&AudioBuffer[0][0]);

	

	while (PDMA0CallBackCount <= (TotalPCMCount/NUONEEX_ENCODE_SAMPLE_PER_FRAME))
	{
		if (bMicBufferReady==TRUE)	
		{
			bMicBufferReady=FALSE;
			if((PDMA0CallBackCount & 1)==1)
				u32DataLength=NuOneEx_EncodeProcess((UINT8*)au32WorkBuf, (UINT8*)u32NuOneEncodeTempBuf,&AudioBuffer[0][0],&i8EncodeDataBuf[0]);		
			else
				u32DataLength=NuOneEx_EncodeProcess((UINT8*)au32WorkBuf, (UINT8*)u32NuOneEncodeTempBuf,&AudioBuffer[1][0],&i8EncodeDataBuf[0]);			

			u32EncodeDataSize+=u32DataLength;
			//Write ready buffer data
			SPIFlash_Write(&g_sSpiFlash,FlashRecordAddr,(PUINT8)&i8EncodeDataBuf[0],u32DataLength);			
			FlashRecordAddr = FlashRecordAddr + u32DataLength;
			
		}		//end of if(RecordDataReady==0)
	}	

	NuOneEx_EncodeEnd ((UINT8*)au32WorkBuf, (UINT8*)u32NuOneEncodeTempBuf, &sAudioChunkHeader, u32EncodeDataSize);
	SPIFlash_Write(&g_sSpiFlash,RecordStartAddr,(PUINT8)&sAudioChunkHeader,0xC);
	

	PDMA_Close();
 	ADC_Close();

}
