/*
  Copyright (c) 2015 Arduino LLC.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "Arduino.h"

#ifdef __cplusplus
 extern "C" {
#endif

/* Error processing */

static uint8_t errorNum=0;
static uint8_t errorId=0xff;
unsigned int wiring_digital_errorNum() {return (unsigned int)errorNum;}

#if defined WIRING_DIGITAL_DEBUG
void dbg_str(char *dbgStr);
void dbg_uint8(uint8_t ulPin);
#endif 

void __errorNow(char id)
{
  if (0xff != errorNum)
  {
    errorNum++;
    errorId=id;
  } 
}
void errorNow(char id) __attribute__((weak, alias ("__errorNow")));

void __pinModExt( uint32_t ulPin, uint32_t ulMode ) {errorNow(0);}
void pinModExt(uint32_t ulPin, uint32_t ulMode) __attribute__((weak, alias ("__pinModExt")));

void __digitalWrExt( uint32_t ulPin, uint32_t ulMode ) {errorNow(1);}
void digitalWrExt(uint32_t ulPin, uint32_t ulMode) __attribute__((weak, alias ("__digitalWrExt")));

int __digitalRdExt( uint32_t ulPin ) {errorNow(2);return 0;}
int digitalRdExt( uint32_t ulPin )  __attribute__((weak, alias ("__digitalRdExt")));

void pinMode( uint32_t ulPin, uint32_t ulMode )
{
  // Handle the case the pin isn't usable as PIO
  if ( g_APinDescription[ulPin].ulPinType == PIO_NOT_A_PIN )
  {
    return ;
  }
  if ( ulPin >= thisVariantNumPins)
  {
    pinModExt( ulPin, ulMode );
    #if defined WIRING_DIGITAL_DEBUG
    dbg_str(" pinMode err=");
    dbg_uint8(ulPin);
    #endif

    return ;
  }
  EPortType port = g_APinDescription[ulPin].ulPort;
  uint32_t pin = g_APinDescription[ulPin].ulPin;
  uint32_t pinMask = (1ul << pin);

  // Set pin mode according to chapter '22.6.3 I/O Pin Configuration'
  switch ( ulMode )
  {
    case INPUT:
      // Set pin to input mode
      PORT->Group[port].PINCFG[pin].reg=(uint8_t)(PORT_PINCFG_INEN) ;
      PORT->Group[port].DIRCLR.reg = pinMask ;
    break ;

    case INPUT_PULLUP:
      // Set pin to input mode with pull-up resistor enabled
      PORT->Group[port].PINCFG[pin].reg=(uint8_t)(PORT_PINCFG_INEN|PORT_PINCFG_PULLEN) ;
      PORT->Group[port].DIRCLR.reg = pinMask ;

      // Enable pull level (cf '22.6.3.2 Input Configuration' and '22.8.7 Data Output Value Set')
      PORT->Group[port].OUTSET.reg = pinMask ;
    break ;

    case INPUT_PULLDOWN:
      // Set pin to input mode with pull-down resistor enabled
      PORT->Group[port].PINCFG[pin].reg=(uint8_t)(PORT_PINCFG_INEN|PORT_PINCFG_PULLEN) ;
      PORT->Group[port].DIRCLR.reg = pinMask ;

      // Enable pull level (cf '22.6.3.2 Input Configuration' and '22.8.6 Data Output Value Clear')
      PORT->Group[port].OUTCLR.reg = pinMask ;
    break ;

    case OUTPUT:
      // enable input, to support reading back values, with pullups disabled
      PORT->Group[port].PINCFG[pin].reg=(uint8_t)(PORT_PINCFG_INEN) ;

      // Set pin to output mode
      PORT->Group[port].DIRSET.reg = pinMask ;
    break ;

    default:
      // do nothing
    break ;
  }
}

void digitalWrite( uint32_t ulPin, uint32_t ulVal )
{
  // Handle the case the pin isn't usable as PIO
  if ( g_APinDescription[ulPin].ulPinType == PIO_NOT_A_PIN )
  {
    return ;
  }
  if ( ulPin >= thisVariantNumPins)
  {
    digitalWrExt( ulPin, ulVal );
    #if defined WIRING_DIGITAL_DEBUG
    dbg_str(" digitalWrite err=");
    dbg_uint8(ulPin);
    #endif
    return ;
  }
  EPortType port = g_APinDescription[ulPin].ulPort;
  uint32_t pin = g_APinDescription[ulPin].ulPin;
  uint32_t pinMask = (1ul << pin);

  if ( (PORT->Group[port].DIRSET.reg & pinMask) == 0 ) {
    // the pin is not an output, disable pull-up if val is LOW, otherwise enable pull-up
    PORT->Group[port].PINCFG[pin].bit.PULLEN = ((ulVal == LOW) ? 0 : 1) ;
  }

  switch ( ulVal )
  {
    case LOW:
      PORT->Group[port].OUTCLR.reg = pinMask;
    break ;

    default:
      PORT->Group[port].OUTSET.reg = pinMask;
    break ;
  }

  return ;
}

int digitalRead( uint32_t ulPin )
{
  // Handle the case the pin isn't usable as PIO
  if ( g_APinDescription[ulPin].ulPinType == PIO_NOT_A_PIN )
  {
    return LOW ;
  }
  if ( ulPin >= thisVariantNumPins)
  {
    digitalRdExt( ulPin );
    #if defined WIRING_DIGITAL_DEBUG
    dbg_str(" digitalRead err=");
    dbg_uint8(ulPin);
    #endif
    return LOW ;
  }

  if ( (PORT->Group[g_APinDescription[ulPin].ulPort].IN.reg & (1ul << g_APinDescription[ulPin].ulPin)) != 0 )
  {
    return HIGH ;
  }

  return LOW ;
}

#ifdef __cplusplus
}
#endif

