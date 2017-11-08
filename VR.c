#include "ISD9100.h"//#include "ISD9xx.h"
#include "NVTTypes.h"//#include "base_types.h"
#include "CSpotterSDKApi.h"
#include "OverLapRam.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Define Function Prototypes                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void Wave_StartRecord(void);
void Wave_StopRecord(void);
INT Wave_GetSample(SHORT **lppsSample);
INT Wave_UnlockSample(SHORT **lppsSample);
INT Wave_CheckCpuOverload(void);
void Wave_ClearCpuOverload(void);
//void ShowLED(int32_t n);

/*---------------------------------------------------------------------------------------------------------*/
/* Define macro & global variables                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#define k_nSampleRate		(16000)

#define k_nStateSize		(64)
INT nMemSize = k_nMemSize;

__align(4) BYTE lpbyState[k_nStateSize] = { 0 };

#define k_nMaxStageWait	(2 * k_nSampleRate)
#define k_nMaxWait			(k_nMaxStageWait + (2 * k_nSampleRate))
#define k_nMaxBufTime		200

/*---------------------------------------------------------------------------------------------------------*/
/*  Main Function									                                           			                         */
/*---------------------------------------------------------------------------------------------------------*/
/*
 *DoVR_get 一般辨識，有結果回傳ID。
 */
int32_t DoVR_get(uint32_t hCSpotter, uint32_t bShowOnly, uint32_t nTimeOut, uint32_t nPinOn, int (*funcCheckKeyPress)(void))
{
	INT nRet;
	INT	nID, nLED, nWait;
	SHORT *lpsSample;
	INT nNumSample;

	Wave_StartRecord();
	nWait = 0;
	nRet = 0;

	CSpotter_Reset((HANDLE)hCSpotter);

	while (1)
	{
		if (funcCheckKeyPress && (nID = funcCheckKeyPress()) > 0)
		{
			nRet = nID;
			break;
		}
		nNumSample = Wave_GetSample(&lpsSample);
		if (nNumSample < 0)
			break;
		if (nNumSample == 0)
			continue;
		if (CSpotter_AddSample((HANDLE)hCSpotter, lpsSample, nNumSample) == CSPOTTER_SUCCESS)
		{
			nID = CSpotter_GetResult((HANDLE)hCSpotter);
			nLED = nID + 1;
			if (Wave_CheckCpuOverload())
			{
				nLED |= 0x40;
				Wave_ClearCpuOverload();
			}
			nLED |= nPinOn;
//			ShowLED(nLED);
			if (bShowOnly == 0)
			{
				nRet = nID + 1;
				break;
			}
			nWait = 0;
		}

		Wave_UnlockSample(&lpsSample);

		nWait++;
		if (nTimeOut > 0 && nWait >= nTimeOut)
			break;
	}
	
	CSpotter_Release((HANDLE)hCSpotter);

	Wave_StopRecord();

	return nRet;
}

/*
 *DoVR_create_sep 是CSpotter Init
 */
uint32_t DoVR_create_sep(uint32_t lpbyBaseModel, uint32_t lpbyModel)
{
	HANDLE hCSpotter = NULL;
	INT n, nErr;

	n = CSpotter_GetMemoryUsage_Sep((BYTE *)lpbyBaseModel, (BYTE *)lpbyModel, k_nMaxBufTime);

	hCSpotter = CSpotter_Init_Sep((BYTE *)lpbyBaseModel, (BYTE *)lpbyModel, k_nMaxBufTime, overlapMem.vrUseMemPool.lpbyMemPool, nMemSize, lpbyState, k_nStateSize, &nErr);
	if (hCSpotter == NULL)
		return NULL;
	return (uint32_t)hCSpotter;
}

/*
 *DoVR_sep 用於一般辨識
 */
int32_t DoVR_sep(uint32_t lpbyBaseModel, uint32_t lpbyModel, uint32_t bShowOnly, uint32_t nTimeOut, uint32_t nPinOn, int (*funcCheckKeyPress)(void))
{
	uint32_t hCSpotter = DoVR_create_sep(lpbyBaseModel, lpbyModel);
	if (hCSpotter == NULL)
		return -1;
	return DoVR_get(hCSpotter, bShowOnly, nTimeOut, nPinOn, funcCheckKeyPress);
}

/*
 *DoVR_create_multi 用於兩階段辨識當中，把Trigger Model和command model合在一起
 *並且CSpotter Init
 */
uint32_t DoVR_create_multi(uint32_t lpbyBaseModel, uint32_t lppbyModel, uint32_t nNumModel)
{
	HANDLE hCSpotter = NULL;
	INT n, nErr;

	n = CSpotter_GetMemoryUsage_Multi((BYTE *)lpbyBaseModel, (BYTE **)lppbyModel, nNumModel, k_nMaxBufTime);

	hCSpotter = CSpotter_Init_Multi((BYTE *)lpbyBaseModel, (BYTE **)lppbyModel, nNumModel, k_nMaxBufTime, overlapMem.vrUseMemPool.lpbyMemPool, nMemSize, lpbyState, k_nStateSize, &nErr);
	if (hCSpotter == NULL)
		return NULL;
	return (uint32_t)hCSpotter;
}

