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
#include "modules/third_party/rl_agent/rl_agent.h"

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

// TODO: Implement AlphaCC version of MaybeTriggerOnNetworkChanged
// where target_rate is set as latest action of the RL policy
GoogCcNetworkController::GoogCcNetworkController(NetworkControllerConfig config,
                                                 GoogCcConfig alpha_cc_config)
    : key_value_config_(config.key_value_config ? config.key_value_config
                                                : &trial_based_config_),
      event_log_(config.event_log),
      safe_reset_on_route_change_("Enabled"),
      safe_reset_acknowledged_rate_("ack"),
      use_min_allocatable_as_lower_bound_(
          IsNotDisabled(key_value_config_, "WebRTC-Bwe-MinAllocAsLowerBound")),
      bandwidth_estimation_(
          std::make_unique<SendSideBandwidthEstimation>(event_log_)),
      initial_config_(config),
      last_estimated_bitrate_bps_(DataRate::KilobitsPerSec(300)), // 300Kbps
      pacing_factor_(config.stream_based_config.pacing_factor.value_or(
          kDefaultPaceMultiplier)),
      max_padding_rate_(config.stream_based_config.max_padding_rate.value_or(
          DataRate::Zero())),
      min_total_allocated_bitrate_(
          config.stream_based_config.min_total_allocated_bitrate.value_or(
              DataRate::Zero())) {
  RTC_DCHECK(config.constraints.at_time.IsFinite());
  ParseFieldTrial(
      {&safe_reset_on_route_change_, &safe_reset_acknowledged_rate_},
      key_value_config_->Lookup("WebRTC-Bwe-SafeResetOnRouteChange"));
  RTC_LOG(LS_VERBOSE) << "Using AlphaCC:"
  << " pacing_factor_ init as " << pacing_factor_
  << " min_total_allocated_bitrate_ init as (kbps) " << min_total_allocated_bitrate_.kbps()
  << " max_padding_rate_ init as (kbps) " << max_padding_rate_.kbps();
}

GoogCcNetworkController::~GoogCcNetworkController() {}

NetworkControlUpdate GoogCcNetworkController::OnNetworkAvailability(
    NetworkAvailability msg) {
  RTC_LOG(LS_VERBOSE) << "AlphaCC: OnNetworkAvailability called";
  return NetworkControlUpdate();
}

NetworkControlUpdate GoogCcNetworkController::OnNetworkRouteChange(
    NetworkRouteChange msg) {
  RTC_LOG(LS_VERBOSE) << "AlphaCC: OnNetworkRouteChange called";
  return NetworkControlUpdate();
}

void GoogCcNetworkController::SendState() {
  if (receiver_side_thp_v.size() > 0) {
    CompAverageReceiverSideThroughput();
  }

  RTC_LOG(LS_INFO) << "AlphaCC: SendState:"
  << ",LossRate " << last_loss_rate_
  << ",RTT " << last_rtt_ms_
  << ",DelayInterval " << last_delay_interval_ms_
  << ",RecvThp " << last_avg_receiver_side_thp_;
  rl_agent::SendState(last_loss_rate_, last_rtt_ms_, last_delay_interval_ms_, last_avg_receiver_side_thp_);
}

NetworkControlUpdate GoogCcNetworkController::OnProcessInterval(
    ProcessInterval msg) {
  RTC_LOG(LS_VERBOSE) << "AlphaCC: OnProcessInterval called";
  // Check the file that contains latest bwe only after it is updated

  last_estimated_bitrate_bps_ = DataRate::BitsPerSec(rl_agent::GetBwe());
  NetworkControlUpdate update;
  update.target_rate = TargetTransferRate();
  update.target_rate->network_estimate.at_time = msg.at_time;
  update.target_rate->network_estimate.bandwidth = last_estimated_bitrate_bps_;
  update.target_rate->network_estimate.loss_rate_ratio =
      bandwidth_estimation_->fraction_loss() / 255.0f;
  update.target_rate->network_estimate.round_trip_time =
      bandwidth_estimation_->round_trip_time();

  // Following the default bwe_period in GCC's delay_based_bwe_
  update.target_rate->network_estimate.bwe_period = TimeDelta::Seconds(3);
  update.target_rate->at_time = msg.at_time;
  update.target_rate->target_rate = last_estimated_bitrate_bps_;
  update.target_rate->stable_target_rate = last_estimated_bitrate_bps_;
  update.pacer_config = GetPacingRates(msg.at_time);
  // Not used (since we don't use a congestion window pushback controller)
  update.congestion_window = current_data_window_;
  return update;
  return NetworkControlUpdate();
}

PacerConfig GoogCcNetworkController::GetPacingRates(Timestamp at_time) const {
  // Pacing rate is based on target rate before congestion window pushback,
  // because we don't want to build queues in the pacer when pushback occurs.
  DataRate pacing_rate =
      std::max(min_total_allocated_bitrate_, last_estimated_bitrate_bps_) *
      pacing_factor_;
  DataRate padding_rate =
      std::min(max_padding_rate_, last_estimated_bitrate_bps_);
  RTC_LOG(LS_VERBOSE) << "AlphaCC: GetPacingRates called: "
  << " min_total_allocated_bitrate_ (kbps) " << min_total_allocated_bitrate_.kbps()
  << " last_estimated_bitrate_bps_ (kbps) " << last_estimated_bitrate_bps_.kbps()
  << " max_padding_rate_ (kbps) " << max_padding_rate_.kbps();
  PacerConfig msg;
  msg.at_time = at_time;
  msg.time_window = TimeDelta::Seconds(1);
  msg.data_window = pacing_rate * msg.time_window;
  msg.pad_window = padding_rate * msg.time_window;
  return msg;
}


