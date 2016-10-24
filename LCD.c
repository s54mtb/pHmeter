/**
  ******************************************************************************
  * @file    lcd.c
  * @author  e.pavlin.si
  * @brief   HD4478 LCD routines
  ******************************************************************************
  * @attention
  * Part of the code adapted from 
	*       https://stm32f4-discovery.net/download/tm_stm32f4_hd44780/
	* Rewritten to "bare metal" without any additional HAL or other libraries. 
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
#include <stdio.h>

// stm32f070x6.h
/*----------------------------------------------------------------------------
 *       'LCD_Thread': LCD Display thread
 *---------------------------------------------------------------------------*/

/// LCD pinout definitions
//RS - Register select pin
#define HD44780_RSPORT     GPIOB
#define HD44780_RSPINn     1
//E - Enable pin
#define HD44780_EPORT      GPIOA
#define HD44780_EPINn      4
//D4 - Data 4 pin
#define HD44780_D4PORT     GPIOA
#define HD44780_D4PINn     3
//D5 - Data 5 pin
#define HD44780_D5PORT     GPIOA
#define HD44780_D5PINn     2
//D6 - Data 6 pin
#define HD44780_D6PORT     GPIOA
#define HD44780_D6PINn     1
//D7 - Data 7 pin
#define HD44780_D7PORT     GPIOA
#define HD44780_D7PINn     0

#define HD44780_RSPIN		((uint16_t)(1U<<HD44780_RSPINn))
#define HD44780_EPIN		((uint16_t)(1U<<HD44780_EPINn))
#define HD44780_D4PIN		((uint16_t)(1U<<HD44780_D4PINn))
#define HD44780_D5PIN		((uint16_t)(1U<<HD44780_D5PINn))
#define HD44780_D6PIN		((uint16_t)(1U<<HD44780_D6PINn))
#define HD44780_D7PIN		((uint16_t)(1U<<HD44780_D7PINn))

void LCD_Thread (void const *argument);                             // thread function
osThreadId LCD_tid_Thread;                                          // thread id
osThreadDef (LCD_Thread, osPriorityNormal, 1, 1024);                   // thread object


void HD44780_Init(uint8_t cols, uint8_t rows);
void HD44780_DisplayOn(void);
void HD44780_DisplayOff(void);
void HD44780_Clear(void);
void HD44780_Puts(uint8_t x, uint8_t y, char* str);
void HD44780_BlinkOn(void);
void HD44780_BlinkOff(void);
void HD44780_CursorOn(void);
void HD44780_CursorOff(void);
void HD44780_ScrollLeft(void);
void HD44780_ScrollRight(void);
void HD44780_CreateChar(uint8_t location, uint8_t* data);
void HD44780_PutCustom(uint8_t x, uint8_t y, uint8_t location);

extern uint32_t SystemCoreClock;

/* Private HD44780 structure */
typedef struct {
	uint8_t DisplayControl;
	uint8_t DisplayFunction;
	uint8_t DisplayMode;
	uint8_t Rows;
	uint8_t Cols;
	uint8_t currentX;
	uint8_t currentY;
} HD44780_Options_t;


static void HD44780_InitPins(void);
static void HD44780_Cmd(uint8_t cmd);
static void HD44780_Cmd4bit(uint8_t cmd);
static void HD44780_Data(uint8_t data);
static void HD44780_CursorSet(uint8_t col, uint8_t row);

/* Private variable */
static HD44780_Options_t HD44780_Opts;

static void HD44780_Delay(uint32_t us);

/* Pin definitions */ 
#define HD44780_RS_LOW              HD44780_RSPORT->BSRR = HD44780_RSPIN<<16
#define HD44780_RS_HIGH             HD44780_RSPORT->BSRR = HD44780_RSPIN
#define HD44780_E_LOW               HD44780_EPORT->BSRR = HD44780_EPIN<<16
#define HD44780_E_HIGH              HD44780_EPORT->BSRR = HD44780_EPIN

#define HD44780_E_BLINK             HD44780_E_HIGH; HD44780_Delay(20); HD44780_E_LOW; HD44780_Delay(20)

