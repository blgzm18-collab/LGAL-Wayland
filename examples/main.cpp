// examples/main.cpp
#include "lgal_shm.hpp"
#include <iostream>
#include <raylib.h>

using namespace lgal::core;

static void drawGradient(SharedBuffer& buf, uint32_t frame) {
    const uint32_t pitch = buf.stride / 4;   // pixels per row
    for (uint32_t y = 0; y < buf.height; ++y) {
        for (uint32_t x = 0; x < buf.width; ++x) {
            const uint32_t r = (x + frame) & 0xFF;
            const uint32_t g = (y + frame / 2) & 0xFF;
            const uint32_t b = 0x80;
            buf.memory[y * pitch + x] = (r << 16) | (g << 8) | b;   // XRGB8888
        }
    }
}

int main() {
    WindowConfig config;
    config.title  = "LGAL Sandbox";
    config.appId  = "lgal.sandbox";
    config.width  = 800;
    config.height = 600;

    Window window(config);
    if (!window.isValid()) {
        std::cerr << "[LGAL Sandbox] Window creation failed (are you on a Wayland session?)\n";
        return 1;
    }

    // Declared after the window so it is destroyed first
    Swapchain swapchain;

    std::cout << "[LGAL Sandbox] Window open ("
              << (window.hasServerDecorations() ? "server-side decorations" : "no decorations")
              << "). Close it to exit.\n";

    uint32_t frame = 0;
    // pollEvents(16) sleeps until an event arrives or 16 ms pass, so the loop
    // doesn't spin the CPU
    while (window.pollEvents(16)) {
        // (Re)create buffers on first run and whenever the window is resized
        if (window.consumeResize() || !swapchain.isValid()) {
            if (!swapchain.create(window.getNativeShm(), window.getWidth(), window.getHeight())) {
                std::cerr << "[LGAL Sandbox] Buffer creation failed\n";
                return 1;
            }
        }

        // Only draw when the compositor wants a new frame and a buffer is free
        if (!window.readyForFrame()) continue;
        SharedBuffer* buf = swapchain.acquire();
        if (!buf) continue;

        drawGradient(*buf, frame++);
        window.present(*buf);
    }

    std::cout << "[LGAL Sandbox] Exiting\n";
    return 0;
}