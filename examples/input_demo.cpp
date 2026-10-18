// examples/input_demo.cpp - Keyboard and mouse input demonstration
#include "lgal_shm.hpp"
#include "lgal_input.hpp"
#include <iostream>

using namespace lgal::core;

static void drawUI(SharedBuffer& buf, const Input& input) {
    // Clear to dark background
    const uint32_t bg = 0x1a1a1a;
    for (size_t i = 0; i < buf.width * buf.height; ++i) {
        buf.memory[i] = bg;
    }

    // Draw a crosshair at the mouse position
    const int32_t mx = input.getMouse().getX();
    const int32_t my = input.getMouse().getY();
    const uint32_t cross = 0xFFFFFF;
    const int32_t size = 15;

    if (input.getMouse().isOverWindow()) {
        // Horizontal line
        for (int32_t x = mx - size; x <= mx + size; ++x) {
            if (x >= 0 && x < static_cast<int32_t>(buf.width)) {
                buf.memory[my * buf.width + x] = cross;
            }
        }
        // Vertical line
        for (int32_t y = my - size; y <= my + size; ++y) {
            if (y >= 0 && y < static_cast<int32_t>(buf.height)) {
                buf.memory[y * buf.width + mx] = cross;
            }
        }
    }

    // Draw button state indicators in the top-left corner
    const uint32_t text_x = 10, text_y = 10, pitch = buf.width;
    auto drawChar = [&](char c, uint32_t x, uint32_t y, uint32_t color) {
        // Simple 3x5 pixel glyphs (very minimal)
        if (c == 'L' && input.getMouse().isButtonDown(1)) {
            for (uint32_t dy = 0; dy < 5; ++dy) {
                for (uint32_t dx = 0; dx < 3; ++dx) {
                    buf.memory[(y + dy) * pitch + (x + dx)] = color;
                }
            }
        } else if (c == 'M' && input.getMouse().isButtonDown(2)) {
            for (uint32_t dy = 0; dy < 5; ++dy) {
                for (uint32_t dx = 0; dx < 3; ++dx) {
                    buf.memory[(y + dy) * pitch + (x + dx)] = color;
                }
            }
        } else if (c == 'R' && input.getMouse().isButtonDown(3)) {
            for (uint32_t dy = 0; dy < 5; ++dy) {
                for (uint32_t dx = 0; dx < 3; ++dx) {
                    buf.memory[(y + dy) * pitch + (x + dx)] = color;
                }
            }
        }
    };

    const uint32_t button_color = 0x00FF00;
    drawChar('L', text_x, text_y, button_color);
    drawChar('M', text_x + 8, text_y, button_color);
    drawChar('R', text_x + 16, text_y, button_color);

    // Print key states in text (simple approach: just fill rectangles for held keys)
    // WASD movement keys
    uint32_t key_y = text_y + 20;
    const KeyboardState& kb = input.getKeyboard();
    const uint32_t key_w = input.getMouse().isButtonDown(1) ? 0xFF6633 : 0x333333;
    const uint32_t key_a = input.getMouse().isButtonDown(1) ? 0xFF6633 : 0x333333;
    const uint32_t key_s = input.getMouse().isButtonDown(1) ? 0xFF6633 : 0x333333;
    const uint32_t key_d = input.getMouse().isButtonDown(1) ? 0xFF6633 : 0x333333;

    // Draw key indicators if keys are pressed
    if (kb.isKeyDown(17)) {  // W
        for (uint32_t y = key_y; y < key_y + 8; ++y)
            for (uint32_t x = text_x; x < text_x + 6; ++x)
                buf.memory[y * pitch + x] = 0xFF6633;
    }
    if (kb.isKeyDown(30)) {  // A
        for (uint32_t y = key_y + 10; y < key_y + 18; ++y)
            for (uint32_t x = text_x; x < text_x + 6; ++x)
                buf.memory[y * pitch + x] = 0xFF6633;
    }
    if (kb.isKeyDown(31)) {  // S
        for (uint32_t y = key_y + 10; y < key_y + 18; ++y)
            for (uint32_t x = text_x; x < text_x + 6; ++x)
                buf.memory[y * pitch + x] = 0xFF6633;
    }
    if (kb.isKeyDown(32)) {  // D
        for (uint32_t y = key_y + 10; y < key_y + 18; ++y)
            for (uint32_t x = text_x + 8; x < text_x + 14; ++x)
                buf.memory[y * pitch + x] = 0xFF6633;
    }
}

int main() {
    WindowConfig config;
    config.title  = "LGAL Input Demo";
    config.appId  = "lgal.input-demo";
    config.width  = 1280;
    config.height = 720;

    Window window(config);
    if (!window.isValid()) {
        std::cerr << "[Input Demo] Window creation failed\n";
        return 1;
    }

    Swapchain swapchain;
    std::cout << "[Input Demo] Window open. Move the mouse, click buttons, and press keys.\n"
              << "[Input Demo] WASD and mouse buttons show in the top-left corner.\n"
              << "[Input Demo] Close the window to exit.\n";

    while (window.pollEvents(16)) {
        if (window.consumeResize() || !swapchain.isValid()) {
            if (!swapchain.create(window.getNativeShm(), window.getWidth(), window.getHeight())) {
                std::cerr << "[Input Demo] Buffer creation failed\n";
                return 1;
            }
        }

        if (!window.readyForFrame()) continue;
        SharedBuffer* buf = swapchain.acquire();
        if (!buf) continue;

        drawUI(*buf, window.getInput());
        window.present(*buf);
    }

    std::cout << "[Input Demo] Exiting\n";
    return 0;
}