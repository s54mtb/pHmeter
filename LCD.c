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
#define LCD_RSPORT     GPIOB
#define LCD_RSPINn     1
//E - Enable pin
#define LCD_EPORT      GPIOA
#define LCD_EPINn      4
//D4 - Data 4 pin
#define LCD_D4PORT     GPIOA
#define LCD_D4PINn     3
//D5 - Data 5 pin
#define LCD_D5PORT     GPIOA
#define LCD_D5PINn     2
//D6 - Data 6 pin
#define LCD_D6PORT     GPIOA
#define LCD_D6PINn     1
//D7 - Data 7 pin
#define LCD_D7PORT     GPIOA
#define LCD_D7PINn     0

#define LCD_RSPIN		((uint16_t)(1U<<LCD_RSPINn))
#define LCD_EPIN		((uint16_t)(1U<<LCD_EPINn))
#define LCD_D4PIN		((uint16_t)(1U<<LCD_D4PINn))
#define LCD_D5PIN		((uint16_t)(1U<<LCD_D5PINn))
#define LCD_D6PIN		((uint16_t)(1U<<LCD_D6PINn))
#define LCD_D7PIN		((uint16_t)(1U<<LCD_D7PINn))

void LCD_Init(uint8_t cols, uint8_t rows);
void LCD_DisplayOn(void);
void LCD_DisplayOff(void);
void LCD_Clear(void);
void LCD_Puts(uint8_t x, uint8_t y, char* str);
void LCD_BlinkOn(void);
void LCD_BlinkOff(void);
void LCD_CursorOn(void);
void LCD_CursorOff(void);
void LCD_ScrollLeft(void);
void LCD_ScrollRight(void);
void LCD_CreateChar(uint8_t location, uint8_t* data);
void LCD_PutCustom(uint8_t x, uint8_t y, uint8_t location);

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
	uint8_t Initialized;
} LCD_Options_t;


static void LCD_InitPins(void);
static void LCD_Cmd(uint8_t cmd);
static void LCD_Cmd4bit(uint8_t cmd);
static void LCD_Data(uint8_t data);
static void LCD_CursorSet(uint8_t col, uint8_t row);

/* Private variable */
static LCD_Options_t LCD_Opts;

static void LCD_Delay(uint32_t us);

/* Pin definitions */ 
#define LCD_RS_LOW              LCD_RSPORT->BSRR = LCD_RSPIN<<16
#define LCD_RS_HIGH             LCD_RSPORT->BSRR = LCD_RSPIN
#define LCD_E_LOW               LCD_EPORT->BSRR = LCD_EPIN<<16
#define LCD_E_HIGH              LCD_EPORT->BSRR = LCD_EPIN

#define LCD_E_BLINK             LCD_E_HIGH; LCD_Delay(20); LCD_E_LOW; LCD_Delay(20)

/* Commands*/
#define LCD_CLEARDISPLAY        0x01
#define LCD_RETURNHOME          0x02
#define LCD_ENTRYMODESET        0x04
#define LCD_DISPLAYCONTROL      0x08
#define LCD_CURSORSHIFT         0x10
#define LCD_FUNCTIONSET         0x20
#define LCD_SETCGRAMADDR        0x40
#define LCD_SETDDRAMADDR        0x80

/* Flags for display entry mode */
#define LCD_ENTRYRIGHT          0x00
#define LCD_ENTRYLEFT           0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

/* Flags for display on/off control */
#define LCD_DISPLAYON           0x04
#define LCD_CURSORON            0x02
#define LCD_BLINKON             0x01

/* Flags for display/cursor shift */
#define LCD_DISPLAYMOVE         0x08
#define LCD_CURSORMOVE          0x00
#define LCD_MOVERIGHT           0x04
#define LCD_MOVELEFT            0x00

/* Flags for function set */
#define LCD_8BITMODE            0x10
#define LCD_4BITMODE            0x00
#define LCD_2LINE               0x08
#define LCD_1LINE               0x00
#define LCD_5x10DOTS            0x04
#define LCD_5x8DOTS             0x00

