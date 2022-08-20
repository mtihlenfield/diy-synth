#ifndef __HARDWARE_CONFIG_H__
#define __HARDWARE_CONFIG_H__

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
#define LED4_PIN 12 // GP12

#define SHIFT_REG_CLK_PIN 27 // GP27
#define SHIFT_REG_DATA_PIN 28 // GP28

// Total number of shift register outputs
#define SHIFT_REG_OUTPUTS 16

// NOTE rows are held high,
#define MATRIX_ROWS 6
#define MATRIX_COLS 10
#define MATRIX_R1_PIN 16
#define MATRIX_R2_PIN 17
#define MATRIX_R3_PIN 18
#define MATRIX_R4_PIN 19
#define MATRIX_R5_PIN 20
#define MATRIX_R6_PIN 21
#define NUM_MATRIX_KEYS (MATRIX_ROWS * MATRIX_COLS)

#define MODEL_SEL_PIN 22 // GP22

#define SYNC_CN_PIN 2 // GP2
#define SYNC_IN_PIN 3 // GP3
#define SYNC_OUT_PIN 4 // GP4

#define ANALOG_IN_PIN 26 // GP26
#define ANALOG_IN_CHANNEL 0

#define AN_ADDR_A_PIN 6 // GP6
#define AN_ADDR_B_PIN 7 // GP7
#define AN_ADDR_C_PIN 8 // GP8

#define NUM_ANALOG_INPUTS 5

#define ANALOG_ADDR_MASK (7 << 6)
#define MASK_CLK_SPEED 0
#define MASK_PORTAMENTO (1 << 6)
#define MASK_GATE_TIME (2 << 6)
#define MASK_CLK_DIV (3 << 6)
#define MASK_SUB_MODE (4 << 6)
#define MASK_GND (5 << 6)

#endif
