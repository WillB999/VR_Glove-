/******************************************************************************
 * @file     main.c
 * @version  V3.00
 * $Revision: 1 $
 * $Date: 14/07/10 10:14a $
 * @brief    This sample code demo semihost function
 *
 * @note
 * Copyright (C) 2014 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include <stdio.h>
#include "ISD9100.h"
#include "SPIFlash.h"
#include "ConfigIO.h"
#include "AudioRom.h"
#include "./NuOne/PlaySpiNuOne.h"
#include "NVTTypes.h"

#define TEST_CODE                   FALSE //  TRUE  // 
#define FIXED_ISSUE                 TRUE

#define BIN_START_ADDR              (0xB000)	

#define SPI_SDMODEL_ADDR            (0x0000)
#define SPI_BIN_START_ADDR          (SPI_SDMODEL_ADDR + SDMODEL_SIZE)

#define PROMPT_MAX                  (32)		      // Max 32 Prompt
#define SPI0_CLOCK                  12000000   	      // ConfigIP.h

#define TRIGGER_PORTB_PINS_MASK	    (BIT6)              // (TG6_PIN_MASK)

#define PIN_LOW                     (0)
#define PIN_HIGH                    (1)

/***********************************************************/
#define NUMBER_OF_GROUPS     2                  // # of groups defined in UI modeling tool
#define NUMBER_OF_KEYWORDS   4                  // # of keywords defined in 1st group of UI modeling tool
/***********************************************************/
//
// VR commands
//
#define PLAY_MUSIC                  (0x01)
#define PLAY_MUSIC_1                (0x02)
#define PLAY_MUSIC_2                (0x03)
#define PLAY_MUSIC_3                (0x04)

#define SKIP_SONG                   (0x05)
#define GO_BACK_SONG                (0x06)
#define REPLAY_SONG                 (0x07)

#define STOP_MUSIC                  (0x08)
#define STOP_MUSIC_1                (0x09)
#define STOP_MUSIC_2                (0x0A)
#define STOP_MUSIC_3                (0x0B)

#define ANSWER_CALLER               (0x0C)
#define DISCONNECT_CALLER           (0x0D)
#define IGNORE_CALL                 (0x0E)

#define VOLUME_UP_CMD               (0x0F)
#define VOLUME_DOWN_CMD             (0x10)

#define ANSWER_CMD                  (0x11)

#define VIBRATOR_ON_TIME            ((uint16_t)500)
#define VIBRATOR_OFF_TIME           ((uint16_t)3000)

// Timer1 - duty cycle = 50%
#define ON_TIME                     ((uint16_t)10)   // = 13.325ms
#define OFF_TIME                    ((uint16_t)10)


// Port PA Interface command pins
#define IFC_PLAY_MUSIC_PIN          (BIT3)
#define IFC_NEXT_SONG_PIN           (BIT5)
#define IFC_PREVIOUS_SONG_PIN       (BIT4)
#define IFC_PAUSE_MUSIC_PIN         (BIT3)
#define IFC_VOLUME_UP_PIN           (BIT1)
#define IFC_VOLUME_DOWN_PIN         (BIT2)
#define IFC_ANSWER_PHONE_PIN        (BIT0)
#define IFC_HANGUP_PHONE_PIN        (BIT0)
#define IFC_IGNORE_CALLER_PIN       (BIT0)

// Port PB interface pins
#define IFC_MOTOR_CONTROL_PIN6      (BIT6)
#define IFC_MOTOR_ON_OFF_PIN5       (BIT5)

S_SPIFLASH_HANDLER g_sSpiFlash;

int UnpackBin(uint32_t lpbyFlashAddr, char *lppbyModel[], int nMaxNumModel, int *lpnFlashSize);

__IO uint16_t tMotorCounter = 0, tTPCounter = 0;
__IO BOOL Motor_control_bit = FALSE, vibrator_On = FALSE, timer0_open = FALSE;
__IO BOOL Bit_On = FALSE, timer1_open = FALSE, testPoint_On = FALSE;

//extern void PlayAdpcm(uint32_t PlayStartAddr, BOOL bFromSPI);
extern int32_t DoVR_sep(uint32_t lpbyBaseModel, uint32_t lpbyModel, uint32_t bShowOnly, uint32_t nTimeOut, uint32_t nPinOn, 
                        int (*funcCheckKeyPress)(void));
