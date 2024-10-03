/*********************************Copyright (c)********************************* 
**                                
**                                 FIVE??? 
** 
**---------------------------------File Info------------------------------------ 
** File Name:               shell_hal.h 
** Last modified Date:      2014/5/26 14:22:35 
** Last Version:            V1.0   
** Description:             ??Shell???? 
** 
**------------------------------------------------------------------------------ 
** Created By:              wanxuncpx 
** Created date:            2014/5/26 14:22:34 
** Version:                 V2 
** Descriptions:            ???STM32?? 
**------------------------------------------------------------------------------ 
** Libraries:               STM32F10x_StdPeriph_Driver 
** version                  V3.5 
*******************************************************************************/  
  
/****************************************************************************** 
????: 
******************************************************************************/  
  
/****************************************************************************** 
*********************************  ? ? ? ? ******************************** 
******************************************************************************/  
  
#ifndef _SHELL_HAL_  
#define _SHELL_HAL_  
/****************************************************************************** 
********************************* ?????? ******************************** 
******************************************************************************/  
//?????  
#include "main.h"
  
/****************************************************************************** 
******************************** ? ? ? ? ? ******************************* 
******************************** MNCS_IMAGE??? ***************************** 
******************************************************************************/  
/*---------------------*  
*     UART???? 
*----------------------*/  
//IO??  
#define CONSOLE                 USART3   
#define CONSOLE_TX_PORT         GPIOB  
#define CONSOLE_TX_PIN          GPIO_Pin_10  
#define CONSOLE_RX_PORT         GPIOB  
#define CONSOLE_RX_PIN          GPIO_Pin_11  
  
//????  
#define CONSOLE_GPIO_RCC_INIT() RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE)  
#define CONSOLE_UART_RCC_INIT() RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,ENABLE)  
  
//?????  
#define CONSOLE_UART_PRIO       7       //??[0..15]  
  
//??????  
#define CONSOLE_IRQn            USART3_IRQn;  
#define CONSOLE_IRQHandler      USART3_IRQHandler  
  
/*---------------------*  
*     ??LED?? 
*----------------------*/  
#define LED0_VALID              1           //?????????LED,0:??  
#define LED0_PORT               GPIOB  
#define LED0_PIN                GPIO_Pin_13  
  
#define LED1_VALID              1           //?????????LED,0:??  
#define LED1_PORT               GPIOB  
#define LED1_PIN                GPIO_Pin_15  
  
#define LED2_VALID              0           //?????????LED,0:??  
#define LED2_PORT               GPIOA  
#define LED2_PIN                GPIO_Pin_11  
  
#define LED3_VALID              0           //?????????LED,0:??  
#define LED3_PORT               GPIOA  
#define LED3_PIN                GPIO_Pin_11  
  
#define LED4_VALID              0           //?????????LED,0:??  
#define LED4_PORT               GPIOA  
#define LED4_PIN                GPIO_Pin_11  
  
#define LED5_VALID              0           //?????????LED,0:??  
#define LED5_PORT               GPIOA  
#define LED5_PIN                GPIO_Pin_11  
  
/*---------------------*  
*        ??BASE 
*----------------------*/  
#define TIMEDly                 TIM4  
#define TIMEDly_IRQn            TIM4_IRQn  
#define TIMEDly_IRQHandler      TIM4_IRQHandler  
  
//????              
#define TIMEDly_RCC_INIT()      RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);  
  
//???LGPIO?  
#define LEDx_GPIO_RCC_INIT()    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE)  
//------------------------------------------------------------------------------  
  
/****************************************************************************** 
******************************* ???????? ****************************** 
******************************************************************************/  
/*---------------------*  
*  ???????????? 
*----------------------*/  
#if LED0_VALID  
  #define LED0_ON()             (LED0_PORT->BRR  = LED0_PIN)  
  #define LED0_OFF()            (LED0_PORT->BSRR = LED0_PIN)  
  #define LED0_DIV()            (LED0_PORT->ODR  ^= LED0_PIN)  
#else  
  #define LED0_ON()             __NOP()  
  #define LED0_OFF()            __NOP()  
  #define LED0_DIV()            __NOP()  
#endif  
  
#if LED1_VALID  
  #define LED1_ON()             (LED1_PORT->BRR  = LED1_PIN)  
  #define LED1_OFF()            (LED1_PORT->BSRR = LED1_PIN)  
  #define LED1_DIV()            (LED1_PORT->ODR ^= LED1_PIN)  
#else  
  #define LED1_ON()             __NOP()  
  #define LED1_OFF()            __NOP()  
  #define LED1_DIV()            __NOP()  
#endif   
  
#if LED2_VALID  
  #define LED2_ON()             (LED2_PORT->BRR  = LED2_PIN)  
  #define LED2_OFF()            (LED2_PORT->BSRR = LED2_PIN)  
  #define LED2_DIV()            (LED2_PORT->ODR ^= LED2_PIN)  
#else  
  #define LED2_ON()             __NOP()  
  #define LED2_OFF()            __NOP()  
  #define LED2_DIV()            __NOP()  
#endif    
  
#if LED3_VALID  
  #define LED3_ON()             (LED3_PORT->BRR  = LED3_PIN)  
  #define LED3_OFF()            (LED3_PORT->BSRR = LED3_PIN)  
  #define LED3_DIV()            (LED3_PORT->ODR ^= LED3_PIN)  
#else  
  #define LED3_ON()             __NOP()  
  #define LED3_OFF()            __NOP()  
  #define LED3_DIV()            __NOP()  
#endif  
  
#if LED4_VALID  
  #define LED4_ON()             (LED4_PORT->BSRR = LED4_PIN)  
  #define LED4_OFF()            (LED4_PORT->BRR  = LED4_PIN)  
  #define LED4_DIV()            (LED4_PORT->ODR ^= LED4_PIN)  
#else  
  #define LED4_ON()             __NOP()  
  #define LED4_OFF()            __NOP()  
  #define LED4_DIV()            __NOP()  
#endif  
  
#if LED5_VALID  
  #define LED5_ON()             (LED5_PORT->BSRR = LED5_PIN)  
  #define LED5_OFF()            (LED5_PORT->BRR  = LED5_PIN)  
  #define LED5_DIV()            (LED5_PORT->ODR ^= LED5_PIN)  
#else  
  #define LED5_ON()             __NOP()  
  #define LED5_OFF()            __NOP()  
  #define LED5_DIV()            __NOP()  
#endif  
/****************************************************************************** 
******************************* printf???? ******************************** 
******************************************************************************/  
/* Private function prototypes -----------------------------------------------*/  
#ifdef __GNUC__  
/* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf 
   set to 'Yes') calls __io_putchar() */  
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)  
#else  
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)  
#endif /* __GNUC__ */  
  
/****************************************************************************** 
***********************************   END  ************************************ 
******************************************************************************/  
#endif  
