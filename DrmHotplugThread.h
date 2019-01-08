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

struct DrmDevice;

struct DrmHotplugThread : public GraphicsThread {
    DrmHotplugThread(DrmDevice& device);

protected:
    void work(std::unique_lock<std::mutex>& lock) override;

private:
    bool receiveEvent(int fd);

    DrmDevice& mDevice;
};

}  // namespace drmfb
}  // namespace V2_1
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
