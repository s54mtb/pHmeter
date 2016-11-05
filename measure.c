#include "cmsis_os.h"                                           // CMSIS RTOS header file
#include "lcd.h"
#include "encoder.h"
#include "ad7715.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

/*----------------------------------------------------------------------------
 *      Main measurement thread
 *---------------------------------------------------------------------------*/
 
typedef enum 
{
	M_MEASURE,
	M_MENU_CAL2,
	M_CAL2_T1,
	M_CAL2_T2,
	M_CAL2_EXIT,
	M_MENU_CAL3,
	M_CAL3_T1,
	M_CAL3_T2,
	M_CAL3_T3,
	M_CAL3_EXIT,
	M_MENU_EXIT,
	
} Measure_state_t;


#define UP 1
#define DN 2

typedef struct
{
	uint16_t AD_point[3];
  uint16_t refpoint[3]; 	
} meas_cal_t;


static meas_cal_t M_cal;
static Measure_state_t MS = M_MEASURE;
 
void Measure_Thread (void const *argument);                  // thread function
osThreadId tid_Measure_Thread;                               // thread id
osThreadDef (Measure_Thread, osPriorityNormal, 1, 1024);     // thread object

int Init_Measure_Thread (void) {

  tid_Measure_Thread = osThreadCreate (osThread(Measure_Thread), NULL);
  if (!tid_Measure_Thread) return(-1);

	Encoder_Set_DestThread(tid_Measure_Thread);
  
  return(0);
}

void M_Load_Default_Cal(meas_cal_t *cal)
{
	/* 3 point cal */
	cal->AD_point[0] = 30610;  cal->refpoint[0] = 4000;
	cal->AD_point[1] = 26650;  cal->refpoint[1] = 7000;
	cal->AD_point[2] = 23940;  cal->refpoint[2] = 9000;

  /* 2 point cal */
	cal->AD_point[0] = 30610;  cal->refpoint[0] = 4000;
	cal->AD_point[1] = 23940;  cal->refpoint[1] = 9000;
	cal->AD_point[2] = 0;  cal->refpoint[2] = 0;

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
	}
	else  // 3 point calibration --- upper segment
	{
		x0 = M_cal.AD_point[1]; y0 = M_cal.refpoint[1];
		x1 = M_cal.AD_point[2]; y1 = M_cal.refpoint[2];
		y = y0 + ((y1 - y0) / (x1 - x0)) * (((double)adc) - x0);
	}
  if (y<0) y = 0;
	if (y>14000) y = 14000;
	return (uint16_t) (y /10);
}

void cls(void)
{
	osDelay(100);
	LCD_Clear();
	osDelay(100);
}

void Update_Readout(uint8_t raw)
{
	int16_t adc, ph; 
	char str[17];

	adc = AD7715_Readout();
	ph = M_pH(adc);
	if (raw)
	{
		snprintf(str, 16, "AD:%d     ", adc);
	}
	else
	{
	  snprintf(str, 16, "pH:%d.%02d     ", ph/100, ph % 100);
	}
	LCD_Puts(0,0,str);
}