extern int32_t DoVR_two_as_one(uint32_t lpbyBaseModel, uint32_t lppbyModel, uint32_t nNumModel, uint32_t bShowOnly,
                               uint32_t nTimeOut, uint32_t nPinOn, int (*funcCheckKeyPress)(void));

void Vibrate_SmartGlove( void );

UINT8 SPIFlash_Initiate(void)
{ 
  UINT16 ui16Temp;
  UINT32 ui32Temp;
  UINT32 u32Count;

  // SPI0: GPA1=SSB00, GPA2=SCLK0, GPA3=MISO0, GPA4=MOSI0 
  SYS->GPA_MFP  = 
    (SYS->GPA_MFP & (~(SYS_GPA_MFP_PA0MFP_Msk|SYS_GPA_MFP_PA1MFP_Msk|SYS_GPA_MFP_PA2MFP_Msk|SYS_GPA_MFP_PA3MFP_Msk)) )
    | (SYS_GPA_MFP_PA0MFP_SPI_MOSI0|SYS_GPA_MFP_PA1MFP_SPI_SCLK|SYS_GPA_MFP_PA2MFP_SPI_SSB0|SYS_GPA_MFP_PA3MFP_SPI_MISO0);	
	
  // Reset IP module
  CLK_EnableModuleClock(SPI0_MODULE);
  SYS_ResetModule(SPI0_RST);
  SPIFlash_Open(SPI0, SPI_SS0, SPI0_CLOCK, &g_sSpiFlash );

  // Make SPI flash leave power down mode if some where or some time had made it entring power down mode
  SPIFlash_PowerDown(&g_sSpiFlash, FALSE);
	
  // Check SPI flash is ready for accessing
  u32Count = ui32Temp = 0;
  while(u32Count!=100)
    {
      SPIFlash_Read(&g_sSpiFlash, 0, (PUINT8) &ui16Temp, 2);
      if ( ui32Temp != (UINT32)ui16Temp )
        {
          ui32Temp = (UINT32)ui16Temp;
          u32Count = 0;
        }
      else
        u32Count++;
    }

  // The following code can be remove to save code if the flash size is not necessary for this application
  SPIFlash_GetChipInfo(&g_sSpiFlash);
  if (g_sSpiFlash.u32FlashSize == 0)
    return 0;
	
  // The above code can be remove to save code if the flash size is not necessary for this application
  return 1;
}


void SetOutputPin(GPIO_T *pPort, UINT16 u16PinMask, UINT16 u8Value)
{
  if (u8Value == PIN_HIGH)
    GPIO_SET_OUT_DATA(pPort,(GPIO_GET_OUT_DATA(pPort)|u16PinMask));
  else
    GPIO_SET_OUT_DATA(pPort,(GPIO_GET_OUT_DATA(pPort)&~u16PinMask));
}


void SetOutputPin_0(GPIO_T *pPort, uint16_t u16PinMask, uint16_t u8Value)
{
  if (u8Value == PIN_HIGH)
    GPIO_SET_OUT_DATA(pPort,(GPIO_GET_OUT_DATA(pPort)|u16PinMask));
  else
    GPIO_SET_OUT_DATA(pPort,(GPIO_GET_OUT_DATA(pPort)&~u16PinMask));
}


void SYS_Init(void)
{
  /* Enable External OSC49M */
  CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);

  CLK_EnableLDO(CLK_LDOSEL_3_0V);
  CLK_EnableModuleClock(TMR0_MODULE);
	CLK_EnableModuleClock(TMR1_MODULE);
}


