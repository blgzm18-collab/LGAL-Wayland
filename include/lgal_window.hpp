// lgal_window.hpp
#pragma once

#include <cstdint>
#include <cstddef>
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include "xdg-decoration-unstable-v1-client-protocol.h"
#include "lgal_input.hpp"

namespace lgal {
namespace core {

    struct WindowConfig {
        const char* title = "LGAL Render Output";
        const char* appId = "lgal.app";
        uint32_t width    = 1280;
        uint32_t height   = 720;
        bool resizable    = true;
        bool preferServerDecorations = true;
    };

    struct SharedBuffer {
        uint32_t* memory = nullptr;
        size_t sizeInBytes = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t stride = 0;
        bool busy = false;
        wl_buffer* waylandBuffer = nullptr;
    };

    class Window {
    public:
        explicit Window(const WindowConfig& config);
        ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        bool isValid() const { return m_valid; }
        bool pollEvents(int timeoutMs = 0);
        bool readyForFrame() const { return m_valid && m_configured && !m_frameCallback; }
        bool present(SharedBuffer& buffer);
        bool consumeResize();

        void setTitle(const char* title);
        bool hasServerDecorations() const { return m_serverDecorations; }

        wl_display* getNativeDisplay() const { return m_display; }
        wl_surface* getNativeSurface() const { return m_surface; }
        wl_shm* getNativeShm() const { return m_shm; }

        uint32_t getWidth() const { return m_width; }
        uint32_t getHeight() const { return m_height; }

        Input& getInput() { return m_input; }
        const Input& getInput() const { return m_input; }

    private:
        bool initWayland(const WindowConfig& config);
        bool flushDisplay();
        void cleanup();

        static void registryHandleGlobal(void* data, wl_registry* registry,
                                         uint32_t id, const char* interface, uint32_t version);
        static void registryHandleGlobalRemove(void* data, wl_registry* registry, uint32_t id);

        static void xdgWmBasePing(void* data, xdg_wm_base* xdg, uint32_t serial);
        static void xdgSurfaceConfigure(void* data, xdg_surface* surface, uint32_t serial);
        static void xdgToplevelConfigure(void* data, xdg_toplevel* toplevel,
                                         int32_t width, int32_t height, wl_array* states);
        static void xdgToplevelClose(void* data, xdg_toplevel* toplevel);
        static void decorConfigure(void* data, zxdg_toplevel_decoration_v1*, uint32_t mode);
        static void frameDone(void* data, wl_callback* callback, uint32_t time);

        static const wl_registry_listener                 s_registryListener;
        static const xdg_wm_base_listener                 s_wmBaseListener;
        static const xdg_surface_listener                 s_surfaceListener;
        static const xdg_toplevel_listener                s_toplevelListener;
        static const zxdg_toplevel_decoration_v1_listener s_decorListener;
        static const wl_callback_listener                 s_frameListener;

        uint32_t m_width;
        uint32_t m_height;
        uint32_t m_pendingWidth;
        uint32_t m_pendingHeight;
        bool m_shouldClose      = false;
        bool m_valid            = false;
        bool m_configured       = false;
        bool m_resizePending    = false;
        bool m_serverDecorations = false;

        wl_display*    m_display    = nullptr;
        wl_registry*   m_registry   = nullptr;
        wl_compositor* m_compositor = nullptr;
        wl_surface*    m_surface    = nullptr;
        wl_shm*        m_shm        = nullptr;
        wl_callback*   m_frameCallback = nullptr;

        xdg_wm_base*  m_xdgWmBase   = nullptr;
        xdg_surface*  m_xdgSurface  = nullptr;
        xdg_toplevel* m_xdgToplevel = nullptr;

        zxdg_decoration_manager_v1*  m_decorManager  = nullptr;
        zxdg_toplevel_decoration_v1* m_toplevelDecor = nullptr;

        Input m_input;
    };

} // namespace core
} // namespace lgal