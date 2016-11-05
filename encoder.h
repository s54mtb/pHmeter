/**
  ******************************************************************************
  * @file    encoder.h
  * @author  e.pavlin.si
  * @brief   rotary encoder header file
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



#ifndef __ENCODER_H__
#define __ENCODER_H__


/// Encoder pinout definitions
//A - Pin for encoder "A" pin ---> triggers EXTI
#define ENCODER_APORT      GPIOA
#define ENCODER_APINn      10

//B - Pin for encoder "B" pin 
#define ENCODER_BPORT      GPIOA
#define ENCODER_BPINn      9

//A - Pin for encoder "K" pin ---> KEy, triggers EXTI
#define ENCODER_KPORT      GPIOF
#define ENCODER_KPINn      0

#define ENCODER_APIN		((uint16_t)(1U<<ENCODER_APINn))
#define ENCODER_BPIN		((uint16_t)(1U<<ENCODER_BPINn))
#define ENCODER_KPIN		((uint16_t)(1U<<ENCODER_KPINn))

#define ENCODER_BUTTON 0x00000001
#define ENCODER_UP     0x00000002
#define ENCODER_DN     0x00000004

void Encoder_Set_DestThread(osThreadId tid);
int16_t Encoder_State(void);

#endif

