// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2019 Stephan Gerhold

#define LOG_TAG "drmfb-composer"

#include <numeric>
#include <android-base/logging.h>
#include <composer-hal/2.1/Composer.h>
#include <sync/sync.h>
#include "DrmComposer.h"
#include "DrmComposerHal.h"

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace drmfb {

android::sp<IComposer> createDrmComposer() {
    auto device = std::make_unique<DrmDevice>();
    if (!device->initialize()) {
        return {};
    }

    return new hal::Composer{std::make_unique<DrmComposerHal>(std::move(device))};
}

DrmComposerHal::DrmComposerHal(std::unique_ptr<DrmDevice> device)
    : mDevice(std::move(device)) {}

bool DrmComposerHal::hasCapability(hwc2_capability_t capability) {
    // TODO: Is there a way to implement them without atomic modesetting?
    return static_cast<IComposer::Capability>(capability)
                == IComposer::Capability::PRESENT_FENCE_IS_NOT_RELIABLE;
}

std::string DrmComposerHal::dumpDebugInfo() {
    return ""; // TODO
}

void DrmComposerHal::registerEventCallback(EventCallback* callback) {
    mCallback = callback;
    mDevice->enable(this);
}

void DrmComposerHal::unregisterEventCallback() {
    LOG(INFO) << "Client destroyed, disabling displays";
    mDevice->disable();

    mCallback = nullptr;
    mLayers.clear();
    mNextLayer = 0;
}

void DrmComposerHal::onHotplug(const DrmDisplay& display, bool connected) {
    mCallback->onHotplug(display.id(),
        connected ? IComposerCallback::Connection::CONNECTED
            : IComposerCallback::Connection::DISCONNECTED);
}

void DrmComposerHal::onVsync(const DrmDisplay& display, int64_t timestamp) {
    mCallback->onVsync(display.id(), timestamp);
}

uint32_t DrmComposerHal::getMaxVirtualDisplayCount() {
    return 0; // Not supported
}

Error DrmComposerHal::createVirtualDisplay(uint32_t /*width*/, uint32_t /*height*/,
        PixelFormat* /*format*/, Display* /*outDisplayId*/) {
    return Error::NO_RESOURCES;
}

Error DrmComposerHal::destroyVirtualDisplay(Display /*displayId*/) {
    return Error::BAD_DISPLAY;
}

Error DrmComposerHal::createLayer(Display displayId, Layer* outLayer) {
    if (!mDevice->getConnectedDisplay(displayId))
        return Error::BAD_DISPLAY;

    *outLayer = mNextLayer++;
    mLayers.emplace(*outLayer, displayId);
    return Error::NONE;
}

Error DrmComposerHal::destroyLayer(Display displayId, Layer layer) {
    auto i = mLayers.find(layer);
    if (i == mLayers.end())
        return Error::BAD_LAYER;
    if (displayId != i->second.displayId)
        return Error::BAD_DISPLAY;

    mLayers.erase(i);
    return Error::NONE;
}


Error DrmComposerHal::getActiveConfig(Display displayId, Config* outConfig) {
    auto display = mDevice->getConnectedDisplay(displayId);
    if (!display)
        return Error::BAD_DISPLAY;

    *outConfig = display->currentMode();
    return Error::NONE;
}

Error DrmComposerHal::getClientTargetSupport(Display displayId,
        uint32_t width, uint32_t height, PixelFormat format, Dataspace dataspace) {
    auto display = mDevice->getConnectedDisplay(displayId);
    if (!display)
        return Error::BAD_DISPLAY;

    return width == static_cast<uint32_t>(display->width(display->currentMode()))
        && height == static_cast<uint32_t>(display->height(display->currentMode()))
        && format == PixelFormat::RGBA_8888 && dataspace == Dataspace::UNKNOWN
            ? Error::NONE : Error::UNSUPPORTED;
}

Error DrmComposerHal::getColorModes(Display displayId, hidl_vec<ColorMode>* outModes) {
    if (!mDevice->getConnectedDisplay(displayId))
        return Error::BAD_DISPLAY;

    *outModes = hidl_vec<ColorMode>{ColorMode::NATIVE};
    return Error::NONE;
}

Error DrmComposerHal::getDisplayAttribute(Display displayId, Config config,
        IComposerClient::Attribute attribute, int32_t* outValue) {
    auto display = mDevice->getConnectedDisplay(displayId);
    if (!display)
        return Error::BAD_DISPLAY;

    switch (attribute) {
    case IComposerClient::Attribute::WIDTH:
        *outValue = display->width(config);
        break;
    case IComposerClient::Attribute::HEIGHT:
        *outValue = display->height(config);
        break;
    case IComposerClient::Attribute::VSYNC_PERIOD:
        *outValue = display->vsyncPeriod(config);
        break;
    case IComposerClient::Attribute::DPI_X:
        *outValue = display->dpiX(config);
        break;
    case IComposerClient::Attribute::DPI_Y:
        *outValue = display->dpiY(config);
        break;
    default:
        return Error::BAD_PARAMETER;
    }

    switch (*outValue) {
    case -1:
        return Error::BAD_CONFIG;
    case 0:
        return Error::UNSUPPORTED;
    default:
        return Error::NONE;
    }
}

Error DrmComposerHal::getDisplayConfigs(Display displayId, hidl_vec<Config>* outConfigs) {
    auto display = mDevice->getConnectedDisplay(displayId);
    if (!display)
        return Error::BAD_DISPLAY;

    outConfigs->resize(display->modeCount());
    std::iota(outConfigs->begin(), outConfigs->end(), 0);
    return Error::NONE;
}

Error DrmComposerHal::getDisplayName(Display displayId, hidl_string* outName) {
    auto display = mDevice->getConnectedDisplay(displayId);
    if (!display)
        return Error::BAD_DISPLAY;

    *outName = display->name();
    return Error::NONE;
}

Error DrmComposerHal::getDisplayType(Display displayId, IComposerClient::DisplayType* outType) {
    if (!mDevice->getConnectedDisplay(displayId))
        return Error::BAD_DISPLAY;

    *outType = IComposerClient::DisplayType::PHYSICAL;
    return Error::NONE;
}

Error DrmComposerHal::getDozeSupport(Display displayId, bool* outSupport) {
    if (!mDevice->getConnectedDisplay(displayId))
        return Error::BAD_DISPLAY;

    *outSupport = false;
    return Error::NONE;
}

Error DrmComposerHal::getHdrCapabilities(Display displayId, hidl_vec<Hdr>* /*outTypes*/,
        float* /*outMaxLuminance*/, float* /*outMaxAverageLuminance*/, float* /*outMinLuminance*/) {
    if (!mDevice->getConnectedDisplay(displayId))
        return Error::BAD_DISPLAY;

    return Error::NONE;
}

Error DrmComposerHal::setActiveConfig(Display displayId, Config config) {
    auto display = mDevice->getConnectedDisplay(displayId);
    if (!display)
        return Error::BAD_DISPLAY;

    return display->setMode(config) ? Error::NONE : Error::BAD_CONFIG;
}

Error DrmComposerHal::setColorMode(Display displayId, ColorMode mode) {
    if (!mDevice->getConnectedDisplay(displayId))
        return Error::BAD_DISPLAY;
    if (mode != ColorMode::NATIVE)
        return Error::UNSUPPORTED;

    return Error::NONE;
}

Error DrmComposerHal::setPowerMode(Display displayId, IComposerClient::PowerMode mode) {
    auto display = mDevice->getConnectedDisplay(displayId);
    if (!display)
        return Error::BAD_DISPLAY;

    switch (mode) {
    case IComposerClient::PowerMode::OFF:
        display->disable();
        return Error::NONE;
    case IComposerClient::PowerMode::ON:
        mDevice->reportExternal();
        return display->enable() ? Error::NONE : Error::NO_RESOURCES;
    case IComposerClient::PowerMode::DOZE:
    case IComposerClient::PowerMode::DOZE_SUSPEND:
        return Error::UNSUPPORTED;
    default:
        return Error::BAD_PARAMETER;
    }
}

Error DrmComposerHal::setVsyncEnabled(Display displayId, IComposerClient::Vsync enabled) {
    auto display = mDevice->getConnectedDisplay(displayId);
    if (!display)
        return Error::BAD_DISPLAY;

    switch (enabled) {
    case IComposerClient::Vsync::ENABLE:
        display->enableVsync();
        return Error::NONE;
    case IComposerClient::Vsync::DISABLE:
        display->disableVsync();
        return Error::NONE;
    default:
        return Error::BAD_PARAMETER;
    }
}

Error DrmComposerHal::setColorTransform(Display /*displayId*/,
        const float* /*matrix*/, int32_t /*hint*/) {
    return Error::UNSUPPORTED;
}

Error DrmComposerHal::setClientTarget(Display /*displayId*/,
        buffer_handle_t target, int32_t acquireFence,
        int32_t /*dataspace*/, const std::vector<hwc_rect_t>& /*damage*/) {
    mBuffer = target;
    mAcquireFence.reset(acquireFence);
    return Error::NONE;
}

Error DrmComposerHal::setOutputBuffer(Display /*displayId*/,
        buffer_handle_t /*buffer*/, int32_t /*releaseFence*/) {
    return Error::BAD_DISPLAY; // Virtual display
}

Error DrmComposerHal::validateDisplay(Display displayId, std::vector<Layer>* outChangedLayers,
        std::vector<IComposerClient::Composition>* outCompositionTypes,
        uint32_t* /*outDisplayRequestMask*/, std::vector<Layer>* /*outRequestedLayers*/,
        std::vector<uint32_t>* /*outRequestMasks*/) {

    for (auto& it : mLayers) {
        auto layer = it.second;
        if (layer.displayId == displayId
                && layer.composition != IComposerClient::Composition::CLIENT) {
            // Force client composition for all layers
            outChangedLayers->push_back(it.first);
            outCompositionTypes->push_back(IComposerClient::Composition::CLIENT);
        }
    }

    return Error::NONE;
}

Error DrmComposerHal::acceptDisplayChanges(Display /*displayId*/) {
    return Error::NONE;
}

Error DrmComposerHal::presentDisplay(Display displayId, int32_t* /*outPresentFence*/,
        std::vector<Layer>* /*outLayers*/, std::vector<int32_t>* /*outReleaseFences*/) {
    auto display = mDevice->getConnectedDisplay(displayId);
    if (!display)
        return Error::BAD_DISPLAY;
    if (!mBuffer)
        return Error::NO_RESOURCES;

    if (mAcquireFence >= 0) {
        sync_wait(mAcquireFence, -1);
        mAcquireFence.reset();
    }

    display->present(mBuffer);
    // TODO: Present/release fence
    return Error::NONE;
}

Error DrmComposerHal::setLayerCursorPosition(Display /*displayId*/,
        Layer /*layer*/, int32_t /*x*/, int32_t /*y*/) {
    return Error::NONE; // Ignored
}

Error DrmComposerHal::setLayerBuffer(Display /*displayId*/, Layer /*layer*/,
        buffer_handle_t /*buffer*/, int32_t acquireFence) {

    if (acquireFence >= 0) {
        /*
         * This buffer will be only used by client composition.
         * During client composition, SurfaceFlinger will wait for the buffer
         * (if necessary), so there is no need to do it here.
         */
        close(acquireFence);
    }

    return Error::NONE; // Ignored
}

Error DrmComposerHal::setLayerSurfaceDamage(Display /*displayId*/,
        Layer /*layer*/, const std::vector<hwc_rect_t>& /*damage*/) {
    return Error::NONE; // Ignored
}

Error DrmComposerHal::setLayerBlendMode(Display /*displayId*/,
        Layer /*layer*/, int32_t /*mode*/) {
    return Error::NONE; // Ignored
}

Error DrmComposerHal::setLayerColor(Display /*displayId*/,
        Layer /*layer*/, IComposerClient::Color /*color*/) {
    return Error::NONE; // Ignored
}

Error DrmComposerHal::setLayerCompositionType(Display displayId, Layer layer, int32_t type) {
    auto i = mLayers.find(layer);
    if (i == mLayers.end())
        return Error::BAD_LAYER;
    if (displayId != i->second.displayId)
        return Error::BAD_DISPLAY;

    i->second.composition = static_cast<IComposerClient::Composition>(type);
    return Error::NONE;
}

Error DrmComposerHal::setLayerDataspace(Display /*displayId*/,
        Layer /*layer*/, int32_t /*dataspace*/) {
    return Error::NONE; // Ignored
}

Error DrmComposerHal::setLayerDisplayFrame(Display /*displayId*/,
        Layer /*layer*/, const hwc_rect_t& /*frame*/) {
    return Error::NONE; // Ignored
}

Error DrmComposerHal::setLayerPlaneAlpha(Display /*displayId*/,
        Layer /*layer*/, float /*alpha*/) {
    return Error::NONE; // Ignored
}

Error DrmComposerHal::setLayerSidebandStream(Display /*displayId*/,
        Layer /*layer*/, buffer_handle_t /*stream*/) {
    return Error::NONE; // Ignored
}

Error DrmComposerHal::setLayerSourceCrop(Display /*displayId*/,
        Layer /*layer*/, const hwc_frect_t& /*crop*/) {
    return Error::NONE; // Ignored
}

Error DrmComposerHal::setLayerTransform(Display /*displayId*/,
        Layer /*layer*/, int32_t /*transform*/) {
    return Error::NONE; // Ignored
}

Error DrmComposerHal::setLayerVisibleRegion(Display /*displayId*/,
        Layer /*layer*/, const std::vector<hwc_rect_t>& /*visible*/) {
    return Error::NONE; // Ignored
}

Error DrmComposerHal::setLayerZOrder(Display /*displayId*/,
        Layer /*layer*/, uint32_t /*z*/) {
    return Error::NONE; // Ignored
}

}  // namespace drmfb
}  // namespace V2_1
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
