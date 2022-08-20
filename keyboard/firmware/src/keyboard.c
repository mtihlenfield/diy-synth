#include <stdio.h>
#include <stddef.h>
#include <math.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "hardware/irq.h"

#include "hardware_config.h"
#include "mcp4921.h"
#include "io.h"
#include "lkp_stack.h"

#define OCTAVE_SHIFT_MAX 1
#define OCTAVE_SHIFT_MIN -1

float cv_octave[12] = {
    0.0833, 0.1667, 0.2550, 0.3333, 0.4167,
    0.5000, 0.5833, 0.6667, 0.7500, 0.8333,
    0.9167, 1.000
};

struct keyboard_state {
    struct lkp_stack key_press_stack;
    int8_t octave_shift;
    struct mcp4921 dac;
} g_state;

static int init_cv_dac(struct mcp4921 *dac)
{
    memset(dac, 0, sizeof(struct mcp4921));

    // Setting up the pins we will use to communicate with the DAC
    dac->clk_pin = DAC_PIN_SCK; // Clock pin
    dac->cs_pin = DAC_PIN_CS; // Chip select pin
    dac->mosi_pin = DAC_PIN_MOSI; // Data out pin
    dac->spi_inst = spi1; // We'll use the spi1 peripheral

    dac->refv = DAC_REFV; // This is the reference voltage that is set by out TL431.
    dac->clock_speed = DAC_CLK_SPEED; // SPI Clock speed
    mcp4921_set_dac(dac, MCP4921_DAC_A); // Our DAC has two channels. We're only going to use channel A
    mcp4921_set_buff(dac, MCP4921_VREF_BUFFERED);
    mcp4921_set_gain(dac, MCP4921_GAIN_1X);
    mcp4921_set_shdn(dac, MCP4921_SHDN_ON);

    return mcp4921_init(dac);
}

/*
 * Brings the gate signal high if gate_state
 * is 1, or low if gate_state is 0.
 */
static inline void set_gate(uint8_t gate_state)
{
    gpio_put(GATE_OUT_PIN, gate_state);
}

/*
 * Deterines what the CV value should be for the given key
 */
static inline float key_to_cv(struct keyboard_state *state, enum key_id id)
{
    float key_octave = floor(id / 12);
    uint8_t note = (id % 12) - 1;

    return key_octave + state->octave_shift + cv_octave[note];
}

static inline void octave_shift(uint8_t direction)
{
    if (direction) {
        if (g_state.octave_shift < OCTAVE_SHIFT_MAX) {
            g_state.octave_shift++;
        }
    } else {
        if (g_state.octave_shift > OCTAVE_SHIFT_MIN) {
            g_state.octave_shift--;
        }
    }
}

/*
 * Set gate up and use the last key that is still pressed to
 * set the CV output.
 */
static inline void play_last_note(struct keyboard_state *state)
{
    uint32_t current_note = lkp_get_last_key(&state->key_press_stack);

    if (!current_note) {
        // No keys currently pressed
        return;
    }

    set_gate(1);
    float cv = key_to_cv(state, current_note);
    mcp4921_set_output(&state->dac, cv / CV_OPAMP_GAIN);
}

void handle_keybed_event(uint8_t event_type, uint32_t key_id)
{
    if (IO_KEY_PRESSED == event_type) {
        lkp_push_key(&g_state.key_press_stack, key_id);

        // If gate is low, this won't matter
        // If gate is high, we want to retrigger it
        set_gate(0);
    } else {
        uint8_t was_last_pressed = lkp_pop_key(&g_state.key_press_stack, key_id);
        if (was_last_pressed) {
            // If this was the last key pressed then either:
            // a) There are no other keys pressed and we want gate low
            // b) There are other keys pressed and we want to retrigger
            set_gate(0);
        } else {
            // Don't need to update gate or CV if a key
            // was released that wasn't the most recent key
            return;
        }
    }

    play_last_note(&g_state);
}

void handle_func_key_event(uint8_t event_type, uint32_t key_id)
{
    if (IO_KEY_RELEASED == event_type) {
        return;
    }

    if (KEY_OCTAVE_UP == key_id) {
        octave_shift(1);
    } else if (KEY_OCTAVE_DOWN == key_id) {
        octave_shift(0);
    }
}

int main(void)
{
    stdio_init_all();

    printf("Starting keyboard controller!\n");

    gpio_init(PWR_LED_PIN);
    gpio_set_dir(PWR_LED_PIN, GPIO_OUT);
    gpio_put(PWR_LED_PIN, 1);

    gpio_init(GATE_OUT_PIN);
    gpio_set_dir(GATE_OUT_PIN, GPIO_OUT);
    set_gate(0);

    memset(&g_state, 0, sizeof(struct keyboard_state));

    if (lkp_stack_init(&g_state.key_press_stack)) {
        printf("Failed to init lkp stack");
        return 1;
    }

    if (init_cv_dac(&g_state.dac)) {
        printf("Failed to init CV DAC");
        return 1;
    }

    if (io_init()) {
        printf("Failed to init key matrix");
        return 1;
    }

    multicore_launch_core1(io_main);

    while (1) {
        while (io_event_queue_ready()) {
            uint8_t event_type = 0;
            uint16_t event_val = 0;

            io_event_t io_event = io_event_queue_pop_blocking();
            io_event_unpack(io_event, &event_type, &event_val);
            printf("io event: type: %d, id: %d\n", event_type, event_val);

	    // switch (event_type) {
            //     case IO_KEY_PRESSED:
	    //     case IO_KEY_RELEASED:
            //         if (io_is_keybed_key(event_val)) {
            //             handle_keybed_event(event_type, event_val);
            //         } else {
            //             handle_func_key_event(event_type, event_val);
            //         }

	    //         break;
	    //     case IO_CLK_SPEED_CHANGED:
	    //     case IO_CLK_DIV_CHANGED:
	    //     case IO_MODE_CHANGED:
	    //     case IO_SUB_MODE_CHANGED:
	    //         printf("This IO event not yet implemented\n");
	    //         break;
	    // }
        }
    }

}
