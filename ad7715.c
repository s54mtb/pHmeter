/**
  ******************************************************************************
  * @file    ad7715.c
  * @author  e.pavlin.si
  * @brief   AD7715 routines
  ******************************************************************************
  * @attention
  * <h2><center>http://e.pavlin.si</center></h2>
  * 
  * This is free and unencumbered software released into the public domain.
  * 
  * Anyone is free to copy, modify, publish, use, compile, sell, or
  * distribute this software, either in source code form or as a compiled
  * binary, for any purpose, commercial or non-commercial, and by any
  * means.
  * 
  * In  jurisdictions that recognize copyright laws, the author or authors
  * of this software dedicate any and all copyright interest in the
  * software to the public domain. We make this dedication for the benefit
  * of the public at large and to the detriment of our heirs and
  * successors. We intend this dedication to be an overt act of
  * relinquishment in perpetuity of all present and future rights to this
  * software under copyright law.
  * 
  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
  * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
  * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
  * OTHER DEALINGS IN THE SOFTWARE.

  * For more information, please refer to <http://unlicense.org>
  *
  ******************************************************************************
  */

#include "cmsis_os.h"                   // CMSIS RTOS header file
#include "stm32f0xx.h"                  // Device header
#include "AD7715.h"
#include <string.h>

/*----------------------------------------------------------------------------
 *       AD7715 Pin definitions
 *---------------------------------------------------------------------------*/
 

/* CLK: PA5 */
#define AD7715_CLKPORT		GPIOA
#define AD7715_CLKPINn		5

/* MOSI: PA7 */
#define AD7715_MOSIPORT		GPIOA
#define AD7715_MOSIPINn   7

/* MISO: PA6 */
#define AD7715_MISOPORT		GPIOA
#define AD7715_MISOPINn 	6

/* CS: PF1 */
#define AD7715_CSPORT		  GPIOF
#define AD7715_CSPINn			1


/*----------------------------------------------------------------------------
 *  End of pin definitions
 */


#define AD7715_MOSIPIN		((uint16_t)(1U<<AD7715_MOSIPINn))  
#define AD7715_MISOPIN		((uint16_t)(1U<<AD7715_MISOPINn))  
#define AD7715_CSPIN			((uint16_t)(1U<<AD7715_CSPINn))  
#define AD7715_CLKPIN			((uint16_t)(1U<<AD7715_CLKPINn)) 



/** Thread definitions */
void AD7715_Thread (void const *argument);                             // thread function
osThreadId AD7715_tid_Thread;                                          // thread id
osThreadDef (AD7715_Thread, osPriorityNormal, 1, 0);                   // thread object


/** Local variables */
static 	uint16_t adcreadout;


/**
  Return last ADC readout 
	*/
uint16_t AD7715_Readout(void)
{
	return adcreadout;
}
	

