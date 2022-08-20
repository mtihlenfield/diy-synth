#include <stdint.h>
#include <string.h>
#include "list.h"

#include "lkp_stack.h"

void lkp_push_key(struct lkp_stack *stack, uint32_t key_id)
{
    struct key_press *new_key_press = 0;
    // Find the first available key press
    for (uint8_t i = 0; i <= MAX_KEY_PRESSES; i++) {
        if (0 == stack->key_press_pool[i].key_id) {
            new_key_press = &stack->key_press_pool[i];
            break;
        }
    }

    if (!new_key_press) {
        // No keys available. Just silently fail so that we can continue
        return;
    }

    new_key_press->key_id = key_id;
    list_add(&new_key_press->list, stack->tail);
    stack->tail = &new_key_press->list;
}

uint8_t lkp_pop_key(struct lkp_stack *stack, uint32_t key_id)
{
    uint8_t was_last_pressed = 0;
    struct key_press *cursor = 0;
    list_for_each_entry(cursor, &stack->stack, list) {
        if (key_id == cursor->key_id) {
            break;
        }
    }

    if (!cursor) {
        // Didn't find the key in the stack. This should mean that we ran out of
        // key_presses when it was pressed. Have to ignore it.
        return 0;
    }

    struct key_press *current_tail = list_entry(stack->tail, struct key_press, list);

    if (cursor == current_tail) {
        stack->tail = current_tail->list.prev;
        was_last_pressed = 1;
    } else {
        was_last_pressed = 0;
    }

    // Setting key_id to 0 releases it back to the pool
    cursor->key_id = 0;
    list_del(&cursor->list);

    return was_last_pressed;
}
