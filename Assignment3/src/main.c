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
#include "em_device.h"
#include "em_chip.h"
#include "em_i2c.h"
#include "i2c_TSL2561.h"
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
void CMU_Setup();
void setupI2C(void);
void GPIO_Setup();
void GPIOINT_Setup();
void i2c_TSL2561_Setup();
/****sleep modes****/
#define EM0 0
#define EM1 1
#define EM2 2
#define EM3 3
//#define I2C1_CMD_START 0x1
#define I2C1_IF_ACK 0x40
#define I2C1_IF_NACK 0x80
#define I2C1_IFC_ACK 0x40
#define I2C1_IFC_NACK 0x80
#define I2C1_IEN_ACK 0x40
#define I2C1_IEN_NACK 0x80
#define slave_addr 0x39
#define Write 0x0
#define Read 0x1
#define LED_PIN 2
#define LED_PORT gpioPortE
#define THLow 0x000f
#define Word_mode 0xA0
#define INT_PIN 0x0200
#define SCL_Port gpioPortC
#define SCL_Pin  5
#define SDL_Pin  4
#define Power_Pin  0
#define Interrupt_Pin  1
#define PortD  gpioPortD
/**************************************************************************//**
 * @brief  Main function
 *****************************************************************************/
void GPIO_ODD_IRQHandler(void)
{
    GPIO_IntClear(INT_PIN);
    I2C1->TXDATA=((slave_addr<<1)|Write);
      I2C1->CMD =I2C_CMD_START;
      while(!(I2C1->IF & I2C_IF_ACK));
      I2C1->IFC = I2C_IFC_ACK;
      I2C1->TXDATA= Word_mode|ADC0_LOW;
      while(!(I2C1->IF & I2C_IF_ACK));
      I2C1->IFC = I2C_IFC_ACK;
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
   	    if(Lux_value<THLow)
   	    	GPIO_PinOutSet(LED_PORT,LED_PIN);
   	    else

   	    	GPIO_PinOutClear(LED_PORT,LED_PIN);

}
int main(void)
{
  /* Chip errata */
  CHIP_Init();
CMU_Setup();
setupI2C();
  GPIO_Setup();
//GPIOINT_Setup();
  i2c_TSL2561_Setup();
}

void CMU_Setup()
{
	CMU_ClockEnable(cmuClock_I2C1, true);
	  CMU_ClockEnable(cmuClock_GPIO, true);
	  CMU_ClockEnable(cmuClock_CORELE, true);
	 // CMU_OscillatorEnable(cmuOsc_LFXO, true, true);
	CMU_ClockEnable(cmuClock_HFPER,true);
}
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
for(int i=0;i<10;i++)
{

}
	  //blockSleepMode(EM1);
	// m  NVIC_EnableIRQ(I2C1_IRQn);
	  }
void i2c_Reg(uint8_t addr_reg,uint8_t reg_value)
{
	uint8_t Addr;
		uint8_t value_reg = (0x80|addr_reg);
		Addr =(slave_addr<<1)|Write;
		I2C1->TXDATA=Addr;
		for(int i=0;i<300;i++)
			{

			}
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
void i2c_TSL2561_Setup()
{
       // GPIOINT_Setup();
	uint8_t INT=(INT_Reg<<4)|PERSIST;
	GPIO_PinOutSet(PortD,Power_Pin);
	for(int i=0;i<300;i++)
	{

	}
	i2c_Reg(I2C_CTRL,Power_up);
	i2c_Reg(I2C_TIMING,TIMING_Value);
	i2c_Reg(I2C_THL0,THL0);
	i2c_Reg(I2C_THL1,THL1);
	i2c_Reg(I2C_THH0,THH0);
	i2c_Reg(I2C_THH1,THH1);
	i2c_Reg(I2C_INT,INT);
	GPIOINT_Setup();
	 }

void GPIOINT_Setup()
{
	 GPIO_PinModeSet(PortD, Interrupt_Pin , gpioModeInput, 1);
	 GPIO_ExtIntConfig(PortD, Interrupt_Pin , 1,false, true, true);
	 NVIC_EnableIRQ(GPIO_ODD_IRQn);

}
void GPIO_Setup()
  {
	  //CMU_ClockEnable(cmuClock_GPIO, true);
	  	GPIO_PinModeSet(LED_PORT, LED_PIN, gpioModePushPull,0);
	  	GPIO_PinModeSet(PortD,Power_Pin, gpioModePushPull,0);
  }
