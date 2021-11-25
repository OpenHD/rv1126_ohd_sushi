/*
 * Isp20StatsBuffer.h - isp20 stats buffer
 *
 *  Copyright (c) 2019 Rockchip Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef ISP20_EVTS_H
#define ISP20_EVTS_H

#include "ICamHw.h"
#include "SensorHw.h"

using namespace XCam;

namespace RkCam {

class Isp20Evt
    : public ispHwEvt_t {
public:
    explicit Isp20Evt(ICamHw *camHw, SmartPtr<BaseSensorHw> sensor) {
        mSensor = sensor;
        mCamHw = camHw;
    };
    virtual ~Isp20Evt() {};

    XCamReturn getEffectiveExpParams (uint32_t frameId, SmartPtr<RkAiqExpParamsProxy>& expParams) {
        XCAM_ASSERT (mSensor.ptr ());
        return mSensor->getEffectiveExpParams(expParams, frameId);
    }

    XCamReturn getEffectiveIspParams(uint32_t frameId, SmartPtr<RkAiqIspMeasParamsProxy>& ispParams) {
        XCAM_ASSERT (mCamHw);
        return mCamHw->getEffectiveIspParams(ispParams, frameId);
    }

    uint32_t sequence;
    uint32_t expDelay;

private:
    // XCAM_DEAD_COPY(Isp20Evt);
    SmartPtr<BaseSensorHw> mSensor;
    ICamHw *mCamHw;
};

}

#endif
