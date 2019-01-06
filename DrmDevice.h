// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2019 Stephan Gerhold

#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <android-base/unique_fd.h>
#include "DrmDisplay.h"
#include "DrmCallback.h"

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace drmfb {

struct DrmDevice {
    DrmDevice(int fd);
    DrmDevice(const std::string& path);
    DrmDevice();

    inline int fd() const { return mFd; }

    DrmDisplay* getConnectedDisplay(uint32_t connector);

    inline const std::vector<uint32_t>& crtcs() { return mCrtcs; }
    uint32_t reserveCrtc(unsigned pipe);
    void freeCrtc(unsigned pipe);

    bool initialize();
    void update();

    inline DrmCallback* callback() { return mCallback; }
    void enable(DrmCallback *callback);
    void disable();

private:
    base::unique_fd mFd;

    // Connector -> Display
    std::unordered_map<uint32_t, std::unique_ptr<DrmDisplay>> mDisplays;

    std::vector<uint32_t> mCrtcs;
    uint32_t mUsedCrtcs = 0; // The CRTCs that are already being used by a display

    DrmCallback* mCallback = nullptr;
};

}  // namespace drmfb
}  // namespace V2_1
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
