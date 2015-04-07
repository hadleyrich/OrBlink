/* Name: main.c
 * Project: hid-custom-rq example
 * Author: Christian Starkjohann
 * Creation Date: 2008-04-07
 * Tabsize: 4
 * Copyright: (c) 2008 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: GNU GPL v2 (see License.txt), GNU GPL v3 or proprietary (CommercialLicense.txt)
 * This Revision: $Id: main.c 692 2008-11-07 15:07:40Z cs $
 */

/*
This example should run on most AVRs with only little changes. No special
hardware resources except INT0 are used. You may have to change usbconfig.h for
different I/O pins for USB. Please note that USB D+ must be the INT0 pin, or
at least be connected to INT0 as well.
We assume that an LED is connected to port B bit 0. If you connect it to a
different port or bit, change the macros below:
*/
#define LED_PORT_DDR        DDRB
#define LED_PORT_OUTPUT     PORTB
#define R_BIT            1
#define G_BIT            0
#define B_BIT            4


#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>  /* for sei() */
#include <util/delay.h>     /* for _delay_ms() */

#include <avr/pgmspace.h>   /* required by usbdrv.h */
extern "C"
{
	#include "usbdrv.h"
}
#include "oddebug.h"        /* This is also an example for using debug macros */
#include "requests.h"       /* The custom request numbers we use */

/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

PROGMEM const char usbHidReportDescriptor[22] = {    /* USB report descriptor */
    0x06, 0x00, 0xff,              // USAGE_PAGE (Generic Desktop)
    0x09, 0x01,                    // USAGE (Vendor Usage 1)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)
    0xc0                           // END_COLLECTION
};
/* The descriptor above is a dummy only, it silences the drivers. The report
 * it describes consists of one byte of undefined data.
 * We don't transfer our data through HID reports, we use custom requests
 * instead.
 */

uint8_t eeprom_r EEMEM = 0;
uint8_t eeprom_g EEMEM = 0;
uint8_t eeprom_b EEMEM = 0;

static uint8_t count = 0;
static volatile uint8_t r = 0;
static volatile uint8_t g = 0;
static volatile uint8_t b = 0;
static uint8_t rb = 0;
static uint8_t gb = 0;
static uint8_t bb = 0;
static uint8_t st = 0;

/* ------------------------------------------------------------------------- */

extern "C" usbMsgLen_t usbFunctionSetup(uchar data[8])
{
	usbRequest_t    *rq = (usbRequest_t *)data;

    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_VENDOR)
	{
		switch(rq->bRequest)
		{
		case CUSTOM_RQ_SET_RED:
			r = rq->wValue.bytes[0];
			break;	
		case CUSTOM_RQ_SET_GREEN:
			g = rq->wValue.bytes[0];
			break;	
		case CUSTOM_RQ_SET_BLUE:
			b = rq->wValue.bytes[0];
			break;	
		case CUSTOM_RQ_STORE:
			st = 1;
			break;
		}
    }
	else
	{
        /* class requests USBRQ_HID_GET_REPORT and USBRQ_HID_SET_REPORT are
         * not implemented since we never call them. The operating system
         * won't call them either because our descriptor defines no meaning.
         */
    }
    return 0;   /* default for not implemented requests: return no data back to host */
}

static void calibrateOscillator(void)
{
uchar       step = 128;
uchar       trialValue = 0, optimumValue;
int         x, optimumDev, targetValue = (unsigned)(1499 * (double)F_CPU / 10.5e6 + 0.5);
 
    /* do a binary search: */
    do{
        OSCCAL = trialValue + step;
        x = usbMeasureFrameLength();    // proportional to current real frequency
        if(x < targetValue)             // frequency still too low
            trialValue += step;
        step >>= 1;
    }while(step > 0);
    /* We have a precision of +/- 1 for optimum OSCCAL here */
    /* now do a neighborhood search for optimum value */
    optimumValue = trialValue;
    optimumDev = x; // this is certainly far away from optimum
    for(OSCCAL = trialValue - 1; OSCCAL <= trialValue + 1; OSCCAL++){
        x = usbMeasureFrameLength() - targetValue;
        if(x < 0)
            x = -x;
        if(x < optimumDev){
            optimumDev = x;
            optimumValue = OSCCAL;
        }
    }
    OSCCAL = optimumValue;
}
 

extern "C" void usbEventResetReady(void)
{
    cli();  // usbMeasureFrameLength() counts CPU cycles, so disable interrupts.
    calibrateOscillator();
    sei();
    eeprom_write_byte(0, OSCCAL);   // store the calibrated value in EEPROM
}

/* ------------------------------------------------------------------------- */
void update_leds()
{
  if (++count == 0) {    
    rb = r;
    gb = g;
    bb = b;
    // check if switch on r, g and b
    if (rb > 0) {        
      PORTB |= (1 << R_BIT);
    } 
    if (gb > 0) {
      PORTB |= (1 << G_BIT);
    } 
    if (bb > 0) {
      PORTB |= (1 << B_BIT);
    } 
  }
  // check if switch off r, g and b
  if (count == rb) {     
    PORTB &= ~(1 << R_BIT);
  }
  if (count == gb) {
    PORTB &= ~(1 << G_BIT);
  }
  if (count == bb) {
    PORTB &= ~(1 << B_BIT);
  }
}

void update_leds2()
{
    /* PWM enable,  */
    GTCCR |= _BV(PWM1B) | _BV(COM1B1);

    TCCR0A |= _BV(WGM00) | _BV(WGM01) | _BV(COM0A1) | _BV(COM0B1);

    /* Start timer 0 and 1 */
    TCCR1 |= _BV(CS10);

    TCCR0B |=  _BV(CS00);

    /* Set PWM value to 0. */
    OCR0A = 255;   // PB0
    OCR0B = 255;   // PB1
    OCR1B = 255;   // PB4
}

int main(void)
{
	uchar   i;

    r = eeprom_read_byte(&eeprom_r);
    g = eeprom_read_byte(&eeprom_g);
    b = eeprom_read_byte(&eeprom_b);

    wdt_enable(WDTO_1S);
    /* Even if you don't use the watchdog, turn it off here. On newer devices,
     * the status of the watchdog (on/off, period) is PRESERVED OVER RESET!
     */
    /* RESET status: all port bits are inputs without pull-up.
     * That's the way we need D+ and D-. Therefore we don't need any
     * additional hardware initialization.
     */

    usbInit();
    usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */
    i = 0;
    while(--i){             /* fake USB disconnect for > 250 ms */
        wdt_reset();
        _delay_ms(1);
    }
    usbDeviceConnect();
    LED_PORT_DDR |= _BV(R_BIT);   /* make the LED bit an output */
    LED_PORT_DDR |= _BV(G_BIT);   /* make the LED bit an output */
    LED_PORT_DDR |= _BV(B_BIT);   /* make the LED bit an output */
	
    sei();

    for(;;){                /* main event loop */
		update_leds();
		if (st) {
            eeprom_write_byte(&eeprom_r, r);
            eeprom_write_byte(&eeprom_g, g);
            eeprom_write_byte(&eeprom_b, b);
		    st = 0;
		}
        wdt_reset();
        usbPoll();
    }
    return 0;
}

/* ------------------------------------------------------------------------- */