void InitialGPIO(void)
{
  /* configure TestPoint pin */
  GPIO_SetMode(PB, BIT0, GPIO_MODE_OUTPUT);

  /* configure TestPoint pin low */
  SetOutputPin_0(PB, BIT0, PIN_LOW);
		
  /* configure output pins */
  GPIO_SetMode(PB, IFC_MOTOR_ON_OFF_PIN5, GPIO_MODE_OUTPUT);
  GPIO_SetMode(PA, OUTPUT_PORTA_PINS_MASK, GPIO_MODE_OUTPUT);

  /* configure pin low */
  SetOutputPin(PA, OUTPUT_PORTA_PINS_MASK, PIN_LOW);
  SetOutputPin(PB, IFC_MOTOR_ON_OFF_PIN5, PIN_LOW);

  /*  Configure PB7 as Quasi-bi-direction mode and enable interrupt by both rising and falling edge trigger */
  GPIO_SetMode(PB, IFC_MOTOR_CONTROL_PIN6, GPIO_MODE_QUASI);
  GPIO_EnableInt(PB, 6, GPIO_INT_BOTH_EDGE);      // pin # is required
		
  NVIC_EnableIRQ(GPAB_IRQn);

  /* Enable interrupt de-bounce function and select de-bounce sampling cycle time */
  GPIO_SET_DEBOUNCE_TIME(GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_1);
  GPIO_ENABLE_DEBOUNCE(PB, IFC_MOTOR_CONTROL_PIN6);

  /* init output pins */
}


void GPAB_IRQHandler(void)
{
  static uint8_t fixed_an_issue = 0;
	
  if (PB->INTSRC & IFC_MOTOR_CONTROL_PIN6)
    {
      PB->INTSRC = IFC_MOTOR_CONTROL_PIN6;
			
#if FIXED_ISSUE
#warning   REMOVE THIS CODE AFTER SHIPPING THE 1ST 30 PIECES
      /**
         issue: PB6 is defaulted ON, process takes affect after the PB6 goes low.
      **/
      if (fixed_an_issue < 1)
        {
          fixed_an_issue++;
          return;
        }
#endif

      if (Motor_control_bit == FALSE)
        {
          Motor_control_bit = TRUE;
          Vibrate_SmartGlove();
        }
      else
        {
          Motor_control_bit = FALSE;
          SetOutputPin(PB, IFC_MOTOR_ON_OFF_PIN5, PIN_LOW);
        }
    }
}


void ActiveTimer( uint16_t vTime )
{
  tTPCounter = (vTime == 0) ? ON_TIME : vTime;

  if (timer1_open == FALSE)
    {
      TIMER_Open( TIMER1, TIMER_PERIODIC_MODE, 1);
      SetOutputPin_0(PB, BIT0, PIN_HIGH);
      TIMER_Start(TIMER1);
      TIMER_EnableInt(TIMER1);                // Enable timer interrupt
      NVIC_EnableIRQ(TMR1_IRQn);
      timer1_open = TRUE;
    }
}


void ActiveVibrator( uint16_t vTime )
{
  tMotorCounter = (vTime == 0) ? VIBRATOR_ON_TIME : vTime;

  if (timer0_open == FALSE)
    {
      TIMER_Open( TIMER0, TIMER_PERIODIC_MODE, 1);
      SetOutputPin(PB, IFC_MOTOR_ON_OFF_PIN5, PIN_HIGH);
      TIMER_Start(TIMER0);
      TIMER_EnableInt(TIMER0);                // Enable timer interrupt
      NVIC_EnableIRQ(TMR0_IRQn);
      timer0_open = TRUE;
    }
}

void Vibrate_SmartGlove( void )
{
  if (Motor_control_bit)
    {
      if (timer0_open == FALSE)
        ActiveVibrator(VIBRATOR_ON_TIME);
      else
        {
          if (tMotorCounter == 0)
            {
              if (vibrator_On == TRUE)
                {
                  tMotorCounter = VIBRATOR_OFF_TIME;
                  vibrator_On = FALSE;
                  SetOutputPin(PB, IFC_MOTOR_ON_OFF_PIN5, PIN_LOW);
                }
              else
                {
                  vibrator_On = TRUE;
                  tMotorCounter = VIBRATOR_ON_TIME;
                  SetOutputPin(PB, IFC_MOTOR_ON_OFF_PIN5, PIN_HIGH);
                }
            }
        }
    }
}


void Toggle_Bit( void )
{
  if (tTPCounter == 0)
    {
      if (Bit_On == TRUE)
        {
          Bit_On = FALSE;
          SetOutputPin(PB, BIT0, PIN_LOW);
          tTPCounter = OFF_TIME;
        }
      else
        {
          Bit_On = TRUE;
          SetOutputPin(PB, BIT0, PIN_HIGH);
          tTPCounter = ON_TIME;
        }
    }
}


