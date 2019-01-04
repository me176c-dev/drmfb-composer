// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2019 Stephan Gerhold

#pragma once

#include <cstdint>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace drmfb {

namespace libdrm {
    bool addFramebuffer(int fd, buffer_handle_t buffer, uint32_t* id);
}

namespace minigbm {
#ifdef USE_MINIGBM
    bool addFramebuffer(int fd, buffer_handle_t buffer, uint32_t* id);
#else
    constexpr bool addFramebuffer(int, buffer_handle_t, uint32_t*) {
        return false;
    }
#endif
}

}  // namespace drmfb
}  // namespace V2_1
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
