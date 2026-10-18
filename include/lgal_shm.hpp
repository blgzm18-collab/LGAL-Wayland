// lgal_shm.hpp
#pragma once

#include "lgal_window.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace lgal {
namespace core {

    class SharedBufferAllocator {
    public:
        // Allocates a shared memory buffer and wraps it in a Wayland wl_buffer.
        // On failure the returned buffer has memory == nullptr.
        static SharedBuffer createBuffer(wl_shm* shm, uint32_t width, uint32_t height);

        // Frees the mapped memory and destroys the Wayland buffer handle
        static void destroyBuffer(SharedBuffer& buffer);
    };

    // Double-buffered set of SharedBuffers. Draw into acquire(), then hand the
    // buffer to Window::present(). Tracks wl_buffer.release so you never draw
    // into a buffer the compositor is still reading (no tearing/flicker).
    //
    // Must be destroyed (or destroy()ed) BEFORE the Window it was created from.
    // Not copyable or movable: the release listeners point into this object.
    class Swapchain {
    public:
        static constexpr size_t kBufferCount = 2;

        Swapchain() = default;
        ~Swapchain() { destroy(); }

        Swapchain(const Swapchain&) = delete;
        Swapchain& operator=(const Swapchain&) = delete;

        // (Re)creates the buffers at the given size, e.g. after a resize.
        bool create(wl_shm* shm, uint32_t width, uint32_t height);
        void destroy();

        bool isValid() const { return m_buffers[0].waylandBuffer != nullptr; }

        // Returns a buffer that is free to draw into, or nullptr if the
        // compositor still holds all of them.
        SharedBuffer* acquire();

    private:
        static void onBufferRelease(void* data, wl_buffer* buffer);
        static const wl_buffer_listener s_bufferListener;

        std::array<SharedBuffer, kBufferCount> m_buffers{};
    };

} // namespace core
} // namespace lgal