void DoCal(uint8_t NumPts, uint8_t Pt)
{
  osEvent evt;
	uint8_t updn = 0, ok = 1, dn = 0;
	uint16_t refpoint, adc1, adc2, stable, diff, i;
	
	char str[17];
	
	refpoint = M_cal.refpoint[Pt-1];
	
	// Set value
	snprintf(str, 16, "Ref. pH(%d)      ",Pt); 
	LCD_Puts(0,0,str);
	while (1)
	{
		// handle encoder signals  
	  evt = osSignalWait (ENCODER_BUTTON, 0);
		if (evt.status == osEventSignal) break; 
		
	  evt = osSignalWait (ENCODER_UP, 0);
		if (evt.status == osEventSignal) updn = UP; 
		
	  evt = osSignalWait (ENCODER_DN, 0);
		if (evt.status == osEventSignal) updn = DN; 
    
		snprintf(str, 16, "Ref pH:%d.%03d       ", refpoint/1000, refpoint % 1000);
		LCD_Puts(0,1,str);
		
		if (updn == DN) 
		{
			if (refpoint > 0) refpoint--;
		}
		if (updn == UP) 
		{
			if (refpoint < 14000) refpoint++;
		}
		updn = 0;
		osThreadYield ();
		
	}

	// confirm ref. value
	while (1)
	{
		// handle encoder signals  
	  evt = osSignalWait (ENCODER_BUTTON, 0);
		if (evt.status == osEventSignal) break; 
		
	  evt = osSignalWait (ENCODER_UP, 0);
		if (evt.status == osEventSignal) updn = UP; 
		
	  evt = osSignalWait (ENCODER_DN, 0);
		if (evt.status == osEventSignal) updn = DN; 
		
		if (ok==1)
		{
		  snprintf(str, 16, "Ref. pH(%d) OK   ",Pt); 
		}
		else
		{
			snprintf(str, 16, "Ref. pH(%d) NIOK ",Pt);
		}
	  LCD_Puts(0,0,str);

		snprintf(str, 16, "Ref pH:%d.%02d       ", refpoint/100, refpoint % 100);
		if (updn != 0) 
		{
			if (ok==1) ok=0; else ok=1;
		}		
		updn = 0;
		osThreadYield ();
	}
	cls();
	
	if (ok == 1)
	{
		// read ADC
	  snprintf(str, 17, "Ref. pH(%d) - CAL",Pt); 
	  LCD_Puts(0,0,str);
		
		stable = 0;
		adc2 = AD7715_Readout();
		// wait for stable result
		while(1)
		{
			diff = 0;
			for (i = 0; i<100; i++)
			{
				adc1 = AD7715_Readout();
				snprintf(str, 16, "AD:%d, S:%d  ", adc1, stable);
				LCD_Puts(0,1,str);
				if (adc1 > adc2) diff += adc1-adc2; else diff += adc2 - adc1;
				adc2 = adc1;
				i++;
			} 
				
			if (diff < 5) stable++; else stable = 0;
			if (stable > 100) break;
      	
			// handle encoder signals  
			evt = osSignalWait (ENCODER_BUTTON, 0);
			if (evt.status == osEventSignal) 
			{ // cancel the process
				ok = 0;
			  break; 
			}
			osThreadYield ();
		}
		
		if (ok)
		{
			cls();
			// Write to flash?
			snprintf(str, 16, "Shranim ?"); 
	    LCD_Puts(0,0,str);
			
			snprintf(str, 16, "T(%d),AD=%d     ", Pt,adc1);
			LCD_Puts(0,1,str);
			
			while (1)
			{
				// handle encoder signals  
				evt = osSignalWait (ENCODER_BUTTON, 0);
				if (evt.status == osEventSignal) break; 
				
				evt = osSignalWait (ENCODER_UP, 0);
				if (evt.status == osEventSignal) updn = UP; 
				
				evt = osSignalWait (ENCODER_DN, 0);
				if (evt.status == osEventSignal) updn = DN; 


				if (dn==1)
				{
					LCD_Puts(11,0," DA "); 
				}
				else
				{
					LCD_Puts(11,0," NE ");
				}

				if (updn != 0) 
				{
					if (dn==1) dn=0; else dn=1;
				}		
				updn = 0;
				
			  osThreadYield ();
			}
			
			if (dn == 1)
			{
				// Store to flash
				
			}
			
		}
		
	}

}