NetworkControlUpdate GoogCcNetworkController::OnRemoteBitrateReport(
    RemoteBitrateReport msg) {
  RTC_LOG(LS_INFO) << "AlphaCC: OnRemoteBitrateReport called";
  return NetworkControlUpdate();
}

NetworkControlUpdate GoogCcNetworkController::OnRoundTripTimeUpdate(
    RoundTripTimeUpdate msg) {
  RTC_LOG(LS_INFO) << "AlphaCC: OnRoundTripTimeUpdate called, RTT (ms) " << msg.round_trip_time.ms();
  if (msg.smoothed)
    return NetworkControlUpdate();
  RTC_DCHECK(!msg.round_trip_time.IsZero());
  bandwidth_estimation_->UpdateRtt(msg.round_trip_time, msg.receive_time);
  last_rtt_ms_ = msg.round_trip_time.ms();
  SendState();
  return NetworkControlUpdate();
}

NetworkControlUpdate GoogCcNetworkController::OnSentPacket(
    SentPacket sent_packet) {
  RTC_LOG(LS_VERBOSE) << "AlphaCC: OnSentPacket called";
  return NetworkControlUpdate();
}

NetworkControlUpdate GoogCcNetworkController::OnStreamsConfig(
    StreamsConfig msg) {
  RTC_LOG(LS_VERBOSE) << "AlphaCC: OnStreamsConfig called";
  return GetDefaultState(msg.at_time);
}

NetworkControlUpdate GoogCcNetworkController::OnReceivedPacket(
    ReceivedPacket received_packet) {
  RTC_LOG(LS_VERBOSE) << "AlphaCC: OnReceivedPacket called";
  return NetworkControlUpdate();
}

// Make this alive since this might be used to tune the birate.
NetworkControlUpdate GoogCcNetworkController::OnTargetRateConstraints(
    TargetRateConstraints constraints) {
  RTC_LOG(LS_VERBOSE) << "AlphaCC: OnTargetRateConstraints called";
  return NetworkControlUpdate();
}

NetworkControlUpdate GoogCcNetworkController::GetDefaultState(
    Timestamp at_time) {
    RTC_LOG(LS_VERBOSE) << "AlphaCC: GetDefaultState called";
  NetworkControlUpdate update;
  update.target_rate = TargetTransferRate();
  update.target_rate->network_estimate.at_time = at_time;
  update.target_rate->network_estimate.bandwidth = last_estimated_bitrate_bps_;
  update.target_rate->network_estimate.loss_rate_ratio =
      bandwidth_estimation_->fraction_loss() / 255.0f;
  update.target_rate->network_estimate.round_trip_time =
      bandwidth_estimation_->round_trip_time();

  // Following the default bwe_period in GCC's delay_based_bwe_
  TimeDelta default_bwe_period = TimeDelta::Seconds(3);
  update.target_rate->network_estimate.bwe_period = default_bwe_period;
  update.target_rate->at_time = at_time;
  update.target_rate->target_rate = last_estimated_bitrate_bps_;

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

  return update;
}

void GoogCcNetworkController::CompAverageReceiverSideThroughput(void) {
  int size = receiver_side_thp_v.size();
  float sum = 0;
  for (float i : receiver_side_thp_v) {
    RTC_LOG(LS_VERBOSE) << "AlphaCC: CompAverageReceiverSideThroughput: item " << i;
    sum = sum + i;
  }

  last_avg_receiver_side_thp_ = sum / size;
  RTC_LOG(LS_INFO) << "AlphaCC: CompAverageReceiverSideThroughput:"
  << " last_avg_receiver_side_thp_ " << last_avg_receiver_side_thp_
  << " size " << size;
  receiver_side_thp_v.clear();
}

NetworkControlUpdate GoogCcNetworkController::OnReceiverSideThroughput(float receiver_side_thp) {
  RTC_LOG(LS_VERBOSE) << "AlphaCC: OnReceiverSideThroughput called: " << receiver_side_thp;
  receiver_side_thp_v.push_back(receiver_side_thp);
  return NetworkControlUpdate();
}

void GoogCcNetworkController::ClampConstraints() {
  RTC_LOG(LS_VERBOSE) << "AlphaCC: ClampConstraints called";
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
  int64_t total_packets_delta =
      msg.packets_received_delta + msg.packets_lost_delta;
  bandwidth_estimation_->UpdatePacketsLost(
      msg.packets_lost_delta, total_packets_delta, msg.receive_time);
  uint8_t fraction_loss = bandwidth_estimation_->fraction_loss();
  auto loss_rate = (fraction_loss * 100) / 255.0f;
  RTC_LOG(LS_INFO) << "AlphaCC: OnTransportLossReport called: "
  << " loss rate (%) " << loss_rate
  << " (total_packets_delta " << total_packets_delta
  << " packets_received_delta " << msg.packets_received_delta
  << " packets_lost_delta " << msg.packets_lost_delta << ")";
  last_loss_rate_ = loss_rate;
  return NetworkControlUpdate();
}

NetworkControlUpdate GoogCcNetworkController::OnTransportPacketsFeedback(
    TransportPacketsFeedback report) {
  RTC_LOG(LS_INFO) << "AlphaCC: OnTransportPacketsFeedback called";
  // TODO: calculate delay interval
  last_delay_interval_ms_ = 0;
  SendState();
  return NetworkControlUpdate();
}

NetworkControlUpdate GoogCcNetworkController::OnNetworkStateEstimate(
    NetworkStateEstimate msg) {
  RTC_LOG(LS_VERBOSE) << "AlphaCC: OnNetworkStateEstimate called";
  return NetworkControlUpdate();
}

}  // namespace webrtc