void TMR0_IRQHandler(void)
{
  if (!Motor_control_bit)
    {
      TIMER_Stop(TIMER0);
      TIMER_Close(TIMER0);			 // Close timer for next test.
      timer0_open = FALSE;
    }
  else
    {
      if (tMotorCounter)
        {
          if (--tMotorCounter == 0)
            Vibrate_SmartGlove();
        }
    }
  TIMER_ClearIntFlag(TIMER0);	
}


void TMR1_IRQHandler(void)
{
  if (tTPCounter)
    {
      if (--tTPCounter == 0)
        Toggle_Bit();
    }
  TIMER_ClearIntFlag(TIMER1);
}


void BioworldEncodeOutput (int ID)
{
  static BOOL music_playing = FALSE;
  static BOOL phone_offhook = FALSE;

  switch (ID)
    {
    case SKIP_SONG:
#if 0
      if (music_playing == TRUE)
#endif
        {
          SetOutputPin(PA, IFC_NEXT_SONG_PIN, PIN_HIGH);
          CLK_SysTickDelay(100000);                           // delay 100 ms
          SetOutputPin(PA, IFC_NEXT_SONG_PIN, PIN_LOW);
        }
      break;
		
    case GO_BACK_SONG:
#if 0
      if (music_playing == TRUE)
#endif
        {
          SetOutputPin(PA, IFC_PREVIOUS_SONG_PIN, PIN_HIGH);
          CLK_SysTickDelay(100000);
          SetOutputPin(PA, IFC_PREVIOUS_SONG_PIN, PIN_LOW);
        }
      break;
		
    case REPLAY_SONG:
#if 0
      if (music_playing == TRUE)
#endif
        {
          SetOutputPin(PA, IFC_PREVIOUS_SONG_PIN, PIN_HIGH);
          CLK_SysTickDelay(100000);
          SetOutputPin(PA, IFC_PREVIOUS_SONG_PIN, PIN_LOW);
          CLK_SysTickDelay(167000);                              // delay 167 ms
          SetOutputPin(PA, IFC_PREVIOUS_SONG_PIN, PIN_HIGH);
          CLK_SysTickDelay(100000);
          SetOutputPin(PA, IFC_PREVIOUS_SONG_PIN, PIN_LOW);
        }
      break;
		
    case PLAY_MUSIC:
    case PLAY_MUSIC_1:
    case PLAY_MUSIC_2:
    case PLAY_MUSIC_3:
#if 0
      if (music_playing == FALSE)
#endif
        {
          music_playing = TRUE;
          SetOutputPin(PA, IFC_PLAY_MUSIC_PIN, PIN_HIGH);
          CLK_SysTickDelay(100000);
          SetOutputPin(PA, IFC_PLAY_MUSIC_PIN, PIN_LOW);
        }
      break;
		
    case STOP_MUSIC:
    case STOP_MUSIC_1:
    case STOP_MUSIC_2:
    case STOP_MUSIC_3:
      SetOutputPin(PA, IFC_PAUSE_MUSIC_PIN, PIN_HIGH);
      CLK_SysTickDelay(100000);
      SetOutputPin(PA, IFC_PAUSE_MUSIC_PIN, PIN_LOW);
      music_playing = FALSE;
      break;
		
    case VOLUME_DOWN_CMD:
      SetOutputPin(PA, IFC_VOLUME_DOWN_PIN, PIN_HIGH);
      CLK_SysTickDelay(100000);
      SetOutputPin(PA, IFC_VOLUME_DOWN_PIN, PIN_LOW);
      break;
		
    case VOLUME_UP_CMD:
      SetOutputPin(PA, IFC_VOLUME_UP_PIN, PIN_HIGH);
      CLK_SysTickDelay(100000);
      SetOutputPin(PA, IFC_VOLUME_UP_PIN, PIN_LOW);
      break;
		
    case ANSWER_CALLER:
    case ANSWER_CMD:
#if 0
      if ((phone_offhook == FALSE) && (Motor_control_bit == TRUE))
#endif
        {
          phone_offhook = TRUE;
          SetOutputPin(PA, IFC_ANSWER_PHONE_PIN, PIN_HIGH);
          CLK_SysTickDelay(100000);
          SetOutputPin(PA, IFC_ANSWER_PHONE_PIN, PIN_LOW);
        }
      break;

    case DISCONNECT_CALLER:
#if 0
      if (phone_offhook == TRUE)
#endif
        {
          phone_offhook = FALSE;
          SetOutputPin(PA, IFC_HANGUP_PHONE_PIN, PIN_HIGH);
          CLK_SysTickDelay(100000);
          SetOutputPin(PA, IFC_HANGUP_PHONE_PIN, PIN_LOW);
        }
      break;

    case IGNORE_CALL:
#if 0
      if (phone_offhook == FALSE)
#endif
        {
          SetOutputPin(PA, IFC_IGNORE_CALLER_PIN, PIN_HIGH);
          CLK_SysTickDelay(167000);                                  // delay 167 ms
          SetOutputPin(PA, IFC_IGNORE_CALLER_PIN, PIN_LOW);
          CLK_SysTickDelay(167000);
          SetOutputPin(PA, IFC_IGNORE_CALLER_PIN, PIN_HIGH);
          CLK_SysTickDelay(167000);
          SetOutputPin(PA, IFC_IGNORE_CALLER_PIN, PIN_LOW);
        }
      break;
		
    default:
      break;
    }

  return;
}


