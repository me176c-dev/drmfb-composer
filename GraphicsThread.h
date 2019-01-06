// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2019 Stephan Gerhold

#pragma once

#include <condition_variable>
#include <mutex>
#include <thread>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace drmfb {

struct GraphicsThread {
    GraphicsThread(std::string name);
    virtual ~GraphicsThread();

    void enable();
    void disable();
    void stop();

    virtual void run() = 0;
    void loop();

private:
    std::mutex mMutex;
    std::thread mThread;
    std::string mName;

    bool mStarted;
    bool mEnabled;
    std::condition_variable mCondition;
};

}  // namespace drmfb
}  // namespace V2_1
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
