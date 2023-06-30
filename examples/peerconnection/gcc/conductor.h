/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef EXAMPLES_PEERCONNECTION_CLIENT_CONDUCTOR_H_
#define EXAMPLES_PEERCONNECTION_CLIENT_CONDUCTOR_H_

#include <deque>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "api/alphacc_config.h"
#include "api/media_stream_interface.h"
#include "api/peer_connection_interface.h"
#include "examples/peerconnection/gcc/main_wnd.h"
#include "examples/peerconnection/gcc/peer_connection_client.h"
#include "pc/test/fake_video_track_source.h"
#include "test/testsupport/frame_writer.h"
#include "test/testsupport/video_frame_writer.h"

namespace webrtc {
class VideoCaptureModule;
}  // namespace webrtc

namespace cricket {
class VideoRenderer;
}  // namespace cricket

class Conductor : public webrtc::PeerConnectionObserver,
                  public webrtc::CreateSessionDescriptionObserver,
                  public PeerConnectionClientObserver,
                  public MainWndCallback {
 public:
  enum CallbackID {
    PEER_CONNECTION_CLOSED = 1,
    NEW_TRACK_ADDED,
    TRACK_REMOVED,
  };

  Conductor(PeerConnectionClient* client, MainWindow* main_wnd);

 protected:
  ~Conductor();
  bool InitializePeerConnection();
  bool ReinitializePeerConnectionForLoopback();
  bool CreatePeerConnection(bool dtls);
  void DeletePeerConnection();
  void AddTracks();

  //
  // PeerConnectionObserver implementation.
  //

  void OnSignalingChange(
      webrtc::PeerConnectionInterface::SignalingState new_state) override {}
  void OnAddTrack(
      rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
      const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&
          streams) override;
  void OnRemoveTrack(
      rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override;
  void OnDataChannel(
      rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override {}
  void OnRenegotiationNeeded() override {}
  void OnIceConnectionChange(
      webrtc::PeerConnectionInterface::IceConnectionState new_state) override {}
  void OnIceGatheringChange(
      webrtc::PeerConnectionInterface::IceGatheringState new_state) override {}
  void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
  void OnIceConnectionReceivingChange(bool receiving) override {}

  //
  // PeerConnectionClientObserver implementation.
  //

  void OnGetMessage(const std::string& message) override;

  void OnPeerDisconnected() override;

  void ConnectToPeer() override;

  //
  // MainWndCallback implementation.
  //

  void UIThreadCallback(int msg_id, void* data) override;

  void Close() override;

  void OnFrameCallback(const webrtc::VideoFrame& video_frame) override;

  // CreateSessionDescriptionObserver implementation.
  void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;
  void OnFailure(webrtc::RTCError error) override;

 protected:
  void ParseMessage(const std::string& msg);

  bool loopback_;
  rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;
  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
      peer_connection_factory_;
  PeerConnectionClient* client_;
  MainWindow* main_wnd_;
  std::deque<std::string*> pending_messages_;
  const webrtc::AlphaCCConfig* alphacc_config_;
  std::shared_ptr<rtc::Event> audio_started_;
  std::string accumulate_message_;
  std::string part_message_;
  std::unique_ptr<webrtc::test::VideoFrameWriter> frame_writer_;
};

#endif  // EXAMPLES_PEERCONNECTION_CLIENT_CONDUCTOR_H_
