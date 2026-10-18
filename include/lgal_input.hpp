// lgal_input.hpp - Keyboard and mouse input
#pragma once

#include <cstdint>
#include <array>
#include <wayland-client.h>

namespace lgal {
namespace core {

    // Key codes: USB HID page 7 (Keyboard/Keypad), offset by 8.
    using KeyCode = uint32_t;

    struct Modifiers {
        bool shift   : 1;
        bool alt     : 1;
        bool ctrl    : 1;
        bool super   : 1;
        bool capsLock : 1;
        bool numLock : 1;

        explicit operator bool() const {
            return shift || alt || ctrl || super || capsLock || numLock;
        }
    };

    class KeyboardState {
    public:
        bool isKeyDown(KeyCode code) const {
            return code < m_keys.size() && m_keys[code];
        }

        Modifiers getModifiers() const { return m_mods; }

    private:
        friend class Input;

        void setKeyDown(KeyCode code, bool down) {
            if (code < m_keys.size()) m_keys[code] = down;
        }

        void setModifiers(Modifiers mods) { m_mods = mods; }

        std::array<bool, 256> m_keys{};
        Modifiers m_mods{};
    };

    class MouseState {
    public:
        int32_t getX() const { return m_x; }
        int32_t getY() const { return m_y; }
        bool isButtonDown(uint32_t button) const;
        bool isOverWindow() const { return m_focus; }

    private:
        friend class Input;

        void setPosition(int32_t x, int32_t y) { m_x = x; m_y = y; }
        void setButtonDown(uint32_t button, bool down);
        void setFocus(bool f) { m_focus = f; }

        int32_t m_x = 0, m_y = 0;
        uint32_t m_buttons = 0;
        bool m_focus = false;
    };

    class Input {
    public:
        Input() = default;
        ~Input();

        Input(const Input&) = delete;
        Input& operator=(const Input&) = delete;

        bool bindSeat(wl_seat* seat);
        void unbindSeat();
        void dispatchEvents();

        const KeyboardState& getKeyboard() const { return m_keyboard; }
        const MouseState& getMouse() const { return m_mouse; }

    private:
        friend class Window;

        static void onSeatCapabilities(void* data, wl_seat* seat, uint32_t caps);
        static void onSeatName(void* data, wl_seat* seat, const char* name);
        static void onKeyboardKeymap(void* data, wl_keyboard* kb, uint32_t format,
                                     int32_t fd, uint32_t size);
        static void onKeyboardEnter(void* data, wl_keyboard* kb, uint32_t serial,
                                    wl_surface* surface, wl_array* keys);
        static void onKeyboardLeave(void* data, wl_keyboard* kb, uint32_t serial,
                                    wl_surface* surface);
        static void onKeyboardKey(void* data, wl_keyboard* kb, uint32_t serial,
                                  uint32_t time, uint32_t key, uint32_t state);
        static void onKeyboardModifiers(void* data, wl_keyboard* kb, uint32_t serial,
                                        uint32_t mods_depressed, uint32_t mods_latched,
                                        uint32_t mods_locked, uint32_t group);
        static void onKeyboardRepeatInfo(void* data, wl_keyboard* kb, int32_t rate, int32_t delay);
        static void onPointerEnter(void* data, wl_pointer* ptr, uint32_t serial,
                                   wl_surface* surface, wl_fixed_t sx, wl_fixed_t sy);
        static void onPointerLeave(void* data, wl_pointer* ptr, uint32_t serial,
                                   wl_surface* surface);
        static void onPointerMotion(void* data, wl_pointer* ptr, uint32_t time,
                                    wl_fixed_t sx, wl_fixed_t sy);
        static void onPointerButton(void* data, wl_pointer* ptr, uint32_t serial,
                                    uint32_t time, uint32_t button, uint32_t state);
        static void onPointerAxis(void* data, wl_pointer* ptr, uint32_t time,
                                  uint32_t axis, wl_fixed_t value);
        static void onPointerFrame(void* data, wl_pointer* ptr);
        static void onPointerAxisSource(void* data, wl_pointer* ptr, uint32_t axis_source);
        static void onPointerAxisStop(void* data, wl_pointer* ptr, uint32_t time, uint32_t axis);
        static void onPointerAxisDiscrete(void* data, wl_pointer* ptr, uint32_t axis, int32_t discrete);

        static const wl_seat_listener s_seatListener;
        static const wl_keyboard_listener s_keyboardListener;
        static const wl_pointer_listener s_pointerListener;

        wl_seat* m_seat = nullptr;
        wl_keyboard* m_keyboard_device = nullptr;
        wl_pointer* m_pointer_device = nullptr;

        KeyboardState m_keyboard;
        MouseState m_mouse;
    };

    class Window;  // forward declare

} // namespace core
} // namespace lgal