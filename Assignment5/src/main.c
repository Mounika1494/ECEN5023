/**************************************************************************//**
 * @file
 * @brief Empty Project
 * @author Energy Micro AS
 * @version 3.20.2
 ******************************************************************************
 * @section License
 * <b>(C) Copyright 2014 Silicon Labs, Error! Hyperlink reference not valid.
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
#include "dmactrl.h"
#include "em_adc.h"
#include "em_dma.h"
#include "em_int.h"
#include "em_i2c.h"
#include "i2c_TSL2561.h"
#include "em_leuart.h"
#include "Buffer.h"
/****sleep modes****/
#define EM0 0
#define EM1 1
#define EM2 2
#define EM3 3
/***switches***/
#define ENERGY_MODE EM3
#define CPU 0
#define calibration 1
#define LIGHT_ONBOARD 0
/****** LETIMER Components*****/
#define LFXOClock 32768
#define ULFRCOClock 1000
#define LED_PORT    gpioPortE
#define LED_PIN     2
#define LED_TEMP_PORT    gpioPortE
#define LED_TEMP_PIN     3
#define Period     6.25
#define OnDuty     0.004
#define LIGHT_SENSE_PORT    gpioPortC
#define LIGHT_SENSE_PIN     6
#define LIGHT_EXCITE_PORT    gpioPortD
#define LIGHT_EXCITE_PIN     6
#define Lower_Temperature 15
#define Upper_Temperature 35
#define Calibration_Temp DEVINFO->CAL
#define Mask _DEVINFO_CAL_TEMP_MASK
#define Shift _DEVINFO_CAL_TEMP_SHIFT
#define Calibration_Register DEVINFO->ADC0CAL2
#define Temp_Reading1v25 _DEVINFO_ADC0CAL2_TEMP1V25_MASK
#define Temp_shift1v25 _DEVINFO_ADC0CAL2_TEMP1V25_SHIFT
/*****array for sleep mode****/
unsigned int sleep_block_counter[EM3+1];
/****I2C****/
#define slave_addr 0x39
#define Write 0x0
#define Read 0x1
#define THLow 0x000f
#define Word_mode 0xE0
#define INT_PIN 0x0002
#define SCL_Port gpioPortC
#define SCL_Pin  5
#define SDL_Pin  4
#define Power_Pin  0
#define Interrupt_Pin  1
#define PortD  gpioPortD
#define Command_Mode 0xC0
#define ADC_Prescale 975000
#define LEUARTPort gpioPortD
#define LEUARTPin 4
int   VDD_Level=2;
double ratio=1;
int Count_LE=0;
uint8_t *z;
/****functions*******/
void CMU_Setup();
void LETIMER0_Setup();
void GPIO_Setup();
void sleep();
void blockSleepMode(unsigned int energymode);
void unblockSleepMode(unsigned int energymode);
void ACMP_Setup();
void calibrate(void);
void setupDma();
void   setupAdc();
void setupI2C(void);
void GPIOINT_Setup();
void i2c_TSL2561_Setup();
#define DMA_CHANNEL_ADC       0

/* DMA callback structure */
DMA_CB_TypeDef cb;
/* ADC Transfer Data */
#define ADCSAMPLES                        500
volatile uint16_t ramBufferAdcData[ADCSAMPLES];
#define ADCSAMPLESPERSEC              75000
int j=0;
long Sum;
uint16_t temp;
uint32_t Average;
float b,u;
int Samples_Count;
char c[5];
int f=2;
char Circ_Buffer[1000];
structure a;
float convertToCelsius(int32_t adcSample)
{
float temp;
/*Factory calibration temperature from device information page. */
float cal_temp_0 = (float)((Calibration_Temp & Mask) >> Shift);
float cal_value_0 = (float)((Calibration_Register
                              &Temp_Reading1v25)>>Temp_shift1v25);

float t_grad = -6.27;
temp = (cal_temp_0 - ((cal_value_0 - adcSample) / t_grad));
return temp;
}