/* Commands*/
#define HD44780_CLEARDISPLAY        0x01
#define HD44780_RETURNHOME          0x02
#define HD44780_ENTRYMODESET        0x04
#define HD44780_DISPLAYCONTROL      0x08
#define HD44780_CURSORSHIFT         0x10
#define HD44780_FUNCTIONSET         0x20
#define HD44780_SETCGRAMADDR        0x40
#define HD44780_SETDDRAMADDR        0x80

/* Flags for display entry mode */
#define HD44780_ENTRYRIGHT          0x00
#define HD44780_ENTRYLEFT           0x02
#define HD44780_ENTRYSHIFTINCREMENT 0x01
#define HD44780_ENTRYSHIFTDECREMENT 0x00

/* Flags for display on/off control */
#define HD44780_DISPLAYON           0x04
#define HD44780_CURSORON            0x02
#define HD44780_BLINKON             0x01

/* Flags for display/cursor shift */
#define HD44780_DISPLAYMOVE         0x08
#define HD44780_CURSORMOVE          0x00
#define HD44780_MOVERIGHT           0x04
#define HD44780_MOVELEFT            0x00

/* Flags for function set */
#define HD44780_8BITMODE            0x10
#define HD44780_4BITMODE            0x00
#define HD44780_2LINE               0x08
#define HD44780_1LINE               0x00
#define HD44780_5x10DOTS            0x04
#define HD44780_5x8DOTS             0x00

static void HD44780_InitPins(void)
{
  RCC->AHBENR |= RCC_AHBENR_GPIOAEN;  /* Enable GPIOA clock         */
  RCC->AHBENR |= RCC_AHBENR_GPIOBEN;  /* Enable GPIOB clock         */

	/* All pins push-pull, no pullup */

	HD44780_RSPORT->MODER   &= ~(3ul << 2*HD44780_RSPINn);
  HD44780_RSPORT->MODER   |=  (1ul << 2*HD44780_RSPINn);
  HD44780_RSPORT->OTYPER  &= ~(1ul <<   HD44780_RSPINn);
  HD44780_RSPORT->OSPEEDR &= ~(3ul << 2*HD44780_RSPINn);
  HD44780_RSPORT->OSPEEDR |=  (1ul << 2*HD44780_RSPINn);
  HD44780_RSPORT->PUPDR   &= ~(3ul << 2*HD44780_RSPINn);

	HD44780_EPORT->MODER   &= ~(3ul << 2*HD44780_EPINn);
  HD44780_EPORT->MODER   |=  (1ul << 2*HD44780_EPINn);
  HD44780_EPORT->OTYPER  &= ~(1ul <<   HD44780_EPINn);
  HD44780_EPORT->OSPEEDR &= ~(3ul << 2*HD44780_EPINn);
  HD44780_EPORT->OSPEEDR |=  (1ul << 2*HD44780_EPINn);
  HD44780_EPORT->PUPDR   &= ~(3ul << 2*HD44780_EPINn);

	HD44780_D4PORT->MODER   &= ~(3ul << 2*HD44780_D4PINn);
  HD44780_D4PORT->MODER   |=  (1ul << 2*HD44780_D4PINn);
  HD44780_D4PORT->OTYPER  &= ~(1ul <<   HD44780_D4PINn);
  HD44780_D4PORT->OSPEEDR &= ~(3ul << 2*HD44780_D4PINn);
  HD44780_D4PORT->OSPEEDR |=  (1ul << 2*HD44780_D4PINn);
  HD44780_D4PORT->PUPDR   &= ~(3ul << 2*HD44780_D4PINn);

	HD44780_D5PORT->MODER   &= ~(3ul << 2*HD44780_D5PINn);
  HD44780_D5PORT->MODER   |=  (1ul << 2*HD44780_D5PINn);
  HD44780_D5PORT->OTYPER  &= ~(1ul <<   HD44780_D5PINn);
  HD44780_D5PORT->OSPEEDR &= ~(3ul << 2*HD44780_D5PINn);
  HD44780_D5PORT->OSPEEDR |=  (1ul << 2*HD44780_D5PINn);
  HD44780_D5PORT->PUPDR   &= ~(3ul << 2*HD44780_D5PINn);

	HD44780_D6PORT->MODER   &= ~(3ul << 2*HD44780_D6PINn);
  HD44780_D6PORT->MODER   |=  (1ul << 2*HD44780_D6PINn);
  HD44780_D6PORT->OTYPER  &= ~(1ul <<   HD44780_D6PINn);
  HD44780_D6PORT->OSPEEDR &= ~(3ul << 2*HD44780_D6PINn);
  HD44780_D6PORT->OSPEEDR |=  (1ul << 2*HD44780_D6PINn);
  HD44780_D6PORT->PUPDR   &= ~(3ul << 2*HD44780_D6PINn);

	HD44780_D7PORT->MODER   &= ~(3ul << 2*HD44780_D7PINn);
  HD44780_D7PORT->MODER   |=  (1ul << 2*HD44780_D7PINn);
  HD44780_D7PORT->OTYPER  &= ~(1ul <<   HD44780_D7PINn);
  HD44780_D7PORT->OSPEEDR &= ~(3ul << 2*HD44780_D7PINn);
  HD44780_D7PORT->OSPEEDR |=  (1ul << 2*HD44780_D7PINn);
  HD44780_D7PORT->PUPDR   &= ~(3ul << 2*HD44780_D7PINn);

}


