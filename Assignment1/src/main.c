#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "em_letimer.h"
#include "em_system.h"

void LETIMER0_IRQHandler(void)
{
  /* Clear LETIMER0 comp0 interrupt flag */

  if(LETIMER_IF_COMP1==1)
  {
	  LETIMER_IntClear(LETIMER0, LETIMER_IF_COMP1);
      GPIO_PinOutSet(gpioPortE,2);
  }
      if(LETIMER_IF_COMP0==1)
      {
    	  LETIMER_IntClear(LETIMER0, LETIMER_IF_COMP0);
      	GPIO_PinOutClear(gpioPortE,2);
      }
  ///Wake up the chip from sleep
}
int main(void)
{
  // Initialize chip
  CHIP_Init();

  // Enable the External Low Frequency oscillator
  CMU_OscillatorEnable(cmuOsc_LFXO,true,true);

  // Enable clock for GPIO module
  CMU_ClockEnable(cmuClock_GPIO, true);

  // HFLE clock to enable access to Low Power Domain registers.
  CMU_ClockEnable(cmuClock_CORELE, true);

  // Select the Low Frequency A clock for the LETIMER0
  CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFXO);

  // Enable clock for LETIMER0 module
  CMU_ClockEnable(cmuClock_LETIMER0, true);

  // Init the LEDs
  GPIO_PinModeSet(gpioPortE,2,gpioModePushPull,0);

  // Select LETIMER0 parameters
  LETIMER_Init_TypeDef letimerInit = LETIMER_INIT_DEFAULT;

  // Enable overflow interrupt
  LETIMER_IntEnable(LETIMER0, LETIMER_IF_COMP1);
  LETIMER_IntEnable(LETIMER0, LETIMER_IF_COMP0);

  // Enable TIMER0 interrupt vector in NVIC
  NVIC_EnableIRQ(LETIMER0_IRQn);

  // Set LETIMER Top value - Comp0 Value / (32768Hz )
  // For 200mS Seconds Comp0 Value= 6553.6
  LETIMER_CompareSet(LETIMER0,0,32768);
  LETIMER_CompareSet(LETIMER0,1,1638);


  // Configure TIMER
  LETIMER_Init(LETIMER0, &letimerInit);

  // Make the COMP0 to be the Top value to loaded into Free Run mode
  LETIMER0->CTRL |= LETIMER_CTRL_COMP0TOP;

}
