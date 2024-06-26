#include <stdio.h>
#include <stddef.h>
#include <math.h>
#include <string.h>
#include "tusb.h"
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

#define DEFAULT_MIDI_VELOCITY 127
#define DEFAULT_MIDI_CHANNEL 0
#define DEFAULT_MIDI_CABLE_NUM 0

// Bumps the CV value up by 1 so that if we are in -O mode we don't have
// negatve CV values
#define DEFAULT_SHIFT 1

float cv_octave[12] = {
    0.0833, 0.1667, 0.2550, 0.3333, 0.4167,
    0.5000, 0.5833, 0.6667, 0.7500, 0.8333,
    0.9167, 1.000
};

#define NUM_OCTAVE_LEDS 3
const uint8_t octave_led_pins[NUM_OCTAVE_LEDS] = {
    OCTAVE_DOWN_LED_PIN, OCTAVE_MID_LED_PIN, OCTAVE_UP_LED_PIN
};

struct keyboard_state {
    struct lkp_stack key_press_stack;
    struct mcp4921 dac;
    int8_t top_octave_offset; // octave offset for middle c and above
    int8_t bottom_octave_offset; // octave offset for keys below middle c
    enum io_event func_key_state; // Whether or not the func key is being pressed
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
static float key_to_cv(struct keyboard_state *state, enum key_id id)
{
    float key_octave = floor(id / 12);
    uint8_t note = (id % 12) - 1;
    int8_t offset = 0;

    if (id >= KEY_MIDDLE_C) {
        offset = g_state.top_octave_offset;
    } else {
        offset = g_state.bottom_octave_offset;
    }

    return key_octave + offset + DEFAULT_SHIFT + cv_octave[note];
}

static uint8_t key_to_midi(struct keyboard_state *state, enum key_id id)
{
    int8_t offset = 0;

    if (id >= KEY_MIDDLE_C) {
        offset = g_state.top_octave_offset;
    } else {
        offset = g_state.bottom_octave_offset;
    }

    return id + MIDI_KEY_OFFSET + (offset * 12);
}

/*
 * Set gate up and use the last key that is still pressed to
 * set the CV output.
 */
static void play_last_note(struct keyboard_state *state)
{
    uint32_t current_note = lkp_get_last_key(&state->key_press_stack);

    if (!current_note) {
        // No keys currently pressed
        return;
    }

    set_gate(1);
    float cv = key_to_cv(state, current_note);

    printf("Setting CV to %fv\n", cv);

    mcp4921_set_output(&state->dac, cv / CV_OPAMP_GAIN);
}

void write_midi_event(uint8_t event_type, uint32_t key_id)
{
    uint8_t msg[3];

    if (KEY_NONE == key_id || KEY_MAX < key_id) {
        printf("Invalid key_id: %d\n", key_id);
        return;
    }

    if (IO_KEY_PRESSED == event_type) {
        msg[0] = 0x90;
        msg[2] = DEFAULT_MIDI_VELOCITY;
    } else {
        msg[0] = 0x80;
        msg[2] = 0;
    }

    msg[0] |= DEFAULT_MIDI_CHANNEL; // message type and channel
    msg[1] = key_to_midi(&g_state, key_id); // note value
    printf("midi note - state: 0x%x, note: %d\n", msg[0], msg[1]);
    tud_midi_stream_write(DEFAULT_MIDI_CABLE_NUM, msg, sizeof(msg));
}

void handle_keybed_event(uint8_t event_type, uint32_t key_id)
{
    if (KEY_NONE == key_id || KEY_MAX < key_id) {
        printf("Invalid key_id: %d\n", key_id);
        return;
    }

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

void update_leds(void)
{
    for (uint8_t i = 0; i < NUM_OCTAVE_LEDS; i++) {
        gpio_put(octave_led_pins[i], 0);
    }

    gpio_put(octave_led_pins[g_state.top_octave_offset + 1], 1);

    if (g_state.bottom_octave_offset == g_state.top_octave_offset) {
        return;
    }

    gpio_put(octave_led_pins[g_state.bottom_octave_offset + 1], 1);
}

void handle_octave_change(uint32_t key_id)
{
    if (IO_KEY_PRESSED != g_state.func_key_state) {
        if (KEY_OCTAVE_DOWN == key_id && g_state.bottom_octave_offset > OCTAVE_SHIFT_MIN) {
            g_state.top_octave_offset -= 1;
        } else if (KEY_OCTAVE_UP == key_id && g_state.top_octave_offset < OCTAVE_SHIFT_MAX) {
            g_state.top_octave_offset += 1;
        }

        g_state.bottom_octave_offset = g_state.top_octave_offset;
    } else {
        if (KEY_OCTAVE_DOWN == key_id && g_state.bottom_octave_offset > OCTAVE_SHIFT_MIN) {
            g_state.bottom_octave_offset -= 1;
        } else if (KEY_OCTAVE_UP == key_id && g_state.bottom_octave_offset < g_state.top_octave_offset) {
            // Not letting bottom offset go higher than the top offset
            g_state.bottom_octave_offset += 1;
        }
    }

    update_leds();
}

void handle_func_key_event(uint8_t event_type, uint32_t key_id)
{
    if (KEY_FUNC == key_id) {
        g_state.func_key_state = event_type;
        return;
    }

    if (IO_KEY_RELEASED == event_type) {
        return;
    }

    if (KEY_OCTAVE_UP == key_id || KEY_OCTAVE_DOWN == key_id) {
        handle_octave_change(key_id);
    }
}

void indicate_boot()
{
    for (uint8_t i = 0; i < 3; i++) {
        for (uint8_t i = 0; i < NUM_OCTAVE_LEDS; i++) {
            sleep_ms(50);
            gpio_put(octave_led_pins[i], 1);
            sleep_ms(50);
            gpio_put(octave_led_pins[i], 0);
        }
    }
}

void clear_midi_in()
{
  uint8_t packet[4];
  while ( tud_midi_available() ) tud_midi_packet_read(packet);
}

int main(void)
{
    stdio_init_all();
    tusb_init();

    printf("Starting keyboard controller!\n");

    gpio_init(PWR_LED_PIN);
    gpio_set_dir(PWR_LED_PIN, GPIO_OUT);
    gpio_put(PWR_LED_PIN, 1);

    for (uint8_t i = 0; i < NUM_OCTAVE_LEDS; i++) {
        gpio_init(octave_led_pins[i]);
        gpio_set_dir(octave_led_pins[i], GPIO_OUT);
    }

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

    mcp4921_set_output(&g_state.dac, 1.0 / CV_OPAMP_GAIN);

    indicate_boot();

    update_leds();

    while (1) {

        tud_task(); // tinyusb device task
        clear_midi_in();

        while (io_event_queue_ready()) {
            uint8_t event_type = 0;
            uint16_t event_val = 0;

            io_event_t io_event = io_event_queue_pop_blocking();
            io_event_unpack(io_event, &event_type, &event_val);
            printf("io event: type: %d, id: %d\n", event_type, event_val);

	    switch (event_type) {
                case IO_KEY_PRESSED:
	        case IO_KEY_RELEASED:
                    if (io_is_keybed_key(event_val)) {
                        write_midi_event(event_type, event_val);
                        handle_keybed_event(event_type, event_val);
                    } else {
                        handle_func_key_event(event_type, event_val);
                    }
	    }
        }
    }

}
