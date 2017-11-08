// Timer0 set at one-shot   @ 1 sec or 1 Hz
// Timer1 set at periodic   @ 1 sec or 1 Hz
// Timer2 set at continuous @ 1/125 sec or 125 Hz
//
// Timer Mode = 0 : one-shot   (couting to the number of TCMPR, only interrupt once)
//            = 1 : periodic   (counting to the number of TCMPR, interrupt when reach TCMPR number then count from 0)
//            = 2 : toggle
//            = 3 : continuous (counting to the end of 24-bit TCMPR, interrupt when reach TCMPR number and keep counting)

#include <stdio.h>
#include "ISD9100.h"

static uint16_t Timer0Counter=0;
static uint16_t Timer1Counter=0;
static uint16_t Timer2Counter=0;

//---------------------------------------------------------------------------------TIMER
void InitTIMER0(void)
{
	/* Step 1. Enable and Select Timer clock source */          
	SYSCLK->CLKSEL1.TMR0_S = 0;	 //Select 12Mhz for Timer0 clock source 
  SYSCLK->APBCLK.TMR0_EN = 1;	 //Enable Timer0 clock source

	/* Step 2. Select Operation mode */	
	TIMER0->TCSR.MODE=0;		     //Select once mode for operation mode

	/* Step 3. Select Time out period = (Period of timer clock input) * (8-bit Prescale + 1) * (24-bit TCMP)*/
	TIMER0->TCSR.PRESCALE=255;	 // Set Prescale [0~255]
	TIMER0->TCMPR = 46875;		   // Set TCMPR [0~16777215]
								               // (1/12000000)*(255+1)* 46875 = 1 sec / 1 Hz

	/* Step 4. Enable interrupt */
	TIMER0->TCSR.IE = 1;
	TIMER0->TISR.TIF = 1;		     //Write 1 to clear for safty		
	NVIC_EnableIRQ(TMR0_IRQn);	 //Enable Timer0 Interrupt

	/* Step 5. Enable Timer module */
	TIMER0->TCSR.CRST = 1;		   //Reset up counter
	TIMER0->TCSR.CEN = 1;		     //Enable Timer0

//  	TIMER0->TCSR.TDR_EN=1;	// Enable TDR function
}

//
// Timer0 interrupt subroutine 
//
void TMR0_IRQHandler(void)
{
	Timer0Counter+=1;
 	TIMER0->TISR.TIF =1; 	   
}

