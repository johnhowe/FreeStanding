/**
 * Handstand Timer
 *
 * John Howe
 */

#include <msp430x20x2.h>
#include <signal.h>
#include "config.h"

#include <msp430x20x2.h>
#include <signal.h>
#include "config.h"

#define true 1
#define false 0

#define TICK_HZ 1000

void initialise(void);
void spiBang(unsigned char byte);
void display(unsigned short number);
short readADC(unsigned short);

unsigned long ticks;

int main(void)
{
        initialise();

        unsigned long nextRefreshTime = 0, hsStart = 0;
        unsigned short isTiming = false, stopCounts = 0, hsTime = 0;

        for (;;) {
                if (ticks >= nextRefreshTime) {
                        nextRefreshTime += TICK_HZ / REFRESH_HZ;

                        short accel = readADC(ADCPIN);

                        if (accel < STARTSTOP_THRESH) {
                                stopCounts = 0;
                                if (!isTiming) {
                                        isTiming = true;
                                        hsStart = ticks;
                                }
                                hsTime = (unsigned short) ((ticks - hsStart) / 100);
                        } else {
                                stopCounts++;
                                if (isTiming && stopCounts > STOPTIME/REFRESH_HZ) {
                                        isTiming = false;
                                }
                        } 

                        display(hsTime);
                }

                LPM1; // Put the device into sleep mode 1
        }
        return 0;
}

/**
 * Configures the peripherals, clocks, timers, I/O ports, variables and clears
 * the display.
 */
void initialise(void) 
{
        /* Stop watchdog timer */
        WDTCTL = WDTPW + WDTHOLD; 

        /* Set internal clock frequency to 1MHz */
        DCOCTL = CALDCO_1MHZ;
        BCSCTL1 = CALBC1_1MHZ;

        /* Initialise Timer_A */
        TACTL = TACLR;
        TACTL |= TASSEL_2; // SMCLK clock source
        TACTL |= ID_0; // Divide input clock by 1
        TACTL |= MC_1; // Timer up to CCR0 mode
        TACCR0 = 980; // Tuned to interrupt at 1ms intervals for 1Mhz timer
        TACTL |= TAIE; // Enable interrupt

        /* Initialise I/O ports */
        P1OUT = 0;
        P1DIR |= ( RED_LED | GREEN_LED | CS_PIN | MOSI_PIN | CLK_PIN ); // Set output pins
        P1DIR &= ~( ADCPIN ); // Set input pins
        ADC10AE0 |= ADCPIN; // Enable ADC

        /* Reset and clear the display of any decomal points */
        spiBang(RESET);
        spiBang(DECIMAL);
        spiBang(DECIMAL2);

        ticks = 0;

        _BIS_SR(GIE); // Global interrupt enable
}

/**
 * Displays a number on the serial 7-seg display
 *
 * The number to be displayed is separated out into individual digits to each
 * be sent to the display. Any decimal point needs to be managed elsewhere.
 * Leading zeroes aren't displayed.
 */
void display(unsigned short number) 
{
        static unsigned short lastNumber = 0;
        if (number != lastNumber) {
                char first = 0, second = 0, third = 0, fourth = 0;
                while (number > 0) {
                        if (number > 9999) {
                                number -= 10000;
                        } else if (number > 999) {
                                first++;
                                number -= 1000;
                        } else if (number > 99) {
                                second++;
                                number -= 100;
                        } else if (number > 9) {
                                third++;
                                number -= 10;
                        } else {
                                fourth++;
                                number -= 1;
                        }
                }

                spiBang(RESET);
                if (first == 0) {
                        spiBang(' ');
                } else {
                        spiBang(first);
                }
                if (first == 0 && second == 0) {
                        spiBang(' ');
                } else {
                        spiBang(second);
                }
                //if (first == 0 && second == 0 && third == 0) {
                //spiBang(' ');
                //} else {
                spiBang(third);
                //}
                spiBang(fourth);
        }
        lastNumber = number;
}

/**
 * Bit-bangs a byte over SPI
 *
 * CPOL = 0, CPHA = 0
 */
void spiBang(unsigned char byte) 
{
        // Just a status LED
        P1OUT |= GREEN_LED;           
        // Enable the SPI device
        P1OUT &= ~CS_PIN;
        // TX byte one bit at a time, starting with most significant bit
        short bit;
        for (bit = 0; bit < 8; bit++) {
                if (byte & 0x80) { 
                        P1OUT |= MOSI_PIN; 
                }
                else {
                        P1OUT &= ~MOSI_PIN; 
                }
                // Drop the last bit 
                byte <<= 1;
                // Slave latches on rising clock edge
                P1OUT |= CLK_PIN;           
                P1OUT &= ~CLK_PIN;        
        }   
        P1OUT &= ~GREEN_LED;
        P1OUT |= CS_PIN;
}

/**
 * Takes a single reading of a specified ADC pin
 */
short readADC(unsigned short pin) 
{
        ADC10CTL0 = ADC10ON + ADC10SHT_1 + SREF_0; // ACD10 on, 8 clock cycles per sample, Use Vcc/Vss references
        ADC10CTL1 = ADC10SSEL_0 + pin; // Select internal ADC clock (~5MHz) and Input channel (pin)
        ADC10CTL0 |= ENC + ADC10SC; // enable and start conversion
        while (!((ADC10CTL1 ^ ADC10BUSY) & ((ADC10CTL0 & ADC10IFG)==ADC10IFG))) { // ensure conversion is complete
                continue;
        }
        ADC10CTL0 &= ~(ADC10IFG +ENC); // disable conversion and clear flag

        short adcReading = ADC10MEM;
        return adcReading;
}


/******************************/
/* Interrupt service routines */
/******************************/

interrupt(TIMERA1_VECTOR) serviceTimerA(void) 
{
        // Clear TimerA interrupt flag
        TACTL &= ~TAIFG;
        // Exit low power mode
        LPM1_EXIT;
        // Update clock
        ticks++;
}


