#ifndef _CONFIG_H_
#define _CONFIG_H_

#define REFRESH_HZ              50      // Main loop frequency
#define STARTSTOP_THRESH        400     // Angle to start timing
#define STOPTIME                200     // "Debouncing" when coming down from HS
#define SCREENSAVER_TIME        300     // Seconds before clearing the display (to conserve power)
#define STARTUP_DELAY           300     // Milliseconds for the device to allow other peripherals to startup
#define MIN_HS_TIME             5       // Minimum length of time of a handstand in tenths of seconds. This is to prevent a score being lost when banging the accelerometer (when coming down from a handstand for eg.)

/* GPIO Pins */
#define ACCEL_PWR               INCH_0  // Accelerometer power output
#define DISP_PWR                INCH_1  // Display power output (to BJT)
#define ACCEL_ADC               INCH_7  // Accelerometer analogue input
/* SPI pins */
#define CS_PIN                  BIT3    // Chip select
#define MOSI_PIN                BIT4    // Master out, slave in
#define CLK_PIN                 BIT5    // Clock

/* Display commands */
#define DECIMAL                 (0x77)  // Follow with a decimal point command
#define BRIGHTNESS              (0x7a)  // Follow with a value from 0 to 254. Stored into non-volatile memory
#define RESET                   (0x76)  // Set cursor to first digit
#define DIGIT1                  (0x7b)
#define DIGIT2                  (0x7c)
#define DIGIT3                  (0x7d)
#define DIGIT4                  (0x7e)

/* Decimal point commands */
#define DECIMAL0                (1<<0)
#define DECIMAL1                (1<<1)
#define DECIMAL2                (1<<2)
#define DECIMAL3                (1<<3)
#define COLON                   (1<<4)
#define APOSTROPHE              (1<<5)

#endif

