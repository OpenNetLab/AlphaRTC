/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "examples/peerconnection/gcc/conductor.h"

#include <stddef.h>
#include <stdint.h>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

#include "absl/memory/memory.h"
#include "absl/types/optional.h"
#include "api/audio/audio_mixer.h"
#include "api/audio_codecs/audio_decoder_factory.h"
#include "api/audio_codecs/audio_encoder_factory.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/audio_options.h"
#include "api/create_peerconnection_factory.h"
#include "api/rtp_sender_interface.h"
#include "api/task_queue/default_task_queue_factory.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "api/video_codecs/video_decoder_factory.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "examples/peerconnection/gcc/defaults.h"
#include "modules/audio_device/include/audio_device.h"
#include "modules/audio_device/include/test_audio_device.h"
#include "modules/audio_processing/include/audio_processing.h"
#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/video_capture_factory.h"
#include "p2p/base/port_allocator.h"
#include "pc/video_track_source.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/rtc_certificate_generator.h"
#include "rtc_base/strings/json.h"
#include "test/frame_generator_capturer.h"
#include "api/test/create_frame_generator.h"
#include "test/vcm_capturer.h"

namespace {
// Names used for a IceCandidate JSON object.
const char kCandidateSdpMidName[] = "sdpMid";
const char kCandidateSdpMlineIndexName[] = "sdpMLineIndex";
const char kCandidateSdpName[] = "candidate";

// Names used for a SessionDescription JSON object.
const char kSessionDescriptionTypeName[] = "type";
const char kSessionDescriptionSdpName[] = "sdp";

class DummySetSessionDescriptionObserver
    : public webrtc::SetSessionDescriptionObserver {
 public:
  static DummySetSessionDescriptionObserver* Create() {
    return new rtc::RefCountedObject<DummySetSessionDescriptionObserver>();
  }
  virtual void OnSuccess() { RTC_LOG(INFO) << __FUNCTION__; }
  virtual void OnFailure(webrtc::RTCError error) {
    RTC_LOG(INFO) << __FUNCTION__ << " " << ToString(error.type()) << ": "
                  << error.message();
  }
};


class FrameGeneratorTrackSource : public webrtc::VideoTrackSource {
 public:
  static rtc::scoped_refptr<FrameGeneratorTrackSource> Create(
      std::shared_ptr<rtc::Event> audio_started_) {
    auto alphaCCConfig = webrtc::GetAlphaCCConfig();
    // Creat an FrameGenerator, responsible for reading y4m files
    std::unique_ptr<webrtc::test::FrameGeneratorInterface> y4m_frame_generator(
        webrtc::test::CreateFromY4mFileFrameGenerator(
            std::vector<std::string>{
                alphaCCConfig->video_file_path}, /* file_path */
            alphaCCConfig->video_width,          /*video_width */
            alphaCCConfig->video_height,         /*video_height*/
            1 /*frame_repeat_count*/));

    // Use FrameGenerator to periodically capture frames
    std::unique_ptr<webrtc::test::FrameGeneratorCapturer> capturer(
        new webrtc::test::FrameGeneratorCapturer(
            webrtc::Clock::GetRealTimeClock(),        /* clock */
            std::move(y4m_frame_generator),           /* frame_generator */
            alphaCCConfig->video_fps,                 /* target_fps*/
            *webrtc::CreateDefaultTaskQueueFactory())); /* task_queue_factory */

    return new rtc::RefCountedObject<FrameGeneratorTrackSource>(
        std::move(capturer), audio_started_);
  }