int32_t DoVR_multi(uint32_t lpbyBaseModel, uint32_t lppbyModel, uint32_t nNumModel, uint32_t bShowOnly, uint32_t nTimeOut, uint32_t nPinOn, int (*funcCheckKeyPress)(void))
{
	uint32_t hCSpotter = DoVR_create_multi(lpbyBaseModel, lppbyModel, nNumModel);
	if (hCSpotter == NULL)
		return -1;
	return DoVR_get(hCSpotter, bShowOnly, nTimeOut, nPinOn, funcCheckKeyPress);
}

/*
 *DoVR_get_two_as_one 用於兩階段辨識，當有Trigger之後，在一定時間內要念之後的command，否則認為失效
 */
int32_t DoVR_get_two_as_one(uint32_t hCSpotter, INT pnWordIdx[], uint32_t bShowOnly, uint32_t nTimeOut, uint32_t nPinOn, int (*funcCheckKeyPress)(void))
{
	INT nRet;
	INT nWordDura, nEndDelay;
	INT nStage, nStageWait;
	INT	nID, nLED, nWait;
	SHORT *lpsSample;
	INT nNumSample;

	Wave_StartRecord();
	nWait = 0;
	nRet = 0;

	CSpotter_Reset((HANDLE)hCSpotter);

	nStage = 0;
	while (1)
	{
		if (funcCheckKeyPress && (nID = funcCheckKeyPress()) > 0)
		{
			nRet = nID;
			break;
		}
		nNumSample = Wave_GetSample(&lpsSample);
		if (nNumSample < 0)
			break;
		if (nNumSample == 0)
			continue;
		if (CSpotter_AddSample((HANDLE)hCSpotter, lpsSample, nNumSample) == CSPOTTER_SUCCESS)
		{
			nID = CSpotter_GetResult((HANDLE)hCSpotter);
			if (nStage == 0)
			{
				if (nID < pnWordIdx[0] || nID >= pnWordIdx[1])
				{
					nLED = 0x80;
					if (Wave_CheckCpuOverload())
					{
						nLED |= 0x40;
						Wave_ClearCpuOverload();
					}
					nLED |= nPinOn;
//					ShowLED(nLED);
					nStage = 1;
				}
			}
			else
			{
				if (nID >= pnWordIdx[0] && nID < pnWordIdx[1])
				{
					CSpotter_GetResultEPD((HANDLE)hCSpotter, &nWordDura, &nEndDelay);
					if (nStageWait - (nWordDura + nEndDelay) < k_nMaxStageWait)
					{
						nLED = nID - pnWordIdx[0] + 1;
						if (Wave_CheckCpuOverload())
						{
							nLED |= 0x40;
							Wave_ClearCpuOverload();
						}
						nLED |= nPinOn;
//						ShowLED(nLED);
						if (bShowOnly == 0)
						{
							nRet = nID + 1;
							break;
						}
						nWait = 0;
					}
					else
					{
//						ShowLED(nPinOn);
					}
					nStage = 0;
				}
			}
			nStageWait = 0;
		}

		Wave_UnlockSample(&lpsSample);

		if (nStage > 0)
		{
			nStageWait += nNumSample;
			if (nStageWait >= k_nMaxWait)
			{
//				ShowLED(nPinOn);
				nStage = 0;
			}
		}

		nWait++;
		if (nTimeOut > 0 && nWait >= nTimeOut)
			break;
	}

	CSpotter_Release((HANDLE)hCSpotter);
	
	Wave_StopRecord();

	return nRet;
}

/*
 *DoVR_two_as_one 用於兩階段辨識，當有Trigger keyword command在辨識後面的command
 *command 是一起辨識，有助於提高辨識率
 */

int32_t DoVR_two_as_one(uint32_t lpbyBaseModel, uint32_t lppbyModel, uint32_t nNumModel, uint32_t bShowOnly, uint32_t nTimeOut, uint32_t nPinOn, int (*funcCheckKeyPress)(void))
{
	uint32_t hCSpotter = DoVR_create_multi(lpbyBaseModel, lppbyModel, nNumModel);	// 0:trigger, 1:command, 2:SD
	INT pnWordIdx[3] = { 0 };
	INT i;
	if (hCSpotter == NULL || nNumModel < 2)
		return -1;
	pnWordIdx[0] = CSpotter_GetNumWord(((BYTE **)lppbyModel)[0]);
	pnWordIdx[1] = pnWordIdx[0] + CSpotter_GetNumWord(((BYTE **)lppbyModel)[1]);
	pnWordIdx[2] = pnWordIdx[1];

	for (i = 0; i < pnWordIdx[0]; i++)
		CSpotter_SetCmdResponseReward((HANDLE)hCSpotter, i, -15);

	if (nNumModel > 2)
	{
		pnWordIdx[2] = pnWordIdx[1] + CSpotter_GetNumWord(((BYTE **)lppbyModel)[2]);
		for (i = pnWordIdx[1]; i < pnWordIdx[2]; i++)
			CSpotter_SetCmdResponseReward((HANDLE)hCSpotter, i, -15);
	}

	CSpotter_SetResponseTime((HANDLE)hCSpotter, 20);

	return DoVR_get_two_as_one(hCSpotter, pnWordIdx, bShowOnly, nTimeOut, nPinOn, funcCheckKeyPress);
}
