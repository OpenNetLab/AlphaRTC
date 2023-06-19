/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef API_TRANSPORT_GOOG_CC_FACTORY_H_
#define API_TRANSPORT_GOOG_CC_FACTORY_H_
#include <memory>

#include "api/network_state_predictor.h"
#include "api/transport/network_control.h"
#include "rtc_base/deprecation.h"

namespace webrtc {
class RtcEventLog;

struct AlphaCcFactoryConfig {
  std::unique_ptr<NetworkStateEstimatorFactory>
      network_state_estimator_factory = nullptr;
  NetworkStatePredictorFactoryInterface* network_state_predictor_factory =
      nullptr;
  bool feedback_only = false;
};

class AlphaCcNetworkControllerFactory
    : public NetworkControllerFactoryInterface {
 public:
  AlphaCcNetworkControllerFactory() = default;
  explicit RTC_DEPRECATED AlphaCcNetworkControllerFactory(
      RtcEventLog* event_log);
  explicit AlphaCcNetworkControllerFactory(
      NetworkStatePredictorFactoryInterface* network_state_predictor_factory);

  explicit AlphaCcNetworkControllerFactory(AlphaCcFactoryConfig config);
  std::unique_ptr<NetworkControllerInterface> Create(
      NetworkControllerConfig config) override;
  TimeDelta GetProcessInterval() const override;

 protected:
  RtcEventLog* const event_log_ = nullptr;
  AlphaCcFactoryConfig factory_config_;
};

// Deprecated, use AlphaCcFactoryConfig to enable feedback only mode instead.
// Factory to create packet feedback only GoogCC, this can be used for
// connections providing packet receive time feedback but no other reports.
class RTC_DEPRECATED AlphaCcFeedbackNetworkControllerFactory
    : public AlphaCcNetworkControllerFactory {
 public:
  explicit AlphaCcFeedbackNetworkControllerFactory(RtcEventLog* event_log);
};

}  // namespace webrtc

#endif  // API_TRANSPORT_GOOG_CC_FACTORY_H_
