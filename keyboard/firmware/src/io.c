#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

#include "hardware_config.h"
#include "io.h"


// Determines how frequently the entire key matrix is
// scanned. One col is scanned per poll, so this number
// is divided by KEYBOARD_COLS to determine how frequently
// the poll function runs
#define KEY_POLL_INTERVAL_US 2000

#define NUM_ANALOG_SAMPLES 16

const uint8_t key_row_pins[MATRIX_ROWS] = {
    MATRIX_R1_PIN, MATRIX_R2_PIN, MATRIX_R3_PIN,
    MATRIX_R4_PIN, MATRIX_R5_PIN, MATRIX_R6_PIN
};

#define AN_EVENT_IDX 0
#define AN_MASK_IDX 1
#define AN_THRESHOLD_IDX 2
#define AN_NUM_CONFIGS 3

#define AN_DEFAULT_THRESHOLD 30
#define AN_READ_SAMPLES 30

const uint16_t g_analog_config[NUM_ANALOG_INPUTS][AN_NUM_CONFIGS] = {
    {IO_CLK_SPEED_CHANGED, MASK_CLK_SPEED, AN_DEFAULT_THRESHOLD},
    {IO_PORTAMENTO_CHANGED, MASK_PORTAMENTO, AN_DEFAULT_THRESHOLD},
    {IO_GATE_TIME_CHANGED, MASK_GATE_TIME, AN_DEFAULT_THRESHOLD},
    {IO_CLK_DIV_CHANGED, MASK_CLK_DIV, AN_DEFAULT_THRESHOLD},
    {IO_SUB_MODE_CHANGED, MASK_SUB_MODE, AN_DEFAULT_THRESHOLD},
};

const uint8_t key_matrix[MATRIX_ROWS][MATRIX_COLS] = {
    {KEY_OCTAVE_UP, KEY_CS1, KEY_G1, KEY_CS2, KEY_G2, KEY_CS3, KEY_G3, KEY_CS4, KEY_G4, KEY_REST},
    {KEY_OCTAVE_DOWN, KEY_D1, KEY_GS1, KEY_D2, KEY_GS2, KEY_D3, KEY_GS3, KEY_D4, KEY_GS4, KEY_HOLD},
    {KEY_PLAY_PAUSE, KEY_DS1, KEY_A1, KEY_DS2, KEY_A2, KEY_DS3, KEY_A3, KEY_DS4, KEY_A4, KEY_FUNC},
    {KEY_RECORD, KEY_E1, KEY_AS1, KEY_E2, KEY_AS2, KEY_E3, KEY_AS3, KEY_E4, KEY_AS4, KEY_MODE},
    {KEY_STOP, KEY_F1, KEY_B1, KEY_F2, KEY_B2, KEY_F3, KEY_B3, KEY_F4, KEY_B4, KEY_NONE},
    {KEY_C1, KEY_FS1, KEY_C2, KEY_FS2, KEY_C3, KEY_FS3, KEY_C4, KEY_FS4, KEY_C5, KEY_NONE},
};

struct io_state {
    uint8_t key_state[NUM_MATRIX_KEYS];
    queue_t event_queue;
    uint8_t current_col;
    repeating_timer_t poll_timer;
    uint16_t analog_values[NUM_ANALOG_INPUTS]; // Indices should match g_analog_config
} g_io_state;

static inline uint16_t io_analog_read(uint32_t mask, uint16_t num_samples)
{
    sleep_us(1); // TODO figure out a better way to make sure timing is correct.
    gpio_set_mask(ANALOG_ADDR_MASK & mask);
    sleep_us(10);

    uint32_t temp = 0;

    for (uint16_t i = 0; i < num_samples; i++) {
        temp += adc_read();
    }

    sleep_us(1);
    gpio_clr_mask(ANALOG_ADDR_MASK);
    sleep_us(10);

    return temp / num_samples;
}

static inline void io_clock_shift_reg(int clk_pin)
{
    sleep_us(1); // TODO figure out a better way to make sure timing is correct.
    gpio_put(SHIFT_REG_CLK_PIN, 1);
    sleep_us(1);
    gpio_put(SHIFT_REG_CLK_PIN, 0);
    sleep_us(1);
}

int io_event_queue_ready(void)
{
    return !queue_is_empty(&g_io_state.event_queue);
}

io_event_t io_event_queue_pop_blocking(void)
{
    io_event_t temp;
    queue_remove_blocking(&g_io_state.event_queue, &temp);

    return temp;
}

