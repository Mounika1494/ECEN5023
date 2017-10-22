/**************************************************************************//**
 * @file
 * @brief Empty Project
 * @author Energy Micro AS
 * @version 3.20.2
 ******************************************************************************
 * @section License
 * <b>(C) Copyright 2014 Silicon Labs, http://www.silabs.com</b>
 *******************************************************************************
 *
 * This file is licensed under the Silicon Labs Software License Agreement. See
 * "http://developer.silabs.com/legal/version/v11/Silicon_Labs_Software_License_Agreement.txt"
 * for details. Before using this software for any purpose, you must agree to the
 * terms of that agreement.
 *
 ******************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "em_device.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_letimer.h"
#include "em_timer.h"
#include "em_gpio.h"
#include "em_chip.h"
#include "em_acmp.h"
#include "em_int.h"
/****sleep modes****/
#define EM0 0
#define EM1 1
#define EM2 2
#define EM3 3
#define ENERGY_MODE EM2
/****** LETIMER Components*****/
#define LFXOClock 32768
#define ULFRCOClock 1000
#define LED_PORT    gpioPortE
#define LED_PIN     2
#define Period     3
#define OnDuty     0.004
#define LIGHT_SENSE_PORT    gpioPortC
#define LIGHT_SENSE_PIN     6
#define LIGHT_EXCITE_PORT    gpioPortD
#define LIGHT_EXCITE_PIN     6
#define calibration 1
/*****array for sleep mode****/
unsigned int sleep_block_counter[EM3+1];
int   VDD_Level=2;
double ratio=1;
/****functions*******/
void CMU_Setup();
void LETIMER0_Setup();
void GPIO_Setup();
void sleep();
void blockSleepMode(unsigned int energymode);
void unblockSleepMode(unsigned int energymode);
void ACMP_Setup();
void calibrate(void);
/****main****/
int main(void)
{
    /* Align different chip revisions */
     CHIP_Init();
     if(calibration==1)
     {
	calibrate();
     }
    CMU_Setup();
    GPIO_Setup();
    ACMP_Setup();
    LETIMER0_Setup();
    while(1)
    {
    	sleep();
    }
}

/***** enabling the clocks required*****/
void CMU_Setup()
{
	/**** enable a different clock for EM3 Mode******/
	//CMU_OscillatorEnable(cmuOsc_LFXO,true,true);
    //CMU_ClockEnable(cmuClock_CORELE, true);
	    if(ENERGY_MODE==EM3)
	    {
	    	CMU_ClockSelectSet(cmuClock_LFA,cmuSelect_ULFRCO);
	    CMU_OscillatorEnable(cmuOsc_LFXO,true,true);
	    }
	    else
	    {
	    CMU_ClockSelectSet(cmuClock_LFA,cmuSelect_LFXO);
	    }

	    CMU_ClockEnable(cmuClock_LETIMER0, false);
	    CMU_ClockEnable(cmuClock_ACMP0, false);
}

