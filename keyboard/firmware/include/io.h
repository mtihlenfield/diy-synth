#ifndef __KEYS_H__
#define __KEYS_H__

#include <stdint.h>

typedef uint32_t io_event_t;

#define IO_EVENT_QUEUE_SIZE 20

#define IO_MODE_ARP 0
#define IO_MODE_SEQ 1

enum io_event {
    IO_KEY_RELEASED = 0,
    IO_KEY_PRESSED = 1,
    IO_CLK_SPEED_CHANGED = 2,
    IO_GATE_TIME_CHANGED = 3,
    IO_PORTAMENTO_CHANGED = 4,
    IO_CLK_DIV_CHANGED = 5,
    IO_SUB_MODE_CHANGED = 6,
    IO_MODE_CHANGED = 7
};

enum key_id {
    KEY_NONE = 0,
    KEY_C1 = 1, KEY_CS1 = 2, KEY_D1 = 3, KEY_DS1 = 4, 
    KEY_E1 = 5, KEY_F1 = 6, KEY_FS1 = 7, KEY_G1 = 8, 
    KEY_GS1 = 9, KEY_A1 = 10, KEY_AS1 = 11, KEY_B1 = 12,
    KEY_C2 = 13, KEY_CS2 = 14, KEY_D2 = 15, KEY_DS2 = 16, 
    KEY_E2 = 17, KEY_F2 = 18, KEY_FS2 = 19, KEY_G2 = 20, 
    KEY_GS2 = 21, KEY_A2 = 22, KEY_AS2 = 23, KEY_B2 = 24,
    KEY_C3 = 25, KEY_CS3 = 26, KEY_D3 = 27, KEY_DS3 = 28, 
    KEY_E3 = 29, KEY_F3 = 30, KEY_FS3 = 31, KEY_G3 = 32, 
    KEY_GS3 = 33, KEY_A3 = 34, KEY_AS3 = 35, KEY_B3 = 36,
    KEY_C4 = 37, KEY_CS4 = 38, KEY_D4 = 39, KEY_DS4 = 40, 
    KEY_E4 = 41, KEY_F4 = 42, KEY_FS4 = 43, KEY_G4 = 44, 
    KEY_GS4 = 45, KEY_A4 = 46, KEY_AS4 = 47, KEY_B4 = 48,
    KEY_C5 = 49, KEY_OCTAVE_UP=50, KEY_OCTAVE_DOWN=51, KEY_PLAY_PAUSE=52,
    KEY_RECORD=53, KEY_STOP=54, KEY_REST=55, KEY_HOLD=56, 
    KEY_FUNC=57, KEY_MODE=58
};

#define MAX_KEYBED_KEY 49

/*
 * Returns 1 if the provided key is a keybed key. Returns
 * 0 if the provided key is a function key
 */
inline bool io_is_keybed_key(uint8_t key_id) 
{
    return MAX_KEYBED_KEY >= key_id;
}

/*
 * Extracts the event type and value from a io_event_t object
 */
inline void io_event_unpack(io_event_t io_event, uint8_t *type, uint16_t *value)
{
    *type = (io_event & 0xff000000) >> 24;
    *value = io_event & 0x00ffffff;
}

/*
 * Creates a io_event_t object from an event type and value
 */
inline io_event_t io_event_create(uint8_t type, uint16_t value)
{
    return (type << 24) | value;
}

/*
 * Returns true if there are io events on the io event queue,
 * false otherwise
 */
int io_event_queue_ready(void);

/*
 * Pops an io event off of the io event queue. If there is no 
 * io event on the queue at the time of calling, the function
 * blocks until one is added.
 */
io_event_t io_event_queue_pop_blocking(void);

/*
 * Initializes the io hardware and state
 */
int io_init(void);

/*
 * Entrypoint for the io code
 */
void io_main(void);

#endif