static void LCD_InitPins(void)
{
  RCC->AHBENR |= RCC_AHBENR_GPIOAEN;  /* Enable GPIOA clock         */
  RCC->AHBENR |= RCC_AHBENR_GPIOBEN;  /* Enable GPIOB clock         */

	/* All pins push-pull, no pullup */

	LCD_RSPORT->MODER   &= ~(3ul << 2*LCD_RSPINn);
  LCD_RSPORT->MODER   |=  (1ul << 2*LCD_RSPINn);
  LCD_RSPORT->OTYPER  &= ~(1ul <<   LCD_RSPINn);
  LCD_RSPORT->OSPEEDR &= ~(3ul << 2*LCD_RSPINn);
  LCD_RSPORT->OSPEEDR |=  (1ul << 2*LCD_RSPINn);
  LCD_RSPORT->PUPDR   &= ~(3ul << 2*LCD_RSPINn);

	LCD_EPORT->MODER   &= ~(3ul << 2*LCD_EPINn);
  LCD_EPORT->MODER   |=  (1ul << 2*LCD_EPINn);
  LCD_EPORT->OTYPER  &= ~(1ul <<   LCD_EPINn);
  LCD_EPORT->OSPEEDR &= ~(3ul << 2*LCD_EPINn);
  LCD_EPORT->OSPEEDR |=  (1ul << 2*LCD_EPINn);
  LCD_EPORT->PUPDR   &= ~(3ul << 2*LCD_EPINn);

	LCD_D4PORT->MODER   &= ~(3ul << 2*LCD_D4PINn);
  LCD_D4PORT->MODER   |=  (1ul << 2*LCD_D4PINn);
  LCD_D4PORT->OTYPER  &= ~(1ul <<   LCD_D4PINn);
  LCD_D4PORT->OSPEEDR &= ~(3ul << 2*LCD_D4PINn);
  LCD_D4PORT->OSPEEDR |=  (1ul << 2*LCD_D4PINn);
  LCD_D4PORT->PUPDR   &= ~(3ul << 2*LCD_D4PINn);

	LCD_D5PORT->MODER   &= ~(3ul << 2*LCD_D5PINn);
  LCD_D5PORT->MODER   |=  (1ul << 2*LCD_D5PINn);
  LCD_D5PORT->OTYPER  &= ~(1ul <<   LCD_D5PINn);
  LCD_D5PORT->OSPEEDR &= ~(3ul << 2*LCD_D5PINn);
  LCD_D5PORT->OSPEEDR |=  (1ul << 2*LCD_D5PINn);
  LCD_D5PORT->PUPDR   &= ~(3ul << 2*LCD_D5PINn);

	LCD_D6PORT->MODER   &= ~(3ul << 2*LCD_D6PINn);
  LCD_D6PORT->MODER   |=  (1ul << 2*LCD_D6PINn);
  LCD_D6PORT->OTYPER  &= ~(1ul <<   LCD_D6PINn);
  LCD_D6PORT->OSPEEDR &= ~(3ul << 2*LCD_D6PINn);
  LCD_D6PORT->OSPEEDR |=  (1ul << 2*LCD_D6PINn);
  LCD_D6PORT->PUPDR   &= ~(3ul << 2*LCD_D6PINn);

	LCD_D7PORT->MODER   &= ~(3ul << 2*LCD_D7PINn);
  LCD_D7PORT->MODER   |=  (1ul << 2*LCD_D7PINn);
  LCD_D7PORT->OTYPER  &= ~(1ul <<   LCD_D7PINn);
  LCD_D7PORT->OSPEEDR &= ~(3ul << 2*LCD_D7PINn);
  LCD_D7PORT->OSPEEDR |=  (1ul << 2*LCD_D7PINn);
  LCD_D7PORT->PUPDR   &= ~(3ul << 2*LCD_D7PINn);

}


static void LCD_Delay(uint32_t us)
{
	us *= 48; 
	
	while (us--) 
		__nop();
	
}


void LCD_Init(uint8_t cols, uint8_t rows) {
	
	LCD_Opts.Initialized = 0;
	/* Init pinout */
	LCD_InitPins();
	
	/* At least 40ms */
	osDelay(45);
	
	/* Set LCD width and height */
	LCD_Opts.Rows = rows;
	LCD_Opts.Cols = cols;
	
	/* Set cursor pointer to beginning for LCD */
	LCD_Opts.currentX = 0;
	LCD_Opts.currentY = 0;
	
	LCD_Opts.DisplayFunction = LCD_4BITMODE | LCD_5x8DOTS | LCD_1LINE;
	if (rows > 1) {
		LCD_Opts.DisplayFunction |= LCD_2LINE;
	}
	
	/* Try to set 4bit mode */
	LCD_Cmd4bit(0x03);
	osDelay(5);
	
	/* Second try */
	LCD_Cmd4bit(0x03);
		osDelay(5);
	
	/* Third goo! */
	LCD_Cmd4bit(0x03);
		osDelay(5);
	
	/* Set 4-bit interface */
	LCD_Cmd4bit(0x02);
	osDelay(1);
	
	/* Set # lines, font size, etc. */
	LCD_Cmd(LCD_FUNCTIONSET | LCD_Opts.DisplayFunction);

	/* Turn the display on with no cursor or blinking default */
	LCD_Opts.DisplayControl = LCD_DISPLAYON;
	LCD_DisplayOn();

	/* Clear lcd */
	LCD_Clear();

	/* Default font directions */
	LCD_Opts.DisplayMode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
	LCD_Cmd(LCD_ENTRYMODESET | LCD_Opts.DisplayMode);

	/* Delay */
		osDelay(5);
	
	LCD_Opts.Initialized = 1;

}