void LETIMER0_Setup(void)
{
	//set le timer
	unsigned int Comp0,Comp1;
	unsigned int LETIMER0_prescaler;
	if(ENERGY_MODE==EM3)
		Comp0 = Period*ULFRCOClock*ratio;
	else
	{
		LETIMER0_prescaler= Period/2;
		CMU->LFAPRESC0&= 0xfffff0ff;//clearing the prescaler register
		CMU->LFAPRESC0|= LETIMER0_prescaler << 8;//shift prescaler into position
		LETIMER0_prescaler = 1 << LETIMER0_prescaler ;//the value of prescaler register
		Comp0=Period*(LFXOClock/LETIMER0_prescaler);// moving the final value to Comp0

	}
	if(ENERGY_MODE==EM3)
		Comp1 = OnDuty*ULFRCOClock*ratio;
		else
		{
			Comp1=OnDuty*(LFXOClock/LETIMER0_prescaler);
		}

    /* Set initial compare values for COMP0 and Comp1 */
    LETIMER_CompareSet(LETIMER0, 0,Comp0);
    LETIMER_CompareSet(LETIMER0,1,Comp1);
    /* Set configurations for LETIMER 0 */
    const LETIMER_Init_TypeDef letimerInit =
    {
        .enable         = true,                   /* start counting when init completed - only with RTC compare match */
        .debugRun       = false,                  /* Counter shall not keep running during debug halt. */
        .rtcComp0Enable = false,                  /* DON'T Start counting on RTC COMP0 match. */
        .rtcComp1Enable = false,                  /* Don't start counting on RTC COMP1 match. */
        .comp0Top       = true,                  /* Load COMP0 register into CNT when counter underflows. COMP is used as TOP */
        .bufTop         = false,                  /* Don't load COMP1 into COMP0 when REP0 reaches 0. */
        .out0Pol        = 0,                      /* Idle value for output 0. */
        .out1Pol        = 0,                      /* Idle value for output 1. */
        .ufoa0          = letimerUFOANone,        /* Pulse output on output 0 */
        .ufoa1          = letimerUFOANone,        /* No output on output 1*/
        .repMode        = letimerRepeatFree,      /*Free mode*/
    };

    /* Initialize LETIMER */
    CMU_ClockEnable(cmuClock_LETIMER0, true);
    LETIMER_Init(LETIMER0, &letimerInit);
   /*****enabling interrupts for COmP1 and UF***/
    LETIMER_IntEnable(LETIMER0,LETIMER_IF_COMP1);
    LETIMER_IntEnable(LETIMER0, LETIMER_IF_UF);
    blockSleepMode(ENERGY_MODE);
    NVIC_EnableIRQ(LETIMER0_IRQn);//adding these interrupts to nested interrupts

}

/****enabling clock and interrupt for GPIO pins***/
void GPIO_Setup()
{
	CMU_ClockEnable(cmuClock_GPIO, true);
	GPIO_PinModeSet(LED_PORT, LED_PIN, gpioModePushPull,0);
	GPIO_PinModeSet(LIGHT_EXCITE_PORT, LIGHT_EXCITE_PIN,gpioModePushPull,0);
	GPIO_PinModeSet(LIGHT_SENSE_PORT, LIGHT_SENSE_PIN,gpioModeDisabled,0);
}

void ACMP_Setup()
{
  blockSleepMode(EM1);
  const ACMP_Init_TypeDef ACMP0_init =
  {
    false,                              /* Full bias current*/
    false,                              /* Half bias current */
    7,                                  /* Biasprog current configuration */
    true,                               /* Enable interrupt for falling edge */
    true,                               /* Enable interrupt for rising edge */
    acmpWarmTime512,                    /* Warm-up time in clock cycles, >140 cycles for 10us with 14MHz */
    acmpHysteresisLevel1,               /* Hysteresis configuration */
    0,                                  /* Inactive comparator output value */
    false,                              /* Enable low power mode */
    VDD_Level,                               /* Vdd reference scaling */
    true,                               /* Enable ACMP */
  };
  CMU_ClockEnable(cmuClock_ACMP0, true);
  /* Init and set ACMP channels */
  ACMP_Init(ACMP0, &ACMP0_init);
  ACMP_ChannelSet(ACMP0,acmpChannelVDD,acmpChannel6);
  unblockSleepMode(EM1);
  while (!(ACMP0->STATUS & ACMP_STATUS_ACMPACT));
  blockSleepMode(EM3);
     }
/****LETMER Handler****/
void LETIMER0_IRQHandler(void)
{
	INT_Disable();
	int intFlags;
	intFlags = LETIMER0->IF;//the value of interrupt flag
	if((intFlags&LETIMER_IF_COMP1)!=0)//as comp1 flag will be set the value will not be equal 0
	{
		GPIO_PinOutSet(LIGHT_EXCITE_PORT, LIGHT_EXCITE_PIN);
	LETIMER_IntClear(LETIMER0, LETIMER_IF_COMP1);//clear the interrupt flag
	}
	if((intFlags&LETIMER_IF_UF)!=0)// as uf flag will be set the value will not be equal to 0
	{

		LETIMER_IntClear(LETIMER0, LETIMER_IF_UF);//clear the interrupt flag
		                                         if(ACMP0->STATUS == 0x03)
		                		 					{
		                		 					if(VDD_Level==61)
		                		 					{
		                		 						GPIO_PinOutClear(LED_PORT,LED_PIN);
		                		 						ACMP_Disable(ACMP0);

		                		 						VDD_Level = 2;

		                		 						ACMP_Setup();
		                		 					    }
		                		 					}
		                		 				else
		                		 						{
		                		 						if(VDD_Level== 2)
		                		 										{
		                		 							GPIO_PinOutSet(LED_PORT,LED_PIN);

		                		 								        ACMP_Disable(ACMP0);
		                		 								      VDD_Level = 61;
		                		 								     ACMP_Setup();
		                		 										}
		                		 					}
		                                         GPIO_PinOutClear(LIGHT_EXCITE_PORT, LIGHT_EXCITE_PIN);

		                		 }
INT_Enable();
			}

