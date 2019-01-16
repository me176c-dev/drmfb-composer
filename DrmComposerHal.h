// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2019 Stephan Gerhold

#pragma once

#include <unordered_map>
#include <android-base/unique_fd.h>
#include <composer-hal/2.1/ComposerHal.h>
#include "DrmDevice.h"

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace drmfb {

using hal::ColorMode;
using hal::Dataspace;
using hal::PixelFormat;
using hal::Hdr;

struct DrmComposerHal : public hal::ComposerHal, DrmCallback {
    DrmComposerHal(std::unique_ptr<DrmDevice> device);

    bool hasCapability(hwc2_capability_t capability) override;
    std::string dumpDebugInfo() override;

    void registerEventCallback(EventCallback* callback) override;
    void unregisterEventCallback() override;

    void onHotplug(const DrmDisplay& display, bool connected) override;
    void onVsync(const DrmDisplay& display, int64_t timestamp) override;

    uint32_t getMaxVirtualDisplayCount() override;
    Error createVirtualDisplay(uint32_t width, uint32_t height, PixelFormat* format,
                                       Display* outDisplay) override;
    Error destroyVirtualDisplay(Display display) override;
    Error createLayer(Display display, Layer* outLayer) override;
    Error destroyLayer(Display display, Layer layer) override;

    Error getActiveConfig(Display display, Config* outConfig) override;
    Error getClientTargetSupport(Display display, uint32_t width, uint32_t height,
                                         PixelFormat format, Dataspace dataspace) override;
    Error getColorModes(Display display, hidl_vec<ColorMode>* outModes) override;
    Error getDisplayAttribute(Display display, Config config,
                                      IComposerClient::Attribute attribute, int32_t* outValue) override;
    Error getDisplayConfigs(Display display, hidl_vec<Config>* outConfigs) override;
    Error getDisplayName(Display display, hidl_string* outName) override;
    Error getDisplayType(Display display, IComposerClient::DisplayType* outType) override;
    Error getDozeSupport(Display display, bool* outSupport) override;
    Error getHdrCapabilities(Display display, hidl_vec<Hdr>* outTypes,
                                     float* outMaxLuminance, float* outMaxAverageLuminance,
                                     float* outMinLuminance) override;

    Error setActiveConfig(Display display, Config config) override;
    Error setColorMode(Display display, ColorMode mode) override;
    Error setPowerMode(Display display, IComposerClient::PowerMode mode) override;
    Error setVsyncEnabled(Display display, IComposerClient::Vsync enabled) override;

    Error setColorTransform(Display display, const float* matrix, int32_t hint) override;
    Error setClientTarget(Display display, buffer_handle_t target, int32_t acquireFence,
                                  int32_t dataspace, const std::vector<hwc_rect_t>& damage) override;
    Error setOutputBuffer(Display display, buffer_handle_t buffer,
                                  int32_t releaseFence) override;
    Error validateDisplay(Display display, std::vector<Layer>* outChangedLayers,
                                  std::vector<IComposerClient::Composition>* outCompositionTypes,
                                  uint32_t* outDisplayRequestMask,
                                  std::vector<Layer>* outRequestedLayers,
                                  std::vector<uint32_t>* outRequestMasks) override;
    Error acceptDisplayChanges(Display display) override;
    Error presentDisplay(Display display, int32_t* outPresentFence,
                                 std::vector<Layer>* outLayers,
                                 std::vector<int32_t>* outReleaseFences) override;

    Error setLayerCursorPosition(Display display, Layer layer, int32_t x, int32_t y) override;
    Error setLayerBuffer(Display display, Layer layer, buffer_handle_t buffer,
                                 int32_t acquireFence) override;
    Error setLayerSurfaceDamage(Display display, Layer layer,
                                        const std::vector<hwc_rect_t>& damage) override;
    Error setLayerBlendMode(Display display, Layer layer, int32_t mode) override;
    Error setLayerColor(Display display, Layer layer, IComposerClient::Color color) override;
    Error setLayerCompositionType(Display display, Layer layer, int32_t type) override;
    Error setLayerDataspace(Display display, Layer layer, int32_t dataspace) override;
    Error setLayerDisplayFrame(Display display, Layer layer, const hwc_rect_t& frame) override;
    Error setLayerPlaneAlpha(Display display, Layer layer, float alpha) override;
    Error setLayerSidebandStream(Display display, Layer layer, buffer_handle_t stream) override;
    Error setLayerSourceCrop(Display display, Layer layer, const hwc_frect_t& crop) override;
    Error setLayerTransform(Display display, Layer layer, int32_t transform) override;
    Error setLayerVisibleRegion(Display display, Layer layer,
                                        const std::vector<hwc_rect_t>& visible) override;
    Error setLayerZOrder(Display display, Layer layer, uint32_t z) override;

private:
    struct HwcLayer {
        HwcLayer(Display displayId) : displayId(displayId) {}
        const Display displayId;
        IComposerClient::Composition composition = IComposerClient::Composition::INVALID;
    };

    std::unique_ptr<DrmDevice> mDevice; // TODO: Support multiple GPUs?
    EventCallback *mCallback = nullptr;

    std::unordered_map<Layer, HwcLayer> mLayers;
    Layer mNextLayer = 0;

    // The next client target buffer to be displayed
    buffer_handle_t mBuffer = nullptr;
    base::unique_fd mAcquireFence;
};

}  // namespace drmfb
}  // namespace V2_1
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
