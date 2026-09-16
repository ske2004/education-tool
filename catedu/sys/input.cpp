#include "input.hpp"
#include <memory.h>

#define DOWN true
#define UP false

static bool boundcheck(int k)
{
    return k >= 0 && k < INPUT_COUNT;
}

static void key_on(InputKey &key, bool down)
{
    key.pressed = down;
    key.held = down;
    key.released = !down;
    key.repeats += int(down);
}

static void write_char(Input &input, uint32_t chr)
{
    if (input.input_len < 31)
    {
        input.input[input.input_len++] = chr;
        input.input[input.input_len] = 0;
    }
}

static void handle_key_event(Input &input, int key_code, bool down)
{
    if (key_code < 0 || key_code >= INPUT_COUNT)
    {
        return;
    }

    InputKey *target = input.k;

    // If a modifier key is held, use the mk array
    if (target[INPUT_CTRL].held || input.k[INPUT_SHIFT].held ||
        target[INPUT_ALT].held)
    {
        target = input.mk;
    }

    key_on(target[key_code], down);

    switch (key_code)
    {
    // Do not mask modifier keys
    case SAPP_KEYCODE_LEFT_CONTROL:
    case SAPP_KEYCODE_RIGHT_CONTROL:
        key_on(input.k[INPUT_CTRL], down);
        break;
    case SAPP_KEYCODE_LEFT_SHIFT:
    case SAPP_KEYCODE_RIGHT_SHIFT:
        key_on(input.k[INPUT_SHIFT], down);
        break;
    case SAPP_KEYCODE_LEFT_ALT:
    case SAPP_KEYCODE_RIGHT_ALT:
        key_on(input.k[INPUT_ALT], down);
        break;
    case SAPP_KEYCODE_ENTER:
        if (down)
        {
            write_char(input, '\n');
        }
        break;
    case SAPP_KEYCODE_BACKSPACE:
        if (down)
        {
            write_char(input, '\b');
        }
        break;
    default:
        break;
    }
}

static int get_key_modifiers(Input &input)
{
    int modifiers = 0;
    if (input.k[INPUT_CTRL].held)
        modifiers |= MOD_CTRL;
    if (input.k[INPUT_SHIFT].held)
        modifiers |= MOD_SHIFT;
    if (input.k[INPUT_ALT].held)
        modifiers |= MOD_ALT;
    return modifiers;
}

Input Input::init()
{
    return Input{};
}

void Input::update()
{
    // Reset deltas
    this->mouse_delta = {0, 0};
    this->mouse_wheel = 0;

    // Reset one-frame input states
    for (int i = 0; i < sizeof(Input::k) / sizeof(InputKey); ++i)
    {
        this->k[i].pressed = false;
        this->k[i].released = false;
        this->k[i].repeats = 0;
    }

    for (int i = 0; i < sizeof(Input::mk) / sizeof(InputKey); ++i)
    {
        this->mk[i].pressed = false;
        this->mk[i].released = false;
        this->mk[i].repeats = 0;
    }

    this->input_len = 0;
    memset(this->input, 0, 32 * sizeof(char));
}

void Input::reset()
{
    this->mouse_delta = {0, 0};
    this->mouse_wheel = 0;

    // Reset one-frame input states
    for (int i = 0; i < sizeof(Input::k) / sizeof(InputKey); ++i)
    {
        this->k[i] = {};
    }

    for (int i = 0; i < sizeof(Input::mk) / sizeof(InputKey); ++i)
    {
        this->mk[i] = {};
    }

    this->input_len = 0;
    memset(this->input, 0, 32 * sizeof(char));
}

void Input::pass_event(const sapp_event *event)
{
    size_t mouse_button = INPUT_MB_LEFT + event->mouse_button;

    switch (event->type)
    {
    case SAPP_EVENTTYPE_KEY_UP:
    case SAPP_EVENTTYPE_KEY_DOWN:
        handle_key_event(*this, event->key_code,
                         event->type == SAPP_EVENTTYPE_KEY_DOWN);
        break;
    case SAPP_EVENTTYPE_MOUSE_UP:
    case SAPP_EVENTTYPE_MOUSE_DOWN:
        if (event->mouse_button >= 0 && event->mouse_button < 3)
            key_on(this->k[mouse_button],
                   event->type == SAPP_EVENTTYPE_MOUSE_DOWN);
        break;
    case SAPP_EVENTTYPE_MOUSE_MOVE:
        this->mouse_pos = {event->mouse_x, event->mouse_y};
        this->mouse_delta += {event->mouse_dx, event->mouse_dy};
        break;
    case SAPP_EVENTTYPE_MOUSE_SCROLL:
        this->mouse_wheel =
            event->scroll_y == 0 ? 0 : (event->scroll_y < 0 ? -1 : 1);
        break;
    case SAPP_EVENTTYPE_CHAR:
        if (event->char_code >= 32 && event->char_code < 127 &&
            this->input_len < 31)
        {
            this->input[this->input_len++] = event->char_code;
            this->input[this->input_len] = 0;
        }
        break;
    default:
        break;
    }
}

void Input::clear(int key)
{
    this->k[key].pressed = false;
    this->k[key].released = false;
    this->mk[key].pressed = false;
    this->mk[key].released = false;
}

bool Input::repeating(int key)
{
    if (boundcheck(key) && this->k[key].repeats > 0)
    {
        this->k[key].repeats--;
        return true;
    }
    return false;
}

bool Input::shortcut(int modifiers, int key)
{
    if (modifiers == get_key_modifiers(*this))
    {
        bool valid = this->mk[key].pressed;
        this->clear(key);
        return valid;
    }
    return false;
}
