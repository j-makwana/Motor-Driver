/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>
#include <util/atomic.h>
#include "main.h"
#include "uart.h"

/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
#if UART_TX_BUFFER_SIZE > 0
static volatile struct
{
   uint8_t buffer[UART_TX_BUFFER_SIZE];
   size_t ptr;
   size_t count;

} tx0;
#endif

/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
static volatile struct
{
#if UART_RX_BUFFER_SIZE > 0
   uint8_t buffer[UART_RX_BUFFER_SIZE];
   size_t ptr;
   size_t count;
#endif
   size_t errors;

} rx0;

/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
ISR(USART0_TXC_vect)
{
   USART0.STATUS = USART_TXCIF_bm;
}

/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
#if UART_TX_BUFFER_SIZE > 0
ISR(USART0_DRE_vect)
{
   if (tx0.count > 0)
   {
      USART0.TXDATAL = tx0.buffer[tx0.ptr++ % UART_TX_BUFFER_SIZE];
      tx0.count--;
   }

   if (tx0.count == 0)
      USART0.CTRLA &= ~USART_DREIE_bm;
}
#endif

/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
#if UART_RX_BUFFER_SIZE > 0
ISR(USART0_RXC_vect)
{
   USART0.STATUS = USART_ISFIF_bm;

   if (USART0.RXDATAH & (USART_BUFOVF_bm | USART_FERR_bm | USART_PERR_bm))
   {
      USART0.RXDATAL;
      rx0.errors++;
   }
   else
   {
      if (rx0.count < UART_RX_BUFFER_SIZE)
         rx0.buffer[(rx0.ptr + rx0.count++) % UART_RX_BUFFER_SIZE] = USART0.RXDATAL;
      else
         rx0.buffer[rx0.ptr++ % UART_RX_BUFFER_SIZE] = USART0.RXDATAL;
   }
}
#endif

/********************************************************************************************************************
 *
 ********************************************************************************************************************/
void uart_tx_enable(bool enable)
{
   if (enable)
   {
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
      {
         USART0.CTRLB |= USART_TXEN_bm;
#if UART_TX_BUFFER_SIZE > 0
         if (tx0.count > 0)
            USART0.CTRLA |= USART_DREIE_bm;
#endif
      }
   }
   else
   {
#if UART_TX_BUFFER_SIZE > 0
      USART0.CTRLA &= ~USART_DREIE_bm;
      while ((USART0.STATUS & USART_DREIF_bm) == 0);
#endif
      USART0.CTRLB &= ~USART_TXEN_bm;
   }
}

/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
void uart_tx(int c)
{
   uint8_t c8 = (uint8_t) c;

#if UART_TX_BUFFER_SIZE > 0
   if (CPU_SREG & CPU_I_bm)
   {
      bool success = false;

      do
      {
         ATOMIC_BLOCK(ATOMIC_FORCEON)
         {
            if (tx0.count < UART_TX_BUFFER_SIZE)
            {
               tx0.buffer[(tx0.ptr + tx0.count++) % UART_TX_BUFFER_SIZE] = c8;
               success = true;
            }

            if (USART0.CTRLB & USART_TXEN_bm)
               USART0.CTRLA |= USART_DREIE_bm;
         }

      } while (!success && (USART0.CTRLB & USART_TXEN_bm));
   }
   else
#endif
   {
      if (USART0.CTRLB & USART_TXEN_bm)
      {
         while ((USART0.STATUS & USART_DREIF_bm) == 0);

#if UART_TX_BUFFER_SIZE > 0
         while (tx0.count > 0)
         {
            USART0.TXDATAL = tx0.buffer[tx0.ptr++ % UART_TX_BUFFER_SIZE];
            while ((USART0.STATUS & USART_DREIF_bm) == 0);
            tx0.count--;
         }
#endif

         USART0.STATUS = USART_TXCIF_bm;
         USART0.TXDATAL = c8;

         while ((USART0.STATUS & USART_TXCIF_bm) == 0);
         USART0.STATUS = USART_TXCIF_bm;
      }
   }
}

/********************************************************************************************************************
 *
 ********************************************************************************************************************/
size_t uart_tx_length()
{
   size_t length = 0;

   if (USART0.CTRLB & USART_TXEN_bm)
   {
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
      {
         length = (USART0.STATUS & USART_TXCIF_bm) ? 0 : 1;
#if UART_TX_BUFFER_SIZE > 0
         length += tx0.count;
#endif
      }
   }

   return length;
}

