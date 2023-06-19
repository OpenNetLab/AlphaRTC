/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "api/transport/alpha_cc_factory.h"

#include <memory>
#include <utility>
#include "absl/memory/memory.h"
#include "modules/congestion_controller/alpha_cc/alpha_cc_network_control.h"
#include "rtc_base/logging.h"

namespace webrtc {
AlphaCcNetworkControllerFactory::AlphaCcNetworkControllerFactory(
    RtcEventLog* event_log)
    : event_log_(event_log) {}

AlphaCcNetworkControllerFactory::AlphaCcNetworkControllerFactory(
    NetworkStatePredictorFactoryInterface* network_state_predictor_factory) {
  factory_config_.network_state_predictor_factory =
      network_state_predictor_factory;
}

AlphaCcNetworkControllerFactory::AlphaCcNetworkControllerFactory(
    AlphaCcFactoryConfig config)
    : factory_config_(std::move(config)) {}

std::unique_ptr<NetworkControllerInterface>
AlphaCcNetworkControllerFactory::Create(NetworkControllerConfig config) {
  if (event_log_)
    config.event_log = event_log_;
  AlphaCcConfig goog_cc_config;
  goog_cc_config.feedback_only = factory_config_.feedback_only;
  if (factory_config_.network_state_estimator_factory) {
    RTC_DCHECK(config.key_value_config);
    goog_cc_config.network_state_estimator =
        factory_config_.network_state_estimator_factory->Create(
            config.key_value_config);
  }
  if (factory_config_.network_state_predictor_factory) {
    goog_cc_config.network_state_predictor =
        factory_config_.network_state_predictor_factory
            ->CreateNetworkStatePredictor();
  }
  return std::make_unique<AlphaCcNetworkController>(config,
                                                   std::move(goog_cc_config));
}

TimeDelta AlphaCcNetworkControllerFactory::GetProcessInterval() const {
  const int64_t kUpdateIntervalMs = 25;
  return TimeDelta::Millis(kUpdateIntervalMs);
}

AlphaCcFeedbackNetworkControllerFactory::AlphaCcFeedbackNetworkControllerFactory(
    RtcEventLog* event_log)
    : AlphaCcNetworkControllerFactory(event_log) {
  factory_config_.feedback_only = true;
}

}  // namespace webrtc