static void HD44780_Delay(uint32_t us)
{
	us *= 48; 
	
	while (us--) 
		__nop();
	
}


void HD44780_Init(uint8_t cols, uint8_t rows) {
	
	/* Init pinout */
	HD44780_InitPins();
	
	/* At least 40ms */
	osDelay(45);
	
	/* Set LCD width and height */
	HD44780_Opts.Rows = rows;
	HD44780_Opts.Cols = cols;
	
	/* Set cursor pointer to beginning for LCD */
	HD44780_Opts.currentX = 0;
	HD44780_Opts.currentY = 0;
	
	HD44780_Opts.DisplayFunction = HD44780_4BITMODE | HD44780_5x8DOTS | HD44780_1LINE;
	if (rows > 1) {
		HD44780_Opts.DisplayFunction |= HD44780_2LINE;
	}
	
	/* Try to set 4bit mode */
	HD44780_Cmd4bit(0x03);
	osDelay(5);
	
	/* Second try */
	HD44780_Cmd4bit(0x03);
		osDelay(5);
	
	/* Third goo! */
	HD44780_Cmd4bit(0x03);
		osDelay(5);
	
	/* Set 4-bit interface */
	HD44780_Cmd4bit(0x02);
	osDelay(1);
	
	/* Set # lines, font size, etc. */
	HD44780_Cmd(HD44780_FUNCTIONSET | HD44780_Opts.DisplayFunction);

	/* Turn the display on with no cursor or blinking default */
	HD44780_Opts.DisplayControl = HD44780_DISPLAYON;
	HD44780_DisplayOn();

	/* Clear lcd */
	HD44780_Clear();

	/* Default font directions */
	HD44780_Opts.DisplayMode = HD44780_ENTRYLEFT | HD44780_ENTRYSHIFTDECREMENT;
	HD44780_Cmd(HD44780_ENTRYMODESET | HD44780_Opts.DisplayMode);

	/* Delay */
		osDelay(5);
}

void HD44780_Clear(void) {
	HD44780_Cmd(HD44780_CLEARDISPLAY);
	//HD44780_Delay(3000);
	osDelay(3);
}

void HD44780_Puts(uint8_t x, uint8_t y, char* str) {
	HD44780_CursorSet(x, y);
	while (*str) {
		if (HD44780_Opts.currentX >= HD44780_Opts.Cols) {
			HD44780_Opts.currentX = 0;
			HD44780_Opts.currentY++;
			HD44780_CursorSet(HD44780_Opts.currentX, HD44780_Opts.currentY);
		}
		if (*str == '\n') {
			HD44780_Opts.currentY++;
			HD44780_CursorSet(HD44780_Opts.currentX, HD44780_Opts.currentY);
		} else if (*str == '\r') {
			HD44780_CursorSet(0, HD44780_Opts.currentY);
		} else {
			HD44780_Data(*str);
			HD44780_Opts.currentX++;
		}
		str++;
	}
}

void HD44780_DisplayOn(void) {
	HD44780_Opts.DisplayControl |= HD44780_DISPLAYON;
	HD44780_Cmd(HD44780_DISPLAYCONTROL | HD44780_Opts.DisplayControl);
}

void HD44780_DisplayOff(void) {
	HD44780_Opts.DisplayControl &= ~HD44780_DISPLAYON;
	HD44780_Cmd(HD44780_DISPLAYCONTROL | HD44780_Opts.DisplayControl);
}

