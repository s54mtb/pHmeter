
#include "cmsis_os.h"                                           // CMSIS RTOS header file
#include "lcd.h"
#include "ad7715.h"
#include <stdio.h>

/*----------------------------------------------------------------------------
 *      Main measurement thread
 *---------------------------------------------------------------------------*/
 
typedef enum 
{
	M_STARTUP,
	M_CALIB,
	M_MEASURE,
	
} Measure_state_t;


typedef struct
{
	uint16_t AD_point[3];
  uint16_t refpoint[3]; 	
} meas_cal_t;


static meas_cal_t M_cal;
static Measure_state_t MS = M_STARTUP;
 
void Measure_Thread (void const *argument);                  // thread function
osThreadId tid_Measure_Thread;                               // thread id
osThreadDef (Measure_Thread, osPriorityNormal, 1, 1024);     // thread object

int Init_Measure_Thread (void) {

  tid_Measure_Thread = osThreadCreate (osThread(Measure_Thread), NULL);
  if (!tid_Measure_Thread) return(-1);
  
  return(0);
}

void M_Load_Default_Cal(meas_cal_t *cal)
{
	cal->AD_point[0] = 13000;  cal->refpoint[0] = 0;
	cal->AD_point[1] = 33000;  cal->refpoint[1] = 7000;
	cal->AD_point[2] = 53000;  cal->refpoint[2] = 13500;
}

uint16_t M_pH(uint16_t adc)
{
	double x0, x1, y0, y1, y;
  if ((M_cal.AD_point[2] == 0)
       | (adc <  M_cal.AD_point[1]) )		// 2 point calibration or lower segment
	{
		x0 = M_cal.AD_point[0]; y0 = M_cal.refpoint[0];
		x1 = M_cal.AD_point[1]; y1 = M_cal.refpoint[1];
		y = y0 + ((y1 - y0) / (x1 - x0)) * (((double)adc) - x0);
		return (uint16_t) (y /10);
	}
	else  // 3 point calibration --- upper segment
	{
		x0 = M_cal.AD_point[1]; y0 = M_cal.refpoint[1];
		x1 = M_cal.AD_point[2]; y1 = M_cal.refpoint[2];
		y = y0 + ((y1 - y0) / (x1 - x0)) * (((double)adc) - x0);
		return (uint16_t) (y /10);
	}
  
}


void Measure_Thread (void const *argument) {

	uint16_t adc, ph; 
	char str[16];
	
	while (HD4478_initialized() == 0) osThreadYield ();  // wait for LCD init
	
	LCD_Puts(0,0,"pH meter....");
	LCD_Puts(3,1,"... init...");
	M_Load_Default_Cal(&M_cal);
	
  while (1) {
    
		adc = AD7715_Readout();
		ph = M_pH(adc);
		
		snprintf(str, 16, "AD:%d     ", adc);
		LCD_Puts(0,1,str);
		snprintf(str, 16, "pH:%d.%d     ", ph/100, ph % 100);
		LCD_Puts(0,0,str);
    osThreadYield ();                                           // suspend thread
  }
}
