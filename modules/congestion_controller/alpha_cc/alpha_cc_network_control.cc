/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "modules/congestion_controller/alpha_cc/alpha_cc_network_control.h"

#include <inttypes.h>
#include <stdio.h>
#include <algorithm>
#include <cstdint>
#include <numeric>
#include <string>
#include <utility>

#include "modules/remote_bitrate_estimator/include/bwe_defines.h"
#include "rtc_base/logging.h"

namespace webrtc {
namespace {

/*
// From RTCPSender video report interval.
constexpr TimeDelta kLossUpdateInterval = TimeDelta::Millis<1000>();
*/
// Pacing-rate relative to our target send rate.
// Multiplicative factor that is applied to the target bitrate to calculate
// the number of bytes that can be transmitted per interval.
// Increasing this factor will result in lower delays in cases of bitrate
// overshoots from the encoder.
const float kDefaultPaceMultiplier = 2.5f;

bool IsNotDisabled(const WebRtcKeyValueConfig* config, absl::string_view key) {
  return config->Lookup(key).find("Disabled") != 0;
}
}  // namespace

GoogCcNetworkController::GoogCcNetworkController(NetworkControllerConfig config,
                                                 GoogCcConfig alpha_cc_config)
    : key_value_config_(config.key_value_config ? config.key_value_config
                                                : &trial_based_config_),
      safe_reset_on_route_change_("Enabled"),
      safe_reset_acknowledged_rate_("ack"),
      last_received_bandwidth_estimate_(DataRate::KilobitsPerSec(300)),
      last_received_pacing_rate_(300000),
      use_min_allocatable_as_lower_bound_(
          IsNotDisabled(key_value_config_, "WebRTC-Bwe-MinAllocAsLowerBound")),
      initial_config_(config),
      pacing_factor_(config.stream_based_config.pacing_factor.value_or(
          kDefaultPaceMultiplier)),
      min_total_allocated_bitrate_(
          config.stream_based_config.min_total_allocated_bitrate.value_or(
              DataRate::Zero())) {
  RTC_DCHECK(config.constraints.at_time.IsFinite());
  ParseFieldTrial(
      {&safe_reset_on_route_change_, &safe_reset_acknowledged_rate_},
      key_value_config_->Lookup("WebRTC-Bwe-SafeResetOnRouteChange"));
}

GoogCcNetworkController::~GoogCcNetworkController() {}

NetworkControlUpdate GoogCcNetworkController::OnNetworkAvailability(
    NetworkAvailability msg) {
  return NetworkControlUpdate();
}

NetworkControlUpdate GoogCcNetworkController::OnNetworkRouteChange(
    NetworkRouteChange msg) {
  return NetworkControlUpdate();
}

NetworkControlUpdate GoogCcNetworkController::OnProcessInterval(
    ProcessInterval msg) {
  return NetworkControlUpdate();
}

NetworkControlUpdate GoogCcNetworkController::OnRemoteBitrateReport(
    RemoteBitrateReport msg) {
  return NetworkControlUpdate();
}

NetworkControlUpdate GoogCcNetworkController::OnRoundTripTimeUpdate(
    RoundTripTimeUpdate msg) {
  return NetworkControlUpdate();
}

NetworkControlUpdate GoogCcNetworkController::OnSentPacket(
    SentPacket sent_packet) {
  return NetworkControlUpdate();
}

NetworkControlUpdate GoogCcNetworkController::OnStreamsConfig(
    StreamsConfig msg) {
  NetworkControlUpdate update;
  update.target_rate = TargetTransferRate();
  update.target_rate->network_estimate.at_time = msg.at_time;
  update.target_rate->network_estimate.bandwidth = last_received_bandwidth_estimate_;
  update.target_rate->network_estimate.loss_rate_ratio =
      last_estimated_fraction_loss_ / 255.0;
  update.target_rate->network_estimate.round_trip_time = TimeDelta::Millis(last_estimated_rtt_ms_);

  TimeDelta default_bwe_period = TimeDelta::Seconds(3);  // the default is 3sec
  update.target_rate->network_estimate.bwe_period = default_bwe_period;
  update.target_rate->at_time = msg.at_time;
  update.target_rate->target_rate = last_received_bandwidth_estimate_;

  //*-----Set pacing & padding_rate-----*//
  int32_t default_pacing_rate = last_received_pacing_rate_;   // default:300000;=> 750000 bps = 750 kbps
  int32_t default_padding_rate = 0;  // default: 0bps = 0kbps
  DataRate pacing_rate = DataRate::BitsPerSec(default_pacing_rate * pacing_factor_);
  DataRate padding_rate = DataRate::BitsPerSec(default_padding_rate);
  PacerConfig message;
  message.at_time = msg.at_time;
  message.time_window = TimeDelta::Seconds(1);
  message.data_window = pacing_rate * message.time_window;
  message.pad_window = padding_rate * message.time_window;

  update.pacer_config = message;

  // *-----Set congestion_window-----*//
  update.congestion_window = current_data_window_;

  return update;
}

NetworkControlUpdate GoogCcNetworkController::OnReceivedPacket(
    ReceivedPacket received_packet) {
  return NetworkControlUpdate();
}

// Make this alive since this might be used to tune the birate.
NetworkControlUpdate GoogCcNetworkController::OnTargetRateConstraints(
    TargetRateConstraints constraints) {
  return NetworkControlUpdate();
}

NetworkControlUpdate GoogCcNetworkController::GetDefaultState(
    Timestamp at_time) {
  //*-----Set target_rate-----*//
  constexpr int32_t default_bitrate_bps = 300000;  // default: 300000 bps = 300 kbps
  DataRate bandwidth = DataRate::BitsPerSec(default_bitrate_bps);
  TimeDelta rtt = TimeDelta::Millis(last_estimated_rtt_ms_);
  NetworkControlUpdate update;
  update.target_rate = TargetTransferRate();
  update.target_rate->network_estimate.at_time = at_time;
  update.target_rate->network_estimate.bandwidth = bandwidth;
  update.target_rate->network_estimate.loss_rate_ratio =
      last_estimated_fraction_loss_ / 255.0;
  update.target_rate->network_estimate.round_trip_time = rtt;

  TimeDelta default_bwe_period = TimeDelta::Seconds(3);  // the default is 3sec
  update.target_rate->network_estimate.bwe_period = default_bwe_period;
  update.target_rate->at_time = at_time;
  update.target_rate->target_rate = bandwidth;

  //*-----Set pacing & padding_rate-----*//
  int32_t default_pacing_rate = 300000;   // default:300000;=> 750000 bps = 750 kbps
  int32_t default_padding_rate = 0;  // default: 0bps = 0kbps
  DataRate pacing_rate = DataRate::BitsPerSec(default_pacing_rate * pacing_factor_);
  DataRate padding_rate = DataRate::BitsPerSec(default_padding_rate);
  PacerConfig msg;
  msg.at_time = at_time;
  msg.time_window = TimeDelta::Seconds(1);
  msg.data_window = pacing_rate * msg.time_window;
  msg.pad_window = padding_rate * msg.time_window;

  update.pacer_config = msg;

  //*-----Set congestion_window-----*//
  update.congestion_window = current_data_window_;

  //*-----Set probe_cluster_configs-----*//

  /*
  ClampConstraints();

  update.probe_cluster_configs = probe_controller_->SetBitrates(
      min_data_rate_.bps(), GetBpsOrDefault(starting_rate_, -1),
      max_data_rate_.bps_or(-1), at_time.ms());
  */

  // MSRA:SetBitrates:32000,300000,-1,139444174
  // 0,-1,-1,139861040
  return update;
}

NetworkControlUpdate GoogCcNetworkController::OnReceiveBwe(BweMessage bwe) {
  RTC_LOG(LS_INFO) << "OnReceiveBwe: " << bwe.target_rate << " bps"
                   << " at time " << bwe.timestamp_ms << " ms"
                   << " pacing rate: " << bwe.pacing_rate << " bps";
  int32_t default_bitrate_bps = static_cast<int32_t>(bwe.target_rate);  // default: 300000 bps = 300 kbps
  DataRate bandwidth = DataRate::BitsPerSec(default_bitrate_bps);
  TimeDelta rtt = TimeDelta::Millis(last_estimated_rtt_ms_);
  NetworkControlUpdate update;
  update.target_rate = TargetTransferRate();
  update.target_rate->network_estimate.at_time = Timestamp::Millis(bwe.timestamp_ms);
  update.target_rate->network_estimate.bandwidth = bandwidth;
  update.target_rate->network_estimate.loss_rate_ratio =
      last_estimated_fraction_loss_ / 255.0;
  update.target_rate->network_estimate.round_trip_time = rtt;

  TimeDelta default_bwe_period = TimeDelta::Seconds(3);  // the default is 3sec
  update.target_rate->network_estimate.bwe_period = default_bwe_period;
  update.target_rate->at_time = Timestamp::Millis(bwe.timestamp_ms);
  update.target_rate->target_rate = bandwidth;
  last_received_bandwidth_estimate_ = bandwidth;

  //*-----Set pacing & padding_rate-----*//
  int32_t default_pacing_rate = static_cast<int32_t>(bwe.pacing_rate); 
  int32_t default_padding_rate = 0;  // default: 0bps = 0kbps
  DataRate pacing_rate = DataRate::BitsPerSec(default_pacing_rate * pacing_factor_);
  DataRate padding_rate = DataRate::BitsPerSec(default_padding_rate);
  last_received_pacing_rate_ = default_pacing_rate;
  PacerConfig msg;
  msg.at_time = Timestamp::Millis(bwe.timestamp_ms);
  msg.time_window = TimeDelta::Seconds(1);
  msg.data_window = pacing_rate * msg.time_window;
  msg.pad_window = padding_rate * msg.time_window;

  update.pacer_config = msg;

  //*-----Set congestion_window-----*//
  update.congestion_window = current_data_window_;
  return update;
}

void GoogCcNetworkController::ClampConstraints() {
  // TODO(holmer): We should make sure the default bitrates are set to 10 kbps,
  // and that we don't try to set the min bitrate to 0 from any applications.
  // The congestion controller should allow a min bitrate of 0.
  min_data_rate_ =
      std::max(min_data_rate_, congestion_controller::GetMinBitrate());
  if (use_min_allocatable_as_lower_bound_)
    min_data_rate_ = std::max(min_data_rate_, min_total_allocated_bitrate_);
  if (max_data_rate_ < min_data_rate_) {
    RTC_LOG(LS_WARNING) << "max bitrate smaller than min bitrate";
    max_data_rate_ = min_data_rate_;
  }
  if (starting_rate_ && starting_rate_ < min_data_rate_) {
    RTC_LOG(LS_WARNING) << "start bitrate smaller than min bitrate";
    starting_rate_ = min_data_rate_;
  }
}

NetworkControlUpdate GoogCcNetworkController::OnTransportLossReport(
    TransportLossReport msg) {
  return NetworkControlUpdate();
}

NetworkControlUpdate GoogCcNetworkController::OnTransportPacketsFeedback(
    TransportPacketsFeedback report) {
  return NetworkControlUpdate();
}

NetworkControlUpdate GoogCcNetworkController::OnNetworkStateEstimate(
    NetworkStateEstimate msg) {
  return NetworkControlUpdate();
}

}  // namespace webrtc