/*************************************************************************//**
 * @brief  Call-back called when transfer is complete
 *****************************************************************************/
void GPIO_ODD_IRQHandler(void)
{

	//INT_Disable();
      GPIO_IntClear(INT_PIN);
      I2C1->TXDATA=((slave_addr<<1)|Write);
      I2C1->CMD =I2C_CMD_START;
      while(!(I2C1->IF & I2C_IF_ACK));
      I2C1->IFC = I2C_IFC_ACK;
      I2C1->TXDATA= Word_mode|ADC0_LOW;
      while(!(I2C1->IF & I2C_IF_ACK));
      I2C1->IFC |= I2C_IFC_ACK;
      I2C1->CMD =I2C_CMD_START;
      I2C1->TXDATA=((slave_addr<<1)|Read);
      while(!(I2C1->IF & I2C_IF_ACK));
      I2C1->IFC = I2C_IFC_ACK;
      while(!(I2C1->IF & I2C_IF_RXDATAV));
      uint16_t temp0 = I2C1->RXDATA;
      I2C1->CMD =I2C_CMD_ACK;
      while(!(I2C1->IF & I2C_IF_RXDATAV));
      uint16_t temp1 = I2C1->RXDATA;
      I2C1->CMD =I2C_CMD_NACK;
      uint16_t Lux_value= temp1<<8 | temp0;
      I2C1->CMD = I2C_CMD_STOP;
      while((I2C1->IF & I2C_IF_MSTOP)==0);
      I2C1->IFC = I2C_IFC_MSTOP;
   	    if(Lux_value<THLow){
   	    	f=1;
   	    	GPIO_PinOutSet(LED_PORT,LED_PIN);
   	    	c[0]='L';
   	    	c[1]='E';
   	    	c[2]='D';
   	    	c[3]='O';
   	    	c[4]='N';
   	    	for(int g=0;g<5;g++)
   	        {
   	          add_data(&a,c[g],1000);
   	    	}
   	    LEUART0->IEN|=LEUART_IEN_TXBL;
   	    }
   	    else
   	    {
   	    	f=1;
            if(Lux_value>THLow)
   	    	GPIO_PinOutClear(LED_PORT,LED_PIN);
            c[0]='L';
   	        c[1]='E';
   	    	c[2]='D';
   	    	c[3]='O';
   	    	c[4]='F';
   	    	for(int g=0;g<5;g++)
   	        {
   	    	   add_data(&a,c[g],1000);
   	    	}

   	    LEUART0->IEN|=LEUART_IEN_TXBL;
   	    }
}

