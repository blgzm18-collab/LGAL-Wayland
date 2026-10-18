/* lgal.h - C API for LGAL */
#ifndef LGAL_H
#define LGAL_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct lgal_window lgal_window_t;

/* A frame to draw into. Pixels are XRGB8888: 0x00RRGGBB. */
typedef struct lgal_frame {
    uint32_t* pixels;
    uint32_t  width;
    uint32_t  height;
    uint32_t  stride;   /* bytes per row */
} lgal_frame_t;

/* Returns NULL on failure (e.g. no Wayland session). */
lgal_window_t* lgal_window_create(uint32_t width, uint32_t height, const char* title);
void           lgal_window_destroy(lgal_window_t* window);

/* Both return false when the window should close. */
bool lgal_window_poll_events(lgal_window_t* window);
bool lgal_window_wait_events(lgal_window_t* window, int timeout_ms);

uint32_t lgal_window_width(const lgal_window_t* window);
uint32_t lgal_window_height(const lgal_window_t* window);

/* Draw loop:
 *   if (lgal_window_begin_frame(w, &f)) { ...write f.pixels...; lgal_window_end_frame(w); }
 * begin_frame returns false when no frame should be drawn right now. */
bool lgal_window_begin_frame(lgal_window_t* window, lgal_frame_t* frame);
bool lgal_window_end_frame(lgal_window_t* window);

#ifdef __cplusplus
}
#endif

#endif /* LGAL_H */