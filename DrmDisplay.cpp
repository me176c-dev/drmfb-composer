// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2019 Stephan Gerhold

#define LOG_TAG "drmfb-display"

#include <array>
#include <xf86drm.h>
#include <android-base/logging.h>
#include "drm_unique_ptr.h"
#include "DrmDisplay.h"
#include "DrmDevice.h"

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace drmfb {

namespace {
constexpr std::array<std::string_view, 19> CONNECTOR_TYPE_NAMES{
    "Unknown", "VGA", "DVI-I", "DVI-D", "DVI-A", "Composite", "SVIDEO",
    "LVDS", "Component", "DIN", "DP", "HDMI-A", "HDMI-B", "TV",
    "eDP", "Virtual", "DSI", "DPI", "Writeback",
};
constexpr std::string_view connectorTypeName(uint32_t connectorType) {
    if (connectorType >= CONNECTOR_TYPE_NAMES.size()) {
        return CONNECTOR_TYPE_NAMES.front();
    }
    return CONNECTOR_TYPE_NAMES[connectorType];
}

std::ostream& operator<<(std::ostream& os, const drmModeModeInfo& mode) {
    return os << mode.name;
}

constexpr int32_t SECOND_NANOS = 1'000'000'000;
constexpr int32_t KINCH_MILLIMETER = 25400;
}

DrmDisplay::DrmDisplay(DrmDevice& device, uint32_t connectorId)
    : mDevice(device), mConnector(connectorId), mVsyncThread(*this) {
    update();
}

DrmDisplay::~DrmDisplay() {
    if (mCrtc)
        mDevice.freeCrtc(mPipe);
}

int32_t DrmDisplay::width(unsigned mode) const {
    return mode < mModes.size() ? mModes[mode].hdisplay : -1;
}

int32_t DrmDisplay::height(unsigned mode) const {
    return mode < mModes.size() ? mModes[mode].vdisplay : -1;
}

int32_t DrmDisplay::vsyncPeriod(unsigned mode) const {
    if (mode >= mModes.size()) return -1;
    auto refresh = mModes[mode].vrefresh;
    return refresh > 0 ? SECOND_NANOS / mModes[mode].vrefresh : 0;
}

int32_t DrmDisplay::dpiX(unsigned mode) const {
    if (mode >= mModes.size()) return -1;
    return mmWidth > 0 ? mModes[mode].hdisplay * KINCH_MILLIMETER / mmWidth : 0;
}

int32_t DrmDisplay::dpiY(unsigned mode) const {
    if (mode >= mModes.size()) return -1;
    return mmHeight > 0 ? mModes[mode].vdisplay * KINCH_MILLIMETER / mmHeight : 0;
}

void DrmDisplay::update() {
    drm::mode::unique_connector_ptr connector{
        drmModeGetConnector(mDevice.fd(), mConnector)};

    auto connected = false;
    if (connector) {
        connected = connector->connection == DRM_MODE_CONNECTED
                        && connector->count_modes > 0;
    } else {
        PLOG(ERROR) << "Failed to get DRM connector " << mConnector;
    }

    if (connected == mConnected)
        return; // Only update on hotplug
    mConnected = connected;

    if (connected) {
        mType = connector->connector_type;
        mName = connectorTypeName(connector->connector_type);
        mName += '-' + std::to_string(connector->connector_type_id);

        mmWidth = connector->mmWidth;
        mmHeight = connector->mmHeight;

        setModes(connector->modes, connector->modes + connector->count_modes);

        LOG(INFO) << "Display " << *this << " connected, "
            << mModes.size() << " mode(s), "
            << "default: " << mModes[mCurrentMode];
        report();
    } else {
        LOG(INFO) << "Display " << *this << " disconnected";

        disableVsync();

        if (mCrtc)
            mDevice.freeCrtc(mPipe);

        mFlipPending = false;
        mModeSet = false;
        mCrtc = 0;

        mFramebuffers.clear();
        mModes.clear();

        report();

        mVsyncThread.stop();
    }
}

void DrmDisplay::setModes(const drmModeModeInfo* begin, const drmModeModeInfo* end) {
    mCurrentMode = 0;
    mModes.clear();

    /*
     * Originally, this simply exposed all modes provided by DRM as
     * available display configs. However, the Android framework
     * skips modes that do not differ by any attributes it is
     * concerned with (e.g. only DRM flags instead of width/height/vrefresh).
     * This may cause it to use the wrong (e.g. interlaced) mode later.
     *
     * To avoid this, only the first mode for each combination of
     * width/height/vrefresh is exposed.
     */

    // Add the preferred mode first
    auto preferred = std::find_if(begin, end,
        [] (const auto& mode) { return mode.type & DRM_MODE_TYPE_PREFERRED; });
    if (preferred != end) {
        mModes.emplace_back(*preferred);
    }

    // Add all other modes; each combination only once
    for (auto mode = begin; mode < end; ++mode) {
        if (std::find_if(mModes.begin(), mModes.end(),
                [mode] (const auto& mode2) {
                    return mode->hdisplay == mode2.hdisplay
                        && mode->vdisplay == mode2.vdisplay
                        && mode->vrefresh == mode2.vrefresh;
                }) == mModes.end()) {
            mModes.emplace_back(*mode);
        }
    }
}

void DrmDisplay::report() {
    if (auto callback = mDevice.callback(); callback) {
        callback->onHotplug(*this, mConnected);
    }
}

void DrmDisplay::vsync(int64_t timestamp) {
    if (auto callback = mDevice.callback(); callback) {
        callback->onVsync(*this, timestamp);
    }
}

bool DrmDisplay::setMode(unsigned mode) {
    if (mCurrentMode == mode)
        return true;
    if (mode >= mModes.size())
        return false;

    LOG(INFO) << "Setting mode " << mModes[mode] << " for display " << *this;

    mCurrentMode = mode;
    mModeSet = false;
    return true;
}

namespace {
void handlePageFlip(int /*fd*/, unsigned int /*sequence*/,
        unsigned int /*tv_sec */, unsigned int /*tv_usec*/, void* user_data) {
    auto display = static_cast<DrmDisplay*>(user_data);
    display->handlePageFlip();
}

drmEventContext pageFlipEvCtx = {
    .version = DRM_EVENT_CONTEXT_VERSION,
    .page_flip_handler = handlePageFlip,
};
}

void DrmDisplay::handlePageFlip() {
    if (mFlipPending) {
        mFlipPending = false;
    } else if (mConnected) {
        LOG(WARNING) << "handlePageFlip() called for display " << *this
            << " without flip pending";
    }
}

void DrmDisplay::awaitPageFlip() const {
    // Wait for the last page flip to complete
    while (mFlipPending) {
        if (drmHandleEvent(mDevice.fd(), &pageFlipEvCtx)) {
            PLOG(ERROR) << "Failed to handle DRM event";
            return;
        }
    }
}

bool DrmDisplay::enable() {
    if (enabled())
        return true;
    if (!mConnected)
        return false;

    drm::mode::unique_connector_ptr connector{
        drmModeGetConnector(mDevice.fd(), mConnector)};

    // Verify that the display is still connected, just to be sure
    if (!connector || connector->connection != DRM_MODE_CONNECTED) {
        update();
        return false;
    }

    LOG(INFO) << "Enabling display " << *this;

    // Attempt to find a CRTC that is not used by any other display
    for (auto i = 0; i < connector->count_encoders; ++i) {
        drm::mode::unique_encoder_ptr encoder{
            drmModeGetEncoder(mDevice.fd(), connector->encoders[i])};
        if (!encoder) {
            PLOG(ERROR) << "Failed to get encoder " << connector->encoders[i];
            continue;
        }

        for (mPipe = 0; mPipe < mDevice.crtcs().size(); ++mPipe) {
            if (!(encoder->possible_crtcs & (1 << mPipe)))
                continue;

            mCrtc = mDevice.reserveCrtc(mPipe);
            if (mCrtc) {
                LOG(INFO) << "Using CRTC " << mCrtc << " for display " << *this;
                return true;
            } else {
                LOG(WARNING) << "CRTC " << mDevice.crtcs()[mPipe]
                    << " for display " << *this
                    << " is already in use by another display";
            }
        }
    }

    LOG(ERROR) << "Failed to find CRTC for display " << *this;
    return false;
}

void DrmDisplay::disable() {
    if (!enabled())
        return;

    LOG(INFO) << "Disabling display " << *this;

    if (mModeSet) {
        mVsyncThread.disable();
        awaitPageFlip();
        if (drmModeSetCrtc(mDevice.fd(), mCrtc, 0, 0, 0, nullptr, 0, nullptr)) {
            PLOG(ERROR) << "Failed to disable display " << *this;
        }
        mModeSet = false;
    }
    mDevice.freeCrtc(mPipe);
    mCrtc = 0;
}

void DrmDisplay::enableVsync() {
    mVsyncEnabled = true;
    mVsyncThread.enable();
}

void DrmDisplay::disableVsync() {
    mVsyncEnabled = false;
    mVsyncThread.disable();
}

void DrmDisplay::present(buffer_handle_t buffer) {
    if (!enabled())
        return;

    auto& fb = mFramebuffers.try_emplace(buffer, mDevice, buffer).first->second;
    if (!fb.id()) {
        // The framebuffer error was already logged
        return;
    }

    awaitPageFlip();

    if (mModeSet) {
        mFlipPending = true;
        if (drmModePageFlip(mDevice.fd(), mCrtc, fb.id(), DRM_MODE_PAGE_FLIP_EVENT, this)) {
            PLOG(ERROR) << "Failed to perform page flip for display " << *this;
            mFlipPending = false;
        }
    } else {
        auto mode = &mModes[mCurrentMode];
        if (drmModeSetCrtc(mDevice.fd(), mCrtc, fb.id(), 0, 0, &mConnector, 1, mode)) {
            PLOG(ERROR) << "Failed to enable CRTC " << mCrtc
                << " for display " << *this;
        } else {
            mModeSet = true;
        }
    }
}

std::ostream& operator<<(std::ostream& os, const DrmDisplay& display) {
    return os << display.mConnector << " (" << display.mName << ")";
}

}  // namespace drmfb
}  // namespace V2_1
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
