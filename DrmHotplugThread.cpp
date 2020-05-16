// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2019 Stephan Gerhold

#define LOG_TAG "drmfb-hotplug"

#include <cutils/uevent.h>
#include <android-base/unique_fd.h>
#include <android-base/logging.h>
#include "DrmHotplugThread.h"
#include "DrmDevice.h"

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace drmfb {

namespace {
// Note: This must be <= /proc/sys/net/core/rmem_max (written in init.rc)
constexpr int RECEIVE_BUFFER = 256 * 1024; // 256 KiB
constexpr int MESSAGE_BUFFER = 1 * 1024; // 1 KiB
}

DrmHotplugThread::DrmHotplugThread(DrmDevice& device)
    : GraphicsThread("drm-hotplug"), mDevice(device) {}

bool DrmHotplugThread::receiveEvent(int fd) {
    char msg[MESSAGE_BUFFER];
    auto n = uevent_kernel_multicast_recv(fd, msg, sizeof(msg));
    if (n < 0)
        return false;

    bool drm = false;
    bool hotplug = false;
    for (auto buf = msg, end = msg + n; buf < end; buf += strlen(buf) + 1) {
        if (!strcmp(buf, "DEVTYPE=drm_minor"))
            drm = true;
        else if (!strcmp(buf, "HOTPLUG=1"))
            hotplug = true;
    }

    return drm && hotplug;
}

void DrmHotplugThread::work(std::unique_lock<std::mutex>& lock) {
    base::unique_fd fd{uevent_open_socket(RECEIVE_BUFFER, true)};
    if (fd < 0) {
        PLOG(ERROR) << "Failed to open uevent socket";
        return;
    }

    loop(lock, [this, &fd] {
        if (receiveEvent(fd)) {
            LOG(DEBUG) << "Received hotplug uevent";
            mDevice.update();
        }
    });
}

}  // namespace drmfb
}  // namespace V2_1
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