 protected:
  explicit FrameGeneratorTrackSource(
      std::unique_ptr<webrtc::test::FrameGeneratorCapturer> capturer,
      std::shared_ptr<rtc::Event> audio_started_)
      : VideoTrackSource(/*remote=*/false), capturer_(std::move(capturer)) {
    // Creat a thread that waits for the audio capturer thread
    // to start
    std::thread waiting_for_audio_started_([this, audio_started_]() {
      auto alphaCCConfig = webrtc::GetAlphaCCConfig();
      // Only wait for audio to start when use audio file
      if (alphaCCConfig->audio_source_option ==
          webrtc::AlphaCCConfig::AudioSourceOption::kAudioFile) {
        audio_started_->Wait(rtc::Event::kForever);
      }
      if (capturer_ && capturer_->Init()) {
        capturer_->Start();
      }
    });
    // Detach() instead of Join(), for non-blocking
    waiting_for_audio_started_.detach();
  }

 private:
  rtc::VideoSourceInterface<webrtc::VideoFrame>* source() override {
    return capturer_.get();
  }

  std::unique_ptr<webrtc::test::FrameGeneratorCapturer> capturer_;
};

class CapturerTrackSource : public webrtc::VideoTrackSource {
 public:
  static rtc::scoped_refptr<CapturerTrackSource> Create() {
    const size_t kWidth = 640;
    const size_t kHeight = 480;
    const size_t kFps = 30;
    std::unique_ptr<webrtc::test::VcmCapturer> capturer;
    std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
        webrtc::VideoCaptureFactory::CreateDeviceInfo());
    if (!info) {
      return nullptr;
    }
    int num_devices = info->NumberOfDevices();
    for (int i = 0; i < num_devices; ++i) {
      capturer = absl::WrapUnique(
          webrtc::test::VcmCapturer::Create(kWidth, kHeight, kFps, i));
      if (capturer) {
        return new rtc::RefCountedObject<CapturerTrackSource>(
            std::move(capturer));
      }
    }

    return nullptr;
  }

 protected:
  explicit CapturerTrackSource(
      std::unique_ptr<webrtc::test::VcmCapturer> capturer)
      : VideoTrackSource(/*remote=*/false), capturer_(std::move(capturer)) {}

 private:
  rtc::VideoSourceInterface<webrtc::VideoFrame>* source() override {
    return capturer_.get();
  }
  std::unique_ptr<webrtc::test::VcmCapturer> capturer_;
};

}  // namespace

Conductor::Conductor(PeerConnectionClient* client, MainWindow* main_wnd)
    : loopback_(false),
      client_(client),
      main_wnd_(main_wnd),
      alphacc_config_(webrtc::GetAlphaCCConfig()),
      audio_started_(std::make_shared<rtc::Event>()) {
  if (alphacc_config_->save_to_file) {
    frame_writer_ = absl::make_unique<webrtc::test::Y4mVideoFrameWriterImpl>(
        alphacc_config_->video_output_path, alphacc_config_->video_output_width,
        alphacc_config_->video_output_height,
        alphacc_config_->video_output_fps);
  } else {
    frame_writer_ = nullptr;
  }
  client_->RegisterObserver(this);
  main_wnd->RegisterObserver(this);
}

Conductor::~Conductor() {
  RTC_DCHECK(!peer_connection_);
}


void Conductor::Close() {
  client_->SignOut();
  DeletePeerConnection();
}

