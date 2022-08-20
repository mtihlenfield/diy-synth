#ifndef __LPK_STACK_H__
#define __LPK_STACK_H__
#include <stdint.h>
#include <string.h>
#include "list.h"

// This is the max number keys that can be pressed at a time
// before other keys start to get stolen. Needs to be sort of
// a balance between speed and functionality. The more keys we
// add the longer it takes to update CV/gate on key events
#define MAX_KEY_PRESSES 20

struct key_press {
    uint32_t key_id;
    struct list_head list;
};

struct lkp_stack {
    struct key_press key_press_pool[MAX_KEY_PRESSES];
    struct list_head stack;
    struct list_head *tail;
};

inline int lkp_stack_init(struct lkp_stack *stack)
{
    memset(stack, 0, sizeof(struct lkp_stack));
    INIT_LIST_HEAD(&stack->stack);
    stack->tail = &stack->stack;

    return 0;
}

/*
 * Push a key press on to the key press stack
 */
void lkp_push_key(struct lkp_stack *stack, uint32_t key_id);

/*
 * Remove a key press from the key buffer
 *
 * Returns 1 if the key being removed is the last key pressed,
 * returns 0 otherwise
 */
uint8_t lkp_pop_key(struct lkp_stack *stack, uint32_t key_id);

/*
 * Returns the last key which is still pressed.
 */
inline uint32_t lkp_get_last_key(struct lkp_stack *stack)
{
    struct key_press *press = list_entry(stack->tail, struct key_press, list);
    return press->key_id;
}

inline void lkp_print(struct lkp_stack *stack)
{
    struct key_press *cursor = 0;
    list_for_each_entry(cursor, &stack->stack, list) {
        printf("%d ", cursor->key_id);
    }
}

#endif