/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
void uart_tx_flush()
{
   if (USART0.CTRLB & USART_TXEN_bm)
   {
      while (uart_tx_length() > 0);
   }
   else
   {
#if UART_TX_BUFFER_SIZE > 0
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
      {
         tx0.count = 0;
      }
#endif
   }
}

/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
void uart_rx_enable(bool enable)
{
   if (enable)
      USART0.CTRLB |= USART_RXEN_bm;
   else
      USART0.CTRLB &= ~USART_RXEN_bm;
}

/********************************************************************************************************************
 *
 ********************************************************************************************************************/
int uart_rx(bool blocking)
{
   int c = EOF;

#if UART_RX_BUFFER_SIZE > 0
   if (CPU_SREG & CPU_I_bm)
   {
      do
      {
         ATOMIC_BLOCK(ATOMIC_FORCEON)
         {
            if (rx0.count > 0)
            {
               c = rx0.buffer[rx0.ptr++ % UART_RX_BUFFER_SIZE];
               rx0.count--;
            }
         }

      } while (blocking && (c == EOF));
   }
   else
#endif
   {
#if UART_RX_BUFFER_SIZE > 0
      if (rx0.count > 0)
      {
         c = rx0.buffer[rx0.ptr++ % UART_RX_BUFFER_SIZE];
         rx0.count--;
      }
      else
#endif
      {
         do
         {
            if (USART0.RXDATAH & (USART_BUFOVF_bm | USART_FERR_bm | USART_PERR_bm))
               rx0.errors++;

            if (USART0.STATUS & USART_RXCIF_bm)
               c = USART0.RXDATAL;

         } while (blocking && (c == EOF));
      }
   }

   return c;
}

/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
size_t uart_rx_errors(bool reset)
{
   size_t errors;

   ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
   {
      errors = rx0.errors;

      if (reset)
         rx0.errors = 0;
   }

   return errors;
}

/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
int uart_rx_peek(size_t index)
{
   int c = EOF;

#if UART_RX_BUFFER_SIZE > 0
   ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
   {
      if (index < rx0.count)
         c = rx0.buffer[(rx0.ptr + index) % UART_RX_BUFFER_SIZE];
   }
#endif

   return c;
}

/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
size_t uart_rx_length()
{
   size_t length = 0;

#if UART_RX_BUFFER_SIZE > 0
   ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
   {
      length = rx0.count;
   }
#endif

   return length;
}

/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
void uart_rx_flush()
{
#if UART_RX_BUFFER_SIZE > 0
   ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
   {
      rx0.ptr = 0;
      rx0.count = 0;
   }
#endif
}

/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
void uart_close()
{
   ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
   {
#if UART_RX_BUFFER_SIZE > 0
      USART0.CTRLA &= ~USART_RXCIE_bm;
#endif
      USART0.CTRLA &= ~USART_DREIE_bm;
      USART0.CTRLB &= ~(USART_RXEN_bm | USART_TXEN_bm);

      USART0.RXDATAL;
      USART0.STATUS = USART_TXCIF_bm;

#ifdef UART_ALTERNATE_PINS
      PORTA.DIRCLR = PIN1_bm;
      PORTA.DIRCLR = PIN2_bm;
#else
      PORTB.DIRCLR = PIN3_bm;
      PORTB.DIRCLR = PIN2_bm;
#endif
   }
}

/*******************************************************************************************************************
 *
 *******************************************************************************************************************/
void uart_init(uint32_t baud)
{
#ifdef UART_ALTERNATE_PINS
   PORTA.DIRSET = PIN1_bm;
   PORTA.DIRCLR = PIN2_bm;
   PORTMUX.CTRLB |= PORTMUX_USART0_ALTERNATE_gc;
#else
   PORTB.DIRSET = PIN3_bm;
   PORTB.DIRCLR = PIN2_bm;
#endif

   USART0.BAUD = F_CPU * 64UL / (16UL * baud);
   USART0.CTRLB |= (USART_RXEN_bm | USART_TXEN_bm);

#if UART_RX_BUFFER_SIZE > 0
   USART0.CTRLA |= USART_RXCIE_bm;
#endif
}
