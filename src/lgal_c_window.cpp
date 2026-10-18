// lgal_c_window.cpp
#include "lgal_window.hpp"

extern "C" {

typedef struct lgal_window lgal_window_t;

lgal_window_t* lgal_window_create(uint32_t width, uint32_t height, const char* title) {
    lgal::core::WindowConfig config;
    config.width = width;
    config.height = height;
    config.title = title;

    return reinterpret_cast<lgal_window_t*>(new lgal::core::Window(config));
}

bool lgal_window_poll_events(lgal_window_t* window) {
    auto* win = reinterpret_cast<lgal::core::Window*>(window);
    return win->pollEvents();
}

void lgal_window_destroy(lgal_window_t* window) {
    delete reinterpret_cast<lgal::core::Window*>(window);
}

}