void HD44780_BlinkOn(void) {
	HD44780_Opts.DisplayControl |= HD44780_BLINKON;
	HD44780_Cmd(HD44780_DISPLAYCONTROL | HD44780_Opts.DisplayControl);
}

void HD44780_BlinkOff(void) {
	HD44780_Opts.DisplayControl &= ~HD44780_BLINKON;
	HD44780_Cmd(HD44780_DISPLAYCONTROL | HD44780_Opts.DisplayControl);
}

void HD44780_CursorOn(void) {
	HD44780_Opts.DisplayControl |= HD44780_CURSORON;
	HD44780_Cmd(HD44780_DISPLAYCONTROL | HD44780_Opts.DisplayControl);
}

void HD44780_CursorOff(void) {
	HD44780_Opts.DisplayControl &= ~HD44780_CURSORON;
	HD44780_Cmd(HD44780_DISPLAYCONTROL | HD44780_Opts.DisplayControl);
}

void HD44780_ScrollLeft(void) {
	HD44780_Cmd(HD44780_CURSORSHIFT | HD44780_DISPLAYMOVE | HD44780_MOVELEFT);
}

void HD44780_ScrollRight(void) {
	HD44780_Cmd(HD44780_CURSORSHIFT | HD44780_DISPLAYMOVE | HD44780_MOVERIGHT);
}

void HD44780_CreateChar(uint8_t location, uint8_t *data) {
	uint8_t i;
	/* We have 8 locations available for custom characters */
	location &= 0x07;
	HD44780_Cmd(HD44780_SETCGRAMADDR | (location << 3));
	
	for (i = 0; i < 8; i++) {
		HD44780_Data(data[i]);
	}
}

void HD44780_PutCustom(uint8_t x, uint8_t y, uint8_t location) {
	HD44780_CursorSet(x, y);
	HD44780_Data(location);
}

/* Private functions */
static void HD44780_Cmd(uint8_t cmd) {
	/* Command mode */
	HD44780_RS_LOW;
	
	/* High nibble */
	HD44780_Cmd4bit(cmd >> 4);
	/* Low nibble */
	HD44780_Cmd4bit(cmd & 0x0F);
}

static void HD44780_Data(uint8_t data) {
	/* Data mode */
	HD44780_RS_HIGH;
	
	/* High nibble */
	HD44780_Cmd4bit(data >> 4);
	/* Low nibble */
	HD44780_Cmd4bit(data & 0x0F);
}

static void HD44780_Cmd4bit(uint8_t cmd) {
	/* Set output port */
	
	HD44780_D7PORT->BSRR = HD44780_D7PIN<<((cmd & 0x08) ? 0 : 16); 
	HD44780_D6PORT->BSRR = HD44780_D6PIN<<((cmd & 0x04) ? 0 : 16); 
	HD44780_D5PORT->BSRR = HD44780_D5PIN<<((cmd & 0x02) ? 0 : 16); 
	HD44780_D4PORT->BSRR = HD44780_D4PIN<<((cmd & 0x01) ? 0 : 16); 
	HD44780_E_BLINK;
}

static void HD44780_CursorSet(uint8_t col, uint8_t row) {
	uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
	
	/* Go to beginning */
	if (row >= HD44780_Opts.Rows) {
		row = 0;
	}
	
	/* Set current column and row */
	HD44780_Opts.currentX = col;
	HD44780_Opts.currentY = row;
	
	/* Set location address */
	HD44780_Cmd(HD44780_SETDDRAMADDR | (col + row_offsets[row]));
}



int Init_LCD_Thread (void) {

  LCD_tid_Thread = osThreadCreate (osThread(LCD_Thread), NULL);
  if (!LCD_tid_Thread) return(-1);
  
  return(0);
}

extern uint16_t AD7715_Readout(void);

void LCD_Thread (void const *argument) {

	char str[16];
	
  HD44780_Init(16, 2);
	
	HD44780_Puts(0, 0, "pH meter....");
	HD44780_Puts(0, 1, "  ... Init.");
	
	
  while (1) {
	
		sprintf(str,"%d ",AD7715_Readout());
		HD44780_Puts(0, 1, str);

    osThreadYield ();                                           // suspend thread
  }
}