void transferComplete(unsigned int channel, bool primary, void *user)
{
	unblockSleepMode(EM1);
	ADC0->CMD&=0x0000;
	ADC0->CMD|=0x0002;
    (void) channel;
    (void) primary;
    (void) user;
    int i;
    for (i=0;i<ADCSAMPLES;i++)
      {
         temp=ramBufferAdcData[i];
         Sum=Sum+temp;
      }
    Average=Sum/ADCSAMPLES;
    Sum = 0;
    b = convertToCelsius(Average);
    z=(uint8_t *)&b;
    for(int g=0;g<5;g++)
     {
      add_data(&a,*(z+g),1000);
     }
     f=0;

     blockSleepMode(EM2);
     LEUART0->IEN|=LEUART_IEN_TXBL;
     NVIC_EnableIRQ(LEUART0_IRQn);

     if(i==ADCSAMPLES)
      {
         if(b>Lower_Temperature&& b<Upper_Temperature)
            {
         	    GPIO_PinOutClear(LED_TEMP_PORT, LED_TEMP_PIN);
            }
         else if (b<Lower_Temperature||b>Upper_Temperature)
           {
         	    GPIO_PinOutSet(LED_TEMP_PORT, LED_TEMP_PIN);
           }
       }
}
/***** enabling the clocks required*****/
void CMU_Setup()
{
	/**** enable a different clock for EM3 Mode******/
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
	    CMU_ClockEnable(cmuClock_LEUART0, true);
	    if(LIGHT_ONBOARD==1)
	    {
	    CMU_ClockEnable(cmuClock_ACMP0, false);
	    }
	    else if(LIGHT_ONBOARD==0)
	    {
	    	CMU_ClockEnable(cmuClock_CORELE, true);
	    	CMU_ClockEnable(cmuClock_HFPER,true);
	    	CMU_ClockEnable(cmuClock_I2C1, true);
	    }
	    if(CPU==0)
	    {
	        CMU_ClockEnable(cmuClock_DMA, true);
	    }
	        CMU_ClockEnable(cmuClock_ADC0, true);

}
void LETIMER0_Setup(void)
{
	float b;
	unsigned int Comp0;
	unsigned int Comp1;
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
	{
		b = OnDuty*ULFRCOClock*ratio;

	if(b > (int)b)
	{
		Comp1=b+1;
	}
	}
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
   // unblockSleepMode(EM1);
    NVIC_EnableIRQ(LETIMER0_IRQn);//adding these interrupts to nested interrupts

}
/****enabling clock and interrupt for GPIO pins***/
void GPIO_Setup()
{
	CMU_ClockEnable(cmuClock_GPIO, true);
	GPIO_PinModeSet(LED_PORT, LED_PIN, gpioModePushPull,0);
	GPIO_PinModeSet(LED_TEMP_PORT, LED_TEMP_PIN, gpioModePushPull,0);
	 /* Enable GPIO for LEUART0. TX is on D4 */
	  GPIO_PinModeSet(LEUARTPort,                /* GPIO port */
			  LEUARTPin,                        /* GPIO port number */
	                  gpioModePushPull,         /* Pin mode is set to push pull */
	                  1);
	if(LIGHT_ONBOARD==1)
	{
	GPIO_PinModeSet(LIGHT_EXCITE_PORT, LIGHT_EXCITE_PIN,gpioModePushPull,0);
	GPIO_PinModeSet(LIGHT_SENSE_PORT, LIGHT_SENSE_PIN,gpioModeDisabled,0);
	}
	if(LIGHT_ONBOARD==0)
	{
	GPIO_PinModeSet(PortD,Power_Pin, gpioModePushPull,0);
}
}
void ACMP_Setup()

{
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
  while (!(ACMP0->STATUS & ACMP_STATUS_ACMPACT));
     }
