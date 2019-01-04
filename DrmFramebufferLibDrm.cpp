// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2019 Stephan Gerhold

#define LOG_TAG "drmfb-framebuffer-libdrm"

#include <android-base/logging.h>
#include <drm/drm_fourcc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <system/graphics.h>

#include <android/gralloc_handle.h>
#include "DrmFramebufferImporter.h"

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace drmfb {
namespace libdrm {

namespace {
static uint32_t convertAndroidToDrmFbFormat(uint32_t format) {
    switch (format) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
        /* Avoid using alpha bits for the framebuffer.
         * They are not supported on older Intel GPUs for primary planes. */
    case HAL_PIXEL_FORMAT_RGBX_8888:
        return DRM_FORMAT_XBGR8888;
    case HAL_PIXEL_FORMAT_RGB_888:
        return DRM_FORMAT_BGR888;
    case HAL_PIXEL_FORMAT_RGB_565:
        return DRM_FORMAT_BGR565;
    case HAL_PIXEL_FORMAT_BGRA_8888:
        return DRM_FORMAT_ARGB8888;
    default:
        LOG(ERROR) << "Unsupported framebuffer format: " << format;
        return 0;
    }
}

void addFramebuffer(int fd, struct gralloc_handle_t* handle, uint32_t* id) {
    uint32_t handles[4] = {};
    uint32_t pitches[4] = {handle->stride};
    uint32_t offsets[4] = {};

    if (drmPrimeFDToHandle(fd, handle->prime_fd, &handles[0])) {
        PLOG(ERROR) << "Failed to get handle for prime fd " << handle->prime_fd;
        return;
    }

    if (drmModeAddFB2(fd, handle->width, handle->height,
            convertAndroidToDrmFbFormat(handle->format),
            handles, pitches, offsets, id, 0)) {
        PLOG(ERROR) << "drmModeAddFB2 failed";
    }
}
}

bool addFramebuffer(int fd, buffer_handle_t buffer, uint32_t* id) {
    if (buffer->numFds != GRALLOC_HANDLE_NUM_FDS
            || buffer->numInts < static_cast<int>(GRALLOC_HANDLE_NUM_INTS))
        return false;

    auto handle = gralloc_handle(buffer);
    if (handle->magic != GRALLOC_HANDLE_MAGIC)
        return false;

    if (handle->version != GRALLOC_HANDLE_VERSION) {
        LOG(ERROR) << "gralloc_handle_t version mismatch: expected "
			<< GRALLOC_HANDLE_VERSION << ", got " << handle->version;
        return true;
    }

    addFramebuffer(fd, handle, id);
    return true;
}

}  // namespace libdrm
}  // namespace implementation
}  // namespace V2_1
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