/** 
  Init AD7715 pins 
*/
void AD7715_InitPins(void)
{
  RCC->AHBENR |= RCC_AHBENR_GPIOAEN;  /* Enable GPIOA clock         */
  RCC->AHBENR |= RCC_AHBENR_GPIOFEN;  /* Enable GPIOF clock         */

	/* CLK push-pull, no pullup */
	AD7715_CLKPORT->MODER   &= ~(3ul << 2*AD7715_CLKPINn);
  AD7715_CLKPORT->MODER   |=  (1ul << 2*AD7715_CLKPINn);
  AD7715_CLKPORT->OTYPER  &= ~(1ul <<   AD7715_CLKPINn);
  AD7715_CLKPORT->OSPEEDR &= ~(3ul << 2*AD7715_CLKPINn);
  AD7715_CLKPORT->OSPEEDR |=  (1ul << 2*AD7715_CLKPINn);
  AD7715_CLKPORT->PUPDR   &= ~(3ul << 2*AD7715_CLKPINn);

	/* MOSI push-pull, no pullup */
	AD7715_MOSIPORT->MODER   &= ~(3ul << 2*AD7715_MOSIPINn);
  AD7715_MOSIPORT->MODER   |=  (1ul << 2*AD7715_MOSIPINn);
  AD7715_MOSIPORT->OTYPER  &= ~(1ul <<   AD7715_MOSIPINn);
  AD7715_MOSIPORT->OSPEEDR &= ~(3ul << 2*AD7715_MOSIPINn);
  AD7715_MOSIPORT->OSPEEDR |=  (1ul << 2*AD7715_MOSIPINn);
  AD7715_MOSIPORT->PUPDR   &= ~(3ul << 2*AD7715_MOSIPINn);

	/* CS push-pull, no pullup */
	AD7715_CSPORT->MODER   &= ~(3ul << 2*AD7715_CSPINn);
  AD7715_CSPORT->MODER   |=  (1ul << 2*AD7715_CSPINn);
  AD7715_CSPORT->OTYPER  &= ~(1ul <<   AD7715_CSPINn);
  AD7715_CSPORT->OSPEEDR &= ~(3ul << 2*AD7715_CSPINn);
  AD7715_CSPORT->OSPEEDR |=  (1ul << 2*AD7715_CSPINn);
  AD7715_CSPORT->PUPDR   &= ~(3ul << 2*AD7715_CSPINn);
	
	/* MISO Input, pullup */
  AD7715_MISOPORT->MODER   &= ~(3ul << 2*AD7715_MISOPINn);
  AD7715_MISOPORT->OSPEEDR &= ~(3ul << 2*AD7715_MISOPINn);
  AD7715_MISOPORT->OSPEEDR |=  (1ul << 2*AD7715_MISOPINn);
  AD7715_MISOPORT->PUPDR   &= ~(3ul << 2*AD7715_MISOPINn);
	AD7715_MISOPORT->PUPDR   |=  (1ul << 2*AD7715_MISOPINn);

	AD7715_CLKPORT->BSRR = AD7715_CLKPIN;    // CLK = 1
	AD7715_CSPORT->BSRR = AD7715_CSPIN;      // CS = 1
	AD7715_MOSIPORT->BSRR = AD7715_MOSIPIN;  // MOSI = 1
	
}

/**
  * Set MOSI line 
  */
void AD7715_SetMOSI(int state)
{
	if (state) 
		AD7715_MOSIPORT->BSRR = AD7715_MOSIPIN;
	else
		AD7715_MOSIPORT->BSRR = AD7715_MOSIPIN << 16;	
}


/**
  * Set CS line
  */
void AD7715_SetCS(int state)
{
	if (state) 
		AD7715_CSPORT->BSRR = AD7715_CSPIN;
	else
		AD7715_CSPORT->BSRR = AD7715_CSPIN << 16;	
}


/** 
  * Set CLK line 
  */
void AD7715_SetCLK(int state)
{
	int i;
	
	for (i=0; i<8; i++) 
  {
		if (state) 
			AD7715_CLKPORT->BSRR = AD7715_CLKPIN;
		else
			AD7715_CLKPORT->BSRR = AD7715_CLKPIN << 16;	
	}
}


/**
  * Pulse SCLK line to 0 and back to 1 
  */
void AD7715_CLKPulse(void)
{
	AD7715_SetCLK(0);
	AD7715_SetCLK(1);
}
	

/**
  * Reset AD7715.... send 32 "ones" 
  */
void AD7715_Reset(void)
{
	int i;
  
	/* Send 32 ones to reset AD7715 */
	AD7715_SetCS(0); 
  AD7715_SetMOSI(1);	
	for (i = 0; i<32; i++)
	{
		AD7715_CLKPulse();
	}
	AD7715_SetCS(1);
}


/**
  * Read MISO line 
  */
uint8_t AD7715_ReadMISO(void)
{
  uint8_t rv = 0;	
	
	if ((AD7715_MISOPORT->IDR & (AD7715_MISOPIN)) != 0) 
	{
    rv = 1;
  }

	return rv;
}



/**
 * Simultaneously transmit and receive a byte on the AD7715 SPI.
 * Returns the received byte.
 */
