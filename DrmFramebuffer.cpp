// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2019 Stephan Gerhold

#define LOG_TAG "drmfb-framebuffer"

#include <android-base/logging.h>
#include "DrmDevice.h"
#include "DrmFramebuffer.h"
#include "DrmFramebufferImporter.h"

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace drmfb {

namespace {
// TODO: Add a proper importer interface
uint32_t addFramebuffer(int fd, buffer_handle_t buffer) {
    uint32_t id = 0;
    if (libdrm::addFramebuffer(fd, buffer, &id))
        return id;
    if (minigbm::addFramebuffer(fd, buffer, &id))
        return id;

    LOG(ERROR) << "No importer available for buffer with "
        << buffer->numFds << " FDs and " << buffer->numInts << " ints";
    return 0;
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
