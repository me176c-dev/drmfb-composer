// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2019 Stephan Gerhold

#pragma once

#include <android/hardware/graphics/composer/2.1/IComposer.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace drmfb {

struct DrmDisplay;

struct DrmCallback {
    virtual ~DrmCallback() = default;
    virtual void onHotplug(const DrmDisplay& display, bool connected) = 0;
    virtual void onVsync(const DrmDisplay& display, int64_t timestamp) = 0;
};

}  // namespace drmfb
}  // namespace V2_1
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
