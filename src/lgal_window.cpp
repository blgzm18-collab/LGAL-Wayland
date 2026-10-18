// lgal_window.cpp
#include "lgal_window.hpp"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <poll.h>

namespace lgal {
namespace core {

    // ---- Listener tables (static members, so they can use private callbacks) ----
    const xdg_wm_base_listener Window::s_wmBaseListener = {
        .ping = Window::xdgWmBasePing,
    };

    const xdg_surface_listener Window::s_surfaceListener = {
        .configure = Window::xdgSurfaceConfigure,
    };

    // Newer xdg-shell versions add configure_bounds / wm_capabilities; they stay
    // nullptr here, which is fine because we bind xdg_wm_base v1.
    const xdg_toplevel_listener Window::s_toplevelListener = {
        .configure = Window::xdgToplevelConfigure,
        .close     = Window::xdgToplevelClose,
    };

    const wl_registry_listener Window::s_registryListener = {
        .global        = Window::registryHandleGlobal,
        .global_remove = Window::registryHandleGlobalRemove,
    };

    const zxdg_toplevel_decoration_v1_listener Window::s_decorListener = {
        .configure = Window::decorConfigure,
    };

    const wl_callback_listener Window::s_frameListener = {
        .done = Window::frameDone,
    };

    // ---- Callbacks ----
    void Window::xdgWmBasePing(void*, xdg_wm_base* xdg, uint32_t serial) {
        xdg_wm_base_pong(xdg, serial);
    }

    void Window::xdgToplevelConfigure(void* data, xdg_toplevel*,
                                      int32_t width, int32_t height, wl_array*) {
        // 0 means "you choose". The size is applied on the following
        // xdg_surface.configure, which is the atomic point.
        auto* window = static_cast<Window*>(data);
        if (width > 0 && height > 0) {
            window->m_pendingWidth  = static_cast<uint32_t>(width);
            window->m_pendingHeight = static_cast<uint32_t>(height);
        }
    }

    void Window::xdgSurfaceConfigure(void* data, xdg_surface* surface, uint32_t serial) {
        auto* window = static_cast<Window*>(data);
        xdg_surface_ack_configure(surface, serial);

        if (window->m_pendingWidth != window->m_width ||
            window->m_pendingHeight != window->m_height) {
            window->m_width  = window->m_pendingWidth;
            window->m_height = window->m_pendingHeight;
            window->m_resizePending = true;
        }
        window->m_configured = true;
    }

    void Window::xdgToplevelClose(void* data, xdg_toplevel*) {
        static_cast<Window*>(data)->m_shouldClose = true;
    }

