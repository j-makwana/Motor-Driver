/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stddef.h>
#include <util/atomic.h>
#include "main.h"
#include "timer.h"

/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
#ifndef TIMER_MSEC
#define TIMER_MSEC 1
#endif

/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
static timer_t* timers[2];
static volatile uint32_t ticks;

/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
static void _timer_update(timer_t* list, uint32_t value)
{
   while (list != NULL)
   {
      if ((list->flags & TIMER_FLAG_ENABLED) && (list->value.reset > 0))
      {
         if (list->flags & TIMER_FLAG_COUNTUP)
         {
            list->value.current += value;

            if (list->value.current >= list->value.reset)
            {
               if (list->flags & TIMER_FLAG_PERIODIC)
                  list->value.current = 0;
               else
                  list->flags &= ~TIMER_FLAG_ENABLED;

               if (list->flags & TIMER_FLAG_EXPIRED)
                  list->flags |= TIMER_FLAG_OVERFLOW;
               else
                  list->flags |= TIMER_FLAG_EXPIRED;

               if (list->fx != NULL)
               {
                  list->flags |= TIMER_FLAG_CALLBACK;
                  list->fx(list);
                  list->flags &= ~TIMER_FLAG_CALLBACK;
               }
            }
         }
         else
         {
            if (list->value.current <= value)
            {
               if (list->flags & TIMER_FLAG_PERIODIC)
                  list->value.current = list->value.reset;
               else
                  list->flags &= ~TIMER_FLAG_ENABLED;

               if (list->flags & TIMER_FLAG_EXPIRED)
                  list->flags |= TIMER_FLAG_OVERFLOW;
               else
                  list->flags |= TIMER_FLAG_EXPIRED;

               if (list->fx != NULL)
               {
                  list->flags |= TIMER_FLAG_CALLBACK;
                  list->fx(list);
                  list->flags &= ~TIMER_FLAG_CALLBACK;
               }
            }
            else
            {
               list->value.current -= value;
            }
         }
      }

      list = list->next;
   }
}

/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
ISR(RTC_CNT_vect)
{
   ticks += TIMER_MSEC;
   _timer_update(timers[0], TIMER_MSEC);
   RTC.INTFLAGS = RTC.INTFLAGS;
}

/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
void timer_add(timer_t* timer, uint16_t flags, uint32_t value, void (*fx)(timer_t*))
{
   if (value > 0)
   {
      if (value < TIMER_MSEC)
         value = TIMER_MSEC;
   }
   else
   {
      flags &= ~TIMER_FLAG_ENABLED;
   }

   timer->flags = flags;

   if (flags & TIMER_FLAG_COUNTUP)
      timer->value.current = 0;
   else
      timer->value.current = value;

   timer->value.reset = value;
   timer->fx = fx;

   ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
   {
      if (flags & TIMER_FLAG_ASYNC)
      {
         timer->next = timers[0];
         timers[0] = timer;
      }
      else
      {
         timer->next = timers[1];
         timers[1] = timer;
      }
   }
}

/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
bool timer_expired(timer_t* timer, bool clear)
{
   bool value;

   ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
   {
      value = (timer->flags & TIMER_FLAG_EXPIRED) ? true : false;

      if (clear)
         timer->flags &= ~TIMER_FLAG_EXPIRED;
   }

   return value;
}

/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
bool timer_overflowed(timer_t* timer, bool clear)
{
   bool value;

   ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
   {
      value = (timer->flags & TIMER_FLAG_OVERFLOW) ? true : false;

      if (clear)
         timer->flags &= ~TIMER_FLAG_OVERFLOW;
   }

   return value;
}

/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
void timer_enable(timer_t* timer, bool enable)
{
   ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
   {
      if (timer->value.reset == 0)
         timer->flags &= ~TIMER_FLAG_PERIODIC;

      if (enable)
      {
         if (timer->value.current > 0)
            timer->flags |= TIMER_FLAG_ENABLED;
         else
            timer->flags |= TIMER_FLAG_EXPIRED;
      }
      else
      {
         timer->flags &= ~TIMER_FLAG_ENABLED;
      }
   }
}

/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
void timer_expire(timer_t* timer)
{
   ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
   {
      timer->flags |= TIMER_FLAG_EXPIRED;

      if (timer->flags & TIMER_FLAG_PERIODIC)
      {
         if (timer->flags & TIMER_FLAG_COUNTUP)
            timer->value.current = 0;
         else
            timer->value.current = timer->value.reset;
      }
      else
      {
         if (timer->flags & TIMER_FLAG_COUNTUP)
            timer->value.current = timer->value.reset;
         else
            timer->value.current = 0;

         timer->flags &= ~TIMER_FLAG_ENABLED;
      }
   }
}

/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
void timer_reset(timer_t* timer)
{
   ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
   {
      timer->flags &= ~(TIMER_FLAG_OVERFLOW | TIMER_FLAG_EXPIRED);

      if (timer->flags & TIMER_FLAG_COUNTUP)
         timer->value.current = 0;
      else
         timer->value.current = timer->value.reset;
   }
}

/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
uint32_t timer_get(timer_t* timer, uint32_t* reset)
{
   uint32_t value;

   ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
   {
      value = timer->value.current;

      if (reset != NULL)
         *reset = timer->value.reset;
   }

   return value;
}

/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
void timer_set(timer_t* timer, uint32_t current, uint32_t reset)
{
   ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
   {
      timer->flags &= ~(TIMER_FLAG_OVERFLOW | TIMER_FLAG_EXPIRED);
      timer->value.current = current;
      timer->value.reset = reset;

      if (reset == 0)
         timer->flags &= ~TIMER_FLAG_PERIODIC;
   }
}

/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
void timer_update()
{
   uint32_t ticks0;

   ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
   {
      ticks0 = ticks;
      ticks = 0;
   }

   if (ticks0 > 0)
      _timer_update(timers[1], ticks0);
}

/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
void timer_init()
{
#if TIMER_MSEC == 1
   RTC.CMP = 32;
   RTC.PER = 32;
#elif TIMER_MSEC == 2
   RTC.CMP = 64;
   RTC.PER = 64;
#endif
   RTC.INTCTRL = RTC_CMP_bm;
   RTC.CTRLA = RTC_RTCEN_bm;
}
