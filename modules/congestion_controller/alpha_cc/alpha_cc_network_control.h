/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MODULES_CONGESTION_CONTROLLER_GOOG_CC_GOOG_CC_NETWORK_CONTROL_H_
#define MODULES_CONGESTION_CONTROLLER_GOOG_CC_GOOG_CC_NETWORK_CONTROL_H_

#include <stdint.h>

#include <deque>
#include <memory>
#include <vector>

#include "absl/types/optional.h"
#include "api/network_state_predictor.h"
#include "api/rtc_event_log/rtc_event_log.h"
#include "api/transport/field_trial_based_config.h"
#include "api/transport/network_control.h"
#include "rtc_base/constructor_magic.h"
#include "rtc_base/experiments/field_trial_parser.h"

namespace webrtc {
struct GoogCcConfig {
  std::unique_ptr<NetworkStateEstimator> network_state_estimator = nullptr;
  std::unique_ptr<NetworkStatePredictor> network_state_predictor = nullptr;
  bool feedback_only = false;
};

class GoogCcNetworkController : public NetworkControllerInterface {
 public:
  GoogCcNetworkController(NetworkControllerConfig config,
                          GoogCcConfig alpha_cc_config);
  ~GoogCcNetworkController() override;

  // NetworkControllerInterface
  NetworkControlUpdate OnNetworkAvailability(NetworkAvailability msg) override;
  NetworkControlUpdate OnNetworkRouteChange(NetworkRouteChange msg) override;
  NetworkControlUpdate OnProcessInterval(ProcessInterval msg) override;
  NetworkControlUpdate OnRemoteBitrateReport(RemoteBitrateReport msg) override;
  NetworkControlUpdate OnRoundTripTimeUpdate(RoundTripTimeUpdate msg) override;
  NetworkControlUpdate OnSentPacket(SentPacket msg) override;
  NetworkControlUpdate OnReceivedPacket(ReceivedPacket msg) override;
  NetworkControlUpdate OnStreamsConfig(StreamsConfig msg) override;
  NetworkControlUpdate OnTargetRateConstraints(
      TargetRateConstraints msg) override;
  NetworkControlUpdate OnTransportLossReport(TransportLossReport msg) override;
  NetworkControlUpdate OnTransportPacketsFeedback(
      TransportPacketsFeedback msg) override;
  NetworkControlUpdate OnNetworkStateEstimate(
      NetworkStateEstimate msg) override;      
  NetworkControlUpdate OnReceiveBwe(BweMessage msg) override;
  NetworkControlUpdate GetNetworkState(Timestamp at_time) const;
  NetworkControlUpdate GetDefaultState(Timestamp at_time);

 private:
  friend class GoogCcStatePrinter;
  std::vector<ProbeClusterConfig> ResetConstraints(
      TargetRateConstraints new_constraints);
  void ClampConstraints();
  void MaybeTriggerOnNetworkChanged(NetworkControlUpdate* update,
                                    Timestamp at_time);
  PacerConfig GetPacingRates(Timestamp at_time) const;
  const FieldTrialBasedConfig trial_based_config_;

  const WebRtcKeyValueConfig* const key_value_config_;
  FieldTrialFlag safe_reset_on_route_change_;
  FieldTrialFlag safe_reset_acknowledged_rate_;
  DataRate last_received_bandwidth_estimate_= DataRate::Zero();
  int32_t last_received_pacing_rate_ = 0;
  const bool use_min_allocatable_as_lower_bound_;

  absl::optional<NetworkControllerConfig> initial_config_;
  DataRate min_target_rate_ = DataRate::Zero();
  DataRate min_data_rate_ = DataRate::Zero();
  DataRate max_data_rate_ = DataRate::PlusInfinity();
  absl::optional<DataRate> starting_rate_;

  bool first_packet_sent_ = false;

  Timestamp next_loss_update_ = Timestamp::MinusInfinity();
  int lost_packets_since_last_loss_update_ = 0;
  int expected_packets_since_last_loss_update_ = 0;

  std::deque<int64_t> feedback_max_rtts_;

  int32_t last_estimated_bitrate_bps_ = 0;
  uint8_t last_estimated_fraction_loss_ = 0;
  int64_t last_estimated_rtt_ms_ = 0;

  double pacing_factor_;
  DataRate min_total_allocated_bitrate_;
  bool previously_in_alr = false;

  absl::optional<DataSize> current_data_window_;

  RTC_DISALLOW_IMPLICIT_CONSTRUCTORS(GoogCcNetworkController);
};

}  // namespace webrtc

#endif  // MODULES_CONGESTION_CONTROLLER_GOOG_CC_GOOG_CC_NETWORK_CONTROL_H_
