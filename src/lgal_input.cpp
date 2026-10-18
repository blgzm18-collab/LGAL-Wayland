// lgal_input.cpp
#include "lgal_input.hpp"
#include <wayland-client.h>
#include <cstring>
#include <unistd.h>

namespace lgal {
namespace core {

    bool MouseState::isButtonDown(uint32_t button) const {
        // button 1=left, 2=middle, 3=right, ... -> check bit (button-1)
        return button > 0 && (m_buttons & (1u << (button - 1)));
    }

    void MouseState::setButtonDown(uint32_t button, bool down) {
        if (button == 0) return;
        const uint32_t bit = 1u << (button - 1);
        if (down) m_buttons |= bit;
        else     m_buttons &= ~bit;
    }

    // ---- Listener tables ----
    const wl_seat_listener Input::s_seatListener = {
        .capabilities = Input::onSeatCapabilities,
        .name = Input::onSeatName,
    };

    const wl_keyboard_listener Input::s_keyboardListener = {
        .keymap = Input::onKeyboardKeymap,
        .enter = Input::onKeyboardEnter,
        .leave = Input::onKeyboardLeave,
        .key = Input::onKeyboardKey,
        .modifiers = Input::onKeyboardModifiers,
        .repeat_info = Input::onKeyboardRepeatInfo,
    };

    const wl_pointer_listener Input::s_pointerListener = {
        .enter = Input::onPointerEnter,
        .leave = Input::onPointerLeave,
        .motion = Input::onPointerMotion,
        .button = Input::onPointerButton,
        .axis = Input::onPointerAxis,
        .frame = Input::onPointerFrame,
        .axis_source = Input::onPointerAxisSource,
        .axis_stop = Input::onPointerAxisStop,
        .axis_discrete = Input::onPointerAxisDiscrete,
    };

    // ---- Callbacks ----
    void Input::onSeatCapabilities(void* data, wl_seat* seat, uint32_t caps) {
        auto* input = static_cast<Input*>(data);

        const bool has_keyboard = (caps & WL_SEAT_CAPABILITY_KEYBOARD);
        const bool has_pointer = (caps & WL_SEAT_CAPABILITY_POINTER);

        if (has_keyboard && !input->m_keyboard_device) {
            input->m_keyboard_device = wl_seat_get_keyboard(seat);
            wl_keyboard_add_listener(input->m_keyboard_device, &s_keyboardListener, input);
        } else if (!has_keyboard && input->m_keyboard_device) {
            wl_keyboard_destroy(input->m_keyboard_device);
            input->m_keyboard_device = nullptr;
        }

        if (has_pointer && !input->m_pointer_device) {
            input->m_pointer_device = wl_seat_get_pointer(seat);
            wl_pointer_add_listener(input->m_pointer_device, &s_pointerListener, input);
        } else if (!has_pointer && input->m_pointer_device) {
            wl_pointer_destroy(input->m_pointer_device);
            input->m_pointer_device = nullptr;
        }
    }

    void Input::onSeatName(void*, wl_seat*, const char*) {}

    void Input::onKeyboardKeymap(void*, wl_keyboard*, uint32_t, int32_t fd, uint32_t) {
        // For now, ignore the keymap (it's an xkbcommon description). A full
        // implementation would parse it and use xkbcommon to translate key codes
        // into actual characters and handle compose sequences.
        close(fd);
    }

    void Input::onKeyboardEnter(void*, wl_keyboard*, uint32_t, wl_surface*, wl_array*) {}
    void Input::onKeyboardLeave(void*, wl_keyboard*, uint32_t, wl_surface*) {}

    void Input::onKeyboardKey(void* data, wl_keyboard*, uint32_t, uint32_t, uint32_t key,
                              uint32_t state) {
        auto* input = static_cast<Input*>(data);
        const bool pressed = (state == WL_KEYBOARD_KEY_STATE_PRESSED);
        input->m_keyboard.setKeyDown(key, pressed);
    }

    void Input::onKeyboardModifiers(void* data, wl_keyboard*, uint32_t,
                                    uint32_t mods_depressed, uint32_t, uint32_t, uint32_t) {
        // Only depressed (currently held) modifiers matter for live state
        auto* input = static_cast<Input*>(data);
        Modifiers mods{};
        mods.shift = (mods_depressed & 1);       // shift
        mods.ctrl = (mods_depressed & 4);        // ctrl
        mods.alt = (mods_depressed & 8);         // alt (mod1)
        mods.super = (mods_depressed & 64);      // super (mod4)
        input->m_keyboard.setModifiers(mods);
    }

    void Input::onKeyboardRepeatInfo(void*, wl_keyboard*, int32_t, int32_t) {}

    void Input::onPointerEnter(void* data, wl_pointer*, uint32_t, wl_surface*,
                               wl_fixed_t sx, wl_fixed_t sy) {
        auto* input = static_cast<Input*>(data);
        input->m_mouse.setPosition(wl_fixed_to_int(sx), wl_fixed_to_int(sy));
        input->m_mouse.setFocus(true);
    }

    void Input::onPointerLeave(void* data, wl_pointer*, uint32_t, wl_surface*) {
        static_cast<Input*>(data)->m_mouse.setFocus(false);
    }

    void Input::onPointerMotion(void* data, wl_pointer*, uint32_t, wl_fixed_t sx, wl_fixed_t sy) {
        auto* input = static_cast<Input*>(data);
        input->m_mouse.setPosition(wl_fixed_to_int(sx), wl_fixed_to_int(sy));
    }

    void Input::onPointerButton(void* data, wl_pointer*, uint32_t, uint32_t, uint32_t button,
                                uint32_t state) {
        auto* input = static_cast<Input*>(data);
        const bool pressed = (state == WL_POINTER_BUTTON_STATE_PRESSED);
        input->m_mouse.setButtonDown(button, pressed);
    }

    void Input::onPointerAxis(void*, wl_pointer*, uint32_t, uint32_t, wl_fixed_t) {}
    void Input::onPointerFrame(void*, wl_pointer*) {}
    void Input::onPointerAxisSource(void*, wl_pointer*, uint32_t) {}
    void Input::onPointerAxisStop(void*, wl_pointer*, uint32_t, uint32_t) {}
    void Input::onPointerAxisDiscrete(void*, wl_pointer*, uint32_t, int32_t) {}

    // ---- Lifetime ----
    bool Input::bindSeat(wl_seat* seat) {
        if (!seat) return false;
        m_seat = seat;
        wl_seat_add_listener(m_seat, &s_seatListener, this);
        return true;
    }

    void Input::unbindSeat() {
        if (m_keyboard_device) {
            wl_keyboard_destroy(m_keyboard_device);
            m_keyboard_device = nullptr;
        }
        if (m_pointer_device) {
            wl_pointer_destroy(m_pointer_device);
            m_pointer_device = nullptr;
        }
        m_seat = nullptr;
    }

    Input::~Input() {
        unbindSeat();
    }

    void Input::dispatchEvents() {
        // Events are already dispatched by the main Wayland event loop
        // (wl_display_dispatch/read_events). This is a placeholder for future
        // work like key repeat handling via timers, or scroll accumulation.
    }

} // namespace core
} // namespace lgal