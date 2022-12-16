#include "macro.h"

/*
 * Returns: Is the macro still running?
 */
bool tick_macro(macro_state_t* macro) {
    if (macro->state == MacroState::Complete) return false;

    if (macro->state == MacroState::Delay || macro->state == MacroState::DelayC) {
        if (millis() < macro->until) return true;

        if (macro->state == MacroState::Delay) {
            if (macro->info.pressed) {
                MiniKeyboard.release(macro->info.pressed);
                macro->info.pressed = KEY_RESERVED;
                macro->state = MacroState::Delay;
                macro->until = millis() + macro->delay;
                return true;
            }
        } else {
            if (macro->info.pressed_c) {
                MiniKeyboard.release(macro->info.pressed_c);
                macro->info.pressed_c = 0;
                macro->state = MacroState::Delay;
                macro->until = millis() + macro->delay;
                return true;
            }
        }

        macro->state = MacroState::Running;
    }

    switch ((MacroOp)(macro->stack++)[0]) {
        case MacroOp::END:
            macro->state = MacroState::Complete;
            return false;
        case MacroOp::DELAY:
            macro->state = MacroState::Delay;
            macro->until = millis() + (macro->stack++)[0];
            break;
        case MacroOp::PAUSE:
            macro->state = MacroState::Delay;
            macro->until = millis() + macro->delay;
            break;
        case MacroOp::SET_PAUSE:
            macro->delay = (macro->stack++)[0];
            break;

        case MacroOp::PRESS:
            macro->info.pressed = (KeyboardKeycode)(macro->stack++)[0];
            MiniKeyboard.press(macro->info.pressed);
            macro->until = millis() + macro->delay;
            macro->state = MacroState::Delay;
            break;
        case MacroOp::PRESS_C:
            macro->info.pressed_c = (macro->stack++)[0];
            MiniKeyboard.press(macro->info.pressed_c);
            macro->until = millis() + macro->delay;
            macro->state = MacroState::DelayC;
            break;

        case MacroOp::DOWN:
            MiniKeyboard.press((KeyboardKeycode)(macro->stack++)[0]);
            break;
        case MacroOp::DOWN_C:
            MiniKeyboard.press((macro->stack++)[0]);
            break;

        case MacroOp::UP:
            MiniKeyboard.release((KeyboardKeycode)(macro->stack++)[0]);
            break;
        case MacroOp::UP_C:
            MiniKeyboard.release((macro->stack++)[0]);
            break;

        default:
            MiniKeyboard.releaseAll();
            macro->state = MacroState::Complete;
            return false;
    }

    return true;
}

macro_state_t* active_macros = NULL;
void tick_all_macros() {
    macro_state_t* head = active_macros;
    macro_state_t* previous = NULL;

    while (head != NULL) {
        macro_state_t* next = head->next;

        if (!tick_macro(head)) {
            if (previous == NULL)
                active_macros = next;
            else
                previous->next = next;

            // free(head);
            head = next;
        } else {
            previous = head;
            head = next;
        }
    }
}

void start_macro(uint8_t* macro) {
    macro_state_t* state = (macro_state_t*)malloc(sizeof(macro_state_t));
    state->stack = macro;
    state->next = active_macros;
    state->info.pressed = KEY_RESERVED;
    state->delay = 30;
    active_macros = state;
}