bool Conductor::InitializePeerConnection() {
  RTC_DCHECK(!peer_connection_factory_);
  RTC_DCHECK(!peer_connection_);

  auto task_queue_factory = webrtc::CreateDefaultTaskQueueFactory();
  rtc::scoped_refptr<webrtc::AudioDeviceModule> audio_device_module = nullptr;

  using AudioSourceOption = webrtc::AlphaCCConfig::AudioSourceOption;
  // Use audio file for audio input
  if (alphacc_config_->audio_source_option == AudioSourceOption::kAudioFile) {
    auto capturer = webrtc::TestAudioDeviceModule::CreateWavFileReader(
        alphacc_config_->audio_file_path, true);

    std::unique_ptr<webrtc::TestAudioDeviceModule::Renderer> renderer;
    if (alphacc_config_->save_to_file) {
      renderer = webrtc::TestAudioDeviceModule::CreateWavFileWriter(
          alphacc_config_->audio_output_path,
          capturer.get()->SamplingFrequency(), capturer.get()->NumChannels());
    } else {
      renderer = webrtc::TestAudioDeviceModule::CreateDiscardRenderer(
          8000 /*sampling frequecy, unused*/, 2 /*num_channels, ununsed*/);
    }

    audio_device_module = webrtc::TestAudioDeviceModule::Create(
        task_queue_factory.get(), std::move(capturer), std::move(renderer),
        audio_started_);
  } else if (alphacc_config_->audio_source_option ==
             AudioSourceOption::kMicrophone) {
    audio_device_module = nullptr;
  }

  peer_connection_factory_ = webrtc::CreatePeerConnectionFactory(
      nullptr /* network_thread */, nullptr /* worker_thread */,
      nullptr /* signaling_thread */, audio_device_module /* default_adm */,
      webrtc::CreateBuiltinAudioEncoderFactory(),
      webrtc::CreateBuiltinAudioDecoderFactory(),
      webrtc::CreateBuiltinVideoEncoderFactory(),
      webrtc::CreateBuiltinVideoDecoderFactory(), nullptr /* audio_mixer */,
      nullptr /* audio_processing */);

  if (!peer_connection_factory_) {
    main_wnd_->MessageBox("Error", "Failed to initialize PeerConnectionFactory",
                          true);
    DeletePeerConnection();
    return false;
  }

  if (!CreatePeerConnection(/*dtls=*/true)) {
    main_wnd_->MessageBox("Error", "CreatePeerConnection failed", true);
    DeletePeerConnection();
  }

  AddTracks();

  // Start the timer for auto close.
  if (alphacc_config_->conn_autoclose != kAutoCloseDisableValue) {
    main_wnd_->StartAutoCloseTimer(alphacc_config_->conn_autoclose * 1000);
  }
  
  return peer_connection_ != nullptr;
}

bool Conductor::ReinitializePeerConnectionForLoopback() {
  loopback_ = true;
  std::vector<rtc::scoped_refptr<webrtc::RtpSenderInterface>> senders =
      peer_connection_->GetSenders();
  peer_connection_ = nullptr;
  if (CreatePeerConnection(/*dtls=*/false)) {
    for (const auto& sender : senders) {
      peer_connection_->AddTrack(sender->track(), sender->stream_ids());
    }
    peer_connection_->CreateOffer(
        this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
  }
  return peer_connection_ != nullptr;
}

bool Conductor::CreatePeerConnection(bool dtls) {
  RTC_DCHECK(peer_connection_factory_);
  RTC_DCHECK(!peer_connection_);

  webrtc::PeerConnectionInterface::RTCConfiguration config;
  config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
  config.enable_dtls_srtp = dtls;
  webrtc::PeerConnectionInterface::IceServer server;
  server.uri = GetPeerConnectionString();
  config.servers.push_back(server);

  peer_connection_ = peer_connection_factory_->CreatePeerConnection(
      config, nullptr, nullptr, this);
  return peer_connection_ != nullptr;
}

void Conductor::DeletePeerConnection() {
  main_wnd_->StopLocalRenderer();
  main_wnd_->StopRemoteRenderer();
  peer_connection_ = nullptr;
  peer_connection_factory_ = nullptr;
  loopback_ = false;
}

//
// PeerConnectionObserver implementation.
//

void Conductor::OnAddTrack(
    rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
    const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&
        streams) {
  RTC_LOG(INFO) << __FUNCTION__ << " " << receiver->id();
  main_wnd_->QueueUIThreadCallback(NEW_TRACK_ADDED,
                                   receiver->track().release());
}

void Conductor::OnRemoveTrack(
    rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) {
  RTC_LOG(INFO) << __FUNCTION__ << " " << receiver->id();
  main_wnd_->QueueUIThreadCallback(TRACK_REMOVED, receiver->track().release());
}

void Conductor::OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
  RTC_LOG(INFO) << __FUNCTION__ << " " << candidate->sdp_mline_index();
  // For loopback test. To save some connecting delay.
  if (loopback_) {
    if (!peer_connection_->AddIceCandidate(candidate)) {
      RTC_LOG(WARNING) << "Failed to apply the received candidate";
    }
    return;
  }

  Json::StyledWriter writer;
  Json::Value jmessage;

  jmessage[kCandidateSdpMidName] = candidate->sdp_mid();
  jmessage[kCandidateSdpMlineIndexName] = candidate->sdp_mline_index();
  std::string sdp;
  if (!candidate->ToString(&sdp)) {
    RTC_LOG(LS_ERROR) << "Failed to serialize candidate";
    return;
  }
  jmessage[kCandidateSdpName] = sdp;

  const std::string msg = writer.write(jmessage);
  client_->SendClientMessage(msg);
}