uint8_t AD7715_transferbyte(uint8_t byte_out)
{
	uint8_t byte_in = 0;
	uint8_t bit;

	for (bit = 0x80; bit; bit >>= 1) 
	{
		AD7715_SetCLK(0);
		/* Shift-out a bit to the MOSI line */
		AD7715_SetMOSI((byte_out & bit) ? 1 : 0);
		/* Pull the clock line high */
		AD7715_SetCLK(1);
		/* Shift-in a bit from the MISO line */
		if (AD7715_ReadMISO() > 0)
			byte_in |= bit;
	}
	return byte_in;
}	


/**
  * Init AD7715 thread 
  */
int Init_AD7715_Thread (void) {

  AD7715_tid_Thread = osThreadCreate (osThread(AD7715_Thread), NULL); 
  if (!AD7715_tid_Thread) return(-1);
  
  return(0);
}


/**
  * AD7715 Thread: perform continuous conversion, calculate calibrated values and 
  * and store the values to buffer.
  */
void AD7715_Thread (void const *argument) 
{

	uint8_t adcbuf[2];
	AD7715_CommReg_t CommReg; 
	AD7715_SetupReg_t SetupReg; 
	
	
	/* Init Pins */
	AD7715_InitPins();
	
	/* Reset AD7715 */
	AD7715_Reset();
	
	/** Write to setup register */
	CommReg.b.DRDY = 0;
	CommReg.b.Zero = 0;
	CommReg.b.RS = AD7715_REG_SETUP;
	CommReg.b.RW = AD7715_RW_WRITE;	
	CommReg.b.STBY = AD7715_STBY_POWERUP;
	CommReg.b.Gain = AD7715_GAIN_1; 

  /** Setup register */
  SetupReg.b.BU = AD7715_BU_BIPOLAR;
	SetupReg.b.BUF = AD7715_BUF_BYPASSED;
	SetupReg.b.CLK = AD7715_CLK_2_4576MHZ;
	SetupReg.b.FS = AD7715_FS_50HZ;
	SetupReg.b.FSYNC = 0;
	SetupReg.b.MD = AD7715_MODE_SELFCAL;
  
	AD7715_SetCS(0);
	AD7715_transferbyte(CommReg.B);
	AD7715_transferbyte(SetupReg.B);
	AD7715_SetCS(1);

  osDelay(5);

  /* Set normal operation */
 	SetupReg.b.MD = AD7715_MODE_NORMAL;
	
	AD7715_SetCS(0);
	AD7715_transferbyte(CommReg.B);
	AD7715_transferbyte(SetupReg.B);
	AD7715_SetCS(1);

	
  while (1) {
		
		/** Read from comm register, poll DRDY */
		CommReg.b.DRDY = 0;
		CommReg.b.Zero = 0;
		CommReg.b.RS = AD7715_REG_COMM;
		CommReg.b.RW = AD7715_RW_READ;	
		CommReg.b.STBY = AD7715_STBY_POWERUP;
		CommReg.b.Gain = AD7715_GAIN_1; 
		
		AD7715_SetCS(0);
		AD7715_transferbyte(CommReg.B);
		CommReg.B = AD7715_transferbyte(0xff);
		AD7715_SetCS(1);
		
		if ((CommReg.b.DRDY) == 0) 
		{
			// read data
			CommReg.b.DRDY = 0;
			CommReg.b.Zero = 0;
			CommReg.b.RS = AD7715_REG_DATA;
			CommReg.b.RW = AD7715_RW_READ;	
			CommReg.b.STBY = AD7715_STBY_POWERUP;
			CommReg.b.Gain = AD7715_GAIN_1;

			AD7715_SetCS(0);
			AD7715_transferbyte(CommReg.B);
			adcbuf[1] = AD7715_transferbyte(0xff);
			adcbuf[0] = AD7715_transferbyte(0xff);
			AD7715_SetCS(1);
			memcpy(&adcreadout, adcbuf, 2);
			
		}
			
		
    osThreadYield ();                                           // suspend thread
  }
}
