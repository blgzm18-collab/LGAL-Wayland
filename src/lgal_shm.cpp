// lgal_shm.cpp
#include "lgal_shm.hpp"
#include <cstdint>
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <unistd.h>

namespace lgal {
namespace core {

    SharedBuffer SharedBufferAllocator::createBuffer(wl_shm* shm, uint32_t width, uint32_t height) {
        SharedBuffer buf;
        if (!shm || width == 0 || height == 0) return buf;

        const uint64_t stride = static_cast<uint64_t>(width) * 4;   // XRGB8888
        const uint64_t size   = stride * height;
        // wl_shm_create_pool takes an int32 size
        if (size > static_cast<uint64_t>(INT32_MAX)) {
            std::cerr << "[LGAL Error] Buffer too large!\n";
            return buf;
        }

        // 1. Anonymous RAM file descriptor (sealable so the compositor is
        //    protected from us shrinking the file under it)
        int fd = memfd_create("lgal_shm_framebuffer", MFD_CLOEXEC | MFD_ALLOW_SEALING);
        if (fd < 0) {
            std::cerr << "[LGAL Error] Failed to execute memfd_create!\n";
            return buf;
        }

        // 2. Set memory size
        if (ftruncate(fd, static_cast<off_t>(size)) < 0) {
            std::cerr << "[LGAL Error] Failed to truncate shared memory file!\n";
            close(fd);
            return buf;
        }
        fcntl(fd, F_ADD_SEALS, F_SEAL_SHRINK | F_SEAL_SEAL);   // best effort

        // 3. Map into our address space
        void* mapped = mmap(nullptr, static_cast<size_t>(size), PROT_READ | PROT_WRITE,
                            MAP_SHARED, fd, 0);
        if (mapped == MAP_FAILED) {
            std::cerr << "[LGAL Error] mmap failed!\n";
            close(fd);
            return buf;
        }

        // 4. Wayland pool + buffer
        wl_shm_pool* pool = wl_shm_create_pool(shm, fd, static_cast<int32_t>(size));
        // The fd was duplicated into the request; our copy is no longer needed.
        // The mapping stays valid after close().
        close(fd);
        if (!pool) {
            std::cerr << "[LGAL Error] wl_shm_create_pool failed!\n";
            munmap(mapped, static_cast<size_t>(size));
            return buf;
        }

        wl_buffer* wlBuffer = wl_shm_pool_create_buffer(
            pool, 0,
            static_cast<int32_t>(width),
            static_cast<int32_t>(height),
            static_cast<int32_t>(stride),
            WL_SHM_FORMAT_XRGB8888);

        // The buffer stays valid after the pool proxy is destroyed
        wl_shm_pool_destroy(pool);

        if (!wlBuffer) {
            std::cerr << "[LGAL Error] wl_shm_pool_create_buffer failed!\n";
            munmap(mapped, static_cast<size_t>(size));
            return buf;
        }

        buf.memory        = static_cast<uint32_t*>(mapped);
        buf.sizeInBytes   = static_cast<size_t>(size);
        buf.width         = width;
        buf.height        = height;
        buf.stride        = static_cast<uint32_t>(stride);
        buf.waylandBuffer = wlBuffer;
        return buf;
    }

    void SharedBufferAllocator::destroyBuffer(SharedBuffer& buffer) {
        if (buffer.waylandBuffer) {
            wl_buffer_destroy(buffer.waylandBuffer);
        }
        if (buffer.memory && buffer.sizeInBytes > 0) {
            munmap(buffer.memory, buffer.sizeInBytes);
        }
        buffer = SharedBuffer{};
    }

    // ---- Swapchain ----
    const wl_buffer_listener Swapchain::s_bufferListener = {
        .release = Swapchain::onBufferRelease,
    };

    void Swapchain::onBufferRelease(void* data, wl_buffer*) {
        static_cast<SharedBuffer*>(data)->busy = false;
    }

    bool Swapchain::create(wl_shm* shm, uint32_t width, uint32_t height) {
        destroy();

        for (auto& slot : m_buffers) {
            slot = SharedBufferAllocator::createBuffer(shm, width, height);
            if (!slot.waylandBuffer) {
                destroy();
                return false;
            }
            // Add the listener after the buffer sits at its final address
            wl_buffer_add_listener(slot.waylandBuffer, &s_bufferListener, &slot);
        }
        return true;
    }

    void Swapchain::destroy() {
        for (auto& slot : m_buffers) {
            SharedBufferAllocator::destroyBuffer(slot);
        }
    }

    SharedBuffer* Swapchain::acquire() {
        for (auto& slot : m_buffers) {
            if (slot.waylandBuffer && !slot.busy) return &slot;
        }
        return nullptr;
    }

} // namespace core
} // namespace lgal