int io_init(void)
{
    gpio_init(SHIFT_REG_CLK_PIN);
    gpio_init(SHIFT_REG_DATA_PIN);
    gpio_set_dir(SHIFT_REG_CLK_PIN, GPIO_OUT);
    gpio_set_dir(SHIFT_REG_DATA_PIN, GPIO_OUT);

    gpio_put(SHIFT_REG_CLK_PIN, 0);

    // Set all of the shift register outputs high
    gpio_put(SHIFT_REG_DATA_PIN, 1);
    for (uint8_t i = 0; i < SHIFT_REG_OUTPUTS; i++) {
        io_clock_shift_reg(SHIFT_REG_CLK_PIN);
    }

    for (uint8_t i = 0; i < MATRIX_ROWS; i++) {
        uint8_t pin = key_row_pins[i];

        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin);
    }

    memset(&g_io_state, 0, sizeof(struct io_state));

    queue_init(&g_io_state.event_queue, sizeof(io_event_t), IO_EVENT_QUEUE_SIZE);

    adc_init();
    adc_gpio_init(ANALOG_IN_PIN);
    adc_select_input(ANALOG_IN_CHANNEL);

    gpio_init(AN_ADDR_A_PIN);
    gpio_set_dir(AN_ADDR_A_PIN, GPIO_OUT);

    gpio_init(AN_ADDR_B_PIN);
    gpio_set_dir(AN_ADDR_B_PIN, GPIO_OUT);

    gpio_init(AN_ADDR_C_PIN);
    gpio_set_dir(AN_ADDR_C_PIN, GPIO_OUT);

    gpio_init(SYNC_CN_PIN);
    gpio_set_dir(SYNC_CN_PIN, GPIO_IN);
    gpio_disable_pulls(SYNC_CN_PIN);

    gpio_init(SYNC_IN_PIN);
    gpio_set_dir(SYNC_IN_PIN, GPIO_IN);
    gpio_disable_pulls(SYNC_IN_PIN);

    gpio_init(SYNC_OUT_PIN);
    gpio_set_dir(SYNC_OUT_PIN, GPIO_OUT);
    gpio_pull_down(SYNC_OUT_PIN);

    return 0;
}

/*
 * Repeating timer function which checks a single column of the matrix for
 * state changes, pushes any changes to the io event queue, and then
 * increments the column counter.
 */
bool io_poll_keys(repeating_timer_t *timer)
{
    if (0 == g_io_state.current_col) {
        gpio_put(SHIFT_REG_DATA_PIN, 0);
    } else if (g_io_state.current_col >= MATRIX_COLS) {
        for (uint8_t i = 0; i < (SHIFT_REG_OUTPUTS - MATRIX_COLS); i++) {
            io_clock_shift_reg(SHIFT_REG_CLK_PIN);
        }

        g_io_state.current_col = 0;

        return true;
    } else {
        gpio_put(SHIFT_REG_DATA_PIN, 1);
    }

    io_clock_shift_reg(SHIFT_REG_CLK_PIN);

    const uint8_t col = g_io_state.current_col;

    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        uint8_t is_pressed = !gpio_get(key_row_pins[row]);
        uint8_t key = key_matrix[row][col];
        uint8_t was_pressed = g_io_state.key_state[key];
        io_event_t io_event;

        // TODO debouncing: http://www.ganssle.com/debouncing-pt2.htm
        // https://my.eng.utah.edu/~cs5780/debouncing.pdf
        // I'm working with conductive elastomer switches on the keybed. I have not seen bouncing
        // on the keybed keys, but I think it's possible, and will almost certainly happen
        // with the buttons I plan to use for other functions
        if (is_pressed && !was_pressed) {
            // TODO without these prints the loop happens too quickly
            io_event = io_event_create(IO_KEY_PRESSED, key);
            g_io_state.key_state[key] = 1;

            queue_try_add(&g_io_state.event_queue, &io_event);
        } else if (!is_pressed && was_pressed) {
            io_event = io_event_create(IO_KEY_RELEASED, key);
            g_io_state.key_state[key] = 0;

            queue_try_add(&g_io_state.event_queue, &io_event);
        }
    }

    g_io_state.current_col++;

    return true;
}

void io_main(void)
{
    add_repeating_timer_us(
        KEY_POLL_INTERVAL_US / MATRIX_COLS,
        io_poll_keys,
        0,
        &g_io_state.poll_timer
    );

    uint8_t sync_cn = 0;
    uint8_t sync = 0;
    uint8_t new_sync_cn = 0;
    uint8_t new_sync = 0;
    while (true) {
        new_sync = gpio_get(SYNC_IN_PIN);
        new_sync_cn = gpio_get(SYNC_CN_PIN);

        if (new_sync_cn != sync_cn) {
            printf("Sync CN: %d\n", new_sync_cn);
            sync_cn = new_sync_cn;
        }

        if (sync_cn) {
            if (new_sync != sync) {
                printf("Sync: %d\n", new_sync);
                sync = new_sync;
            }
        }

    }

    // uint16_t current_value = 0;
    // uint16_t read_result = 0;
    // uint16_t delta = 0;
    // uint16_t threshold = 0;
    // while (true) {
    //     // TODO: There seems to be a dead spot right around 1330-1770 (it moves a little) on all of the pots....
    //     // TODO: Param 0 seems to be affected by the other inputs occasionally...
    //     for (uint8_t i = 0; i < NUM_ANALOG_INPUTS; i++) {
    //         current_value = g_io_state.analog_values[i];
    //         read_result = io_analog_read(g_analog_config[i][AN_MASK_IDX], AN_READ_SAMPLES);

    //         threshold = g_analog_config[i][AN_THRESHOLD_IDX];

    //         delta = abs(current_value - read_result);
    //         if (delta > threshold) {
    //             g_io_state.analog_values[i] = read_result;
    //             printf("Param %d changed to %d. Delta: %d\n", i, read_result, delta);
    //         }
    //     }
    // }



}