void Measure_Thread (void const *argument) {

  osEvent evt;
	uint8_t baton = 0, updn = 0;

	while (HD4478_initialized() == 0) osThreadYield ();  // wait for LCD init
	LCD_Puts(0,0,"pH meter....");
	LCD_Puts(3,1,"... init...");
	M_Load_Default_Cal(&M_cal);
	
	osDelay(1000);
	LCD_Clear();
	osDelay(100);
	
  while (1) {
		// handle encoder signals
	  evt = osSignalWait (ENCODER_BUTTON, 0);
		if (evt.status == osEventSignal) baton = 1; 
		
	  evt = osSignalWait (ENCODER_UP, 0);
		if (evt.status == osEventSignal) updn = UP; 
		
	  evt = osSignalWait (ENCODER_DN, 0);
		if (evt.status == osEventSignal) updn = DN; 
		
		switch (MS)
		{
			case M_MEASURE :
        Update_Readout(0);	
				LCD_Puts(0,1,"                ");			
			  if (baton == 1) MS = M_MENU_CAL2;
			break;
			
			case M_MENU_CAL2 :
				Update_Readout(1);
				LCD_Puts(0,1,"2 tockovna kal. ");
			  if (updn == UP) MS = M_MENU_EXIT;
			  if (updn == DN) MS = M_MENU_CAL3;
			  if (baton == 1) MS = M_CAL2_T1;
			break;

			case M_MENU_CAL3 :
				Update_Readout(1);
				LCD_Puts(0,1,"3 tockovna kal. ");
			  if (updn == UP) MS = M_MENU_CAL2;
			  if (updn == DN) MS = M_MENU_EXIT;			
			  if (baton == 1) MS = M_CAL3_T1;
			break;

			case M_MENU_EXIT :
				Update_Readout(1);
				LCD_Puts(0,1,"Izhod           ");
			  if (updn == UP) MS = M_MENU_CAL3;
			  if (updn == DN) MS = M_MENU_CAL2;
        if (baton == 1) MS = M_MEASURE;		
			break;
			
			case M_CAL2_T1 :
				Update_Readout(1);
			  LCD_Puts(0,1,"2T Kal: Tocka 1   ");
			  if (updn == UP) MS = M_CAL2_EXIT;
			  if (updn == DN) MS = M_CAL2_T2;
			  if (baton == 1) DoCal(2,1);
			break;

			case M_CAL2_T2 :
				Update_Readout(1);
			  LCD_Puts(0,1,"2T Kal: Tocka 2   ");
			  if (updn == UP) MS = M_CAL2_T1;
			  if (updn == DN) MS = M_CAL2_EXIT;
			  if (baton == 1) DoCal(2,2);
			break;

			case M_CAL2_EXIT :
				Update_Readout(1);
			  LCD_Puts(0,1,"2T Kal: Izhod     ");
			  if (updn == UP) MS = M_CAL2_T2;
			  if (updn == DN) MS = M_CAL2_T1;
			  if (baton == 1) MS = M_MENU_CAL2;
			break;

			case M_CAL3_T1 :
				Update_Readout(1);
			  LCD_Puts(0,1,"3T Kal: Tocka 1   ");
			  if (updn == UP) MS = M_CAL3_EXIT;
			  if (updn == DN) MS = M_CAL3_T2;
			  if (baton == 1) DoCal(3,1);
			break;
			
			case M_CAL3_T2 :
				Update_Readout(1);
			  LCD_Puts(0,1,"3T Kal: Tocka 2   ");
			  if (updn == UP) MS = M_CAL3_T1;
			  if (updn == DN) MS = M_CAL3_T3;
			  if (baton == 1) DoCal(3,2);
			break;
			
			case M_CAL3_T3 :
				Update_Readout(1);
			  LCD_Puts(0,1,"3T Kal: Tocka 3   ");
			  if (updn == UP) MS = M_CAL3_T2;
			  if (updn == DN) MS = M_CAL3_EXIT;
			  if (baton == 1) DoCal(3,3);
			break;
			
			case M_CAL3_EXIT :
				Update_Readout(1);
			  LCD_Puts(0,1,"3T Kal: Izhod     ");
			  if (updn == UP) MS = M_CAL3_T3;
			  if (updn == DN) MS = M_CAL3_T1;
			  if (baton == 1) MS = M_MENU_CAL3;
			break;
			
		}
		
		baton = 0;
		updn = 0;
		
    osThreadYield ();                                           // suspend thread
  }
}
