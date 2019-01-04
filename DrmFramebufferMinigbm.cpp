// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2019 Stephan Gerhold

#define LOG_TAG "drmfb-framebuffer-minigbm"

#include <android-base/logging.h>
#include <drm/drm_fourcc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <cros_gralloc_handle.h>
#include <cros_gralloc_helpers.h>
#include "DrmFramebufferImporter.h"

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace drmfb {
namespace minigbm {

namespace {
void addFramebuffer(int fd, cros_gralloc_handle_t handle, int planes, uint32_t* id) {
    uint32_t handles[DRV_MAX_PLANES] = {};
    for (int i = 0; i < planes; ++i) {
        if (drmPrimeFDToHandle(fd, handle->fds[i], &handles[i])) {
            PLOG(ERROR) << "Failed to get handle for prime fd "
                << handle->fds[i] << " (plane " << i << ")";
            return;
        }
    }

    /*
     * Avoid using alpha bits for the framebuffer.
     * They are not supported on older Intel GPUs for primary planes.
     */
    auto format = handle->format;
    if (format == DRM_FORMAT_ABGR8888)
        format = DRM_FORMAT_XBGR8888;

    // TODO: Consider using drmModeAddFB2WithModifiers
    if (drmModeAddFB2(fd, handle->width, handle->height,
            format, handles, handle->strides, handle->offsets, id, 0)) {
        PLOG(ERROR) << "drmModeAddFB2 failed";
    }
}
}

bool addFramebuffer(int fd, buffer_handle_t buffer, uint32_t* id) {
    auto planes = buffer->numFds;
    if (planes < 1 && planes > DRV_MAX_PLANES)
        return false;
    if ((buffer->numInts + planes) < static_cast<int>(handle_data_size))
        return false;

    auto handle = reinterpret_cast<cros_gralloc_handle_t>(buffer);
    if (handle->magic != cros_gralloc_magic)
        return false;

    addFramebuffer(fd, handle, planes, id);
    return true;
}

}  // namespace minigbm
}  // namespace implementation
}  // namespace V2_1
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
