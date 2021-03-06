/**
 *   FreeStanding, a handstand timer using an MSP430.
 *   Copyright (C) 2011, John Howe
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


/* TODO list:
 * Hysteresis in accel - ie 2 thresholds, one rising, one falling. This removes the stopCounts etc.
 */

#include <msp430x20x2.h>
#include "config.h"

//In uniarch there is no more signal.h to sugar coat the interrupts definition, so we do it here
#define interrupt(x) void __attribute__((interrupt (x)))


#define true 1
#define false 0

#define TICK_HZ 1000

#undef CALIBRATION_MODE

void initialise(void);
void spiBang(unsigned char byte);
void display(unsigned short number);
void clearDisplay(void);
short readADC(unsigned short);
interrupt(TIMERA1_VECTOR) serviceTimerA(void);

unsigned long ticks;

int main(void)
{
        initialise();

        unsigned long nextRefreshTime;
        unsigned long hsStart;
        nextRefreshTime = hsStart = 0;
        unsigned short isTiming = false, stopCounts = 0, hsTime = 0;

        for (;;) {
                if (ticks >= nextRefreshTime) {
                        nextRefreshTime += TICK_HZ / REFRESH_HZ;

                        short accel = readADC(ACCEL_ADC);

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

#ifdef CALIBRATION_MODE
                        display(accel);
#else
                        if (hsTime > MIN_HS_TIME) {
                                display(hsTime);
                        }
#endif
                }

                LPM1; // Put the device into sleep mode 1
        }
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
        P1DIR |= ( CS_PIN | MOSI_PIN | CLK_PIN | ACCEL_PWR ); // Set output pins
        P1DIR &= ~( ACCEL_ADC ); // Set input pins
        ADC10AE0 |= ACCEL_ADC; // Enable ADC
        P1OUT |= ACCEL_PWR; // Power on the accelerometer and display
        /// @TODO This may need some dead time here to allow the devices to power up

        _BIS_SR(GIE); // Global interrupt enable

        // Delay a specified period of time to allow the peripheral devices to power up
        short delay = STARTUP_DELAY;
        while (--delay) {
                LPM1;
        }

        clearDisplay();

        ticks = 0;
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

                spiBang(DECIMAL); // Display the decimal point
                spiBang(DECIMAL2);

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
 * Clears the display
 */
void clearDisplay(void)
{
        spiBang(RESET);
        spiBang(' ');
        spiBang(' ');
        spiBang(' ');
        spiBang(' ');
        spiBang(DECIMAL);
        spiBang(0);

        spiBang(BRIGHTNESS);
        spiBang(0xff);
}

/**
 * Bit-bangs a byte over SPI
 *
 * CPOL = 0, CPHA = 0
 */
void spiBang(unsigned char byte)
{
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
        P1OUT |= CS_PIN;
}

/**
 * Takes a single reading of a specified ADC pin
 */
short readADC(unsigned short pin)
{
        const unsigned short siloLength = 4;
        static long adcSilo = 0;

        ADC10CTL0 = ADC10ON + ADC10SHT_1 + SREF_0; // ACD10 on, 8 clock cycles per sample, Use Vcc/Vss references
        ADC10CTL1 = ADC10SSEL_0 + pin; // Select internal ADC clock (~5MHz) and Input channel (pin)
        ADC10CTL0 |= ENC + ADC10SC; // enable and start conversion
        while (!((ADC10CTL1 ^ ADC10BUSY) & ((ADC10CTL0 & ADC10IFG)==ADC10IFG))) { // ensure conversion is complete
                continue;
        }
        ADC10CTL0 &= ~(ADC10IFG +ENC); // disable conversion and clear flag

        short adcSample = ADC10MEM;


        short adcSmooth = adcSilo >> siloLength;
        adcSilo = adcSilo - adcSmooth + adcSample;

        return adcSmooth;
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


