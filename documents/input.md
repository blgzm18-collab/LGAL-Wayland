# Input Handling in LGAL

LGAL provides keyboard and mouse input through the `Input` class, accessed from the `Window`.

## Basic Usage

After creating a window, call `pollEvents()` in your loop to receive input events:

```cpp
Window window(config);
while (window.pollEvents(16)) {  // 16ms timeout
    // Check keyboard state
    if (window.getInput().getKeyboard().isKeyDown(30)) {  // KEY_A
        // Handle 'A' key
    }
    
    // Check mouse state
    const auto& mouse = window.getInput().getMouse();
    int32_t x = mouse.getX();
    int32_t y = mouse.getY();
    bool left_pressed = mouse.isButtonDown(1);  // 1=left, 2=middle, 3=right
    bool over_window = mouse.isOverWindow();
}
```

## Keyboard

Key codes follow USB HID page 7 (Keyboard/Keypad), offset by 8:
- `KEY_A = 30`, `KEY_B = 48`, etc.
- `KEY_SPACE = 44`, `KEY_ESC = 1`, `KEY_ENTER = 28`, `KEY_TAB = 15`
- `KEY_UP = 103`, `KEY_DOWN = 108`, `KEY_LEFT = 105`, `KEY_RIGHT = 106`
- `KEY_LSHIFT = 42`, `KEY_LCTRL = 29`, `KEY_LALT = 56`, `KEY_LMETA = 125`

The header `<linux/input.h>` (if available) has a full list under `#define KEY_*`.

**Keyboard State:**
- `getKeyboard().isKeyDown(code)` - True if the key is currently held
- `getKeyboard().getModifiers()` - Returns a `Modifiers` struct with `.shift`, `.ctrl`, `.alt`, `.super`, `.capsLock`, `.numLock`

**Note:** Text input (character composition, repeating, IME) is not yet implemented. Only key codes.

## Mouse

**Position & Buttons:**
- `getMouse().getX()`, `getMouse().getY()` - Pixel position in window coordinates (0,0 = top-left)
- `getMouse().isButtonDown(button)` - 1=left, 2=middle, 3=right (or higher for extra buttons)
- `getMouse().isOverWindow()` - True if the pointer is over the window

**Note:** Scroll wheel is not yet implemented.

## Example

See `examples/input_demo.cpp` for a complete example that draws a crosshair at the mouse position and shows which buttons and keys are pressed.

Build it with:
```bash
cmake -B build && cmake --build build
./build/lgal_input_demo
```

## Limitations

- **Text input:** Only key codes, not characters. No key repeat handling. No IME support.
- **Scroll wheel:** Axis events are not exposed yet.
- **Pointer shape:** Cursor always uses the default pointer.
- **Multi-touch:** Not supported (Wayland has `wl_touch` but it's not exposed).

## Future Work
ph xkbcommon
- Key repeat via timers
- Scroll wheel exposure
- Custom cursor shapes
- Touch support