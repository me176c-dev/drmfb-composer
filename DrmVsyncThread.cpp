// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2019 Stephan Gerhold

#define LOG_TAG "drmfb-vsync"

#include <time.h>
#include <android-base/logging.h>
#include <xf86drm.h>
#include "DrmVsyncThread.h"
#include "DrmDevice.h"

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace drmfb {

namespace {
constexpr int64_t NANO = 1'000'000'000;
constexpr int64_t DEFAULT_PERIOD = NANO / 60; // 60 Hz
}

DrmVsyncThread::DrmVsyncThread(DrmDisplay& display)
    : GraphicsThread("drm-vsync-" + std::to_string(display.id())),
      mDisplay(display) {}

void DrmVsyncThread::run() {
    auto highCrtc = mDisplay.pipe() << DRM_VBLANK_HIGH_CRTC_SHIFT;
    drmVBlank vBlank{ .request = {
        .type = static_cast<drmVBlankSeqType>(
            DRM_VBLANK_RELATIVE | (highCrtc & DRM_VBLANK_HIGH_CRTC_MASK)),
        .sequence = 1,
    }};

    auto ret = drmWaitVBlank(mDisplay.device().fd(), &vBlank);
    if (ret) {
        PLOG(ERROR) << "drmWaitBlank failed";
        if (errno == EBUSY || waitFallback())
            return;
    } else {
        mTimestamp = vBlank.reply.tval_sec * NANO + vBlank.reply.tval_usec * 1000;
    }

    mDisplay.vsync(mTimestamp);
}

int DrmVsyncThread::waitFallback() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    int64_t period = mDisplay.vsyncPeriod(mDisplay.currentMode());
    if (period <= 0)
        period = DEFAULT_PERIOD;

    int64_t now = ts.tv_sec * NANO + ts.tv_nsec;
    mTimestamp = mTimestamp ? mTimestamp + period : now + period;
    while (mTimestamp < now) {
        mTimestamp += period;
    }

    ts.tv_sec = mTimestamp / NANO;
    ts.tv_nsec = mTimestamp % NANO;

    int ret;
    do {
        ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, nullptr);
    } while (ret == EINTR);
    return ret;
}

}  // namespace drmfb
}  // namespace V2_1
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
