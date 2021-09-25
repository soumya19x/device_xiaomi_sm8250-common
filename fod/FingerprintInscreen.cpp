/*
 * Copyright (C) 2019-2021 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "FingerprintInscreenService"

#include "FingerprintInscreen.h"
#include <android-base/logging.h>
#include <hardware_legacy/power.h>
#include <cmath>
#include <fcntl.h>
#include <fstream>
#include <thread>
#include <poll.h>
#include <sys/stat.h>

#define FINGERPRINT_ACQUIRED_VENDOR 6

#define COMMAND_NIT 10
#define PARAM_NIT_FOD 1
#define PARAM_NIT_NONE 0

#define FOD_UI_PATH "/sys/devices/platform/soc/soc:qcom,dsi-display-primary/fod_ui"

#define BRIGHTNESS_PATH "/sys/class/backlight/panel0-backlight/brightness"

namespace vendor {
namespace lineage {
namespace biometrics {
namespace fingerprint {
namespace inscreen {
namespace V1_0 {
namespace implementation {

static bool readBool(int fd) {
    char c;
    int rc;

    rc = lseek(fd, 0, SEEK_SET);
    if (rc) {
        LOG(ERROR) << "failed to seek fd, err: " << rc;
        return false;
    }

    rc = read(fd, &c, sizeof(char));
    if (rc != 1) {
        LOG(ERROR) << "failed to read bool from fd, err: " << rc;
        return false;
    }

    return c != '0';
template <typename T>
static T get(const std::string& path, const T& def) {
    std::ifstream file(path);
    T result;

    file >> result;
    return file.fail() ? def : result;
}

FingerprintInscreen::FingerprintInscreen() {
    mXiaomiFingerprintService = IXiaomiFingerprint::getService();

    std::thread([this]() {
        int fd = open(FOD_UI_PATH, O_RDONLY);
        if (fd < 0) {
            LOG(ERROR) << "failed to open fd, err: " << fd;
            return;
        }

        struct pollfd fodUiPoll = {
            .fd = fd,
            .events = POLLERR | POLLPRI,
            .revents = 0,
        };

        while (true) {
            int rc = poll(&fodUiPoll, 1, -1);
            if (rc < 0) {
                LOG(ERROR) << "failed to poll fd, err: " << rc;
                continue;
            }

            mXiaomiFingerprintService->extCmd(COMMAND_NIT,
                    readBool(fd) ? PARAM_NIT_FOD : PARAM_NIT_NONE);
        }
    }).detach();
}

Return<void> FingerprintInscreen::onStartEnroll() {
    return Void();
}

Return<void> FingerprintInscreen::onFinishEnroll() {
    return Void();
}

Return<void> FingerprintInscreen::onPress() {
    return Void();
}

Return<void> FingerprintInscreen::onRelease() {
    return Void();
}

Return<void> FingerprintInscreen::onShowFODView() {
    return Void();
}

Return<void> FingerprintInscreen::onHideFODView() {
    return Void();
}

Return<bool> FingerprintInscreen::handleAcquired(int32_t acquiredInfo, int32_t vendorCode) {
    bool handled = false;

    std::lock_guard<std::mutex> _lock(mCallbackLock);
    if (mCallback == nullptr) {
        return false;
    }

    switch (acquiredInfo) {
        case FINGERPRINT_ACQUIRED_VENDOR:
            if (vendorCode == 22) {
                Return<void> ret = mCallback->onFingerDown();
                if (!ret.isOk()) {
                    LOG(ERROR) << "FingerDown() error: " << ret.description();
                }
                handled = true;
            }

            if (vendorCode == 23) {
                Return<void> ret = mCallback->onFingerUp();
                if (!ret.isOk()) {
                    LOG(ERROR) << "FingerUp() error: " << ret.description();
                }
                handled = true;
            }
            break;
    }

    if (!handled)
        LOG(ERROR) << "acquiredInfo: " << acquiredInfo << ", vendorCode: " << vendorCode;

    return handled;
}

Return<bool> FingerprintInscreen::handleError(int32_t error, int32_t vendorCode) {
    LOG(ERROR) << "error: " << error << ", vendorCode: " << vendorCode;
    return false;
}

Return<void> FingerprintInscreen::setLongPressEnabled(bool) {
    return Void();
}

Return<int32_t> FingerprintInscreen::getDimAmount(int32_t) {
    float alpha;
    int realBrightness = get(BRIGHTNESS_PATH, 0);

    if (realBrightness >= 500) {
        alpha = 1.0 - pow(realBrightness / 2047.0 * 430.0 / 600.0, 0.485);
    } else if (realBrightness >= 250) {
        alpha = 1.0 - pow(realBrightness / 2047.0 * 430.0 / 600.0, 0.530);
    } else if (realBrightness > 60) {
         alpha = 1.0 - pow(realBrightness / 1680.0, 0.525);
    } else {
        alpha = 1.0 - pow(realBrightness / 1680.0, 0.475);
    }

    return 255 * alpha;
}

Return<bool> FingerprintInscreen::shouldBoostBrightness() {
    return false;
}

Return<void> FingerprintInscreen::setCallback(const sp<IFingerprintInscreenCallback>& callback) {
    {
        std::lock_guard<std::mutex> _lock(mCallbackLock);
        mCallback = callback;
    }

    return Void();
}

Return<int32_t> FingerprintInscreen::getPositionX() {
    return FOD_POS_X;
}

Return<int32_t> FingerprintInscreen::getPositionY() {
    return FOD_POS_Y;
}

Return<int32_t> FingerprintInscreen::getSize() {
    return FOD_SIZE;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace inscreen
}  // namespace fingerprint
}  // namespace biometrics
}  // namespace lineage
}  // namespace vendor
