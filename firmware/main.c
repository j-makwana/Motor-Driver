/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <util/delay.h>
#include <xc.h>
#include "main.h"
#include "timer.h"
#include "uart.h"

#define BUTTON_TIMER_DEBOUNCE 50
#define BUTTON_FORWARD ((PORTB.IN & PIN6_bm) == 0)
#define BUTTON_REVERSE ((PORTB.IN & PIN7_bm) == 0)
#define PWM_FREQ       50000UL
#define PWM_HPER      ((F_CPU / PWM_FREQ) - 1)

// Pinout reference:
// PB6: Button Forward (active low, external pullup)
// PB7: Button Reverse (active low, external pullup)
// PC0: nSLEEP
// PC4: Motor IN1 (PWM)
// PC5: Motor IN2 (PWM)
#if 0
/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
FUSES =
{
   .WDTCFG = PERIOD_2KCLK_gc,
   .BODCFG = LVL_BODLEVEL0_gc | ACTIVE_ENABLED_gc | SLEEP_ENABLED_gc,
   .OSCCFG = FREQSEL_20MHZ_gc,
   .TCD0CFG = 0x00,
   .SYSCFG0 = CRCSRC_NOCRC_gc | RSTPINCFG_GPIO_gc,
   .SYSCFG1 = SUT_64MS_gc
};
#endif

/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
static struct
{
   struct
   {
      timer_t main;
      timer_t button_forward;
      timer_t button_reverse;

   } timer;
   struct{
      bool button_forward;
      bool button_reverse;
   } button;

} runtime;

/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
static int _fputc(char c, FILE* stream)
{
   if (c == '\n')
      uart_tx('\r');

   uart_tx(c);

   return 0;
}

/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
static FILE uart_stdout = FDEV_SETUP_STREAM(_fputc, NULL, _FDEV_SETUP_WRITE);

/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
static void sys_init()
{
   // F_CPU = 20MHz / 4
   //_PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, CLKCTRL_PDIV_4X_gc | CLKCTRL_PEN_bm);
   _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, 0);

   set_sleep_mode(SLEEP_MODE_IDLE);

   stdout = &uart_stdout;
   stderr = &uart_stdout;
}

/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
void console_tx(int c)
{
   _fputc(c, stdout);
}

/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
int console_rx()
{
   int c = uart_rx(false);

   if (c == '\r')
      c = '\n';

   if (isprint(c) || (c == '\n'))
      console_tx(c);

   return c;
}

void motor_init(){
   //PB6, PB7 <- input pins
   //PB6 IN1 PB7 <- IN2
   PORTB.DIRCLR = (PIN6_bm | PIN7_bm);

   ////PC0 nsleep, output pin driven low
   PORTC.DIRSET = PIN0_bm;
   PORTC.OUTCLR = PIN0_bm;

   //PC4 - IN1 - PB6
     //PC5  - IN2 - PB7
   //set output , pwm mode
   PORTC.DIRSET = (PIN4_bm | PIN5_bm);
 

   
}

void disable_pwm(){
   TCA0.SPLIT.CTRLA &= ~TCA_SPLIT_ENABLE_bm;
}

void forward_pwm_init(bool forward){
      //You want to read the value from PB6 and then drive forward
      if(forward){
      PORTC.OUTCLR = PIN4_bm;
      ////////////set pwm up
      //enable split mode
      TCA0.SPLIT.CTRLD = TCA_SINGLE_SPLITM_bm;
      //ENABLE HMP1 PC4
      TCA0.SPLIT.CTRLB = TCA_SPLIT_HCMP1EN_bm;
      TCA0.SPLIT.CTRLA = (TCA0.SPLIT.CTRLA & ~TCA_SPLIT_CLKSEL_gm) | TCA_SPLIT_CLKSEL_DIV1_gc;

      TCA0.SPLIT.HPER = PWM_HPER;
      TCA0.SPLIT.HCMP1 = TCA0.SPLIT.HPER / 2;
      PORTC.OUTSET = PIN0_bm;
      TCA0.SPLIT.CTRLA |= TCA_SPLIT_ENABLE_bm;
      
      //IN2 = 0
      PORTC.OUTCLR = PIN5_bm;

      }else{
         //REVERSE
               PORTC.OUTCLR = PIN5_bm;
               TCA0.SPLIT.CTRLD = TCA_SINGLE_SPLITM_bm;
               TCA0.SPLIT.CTRLB = TCA_SPLIT_HCMP2EN_bm;
               TCA0.SPLIT.CTRLA = (TCA0.SPLIT.CTRLA & ~TCA_SPLIT_CLKSEL_gm) | TCA_SPLIT_CLKSEL_DIV1_gc;
               TCA0.SPLIT.HPER = PWM_HPER;
               TCA0.SPLIT.HCMP2 = TCA0.SPLIT.HPER / 2;
               TCA0.SPLIT.CTRLA |= TCA_SPLIT_ENABLE_bm;
               //IN1
               PORTC.OUTSET = PIN0_bm;
               PORTC.OUTCLR = PIN4_bm;
      }  
   
}

void drive_motor(){
   //you either drive forwards or reverse depending on the button pressed
   if(runtime.button.button_forward && !runtime.button.button_reverse){
      //drive forward
      disable_pwm();
      forward_pwm_init(true);
   
   }else if(runtime.button.button_reverse && !runtime.button.button_forward){
      //drive reverse
      disable_pwm();
      forward_pwm_init(false);
   }

   if(!runtime.button.button_forward && !runtime.button.button_reverse){
      disable_pwm();

   }
}





static void button_update(){
   if(BUTTON_FORWARD == runtime.button.button_forward){
      timer_enable(&runtime.timer.button_forward, false);
      timer_reset(&runtime.timer.button_forward);
   }else{
      timer_enable(&runtime.timer.button_forward, true);
   }
   if(BUTTON_REVERSE == runtime.button.button_reverse){
      timer_enable(&runtime.timer.button_reverse, false);
      timer_reset(&runtime.timer.button_reverse);
   }
   else {
      timer_enable(&runtime.timer.button_reverse, true);
   }

   if(timer_expired(&runtime.timer.button_forward, true))
   {
      runtime.button.button_forward = BUTTON_FORWARD;

   }
   if(timer_expired(&runtime.timer.button_reverse, true)){
      runtime.button.button_reverse = BUTTON_REVERSE;
   }
}
/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
int main()
{
   sys_init();
   uart_init(9600);
   timer_init();
   motor_init();

    _delay_ms(100);

   printf("\n!BOOT %02X\n", RSTCTRL.RSTFR);
   RSTCTRL.RSTFR = RSTCTRL.RSTFR;

   timer_add(&runtime.timer.main, TIMER_FLAG_PERIODIC | TIMER_FLAG_ENABLED, 1000, NULL);
   timer_add(&runtime.timer.button_forward, 0, BUTTON_TIMER_DEBOUNCE, NULL);
   timer_add(&runtime.timer.button_reverse, 0, BUTTON_TIMER_DEBOUNCE, NULL);
   runtime.button.button_forward = false;
   runtime.button.button_reverse = false;

   sei();

   for (;;)
   {
      timer_update();
      button_update();
      drive_motor();

      if (timer_expired(&runtime.timer.main, true))
         printf("Hello, World! ;)");

      sleep_enable();
      sleep_cpu();
      sleep_disable();
   }

   return 0;
}
