# drmfb-composer
[drmfb-composer] is a simple [HIDL] HAL implementation of [`android.hardware.graphics.composer@2.1`] using
Linux DRM legacy [Kernel Mode Setting].
It specifically does **not** make use of [Atomic Mode Setting] to support older hardware and kernels.

**Note:** For newer devices with stable [Atomic Mode Setting] support, prefer using [drm_hwcomposer].
It is a more complete implementation that can offload parts of the composition from the GPU to hardware.
(See [Comparison to drm_hwcomposer](#comparison-to-drm_hwcomposer))

## Introduction
[drmfb-composer] is mostly a proof of concept: it is a Graphics Composer HAL that works very much like
the old Framebuffer (FB) HALs. This allows implementing additional features like multiple display support,
while still supporting all old hardware.

The main difference from implementations like [drm_hwcomposer] is that it does not do any composition in hardware.
It only handles displaying the results of client (GPU) composition in SurfaceFlinger using [Kernel Mode Setting].
That makes it less efficient, but avoids use of [Atomic Mode Setting] and multiple planes that sometimes cause issues
on older hardware.

It has also turned out to be a great learning experience, especially to understand which features are supposed
to be provided by a Graphics Composer HAL, and how they can be implemented using DRM/KMS.

## Features
- Multiple displays and hotplug
  - **Note:** Although not limited in the Composer HAL, the current Android framework limits this to:
    - Only two displays working at the same time (one _primary_ and one _external_)
    - Hotplugging the first (_primary_) display will result in crashes
- Exposes all available displays modes (e.g. possible lower resolutions or refresh rates)
- Hardware vertical sync (VSYNC) signals

### Comparison to [drm_hwcomposer] (HWC2 HAL)
[drm_hwcomposer] is a more complete and efficient implementation of a HWC2 HAL implemented using [Atomic Mode Setting].
However, it does not run on older hardware without [Atomic Mode Setting] (e.g. radeon) in the kernel.

#### Features missing in [drmfb-composer]

- [Atomic Mode Setting]
- Hardware Composition
  - Currently, [drmfb-composer] lets SurfaceFlinger fall back to client composition using GLES on the GPU for all layers.
    Eventually, some simple composition could be implemented (e.g. cursor planes).
- [Explicit Synchronization] (e.g. Release Fences)
  - `IN_FENCE_FD` and `OUT_FENCE_PTR` only exist as properties for [Atomic Mode Setting]

#### Features missing in [drm_hwcomposer]
These are features that I would like to port to [drm_hwcomposer] eventually.
(Still working on getting [drm_hwcomposer] working on my primary test device.)

- Multiple display support and hotplug: Already proposed as
  [[WIP/RFC] External display hotplug support](https://gitlab.freedesktop.org/drm-hwcomposer/drm-hwcomposer/merge_requests/27)
- XBGR8888 fallback for ABGR8888 for primary planes on older Intel GPUs
- Runtime selection of [libdrm] or [minigbm] importer (currently a compile-time option in [drm_hwcomposer])

### Comparison to [drm_gralloc] (FB HAL)
[drm_gralloc] was one of the first Gralloc HAL implementations for DRM and used to implement a Framebuffer HAL that is
still being used in [Android-x86] today. [drmfb-composer] has a couple of advantages compared to drm_gralloc:

- Modern implementation of a Graphics Composer HAL instead of a legacy FB HAL
- [PRIME file descriptors] instead of [GEM names]
- Works fully binderized and with DRM GPU authentication enabled
- Multiple display support with different content instead of only screen mirroring

### Comparison to [drm_framebuffer] (FB HAL)
[drm_framebuffer] was an earlier version of [drmfb-composer] implemented as a Framebuffer HAL (FB HAL). Implementing a proper
Graphics Composer HAL has a couple of advantages:

- Modern implementation of a Graphics Composer HAL instead of a legacy FB HAL
- No need for changes in the Gralloc HAL (drm_framebuffer was a static library that required changes in the Gralloc HAL)
- Supports both [libdrm] gralloc handle and [minigbm] at the same time  (detected at runtime)
- Multiple display support, hotplug and screen mirroring (by default extra screens are mirrored)

## Requirements
- Linux kernel with a GPU supported by DRM (e.g. i915, radeon, ...)
- Android 9 (Pie) or newer
  - Necessary for properly working multiple display support and hotplugging
  - `android.hardware.graphics.composer@2.1-hal` (a header library [drmfb-composer] is built on) does not exist on previous
    Android versions
- Gralloc HAL: [libdrm] gralloc handle compatible (e.g. [gbm_gralloc]) or [minigbm] (detected at runtime)

## Usage
Add [drmfb-composer] to your Android build tree and build `android.hardware.graphics.composer@2.1-service.drmfb`.

[drmfb-composer] is a binderized HIDL HAL service. Ensure that the binary and init file is pushed to your device:
  - `/vendor/bin/hw/android.hardware.graphics.composer@2.1-service.drmfb`
  - `/vendor/etc/init/android.hardware.graphics.composer@2.1-service.drmfb.rc`

## License
[drmfb-composer] is licensed under the [Apache License, Version 2.0]. Contributions welcome!

[drmfb-composer]: https://github.com/me176c-dev/drmfb-composer
[HIDL]: https://source.android.com/devices/architecture/hidl
[`android.hardware.graphics.composer@2.1`]: https://source.android.com/devices/graphics/implement-hwc
[Kernel Mode Setting]: https://www.kernel.org/doc/html/latest/gpu/drm-kms.html
[Atomic Mode Setting]: https://www.kernel.org/doc/html/latest/gpu/drm-kms.html#atomic-mode-setting
[Explicit Synchronization]: https://source.android.com/devices/graphics/implement-vsync#hardware_composer_integration
[drm_hwcomposer]: https://gitlab.freedesktop.org/drm-hwcomposer/drm-hwcomposer
[libdrm]: https://gitlab.freedesktop.org/mesa/drm
[gbm_gralloc]: https://github.com/robherring/gbm_gralloc
[minigbm]: https://chromium.googlesource.com/chromiumos/platform/minigbm/
[drm_gralloc]: https://android.googlesource.com/platform/external/drm_gralloc/
[Android-x86]: http://www.android-x86.org
[PRIME file descriptors]: https://www.kernel.org/doc/html/latest/gpu/drm-mm.html#prime-buffer-sharing
[GEM names]: https://www.kernel.org/doc/html/latest/gpu/drm-mm.html#gem-objects-naming
[drm_framebuffer]: https://github.com/lambdadroid/drm_framebuffer
[Apache License, Version 2.0]: https://www.apache.org/licenses/LICENSE-2.0