    void Window::decorConfigure(void* data, zxdg_toplevel_decoration_v1*, uint32_t mode) {
        static_cast<Window*>(data)->m_serverDecorations =
            (mode == ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
    }

    void Window::frameDone(void* data, wl_callback* callback, uint32_t) {
        auto* window = static_cast<Window*>(data);
        wl_callback_destroy(callback);
        window->m_frameCallback = nullptr;
    }

    void Window::registryHandleGlobal(void* data, wl_registry* registry,
                                      uint32_t id, const char* interface, uint32_t version) {
        auto* window = static_cast<Window*>(data);

        if (std::strcmp(interface, wl_compositor_interface.name) == 0) {
            // v4 is needed for wl_surface.damage_buffer
            window->m_compositor = static_cast<wl_compositor*>(
                wl_registry_bind(registry, id, &wl_compositor_interface,
                                 std::min<uint32_t>(version, 4)));
        }
        else if (std::strcmp(interface, wl_shm_interface.name) == 0) {
            window->m_shm = static_cast<wl_shm*>(
                wl_registry_bind(registry, id, &wl_shm_interface, 1));
        }
        else if (std::strcmp(interface, xdg_wm_base_interface.name) == 0) {
            window->m_xdgWmBase = static_cast<xdg_wm_base*>(
                wl_registry_bind(registry, id, &xdg_wm_base_interface, 1));
            xdg_wm_base_add_listener(window->m_xdgWmBase, &s_wmBaseListener, window);
        }
        else if (std::strcmp(interface, zxdg_decoration_manager_v1_interface.name) == 0) {
            window->m_decorManager = static_cast<zxdg_decoration_manager_v1*>(
                wl_registry_bind(registry, id, &zxdg_decoration_manager_v1_interface, 1));
        }
        else if (std::strcmp(interface, wl_seat_interface.name) == 0) {
            wl_seat* seat = static_cast<wl_seat*>(
                wl_registry_bind(registry, id, &wl_seat_interface, 7));
            window->m_input.bindSeat(seat);
        }
    }

    void Window::registryHandleGlobalRemove(void*, wl_registry*, uint32_t) {}

    // ---- Lifetime ----
    Window::Window(const WindowConfig& config)
        : m_width(config.width), m_height(config.height),
          m_pendingWidth(config.width), m_pendingHeight(config.height) {
        m_valid = initWayland(config);
        if (!m_valid) {
            std::cerr << "[LGAL Error] Failed to initialize Wayland xdg-shell window!\n";
        }
    }

    Window::~Window() {
        cleanup();
    }

    bool Window::initWayland(const WindowConfig& config) {
        m_display = wl_display_connect(nullptr);
        if (!m_display) {
            std::cerr << "[LGAL Error] Could not connect to a Wayland display "
                         "(is WAYLAND_DISPLAY set?)\n";
            return false;
        }

        m_registry = wl_display_get_registry(m_display);
        wl_registry_add_listener(m_registry, &s_registryListener, this);
        wl_display_roundtrip(m_display);

        if (!m_compositor || !m_shm || !m_xdgWmBase) {
            std::cerr << "[LGAL Error] Compositor is missing wl_compositor, wl_shm or xdg_wm_base\n";
            return false;
        }

        m_surface    = wl_compositor_create_surface(m_compositor);
        m_xdgSurface = xdg_wm_base_get_xdg_surface(m_xdgWmBase, m_surface);
        xdg_surface_add_listener(m_xdgSurface, &s_surfaceListener, this);

        m_xdgToplevel = xdg_surface_get_toplevel(m_xdgSurface);
        xdg_toplevel_add_listener(m_xdgToplevel, &s_toplevelListener, this);
        xdg_toplevel_set_title(m_xdgToplevel, config.title ? config.title : "LGAL");
        xdg_toplevel_set_app_id(m_xdgToplevel, config.appId ? config.appId : "lgal.app");

        if (!config.resizable) {
            xdg_toplevel_set_min_size(m_xdgToplevel, static_cast<int32_t>(config.width),
                                                     static_cast<int32_t>(config.height));
            xdg_toplevel_set_max_size(m_xdgToplevel, static_cast<int32_t>(config.width),
                                                     static_cast<int32_t>(config.height));
        }

        // Ask the compositor to draw the title bar/borders (before first commit)
        if (config.preferServerDecorations && m_decorManager) {
            m_toplevelDecor = zxdg_decoration_manager_v1_get_toplevel_decoration(
                m_decorManager, m_xdgToplevel);
            zxdg_toplevel_decoration_v1_add_listener(m_toplevelDecor, &s_decorListener, this);
            zxdg_toplevel_decoration_v1_set_mode(
                m_toplevelDecor, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
        }

        // Initial commit (no buffer) asks the compositor for the first configure
        wl_surface_commit(m_surface);
        wl_display_roundtrip(m_display);

        // Make sure we really got the first configure before anyone draws
        while (!m_configured) {
            if (wl_display_dispatch(m_display) < 0) return false;
        }
        m_resizePending = false;   // the initial size isn't a "resize" for the caller

        if (config.preferServerDecorations && !m_serverDecorations) {
            std::cerr << "[LGAL Info] No server-side decorations from this compositor; "
                         "the window will have no title bar until client-side decorations exist.\n";
        }
        return true;
    }

    bool Window::flushDisplay() {
        while (wl_display_flush(m_display) < 0) {
            if (errno != EAGAIN) return false;
            // Socket buffer full: wait until it is writable again
            pollfd pfd{};
            pfd.fd = wl_display_get_fd(m_display);
            pfd.events = POLLOUT;
            if (poll(&pfd, 1, -1) < 0 && errno != EINTR) return false;
        }
        return true;
    }

    bool Window::pollEvents(int timeoutMs) {
        if (!m_display || !m_valid) return false;

        // wl_display_dispatch_pending alone never reads from the socket, so
        // pings/close events would never arrive. Use the prepare/read pattern.
        while (wl_display_prepare_read(m_display) != 0) {
            if (wl_display_dispatch_pending(m_display) < 0) { m_valid = false; return false; }
        }
        if (!flushDisplay()) {
            wl_display_cancel_read(m_display);
            m_valid = false;
            return false;
        }

        pollfd pfd{};
        pfd.fd = wl_display_get_fd(m_display);
        pfd.events = POLLIN;

        if (poll(&pfd, 1, timeoutMs) > 0) {
            if (wl_display_read_events(m_display) < 0) { m_valid = false; return false; }
        } else {
            wl_display_cancel_read(m_display);
        }

        if (wl_display_dispatch_pending(m_display) < 0) { m_valid = false; return false; }

        return !m_shouldClose;
    }

    bool Window::present(SharedBuffer& buffer) {
        if (!readyForFrame() || !buffer.waylandBuffer) return false;

        wl_surface_attach(m_surface, buffer.waylandBuffer, 0, 0);
        wl_surface_damage_buffer(m_surface, 0, 0,
                                 static_cast<int32_t>(buffer.width),
                                 static_cast<int32_t>(buffer.height));

        // Ask to be told when this frame is shown, so we pace drawing to the display
        m_frameCallback = wl_surface_frame(m_surface);
        wl_callback_add_listener(m_frameCallback, &s_frameListener, this);

        buffer.busy = true;
        wl_surface_commit(m_surface);
        return flushDisplay();
    }

    bool Window::consumeResize() {
        const bool resized = m_resizePending;
        m_resizePending = false;
        return resized;
    }

    void Window::setTitle(const char* title) {
        if (m_xdgToplevel && title) xdg_toplevel_set_title(m_xdgToplevel, title);
    }

    void Window::cleanup() {
        // Order matters: role objects -> surface -> wm_base (destroying wm_base
        // while surfaces exist is a protocol error).
        m_input.unbindSeat();

        if (m_frameCallback) { wl_callback_destroy(m_frameCallback); m_frameCallback = nullptr; }
        if (m_toplevelDecor) { zxdg_toplevel_decoration_v1_destroy(m_toplevelDecor); m_toplevelDecor = nullptr; }
        if (m_decorManager)  { zxdg_decoration_manager_v1_destroy(m_decorManager);   m_decorManager  = nullptr; }
        if (m_xdgToplevel) { xdg_toplevel_destroy(m_xdgToplevel); m_xdgToplevel = nullptr; }
        if (m_xdgSurface)  { xdg_surface_destroy(m_xdgSurface);   m_xdgSurface  = nullptr; }
        if (m_surface)     { wl_surface_destroy(m_surface);       m_surface     = nullptr; }
        if (m_xdgWmBase)   { xdg_wm_base_destroy(m_xdgWmBase);    m_xdgWmBase   = nullptr; }
        if (m_shm)         { wl_shm_destroy(m_shm);               m_shm         = nullptr; }
        if (m_compositor)  { wl_compositor_destroy(m_compositor); m_compositor  = nullptr; }
        if (m_registry)    { wl_registry_destroy(m_registry);     m_registry    = nullptr; }
        if (m_display)     { wl_display_disconnect(m_display);    m_display     = nullptr; }
        m_valid = false;
    }

} // namespace core
} // namespace lgal