/****LETMER Handler****/
void LETIMER0_IRQHandler(void)
{
//	INT_Disable();
	int intFlags;
	intFlags = LETIMER0->IF;//the value of interrupt flag
	if((intFlags&LETIMER_IF_COMP1)!=0)//as comp1 flag will be set the value will not be equal 0
	{
		if(LIGHT_ONBOARD==1)
		{
		GPIO_PinOutSet(LIGHT_EXCITE_PORT, LIGHT_EXCITE_PIN);
		}
		LETIMER_IntClear(LETIMER0, LETIMER_IF_COMP1);//clear the interrupt flag
	}
	if((intFlags&LETIMER_IF_UF)!=0)// as uf flag will be set the value will not be equal to 0
	{
		LETIMER_IntClear(LETIMER0, LETIMER_IF_UF);//clear the interrupt flag
        if(LIGHT_ONBOARD==1)
        {
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
     else if(LIGHT_ONBOARD==0)
     {
     	if(Count_LE==1)
	    {
		//blockSleepMode(EM1);
		GPIO_PinModeSet(SCL_Port, SCL_Pin, gpioModeWiredAndPullUpFilter, 1);
	    GPIO_PinModeSet(SCL_Port, SDL_Pin, gpioModeWiredAndPullUpFilter, 1);
	    i2c_TSL2561_Setup();
	    Count_LE++;
	    }
	else if(Count_LE==2)//monitoring the interrupts
	{
		Count_LE++;

	}
	else if(Count_LE==3)//disabling the interrupt
	{
		GPIO->IEN=0x0;
		GPIO_PinModeSet(SCL_Port, SCL_Pin, gpioModeDisabled, 0);
		GPIO_PinModeSet(SCL_Port, SDL_Pin, gpioModeDisabled, 0);
		GPIO_PinModeSet(PortD, Interrupt_Pin , gpioModeDisabled, 0);
		GPIO_PinOutClear(PortD,Power_Pin);
		Count_LE=1;
		unblockSleepMode(EM1);
		blockSleepMode(EM3);
		GPIO->IEN=0x0;


	}
	else if(Count_LE==0)//for the first time
	{
		Count_LE++;

	}
	}
    setupAdc();
    blockSleepMode(EM1);
    if(CPU==0)
    {
	   DMA_ActivateBasic(DMA_CHANNEL_ADC,
		                 true,
		                 false,
		                 (void *)ramBufferAdcData,
		                 (void *)&(ADC0->SINGLEDATA),
		                 ADCSAMPLES - 1);
    }
    if(CPU==1)
    {
		ADC_IntEnable(ADC0,ADC_IEN_SINGLE);
		NVIC_EnableIRQ(ADC0_IRQn);
    }
	ADC_Start(ADC0, adcStartSingle);

	}
}

/** This code is originally Silicon Labs Copyright grants permission to everyone to use the software for
 * any purpose including commercial applications and to alter it and redistribute it freely is not miss represented
 * altered source version must be plainly marked and this notice cannot be altered or removed from any source distribution
 * Name of routines include
 * void blockSleepMode(unsigned int energymode)
 * void unblocksleepMode(unsigned int energymode)
 * void sleep(void)
 * float convertToCelsius(int32_t adcSample)
 *Some idea of the program may be taken from lectures of the professor
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
	INT_Disable();
	sleep_block_counter[energymode]++;
	INT_Enable();
}/****unblocking the modes which were blocked before so that other peripherals can go that energy modes**/
void unblockSleepMode(unsigned int energymode)
{
	INT_Disable();
	if(sleep_block_counter[energymode]>0)
	sleep_block_counter[energymode]--;
	else
		sleep_block_counter[energymode]=0;
	INT_Enable();
}
void calibrate(void)
{
	INT_Disable();
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

        //Save 32 bit ULFRCO count value in after cascading timer 0 and imer 1
        ULFRCOCOUNT=((timer1_count << 16)|(timer0_count));

      ratio=(double)LFXOCOUNT/ULFRCOCOUNT;
      INT_Enable();
}
void setupAdc(void)
{
  ADC_Init_TypeDef        adcInit       = ADC_INIT_DEFAULT;
  ADC_InitSingle_TypeDef  adcInitSingle = ADC_INITSINGLE_DEFAULT;
  adcInit.prescale = ADC_PrescaleCalc(ADC_Prescale, 0); /* Set prescaler for sampling of 10,000 samples per second */
  ADC_Init(ADC0, &adcInit);

  adcInitSingle.input     =  adcSingleInputTemp;  /* Reference */
  adcInitSingle.leftAdjust = false;
  adcInitSingle.rep=true;
  adcInitSingle.prsEnable = true;
  adcInitSingle.prsSel    = adcPRSSELCh0;           /* Triggered by PRS CH0 */
  ADC_InitSingle(ADC0, &adcInitSingle);
}
void setupDma(void)
{
  DMA_Init_TypeDef        dmaInit;
  DMA_CfgChannel_TypeDef  chnlCfg;
  DMA_CfgDescr_TypeDef    descrCfg;

  /* Initializing the DMA */
  dmaInit.hprot        = 0;
  dmaInit.controlBlock = dmaControlBlock;
  DMA_Init(&dmaInit);

  /* Setting up call-back function */
  cb.cbFunc  = transferComplete;
  cb.userPtr = NULL;

  /* Setting up channel */
  chnlCfg.highPri   = false;
  chnlCfg.enableInt = true;
  chnlCfg.select    = DMAREQ_ADC0_SINGLE;
  chnlCfg.cb        = &cb;
  DMA_CfgChannel(DMA_CHANNEL_ADC, &chnlCfg);

  /* Setting up channel descriptor */
  descrCfg.dstInc  = dmaDataInc2;
  descrCfg.srcInc  = dmaDataIncNone;
  descrCfg.size    = dmaDataSize2;
  descrCfg.arbRate = dmaArbitrate1;
  descrCfg.hprot   = 0;
  DMA_CfgDescr(DMA_CHANNEL_ADC, true, &descrCfg);
}
/****when DMA is not there****/
void ADC0_IRQHandler(void)
{
//INT_Disable();

	if(Samples_Count<ADCSAMPLES)
	{
	ramBufferAdcData[Samples_Count]=ADC0->SINGLEDATA;
	}
	else
	{
		ADC0->CMD&=0x0000;
		ADC0->CMD|=0x0002;
		unblockSleepMode(EM1);

    for (int j=0;j<ADCSAMPLES;j++)
          	{
          		temp=ramBufferAdcData[j];
          		Sum=Sum+temp;
          	}
          	Average=Sum/ADCSAMPLES;
          	Sum = 0;
          	  b = convertToCelsius(Average);
          	z=(uint8_t *)&b;

          	 		          	f=0;

          	 		         blockSleepMode(EM2);
          	 		          	 	 NVIC_EnableIRQ(LEUART0_IRQn);

          	 if(b>Lower_Temperature&& b<Upper_Temperature)
          	          		  {
          	          			 GPIO_PinOutClear(LED_TEMP_PORT, LED_TEMP_PIN);
          	          		  }
          	          		  else if (b<Lower_Temperature||b>Upper_Temperature)
          	          		  {
          	          			 GPIO_PinOutSet(LED_TEMP_PORT, LED_TEMP_PIN);
          	          		  }
          	   Samples_Count=0;

	}
	Samples_Count++;
	ADC_IntClear(ADC0, ADC_IFC_SINGLE);
}
/**writing values  into the Sensor registers**/
void i2c_Reg(uint8_t addr_reg,uint8_t reg_value)
{
	    uint8_t Addr;
		uint8_t value_reg = (Command_Mode|addr_reg);
		Addr =(slave_addr<<1)|Write;
		I2C1->TXDATA=Addr;
		for(int i=0;i<300;i++);
		I2C1->CMD = I2C_CMD_START;
		I2C1->IFC = I2C_IFC_START;
		while(!(I2C1->IF & I2C_IF_ACK));
		I2C1->IFC = I2C_IFC_ACK;
		I2C1->TXDATA = value_reg;
		while(!(I2C1->IF & I2C_IF_ACK));
		I2C1->IFC = I2C_IFC_ACK;
		I2C1->TXDATA = reg_value;
		while(!(I2C1->IF & I2C_IF_ACK));
		I2C1->IFC = I2C_IFC_ACK;
		I2C1->CMD = I2C_CMD_STOP;
		while((I2C1->IF & I2C_IF_MSTOP)==0);
		I2C1->IFC = I2C_IFC_MSTOP;
}
void i2c_TSL2561_Setup()/**sensor setup***/
{
       // GPIOINT_Setup();
	uint8_t INT=(INT_Reg<<4)|PERSIST;
	GPIO_PinOutSet(PortD,Power_Pin);
	for(int i=0;i<300;i++);
	i2c_Reg(I2C_CTRL,Power_up);
	i2c_Reg(I2C_TIMING,TIMING_Value);
	i2c_Reg(I2C_THL0,THL0);
	i2c_Reg(I2C_THL1,THL1);
	i2c_Reg(I2C_THH0,THH0);
	i2c_Reg(I2C_THH1,THH1);
	i2c_Reg(I2C_INT,INT);
	GPIOINT_Setup();
	 }
/**** enabling gpio interrupt***/
void GPIOINT_Setup()
{
	 GPIO_PinModeSet(PortD, Interrupt_Pin , gpioModeInput, 1);
	 GPIO_ExtIntConfig(PortD, Interrupt_Pin , 1,false, true, true);
	 NVIC_EnableIRQ(GPIO_ODD_IRQn);

}
/*****setup system I2c****/
void setupI2C(void)
{
	I2C_Init_TypeDef i2cInit =
		{
	     .enable   = true,
		 .master   = true,
		 .refFreq  = 0,
		 .freq     = I2C_FREQ_STANDARD_MAX,
		 .clhr     = i2cClockHLRStandard,
		};
	GPIO_PinModeSet(SCL_Port, SCL_Pin, gpioModeWiredAndPullUpFilter, 1);
	GPIO_PinModeSet(SCL_Port, SDL_Pin, gpioModeWiredAndPullUpFilter, 1);
	I2C1->ROUTE = I2C_ROUTE_SDAPEN |
	                  I2C_ROUTE_SCLPEN |
	                  (0 << _I2C_ROUTE_LOCATION_SHIFT);
	I2C_Init(I2C1, &i2cInit);
	if(I2C1->STATE&I2C_STATE_BUSY)
	{
		I2C1->CMD=I2C_CMD_ABORT;
	}
	for (int i = 0; i < 9; i++)
	  {
	     GPIO_PinModeSet(SCL_Port, SCL_Pin, gpioModeWiredAnd, 0);
	     GPIO_PinModeSet(SCL_Port, SCL_Pin, gpioModeWiredAnd, 1);
	  }

	  /***enable interrupts***/
	 I2C1->IEN|=I2C_IEN_ACK;
	 I2C1->IEN|=I2C_IEN_NACK;
	 I2C1->IEN|=I2C_IEN_RXDATAV;
	 for(int i=0;i<10;i++);
}
void initLeuart(void)
{
   CMU_ClockSelectSet(cmuClock_LFB, cmuSelect_LFXO);
   /* Reseting and initializing LEUART1 */
   LEUART_Init_TypeDef leuart1Init =
	{
	  //.enable   = leuartEnableTx,       /* Activate data reception on LEUn_TX pin. */
	  .refFreq  = 0,                    /* Inherit the clock frequenzy from the LEUART clock source */
	  .baudrate = 9600,                 /* Baudrate = 9600 bps */
	  .databits = leuartDatabits8,      /* Each LEUART frame containes 8 databits */
	  .parity   = leuartNoParity,       /* No parity bits in use */
	  .stopbits = leuartStopbits1,      /* Setting the number of stop bits in a frame to 1 bitperiods */
	};

    LEUART_Reset(LEUART0);
    LEUART_Init(LEUART0, &leuart1Init);
    LEUART0->ROUTE = LEUART_ROUTE_TXPEN | _LEUART_ROUTE_LOCATION_LOC0;
    LEUART0->CMD|=LEUART_CMD_TXEN;
    LEUART0->IEN|=LEUART_IEN_TXBL;

}
void LEUART0_IRQHandler()
{
	INT_Disable();
	LEUART0->IEN=0X00000000;
	int result;
	while(1)
	{
		result=get_data(&a,1000);
		if(result==255)
			break;
		LEUART_Tx(LEUART0,result);
	}

		INT_Enable();
}
/****main****/
int main(void)
{
    /* Align different chip revisions */
     CHIP_Init();
     a.Buffer=Circ_Buffer;
     a.HEAD=Circ_Buffer;
     a.TAIL=Circ_Buffer;
     if(calibration==1)
     {
	   calibrate();
     }
    CMU_Setup();
    GPIO_Setup();
    if(LIGHT_ONBOARD==1)
    {
        ACMP_Setup();
    }
    else if(LIGHT_ONBOARD==0)
    {
    	setupI2C();
    }
    if(CPU==0)
    {
	    setupDma();
    }
    initLeuart();
    LETIMER0_Setup();

    while(1)
      {
        	sleep();
      }
}
