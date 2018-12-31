// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2019 Stephan Gerhold

#pragma once

#include <cstdint>
#include <cutils/native_handle.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace drmfb {

struct DrmDevice;

struct DrmFramebuffer {
    DrmFramebuffer(const DrmDevice& device, buffer_handle_t buffer);
    ~DrmFramebuffer();

    inline uint32_t id() const { return mId; }

private:
    const DrmDevice& mDevice;
    const uint32_t mId;
};

}  // namespace drmfb
}  // namespace V2_1
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