/** This code is originally Silicon Labs Copyright grants permission to everyone to use the software for
 * any purpose including commercial applications and to alter it and redistribute it freely is not miss represented
 * altered source version must be plainly marked and this notice cannot be altered or removed from any source distribution
 * Name of routines include
 * void blockSleepMode(unsigned int energymode)
 * void unblocksleepMode(unsigned int energymode)
 * void sleep(void)
 * and may be taken from lectures of the professor
 */
/**** for making the cpu based on the energy mode*****/
void sleep(void)
{
	if(sleep_block_counter[EM0]>0){}
	else if(sleep_block_counter[EM1]>0)
		EMU_EnterEM1();
	else if(sleep_block_counter[EM2]>0)
		EMU_EnterEM2(true);
	else if(sleep_block_counter[EM3]>0)
		EMU_EnterEM3(true);
}
/**** blocking some modes so that cpu does'nt go below the energy mode given****/
void blockSleepMode(unsigned int energymode)
{
	sleep_block_counter[energymode]++;
}/****unblocking the modes which were blocked before so that other peripherals can go that energy modes**/
void unblockSleepMode(unsigned int energymode)
{
	if(sleep_block_counter[energymode]>0)
	sleep_block_counter[energymode]--;
	else
		sleep_block_counter[energymode]=0;
}
void calibrate(void)
{
	int Flags;
	uint32_t timer1_count;
	uint32_t timer0_count;

	uint32_t LFXOCOUNT=0X0000;
	uint32_t ULFRCOCOUNT=0X0000;

        CMU_OscillatorEnable(cmuOsc_LFXO,true,true);

        CMU_ClockEnable(cmuClock_CORELE, true); /* Enable CORELE clock */
        CMU_ClockSelectSet(cmuClock_LFA,cmuSelect_LFXO); /* Select LFXO as clock source for LFA for EM0 to EM2 */


        /* Select TIMER0 parameters */
        TIMER_Init_TypeDef timer0Init =
         {
		    .enable     = false,
		    .debugRun   = true,
		    .prescale   = timerPrescale1,
		    .clkSel     = timerClkSelHFPerClk,
		    .fallAction = timerInputActionNone,
		    .riseAction = timerInputActionNone,
		    .mode       = timerModeUp,
		    .dmaClrAct  = false,
		    .quadModeX4 = false,
		    .oneShot    = false,
		    .sync       = false,
        };

	/* Select TIMER1 parameters */
        TIMER_Init_TypeDef timer1Init =
        {
		  	    .enable     = false,
		  	    .debugRun   = true,
		  	    .prescale   = timerPrescale1,
		  	    .clkSel     = timerClkSelCascade,
		  	    .fallAction = timerInputActionNone,
		  	    .riseAction = timerInputActionNone,
		  	    .mode       = timerModeUp,
		  	    .dmaClrAct  = false,
		  	    .quadModeX4 = false,
		  	    .oneShot    = false,
		  	    .sync       = true,
        };

	  //Clear all timer0 and timer1 interrupts
	  int  IntFlags1=TIMER1->IF;
           TIMER1->IFC=IntFlags1;
          int  IntFlags0=TIMER0->IF;
            TIMER0->IFC=IntFlags0;

           /* Enable TIMER0 and TIMER1 interrupt vector in NVIC */
	    NVIC_EnableIRQ(TIMER0_IRQn);
	    NVIC_EnableIRQ(TIMER1_IRQn);
 
       	   /* Set configurations for LETIMER 0 */
	   const LETIMER_Init_TypeDef letimerInit =
	    {
		.enable = false, /* Start counting when init completed*/
		.debugRun = false, /* Counter shall not keep running during debug halt. */
		.rtcComp0Enable = false, /* Start counting on RTC COMP0 match. */
		.rtcComp1Enable = false, /* Don't start counting on RTC COMP1 match. */
		.comp0Top = true, /* Load COMP0 register into CNT when counter underflows. COMP is used as TOP */
		.bufTop = false, /* Don't load COMP1 into COMP0 when REP0 reaches 0. */
		.out0Pol = 0, /* Idle value for output 0. */
		.out1Pol = 0, /* Idle value for output 1. */
		.ufoa0 = letimerUFOANone, /* PwM output on output 0 */
		.ufoa1 = letimerUFOANone, /* No output on output 1*/
		.repMode = letimerRepeatFree /* Count while REP != 0 */
		};

       //set LED on time
		LETIMER0->CNT=32769; // set count for 1s
		TIMER0->CNT=0x0000;  //Set Timer0 initial count to 0
		TIMER1->CNT=0x0000;  //Set Timer1 initial count to 0



		//Clear all interrupts
		        Flags=LETIMER0->IF;
		        LETIMER0->IFC=Flags;


          /* Enable LETIMER0 interrupt vector in NVIC*/
		NVIC_EnableIRQ(LETIMER0_IRQn);

		//Enable clock for timers
		CMU_ClockEnable(cmuClock_LETIMER0, true); /*Enable clock for LETIMER0*/
		CMU_ClockEnable(cmuClock_TIMER0, true); /*Enable clock for TIMER0*/
		CMU_ClockEnable(cmuClock_TIMER1, true); /*Enable clock for TIMER0*/

	   /* Configure TIMER0,TIMER1 and LETIMER0 */
		LETIMER_Init(LETIMER0, &letimerInit);
		TIMER_Init(TIMER0, &timer0Init);
		TIMER_Init(TIMER1, &timer1Init);

		//enable timers to start count
                LETIMER_Enable(LETIMER0, true);
                TIMER_Enable(TIMER0, true);
                TIMER_Enable(TIMER1, true);

        while(LETIMER0->CNT!=0x0000);

        //Disable timers when LETIMER0 underflows
        LETIMER_Enable(LETIMER0, false);
        TIMER_Enable(TIMER0, false);
        TIMER_Enable(TIMER1, false);

        //Load timer 0 and 1 counts
        timer1_count=TIMER1->CNT;
        timer0_count=TIMER0->CNT;

        LFXOCOUNT=((timer1_count << 16)|(timer0_count));

        TIMER0->CNT=0x000;
        TIMER1->CNT=0x000;

        //Set necessary clocks for ULFRCO
          CMU_ClockSelectSet(cmuClock_LFA,cmuSelect_ULFRCO);
          CMU_OscillatorEnable(cmuOsc_LFXO,false,false);

          //Enable clock for timers
          CMU_ClockEnable(cmuClock_LETIMER0, true); /*Enable LETIMER0*/
          CMU_ClockEnable(cmuClock_TIMER0, true); /*Enable  TIMER0*/
          CMU_ClockEnable(cmuClock_TIMER1, true); /*Enable TIMER1*/

       LETIMER_Init(LETIMER0, &letimerInit);
       LETIMER0->CNT=1000;                   //count for 1s in ULFRCO

       LETIMER_Enable(LETIMER0, true);


       TIMER_Enable(TIMER1, true);
       TIMER_Enable(TIMER0, true);

       while(LETIMER0->CNT!=0x0000);


       TIMER_Enable(TIMER1, false);
       TIMER_Enable(TIMER0, false);

       LETIMER_Enable(LETIMER0, false);

        timer1_count=TIMER1->CNT;
        timer0_count=TIMER0->CNT;
       //LETIMER_Enable(LETIMER0, true);

        //Save 32 bit ULFRCO count value in after cascading timer 0 and imer 1
        ULFRCOCOUNT=((timer1_count << 16)|(timer0_count));

      ratio=(double)LFXOCOUNT/ULFRCOCOUNT;
}
