// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2019 Stephan Gerhold

#pragma once

#include <xf86drmMode.h>

namespace drm {
namespace mode {

namespace {
template <typename T, auto fn>
using fn_unique_ptr = std::unique_ptr<T, std::integral_constant<decltype(fn), fn>>;
}

using unique_res_ptr = fn_unique_ptr<drmModeRes, drmModeFreeResources>;
using unique_connector_ptr = fn_unique_ptr<drmModeConnector, drmModeFreeConnector>;
using unique_encoder_ptr = fn_unique_ptr<drmModeEncoder, drmModeFreeEncoder>;
}
}
