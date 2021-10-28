/*
  gpio.c
 
   Created on: Dec 12, 2018
       Author: Dan Walkes
   Updated by Dave Sluiter Dec 31, 2020. Minor edits with #defines.

   March 17
   Dave Sluiter: Use this file to define functions that set up or control GPIOs.

 */




#include "gpio.h"





// Set GPIO drive strengths and modes of operation
void gpioInit()
{

  // Student Edit:

	// GPIO_DriveStrengthSet(LED0_port, gpioDriveStrengthStrongAlternateStrong);
	GPIO_DriveStrengthSet(LED0_port, gpioDriveStrengthWeakAlternateWeak);
	GPIO_PinModeSet(LED0_port, LED0_pin, gpioModePushPull, false);

	// GPIO_DriveStrengthSet(LED1_port, gpioDriveStrengthStrongAlternateStrong);
	GPIO_DriveStrengthSet(LED1_port, gpioDriveStrengthWeakAlternateWeak);
	GPIO_PinModeSet(LED1_port, LED1_pin, gpioModePushPull, false);



} // gpioInit()




void gpioLed0SetOn()
{
	GPIO_PinOutSet(LED0_port,LED0_pin);
}


void gpioLed0SetOff()
{
	GPIO_PinOutClear(LED0_port,LED0_pin);
}


void gpioLed1SetOn()
{
	GPIO_PinOutSet(LED1_port,LED1_pin);
}


void gpioLed1SetOff()
{
	GPIO_PinOutClear(LED1_port,LED1_pin);
}


void PB0_Init(void) {
    GPIO_PinModeSet(PB0_port, PB0_pin, gpioModeInputPullFilter, true);
    GPIO_ExtIntConfig(PB0_port, PB0_pin, PB0_pin, true, true, true);
    NVIC_EnableIRQ(GPIO_EVEN_IRQn);
}


void PB1_Init(void) {
    GPIO_PinModeSet(PB1_port, PB1_pin, gpioModeInputPullFilter, true);
    GPIO_ExtIntConfig(PB1_port, PB1_pin, PB1_pin, true, true, true);
    NVIC_EnableIRQ(GPIO_ODD_IRQn);
}


void gpioSensorEnSetOn(void) {
    GPIO_DriveStrengthSet(LCD_ENABLE_PORT, gpioDriveStrengthWeakAlternateWeak);
    GPIO_PinModeSet(LCD_ENABLE_PORT, LCD_ENABLE_PIN, gpioModePushPull, false);
    GPIO_PinOutSet(LCD_ENABLE_PORT, LCD_ENABLE_PIN);
}


void gpioSetDisplayExtcomin(int on) {
    if (on) {
        GPIO_PinOutSet(LCD_EXCOMIN_PORT, LCD_EXCOMIN_PIN);
    }
    else {
        GPIO_PinOutClear(LCD_EXCOMIN_PORT, LCD_EXCOMIN_PIN);
    }
}

