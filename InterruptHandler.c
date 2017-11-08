#include <stdio.h>
#include "PdmaCtrl.h"
#include "ConfigApp.h"
#include "OverLapRam.h"
#include "./NuOne/pdma_framework.h"
#include "./NuOne/PlaySpiNuOne.h"

extern uint32_t g_nCallbackCounter;
extern __align(4) int32_t g_bBlockReady[2];
extern __align(4) int32_t g_bCpuOverload;
extern __align(4) int32_t g_bRecordStart;



//---------------------------------------------------------------------------------------------------------
// PDMA interrupt handler                                                 
//---------------------------------------------------------------------------------------------------------
void PDMA_IRQHandler()
{
#if ((APU_ENABLE) && (APU_PDMA_ENABLE))

	if(PDMA_GET_CH_INT_STS(eDRVPDMA_CHANNEL_1)&PDMA_CHIF_WAIF_Msk)
	{
		PDMA_CLR_CH_INT_FLAG(eDRVPDMA_CHANNEL_1, PDMA_CHIF_WAIF_Msk);
		bBufferEmpty=TRUE;
		PDMA1CallBackCount++;
	}

#endif
	
#if ((ADC_ENABLE) && (ADC_PDMA_ENABLE))
	if (PDMA_GET_CH_INT_STS(ADC_PDMA_CH)&PDMA_CHIF_TXOKIF_Msk)
	{
		PDMA_CLR_CH_INT_FLAG(ADC_PDMA_CH,PDMA_CHIF_TXOKIF_Msk );
		g_nCallbackCounter++;
		if (g_bRecordStart)
		{
			if ((g_nCallbackCounter & 1) == 1)
			{
				g_bBlockReady[0] = 1;
				if (g_bBlockReady[1])
					g_bCpuOverload = 1;
				PdmaCtrl_Start(ADC_PDMA_CH, (uint32_t *)&ADC->DAT, (uint32_t *)&overlapMem.vrUseMemPool.MicBuffer[1][0], AUDIOBUFFERSIZE);
			}
			else
			{
				g_bBlockReady[1] = 1;
				if (g_bBlockReady[0])
					g_bCpuOverload = 1;
				PdmaCtrl_Start(ADC_PDMA_CH, (uint32_t *)&ADC->DAT, (uint32_t *)&overlapMem.vrUseMemPool.MicBuffer[0][0], AUDIOBUFFERSIZE);
			}
		}
	}
#endif
}


