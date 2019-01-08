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

protected:
    virtual void run() {};
    virtual void work(std::unique_lock<std::mutex>& lock);

    template<typename F>
    void loop(std::unique_lock<std::mutex>& lock, F f) {
        while (mEnabled) {
            lock.unlock();
            f();
            lock.lock();
        }
    }

private:
    void main();

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