int UnpackBin(uint32_t lpbyFlashAddr, char *lppbyModel[], int nMaxNumModel, int *lpnFlashSize)
{
  int *lpnBin = (int *)lpbyFlashAddr;
  int nNumBin = lpnBin[0];
  int *lpnBinSize = lpnBin + 1;
  int i;

  lppbyModel[0] = (char *)(lpnBinSize + nNumBin);
  for (i = 1; i < nNumBin; i++){
    if (i >= nMaxNumModel)
      break;
    lppbyModel[i] = lppbyModel[i-1] + lpnBinSize[i-1];
  }

  *lpnFlashSize = (uint32_t)(lppbyModel[i-1] + lpnBinSize[i-1]) - lpbyFlashAddr;
  *lpnFlashSize = ((*lpnFlashSize + 0xFFF) >> 12) << 12;	// 4KB alignment for SPI

  return i;
}


int CheckKeyPress(void)
{
  int nDetCnt=0;

  while((GPIO_GET_IN_DATA(PA)&TRIGGER_PORTB_PINS_MASK) != TRIGGER_PORTB_PINS_MASK)
    {
      nDetCnt++;
      if (nDetCnt >= 20)
        return 1;
    }

  return 0;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Main function                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
int main(void)
{
  char *lppbyModel[2];	  // 0:base, 1:command
  int nID, nBinSize;

  SYS_Unlock();
  SYS_Init();
  InitialGPIO();

#if TEST_CODE   // test code
  while (1)
    {	
      OutputPin_Set(PA, OUTPUT_PORTA_PINS_MASK, 1);                  // init pins

      OutputPin_Set(PA, OUTPUT_PORTA_PINS_MASK, 0);                  // init pins
    }
#endif


  //Must adjust parameter with model amount	
#if (NUMBER_OF_GROUPS == 1)
  if (UnpackBin((uint32_t)BIN_START_ADDR, lppbyModel, 2, &nBinSize) < 2)	   // only one group
#elif (NUMBER_OF_GROUPS == 2)
    if (UnpackBin((uint32_t)BIN_START_ADDR, lppbyModel, 3, &nBinSize) < 3)	   // Keyword, two groups 
#elif (NUMBER_OF_GROUPS == 3)
      if (UnpackBin((uint32_t)BIN_START_ADDR, lppbyModel, 4, &nBinSize) < 4)	   // three groups 
#endif
        goto L_ERROR;

  ActiveTimer(OFF_TIME);

  while(1)
    {
#if (NUMBER_OF_GROUPS == 1)
      if ((nID = DoVR_sep((uint32_t)lppbyModel[0], (uint32_t)lppbyModel[1], 0, 500/*Delay is n * 10 ms*/, 0x0, NULL)) < 0)              // only one group  5 seconds
        break;
      if (nID > 0)               // null - timed out
        {			
          BioworldEncodeOutput(nID);
        }
#else
      if ((nID = DoVR_two_as_one((uint32_t)lppbyModel[0], (uint32_t)(lppbyModel + 1), 2, 0, 0, 0x0, NULL/*CheckKeyPress*/)) <= 0)       // keyword + a group
        break;			
      if (nID > 0)               // null - timed out
        {			
          BioworldEncodeOutput(nID - NUMBER_OF_KEYWORDS);
        }
#endif
    }

 L_ERROR:
  while(1){;}
}
