// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2019 Stephan Gerhold

#pragma once

#include "GraphicsThread.h"

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace drmfb {

struct DrmDisplay;

struct DrmVsyncThread : public GraphicsThread {
    DrmVsyncThread(DrmDisplay& display);
    void run() override;

private:
    int waitFallback();

    DrmDisplay& mDisplay;
    int64_t mTimestamp = 0;
};

}  // namespace drmfb
}  // namespace V2_1
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
