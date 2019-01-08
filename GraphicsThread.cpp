// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2019 Stephan Gerhold

#define LOG_TAG "drmfb-thread"

#include <android-base/logging.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <system/graphics.h>
#include "GraphicsThread.h"

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace drmfb {

GraphicsThread::GraphicsThread(std::string name)
    : mName(std::move(name)) {}

GraphicsThread::~GraphicsThread() {
    stop();
}

void GraphicsThread::enable() {
    {
        std::scoped_lock lock{mMutex};
        mEnabled = true;

        if (!mStarted) {
            mStarted = true;
            mThread = std::thread(&GraphicsThread::main, this);
            return;
        }
    }

    mCondition.notify_all();
}

void GraphicsThread::disable() {
    std::scoped_lock lock{mMutex};
    mEnabled = false;
}

void GraphicsThread::stop() {
    {
        std::scoped_lock lock{mMutex};
        if (!mStarted)
            return;

        mEnabled = false;
        mStarted = false;
    }

    mCondition.notify_all();
    mThread.join();
}

void GraphicsThread::main() {
    setpriority(PRIO_PROCESS, 0, HAL_PRIORITY_URGENT_DISPLAY);
    prctl(PR_SET_NAME, mName.c_str(), 0, 0, 0);

    LOG(DEBUG) << "Starting thread " << mName;

    std::unique_lock lock{mMutex};
    while (mStarted) {
        work(lock);
        mCondition.wait(lock, [this] { return mEnabled || !mStarted; });
    }

    LOG(DEBUG) << "Stopping thread " << mName;
}

void GraphicsThread::work(std::unique_lock<std::mutex>& lock) {
    loop(lock, [this] { run(); });
}

}  // namespace drmfb
}  // namespace V2_1
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
