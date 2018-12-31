// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2019 Stephan Gerhold

#define LOG_TAG "drmfb-framebuffer"

#include <android-base/logging.h>
#include <drm/drm_fourcc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <system/graphics.h>
#include <android/gralloc_handle.h>
#include "DrmFramebuffer.h"
#include "DrmDevice.h"

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace drmfb {

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

uint32_t addFramebuffer(int fd, buffer_handle_t buffer) {
    auto handle = gralloc_handle(buffer);
    if (!handle) {
        LOG(ERROR) << "Failed to add framebuffer for buffer " << buffer
            << ": Not a libdrm gralloc handle";
        return 0;
    }

    // Lookup the handle for the prime fd
    uint32_t id;
    if (drmPrimeFDToHandle(fd, handle->prime_fd, &id)) {
        PLOG(ERROR) << "Failed to add framebuffer for buffer " << buffer
            << ": Failed to get handle for prime fd " << handle->prime_fd;
        return 0;
    }

    // Add a new framebuffer
    uint32_t pitches[4] = { handle->stride, 0, 0, 0 };
    uint32_t offsets[4] = { 0, 0, 0, 0 };
    uint32_t handles[4] = { id, 0, 0, 0 };

    if (drmModeAddFB2(fd, handle->width, handle->height,
            convertAndroidToDrmFbFormat(handle->format),
            handles, pitches, offsets, &id, 0)) {
        PLOG(ERROR) << "drmModeAddFB2 failed for buffer " << buffer;
        return 0;
    }

    return id;
}
}

DrmFramebuffer::DrmFramebuffer(const DrmDevice& device, buffer_handle_t buffer)
    : mDevice(device), mId(addFramebuffer(device.fd(), buffer)) {}

DrmFramebuffer::~DrmFramebuffer() {
    if (mId)
        drmModeRmFB(mDevice.fd(), mId);
}

}  // namespace drmfb
}  // namespace V2_1
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