uint8_t HD4478_initialized(void)
{
	return 	LCD_Opts.Initialized;
}


void LCD_Clear(void) {
	LCD_Cmd(LCD_CLEARDISPLAY);
	//LCD_Delay(3000);
	osDelay(3);
}

void LCD_Puts(uint8_t x, uint8_t y, char* str) {
	LCD_CursorSet(x, y);
	while (*str) {
		if (LCD_Opts.currentX >= LCD_Opts.Cols) {
			LCD_Opts.currentX = 0;
			LCD_Opts.currentY++;
			LCD_CursorSet(LCD_Opts.currentX, LCD_Opts.currentY);
		}
		if (*str == '\n') {
			LCD_Opts.currentY++;
			LCD_CursorSet(LCD_Opts.currentX, LCD_Opts.currentY);
		} else if (*str == '\r') {
			LCD_CursorSet(0, LCD_Opts.currentY);
		} else {
			LCD_Data(*str);
			LCD_Opts.currentX++;
		}
		str++;
	}
}

void LCD_DisplayOn(void) {
	LCD_Opts.DisplayControl |= LCD_DISPLAYON;
	LCD_Cmd(LCD_DISPLAYCONTROL | LCD_Opts.DisplayControl);
}

void LCD_DisplayOff(void) {
	LCD_Opts.DisplayControl &= ~LCD_DISPLAYON;
	LCD_Cmd(LCD_DISPLAYCONTROL | LCD_Opts.DisplayControl);
}

void LCD_BlinkOn(void) {
	LCD_Opts.DisplayControl |= LCD_BLINKON;
	LCD_Cmd(LCD_DISPLAYCONTROL | LCD_Opts.DisplayControl);
}

void LCD_BlinkOff(void) {
	LCD_Opts.DisplayControl &= ~LCD_BLINKON;
	LCD_Cmd(LCD_DISPLAYCONTROL | LCD_Opts.DisplayControl);
}

void LCD_CursorOn(void) {
	LCD_Opts.DisplayControl |= LCD_CURSORON;
	LCD_Cmd(LCD_DISPLAYCONTROL | LCD_Opts.DisplayControl);
}

void LCD_CursorOff(void) {
	LCD_Opts.DisplayControl &= ~LCD_CURSORON;
	LCD_Cmd(LCD_DISPLAYCONTROL | LCD_Opts.DisplayControl);
}

void LCD_ScrollLeft(void) {
	LCD_Cmd(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}

void LCD_ScrollRight(void) {
	LCD_Cmd(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

void LCD_CreateChar(uint8_t location, uint8_t *data) {
	uint8_t i;
	/* We have 8 locations available for custom characters */
	location &= 0x07;
	LCD_Cmd(LCD_SETCGRAMADDR | (location << 3));
	
	for (i = 0; i < 8; i++) {
		LCD_Data(data[i]);
	}
}

void LCD_PutCustom(uint8_t x, uint8_t y, uint8_t location) {
	LCD_CursorSet(x, y);
	LCD_Data(location);
}

/* Private functions */
static void LCD_Cmd(uint8_t cmd) {
	/* Command mode */
	LCD_RS_LOW;
	
	/* High nibble */
	LCD_Cmd4bit(cmd >> 4);
	/* Low nibble */
	LCD_Cmd4bit(cmd & 0x0F);
}

static void LCD_Data(uint8_t data) {
	/* Data mode */
	LCD_RS_HIGH;
	
	/* High nibble */
	LCD_Cmd4bit(data >> 4);
	/* Low nibble */
	LCD_Cmd4bit(data & 0x0F);
}

static void LCD_Cmd4bit(uint8_t cmd) {
	/* Set output port */
	
	LCD_D7PORT->BSRR = LCD_D7PIN<<((cmd & 0x08) ? 0 : 16); 
	LCD_D6PORT->BSRR = LCD_D6PIN<<((cmd & 0x04) ? 0 : 16); 
	LCD_D5PORT->BSRR = LCD_D5PIN<<((cmd & 0x02) ? 0 : 16); 
	LCD_D4PORT->BSRR = LCD_D4PIN<<((cmd & 0x01) ? 0 : 16); 
	LCD_E_BLINK;
}

static void LCD_CursorSet(uint8_t col, uint8_t row) {
	uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
	
	/* Go to beginning */
	if (row >= LCD_Opts.Rows) {
		row = 0;
	}
	
	/* Set current column and row */
	LCD_Opts.currentX = col;
	LCD_Opts.currentY = row;
	
	/* Set location address */
	LCD_Cmd(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}


