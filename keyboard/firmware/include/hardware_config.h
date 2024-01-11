#ifndef __HARDWARE_CONFIG_H__
#define __HARDWARE_CONFIG_H__

#define SER_CLK 1 // GPIO1 - PIN 2
#define SER_DATA 0 // GPIO0 - PIN 1

#define OCTAVE_DOWN_LED_PIN 15 // GPIO15 - PIN 20
#define OCTAVE_MID_LED_PIN 14 // GPIO15 - PIN 19
#define OCTAVE_UP_LED_PIN 13 // GPIO15 - PIN 17

#define DAC_SPI_PORT spi1
#define DAC_PIN_CS 9 // GP9
#define DAC_PIN_SCK 10 // GP10
#define DAC_PIN_MOSI 11 // GP11
#define DAC_REFV 2.5 // Using a TL431 in it's default state
#define DAC_CLK_SPEED (1000 * 1000) // 1Mhz

// Amount of gain applied to the CV signal by our op-amp
// configuration.
#define CV_OPAMP_GAIN 3.2

#define GATE_OUT_PIN 5 // GP5

#define PWR_LED_PIN 25 // GP25
#define LED1_PIN 15 // GP15
#define LED2_PIN 14 // GP14
#define LED3_PIN 13 // GP13

#define SHIFT_REG_CLK_PIN 17 // GPIO17 - PIN 22
#define SHIFT_REG_DATA_PIN 16 // GPIO16 - PIN 21

// Total number of shift register outputs
#define SHIFT_REG_OUTPUTS 16

// NOTE rows are held high,
#define MATRIX_ROWS 6
#define MATRIX_COLS 10
#define MATRIX_R1_PIN 26 // GPIO26 - PIN 31
#define MATRIX_R2_PIN 22 // GPIO22 - PIN 29
#define MATRIX_R3_PIN 21 // GPIO21 - PIN 27
#define MATRIX_R4_PIN 20 // GPIO20 - PIN 26
#define MATRIX_R5_PIN 19 // GPIO19 - PIN 25
#define MATRIX_R6_PIN 18 // GPIO18 - PIN 24
#define NUM_MATRIX_KEYS (MATRIX_ROWS * MATRIX_COLS)

#endif
