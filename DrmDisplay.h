// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2019 Stephan Gerhold

#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <xf86drmMode.h>
#include "DrmFramebuffer.h"
#include "DrmVsyncThread.h"

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace drmfb {

struct DrmDevice;

struct DrmDisplay {
    DrmDisplay(DrmDevice &device, uint32_t connectorId);
    ~DrmDisplay();

    inline DrmDevice& device() const { return mDevice; }
    inline uint32_t id() const { return mConnector; }
    inline unsigned pipe() const { return mPipe; }
    inline const std::string& name() const { return mName; }
    inline unsigned modeCount() const { return mModes.size(); }
    inline unsigned currentMode() const { return mCurrentMode; }
    inline bool connected() const { return mConnected; }
    inline bool enabled() const { return !!mCrtc; }
    inline bool internal() const {
        return mType == DRM_MODE_CONNECTOR_LVDS || mType == DRM_MODE_CONNECTOR_eDP
            || mType == DRM_MODE_CONNECTOR_VIRTUAL || mType == DRM_MODE_CONNECTOR_DSI;
    }

    int32_t width(unsigned mode) const;
    int32_t height(unsigned mode) const;
    int32_t vsyncPeriod(unsigned mode) const;
    int32_t dpiX(unsigned mode) const;
    int32_t dpiY(unsigned mode) const;

    bool setMode(unsigned mode);

    void update();
    void report();
    void vsync(int64_t timestamp);

    bool enable();
    void disable();

    void enableVsync();
    void disableVsync();

    void present(buffer_handle_t buffer);
    void handlePageFlip();

    friend std::ostream& operator<<(std::ostream& os, const DrmDisplay& display);

private:
    void setModes(const drmModeModeInfo* begin, const drmModeModeInfo* end);
    void awaitPageFlip() const;

    DrmDevice& mDevice;
    uint32_t mConnector;

    std::string mName;
    uint32_t mType;

    // Updated on each hotplug (disconnect and re-connect)
    uint32_t mmWidth, mmHeight;
    std::vector<drmModeModeInfo> mModes;
    unsigned mCurrentMode;

    uint32_t mCrtc = 0; // Selected when display is powered on
    unsigned mPipe;

    bool mConnected = false;
    bool mModeSet = false;
    bool mFlipPending = false;
    bool mVsyncEnabled = false;

    // TODO: Clean up framebuffers
    std::unordered_map<buffer_handle_t, DrmFramebuffer> mFramebuffers;

    DrmVsyncThread mVsyncThread;
};

std::ostream& operator<<(std::ostream& os, const DrmDisplay& display);

}  // namespace drmfb
}  // namespace V2_1
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