//
// PeerConnectionClientObserver implementation.
//

// void Conductor::OnSignedIn() {
//   RTC_LOG(INFO) << __FUNCTION__;
//   main_wnd_->SwitchToPeerList(client_->peers());
// }

// void Conductor::OnPeerConnected(int id, const std::string& name) {
//   RTC_LOG(INFO) << __FUNCTION__;
//   // Refresh the list if we're showing it.
//   if (main_wnd_->current_ui() == MainWindow::LIST_PEERS)
//     main_wnd_->SwitchToPeerList(client_->peers());
// }

void Conductor::OnPeerDisconnected() {
  RTC_LOG(INFO) << __FUNCTION__;
  RTC_LOG(INFO) << "Our peer disconnected";
  main_wnd_->QueueUIThreadCallback(PEER_CONNECTION_CLOSED, NULL);
}

//
// MainWndCallback implementation.
//


void Conductor::ConnectToPeer() {
  if (peer_connection_.get()) {
    main_wnd_->MessageBox(
        "Error", "We only support connecting to one peer at a time", true);
    return;
  }
  if (InitializePeerConnection()) {
    peer_connection_->CreateOffer(
        this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
  } else {
    main_wnd_->MessageBox("Error", "Failed to initialize PeerConnection", true);
  }
}


void Conductor::AddTracks() {
  if (!peer_connection_->GetSenders().empty()) {
    return;  // Already added tracks.
  }

  rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track(
      peer_connection_factory_->CreateAudioTrack(
          kAudioLabel, peer_connection_factory_->CreateAudioSource(
                           cricket::AudioOptions())));
  auto result_or_error = peer_connection_->AddTrack(audio_track, {kStreamId});
  if (!result_or_error.ok()) {
    RTC_LOG(LS_ERROR) << "Failed to add audio track to PeerConnection: "
                      << result_or_error.error().message();
  }

  rtc::scoped_refptr<webrtc::VideoTrackSource> video_device;
  using VideoSourceOption = webrtc::AlphaCCConfig::VideoSourceOption;

  switch (alphacc_config_->video_source_option) {
    case VideoSourceOption::kVideoDisabled:
      video_device = webrtc::FakeVideoTrackSource::Create();
      break;
    case VideoSourceOption::kWebcam:
      video_device = CapturerTrackSource::Create();
      break;
    case VideoSourceOption::kVideoFile:
      video_device = FrameGeneratorTrackSource::Create(audio_started_);
      break;
    default:
      RTC_NOTREACHED();
  }

  if (video_device) {
    rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track_(
        peer_connection_factory_->CreateVideoTrack(kVideoLabel, video_device));
    main_wnd_->StartLocalRenderer(video_track_);

    result_or_error = peer_connection_->AddTrack(video_track_, {kStreamId});
    if (!result_or_error.ok()) {
      RTC_LOG(LS_ERROR) << "Failed to add video track to PeerConnection: "
                        << result_or_error.error().message();
    }
  } else {
    RTC_LOG(LS_ERROR) << "OpenVideoCaptureDevice failed";
  }

  main_wnd_->SwitchToStreamingUI();
}

// void Conductor::DisconnectFromCurrentPeer() {
//   RTC_LOG(INFO) << __FUNCTION__;
//   if (peer_connection_.get()) {
//     client_->SendHangUp(peer_id_);
//     DeletePeerConnection();
//   }

//   if (main_wnd_->IsWindow())
//     main_wnd_->SwitchToPeerList(client_->peers());
// }

void Conductor::UIThreadCallback(int msg_id, void* data) {
  switch (msg_id) {
    case PEER_CONNECTION_CLOSED: {
      RTC_LOG(INFO) << "PEER_CONNECTION_CLOSED";
      DeletePeerConnection();
      break;
    }

    case NEW_TRACK_ADDED: {
      auto* track = reinterpret_cast<webrtc::MediaStreamTrackInterface*>(data);
      if (track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
        auto* video_track = static_cast<webrtc::VideoTrackInterface*>(track);
        main_wnd_->StartRemoteRenderer(video_track);
      }
      track->Release();
      break;
    }

    case TRACK_REMOVED: {
      // Remote peer stopped sending a track.
      auto* track = reinterpret_cast<webrtc::MediaStreamTrackInterface*>(data);
      track->Release();
      break;
    }

    default: {
      RTC_NOTREACHED();
      break;
    }
  }
}

void Conductor::OnFrameCallback(const webrtc::VideoFrame& video_frame) {
  if (alphacc_config_->save_to_file) {
    frame_writer_->WriteFrame(video_frame);
  }
}

void Conductor::OnSuccess(webrtc::SessionDescriptionInterface* desc) {
  peer_connection_->SetLocalDescription(
      DummySetSessionDescriptionObserver::Create(), desc);

  std::string sdp;
  desc->ToString(&sdp);

  // For loopback test. To save some connecting delay.
  if (loopback_) {
    // Replace message type from "offer" to "answer"
    std::unique_ptr<webrtc::SessionDescriptionInterface> session_description =
        webrtc::CreateSessionDescription(webrtc::SdpType::kAnswer, sdp);
    peer_connection_->SetRemoteDescription(
        DummySetSessionDescriptionObserver::Create(),
        session_description.release());
    return;
  }

  Json::StyledWriter writer;
  Json::Value jmessage;
  jmessage[kSessionDescriptionTypeName] =
      webrtc::SdpTypeToString(desc->GetType());
  jmessage[kSessionDescriptionSdpName] = sdp;

  const std::string msg = writer.write(jmessage);
  client_->SendClientMessage(msg);
}

void Conductor::OnFailure(webrtc::RTCError error) {
  RTC_LOG(LERROR) << ToString(error.type()) << ": " << error.message();
}


//
// PeerConnectionClientObserver implementation.
//
void Conductor::ParseMessage(const std::string& message) {
  Json::Reader reader;
  Json::Value jmessage;
  if (!reader.parse(message, jmessage)) {
    RTC_LOG(WARNING) << "Received unknown message. " << message;
    return;
  }
  std::string type_str;
  std::string json_object;

  rtc::GetStringFromJsonObject(jmessage, kSessionDescriptionTypeName,
                               &type_str);
  if (!type_str.empty()) {
    if (type_str == "offer-loopback") {
      // This is a loopback call.
      // Recreate the peerconnection with DTLS disabled.
      if (!ReinitializePeerConnectionForLoopback()) {
        RTC_LOG(LS_ERROR) << "Failed to initialize our PeerConnection instance";
        DeletePeerConnection();
        client_->SignOut();
      }
      return;
    }
    absl::optional<webrtc::SdpType> type_maybe =
        webrtc::SdpTypeFromString(type_str);
    if (!type_maybe) {
      RTC_LOG(LS_ERROR) << "Unknown SDP type: " << type_str;
      return;
    }
    webrtc::SdpType type = *type_maybe;
    std::string sdp;
    if (!rtc::GetStringFromJsonObject(jmessage, kSessionDescriptionSdpName,
                                      &sdp)) {
      RTC_LOG(WARNING) << "Can't parse received session description message.";
      return;
    }
    webrtc::SdpParseError error;
    std::unique_ptr<webrtc::SessionDescriptionInterface> session_description =
        webrtc::CreateSessionDescription(type, sdp, &error);
    if (!session_description) {
      RTC_LOG(WARNING) << "Can't parse received session description message. "
                       << "SdpParseError was: " << error.description;
      return;
    }
    RTC_LOG(INFO) << " Received session description :" << message;
    peer_connection_->SetRemoteDescription(
        DummySetSessionDescriptionObserver::Create(),
        session_description.release());
    if (type == webrtc::SdpType::kOffer) {
      peer_connection_->CreateAnswer(
          this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
    }
  } else {
    std::string sdp_mid;
    int sdp_mlineindex = 0;
    std::string sdp;
    if (!rtc::GetStringFromJsonObject(jmessage, kCandidateSdpMidName,
                                      &sdp_mid) ||
        !rtc::GetIntFromJsonObject(jmessage, kCandidateSdpMlineIndexName,
                                   &sdp_mlineindex) ||
        !rtc::GetStringFromJsonObject(jmessage, kCandidateSdpName, &sdp)) {
      RTC_LOG(WARNING) << "Can't parse received message.";
      return;
    }
    webrtc::SdpParseError error;
    std::unique_ptr<webrtc::IceCandidateInterface> candidate(
        webrtc::CreateIceCandidate(sdp_mid, sdp_mlineindex, sdp, &error));
    if (!candidate.get()) {
      RTC_LOG(WARNING) << "Can't parse received candidate message. "
                       << "SdpParseError was: " << error.description;
      return;
    }
    if (!peer_connection_->AddIceCandidate(candidate.get())) {
      RTC_LOG(WARNING) << "Failed to apply the received candidate";
      return;
    }
    RTC_LOG(INFO) << " Received candidate :" << message;
  }
}

void Conductor::OnGetMessage(const std::string& new_message) {
  RTC_DCHECK(!new_message.empty());

  if (!peer_connection_.get()) {
    if (!InitializePeerConnection()) {
      RTC_LOG(LS_ERROR) << "Failed to initialize our PeerConnection instance";
      client_->SignOut();
      return;
    }
  }

  // append new message to accumulate_message_
  accumulate_message_ += new_message;
  // create newbuffer to store the value of accumulate_message_
  std::unique_ptr<char[]> uniq_char(new char[0xffff]);
  char* pbuffer = uniq_char.get();
  strcpy(pbuffer, accumulate_message_.data());
  // check if terminal symbol of message exists in msg and locate it
  char* locate = strstr(pbuffer, messageTerminate);
  if (locate != NULL) {
    // empty the accumulate_message_ to prepare for the ParseMessage
    accumulate_message_ = "";
    // split the message using terminal symbol
    while (locate != NULL) {
      // send the splited message to conductor
      accumulate_message_.append(pbuffer, locate - pbuffer);
      ParseMessage(accumulate_message_);
      // finish to parse one messge, empty the accumulate_message_
      accumulate_message_ = "";
      // move pbuffer point to jump over terminal symbol
      pbuffer = locate + strlen(messageTerminate);
      // check if terminal symbol of message exists
      locate = strstr(pbuffer, messageTerminate);
    }
    // if the end of message leaves some without terminal, append to
    // accumulate_message_
    if (strlen(pbuffer) != 0) {
      accumulate_message_.append(pbuffer);
    }
  }
  // if no terminal in the accumulate_message_,just return and it will deal with
  // in the next call
}
