#include "stm32f0xx.h"                  // Device header
#include "RTE_Components.h"             // Component selection
#include "cmsis_os.h"                   // ARM::CMSIS:RTOS:Keil RTX


void Encoder_Thread (void const *argument);                             // thread function
osThreadId Encoder_tid_Thread;                                          // thread id
osThreadDef (Encoder_Thread, osPriorityNormal, 1, 0);                   // thread object



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

void Encoder_Init(void)
{
	/** Enable GPIO clocks for GPIOA and GPIOF */
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
	RCC->AHBENR |= RCC_AHBENR_GPIOFEN;

  /* Init GPIO Pins */
	/* Encoder Pin A */
  ENCODER_APORT->MODER   &= ~(3ul << 2*ENCODER_APINn);
  ENCODER_APORT->OSPEEDR &= ~(3ul << 2*ENCODER_APINn);
  ENCODER_APORT->OSPEEDR |=  (1ul << 2*ENCODER_APINn);
  ENCODER_APORT->PUPDR   &= ~(3ul << 2*ENCODER_APINn);
	ENCODER_APORT->PUPDR   |=  (1ul << 2*ENCODER_APINn);

	/* Encoder Pin B */
  ENCODER_BPORT->MODER   &= ~(3ul << 2*ENCODER_BPINn);
  ENCODER_BPORT->OSPEEDR &= ~(3ul << 2*ENCODER_BPINn);
  ENCODER_BPORT->OSPEEDR |=  (1ul << 2*ENCODER_BPINn);
  ENCODER_BPORT->PUPDR   &= ~(3ul << 2*ENCODER_BPINn);
	ENCODER_BPORT->PUPDR   |=  (1ul << 2*ENCODER_BPINn);
	
	/* Encoder Key pin */
  ENCODER_KPORT->MODER   &= ~(3ul << 2*ENCODER_KPINn);
  ENCODER_KPORT->OSPEEDR &= ~(3ul << 2*ENCODER_KPINn);
  ENCODER_KPORT->OSPEEDR |=  (1ul << 2*ENCODER_KPINn);
  ENCODER_KPORT->PUPDR   &= ~(3ul << 2*ENCODER_KPINn);
	ENCODER_KPORT->PUPDR   |=  (1ul << 2*ENCODER_KPINn);
	
	/* Enable SYSCFG Clock */
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

  /* Map EXTI10 line to PA10 */
	SYSCFG->EXTICR[3] &= (uint16_t)~SYSCFG_EXTICR3_EXTI10_PA;
	SYSCFG->EXTICR[3] |= (uint16_t)SYSCFG_EXTICR3_EXTI10_PA;
	
  /* Map EXTI0 line to PF0 */
	SYSCFG->EXTICR[0] &= (uint16_t)~SYSCFG_EXTICR1_EXTI0_PF;
	SYSCFG->EXTICR[0] |= (uint16_t)SYSCFG_EXTICR1_EXTI0_PF;



	/* EXTI0 line interrupts: set falling-edge trigger */
	EXTI->FTSR |= EXTI_FTSR_TR0;
  /* EXTI0 line interrupts: clear rising-edge trigger */
	EXTI->RTSR &= ~EXTI_RTSR_TR0;

	/* EXTI10 line interrupts: set falling-edge trigger */
	EXTI->FTSR |= EXTI_FTSR_TR10;
  /* EXTI10 line interrupts: clear rising-edge trigger */
	EXTI->RTSR &= ~EXTI_RTSR_TR10;

	/* Unmask interrupts from EXTI0 line */
	EXTI->IMR |= EXTI_IMR_MR0;

  /* Unmask interrupts from EXTI10 line */
	EXTI->IMR |= EXTI_IMR_MR10;

	/* Assign EXTI interrupt priority = 0 in NVIC */
	NVIC_SetPriority(EXTI4_15_IRQn, 2);
	NVIC_SetPriority(EXTI0_1_IRQn, 2);
	
	/* Enable EXTI interrupts in NVIC */
	NVIC_EnableIRQ(EXTI4_15_IRQn);			/** PA10 */
	NVIC_EnableIRQ(EXTI0_1_IRQn);			  /** PF0 */
	
}

// Za encoder
void EXTI4_15_IRQHandler(void)
{
	if ((EXTI->PR & EXTI_PR_PR10) != 0)
	{
		EXTI->PR |= EXTI_PR_PR10;
	}
}


// Za tipko....

void EXTI0_1_IRQHandler(void)
{
	if( (EXTI->IMR & EXTI_IMR_MR0) && (EXTI->PR & EXTI_PR_PR0))
	{
		// Clear EXTI interrupt pending flag (EXTI->PR).
		EXTI->PR |= EXTI_PR_PR0 ;
	}
}


int Init_Encoder_Thread (void) {

  Encoder_tid_Thread = osThreadCreate (osThread(Encoder_Thread), NULL);
  if (!Encoder_tid_Thread) return(-1);
  
  return(0);
}


void Encoder_Thread (void const *argument) {
	
  Encoder_Init();
	
  while (1) {
    osThreadYield ();                                           // suspend thread
  }
}




