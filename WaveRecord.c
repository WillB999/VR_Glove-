#include <stdio.h>
#include "ISD9100.h"
#include "pdma.h"
#include "PdmaCtrl.h"
//#include "dpwm.h"
//#include "adpcm.h"
//#include "SPIFlash.h"
#include "micspk.h"
#include "ConfigApp.h"
#include "ConfigIO.h"
#include "OverLapRam.h"

uint32_t g_nCallbackCounter = 0;
__align(4) int32_t g_bBlockReady[2] = {0,0};
__align(4) int32_t g_bCpuOverload = 0;
__align(4) int32_t g_bRecordStart = 0;
uint32_t g_nMarqueeLEDCounter = 0;
__align(4) int32_t g_nMarqueeLEDUpDown = 0;	//0up	1down
__align(4) int32_t g_bMarqueeLEDStart = 0;

/*
void Wave_Initial(void)
{
  printf("+-------------------------------------------------------------------------+\n");
  printf("|       Recroding Start												      |\n");
  printf("+-------------------------------------------------------------------------+\n");

	//InitialADC();	  		//ADC initialization
	//InitialADC_Remote();
  	//DrvPDMA_Init();			//PDMA initialization
	//UNLOCKREG();

	g_nCallbackCounter = 0;
	g_bBlockReady[0] = g_bBlockReady[1] = 0;
	g_bCpuOverload = 0;

	//PDMA0forMIC((uint32_t)&overlapMem.vrUseMemPool.MicBuffer[0][0]);
}*/

void Wave_StartRecord(void)
{
	//Wave_Initial();

	PdmaCtrl_Init();
	MIC_Open();
	ADC_ENABLE_PGA(ADC, ADC_PGACTL_REFSEL_VMID, ADC_PGACTL_BOSST_GAIN_0DB);
	//ADC_SetPGAGaindB(ADC_PGA_GAIN);
	ADC_ENABLE_SIGNALPOWER(ADC,
	                       ADC_SIGCTL_ADCMOD_POWER|
						   ADC_SIGCTL_IBGEN_POWER|
	                       ADC_SIGCTL_BUFADC_POWER|
	                       ADC_SIGCTL_BUFPGA_POWER|ADC_SIGCTL_ZCD_POWER);
	ALC->CTL = 0x1E01C32F;//SW 0827.......too lazy
	ADC_SET_OSRATION(ADC, 2);
	ADC_SET_SDCLKDIV(ADC, 0x10);
	g_nCallbackCounter = 0;
	g_bBlockReady[0] = g_bBlockReady[1] = 0;
	g_bCpuOverload = 0;
	g_bRecordStart = 1;
	MIC_Start();
	PdmaCtrl_Start(ADC_PDMA_CH, (uint32_t *)&ADC->DAT, (uint32_t *)&overlapMem.vrUseMemPool.MicBuffer[0][0], AUDIOBUFFERSIZE);

}


void Wave_StopRecord(void)
{
	g_bRecordStart = 0;
	
	PdmaCtrl_Stop(ADC_PDMA_CH);//(APU_PDMA_CH);
	PdmaCtrl_Close(ADC_PDMA_CH);//(APU_PDMA_CH);
	MIC_Close();
}


int32_t Wave_GetSample(INT16 **lppsSample)
{
	while (g_bRecordStart)
	{
		if (g_bBlockReady[0])
		{
			*lppsSample = (INT16 *)overlapMem.vrUseMemPool.MicBuffer[0];
			return AUDIOBUFFERSIZE;
		}
		else if (g_bBlockReady[1])
		{
			*lppsSample = (INT16 *)overlapMem.vrUseMemPool.MicBuffer[1];
			return AUDIOBUFFERSIZE;
		}
		return 0;
	}
	return -1;
}

int32_t Wave_UnlockSample(INT16 **lppsSample)
{
		if (*lppsSample == (INT16 *)overlapMem.vrUseMemPool.MicBuffer[0])
			g_bBlockReady[0] = 0;
		if (*lppsSample == (INT16 *)overlapMem.vrUseMemPool.MicBuffer[1])
			g_bBlockReady[1] = 0;
		*lppsSample = NULL;
		return 0;
}

int32_t Wave_CheckCpuOverload(void)
{
	return g_bCpuOverload;
}

void Wave_ClearCpuOverload(void)
{
	g_bCpuOverload = 0;
}
