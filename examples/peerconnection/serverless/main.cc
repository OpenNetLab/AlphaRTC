/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "conductor.h"
#include "defaults.h"
#include "logger.h"
#include "peer_connection_client.h"

#ifdef WIN32
#include "rtc_base/win32_socket_init.h"
#include "rtc_base/win32_socket_server.h"
#endif

#include "api/alphacc_config.h"
#include "rtc_base/ssl_adapter.h"
#include "rtc_base/string_utils.h"  // For ToUtf8
#include "system_wrappers/include/field_trial.h"

#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <thread>

#include <errno.h>
#include <stdio.h>
#include <string.h>

class VideoRenderer : public rtc::VideoSinkInterface<webrtc::VideoFrame> {
 public:
  VideoRenderer(webrtc::VideoTrackInterface* track_to_render,
                MainWndCallback* callback)
      : track_(track_to_render), callback_(callback) {
    track_->AddOrUpdateSink(this, rtc::VideoSinkWants());
  }
  ~VideoRenderer() { track_->RemoveSink(this); }
  void OnFrame(const webrtc::VideoFrame& frame) {
    callback_->OnFrameCallback(frame);
  }

 private:
  rtc::scoped_refptr<webrtc::VideoTrackInterface> track_;
  MainWndCallback* callback_;
};

class MainWindowMock : public MainWindow {
 private:
  std::unique_ptr<VideoRenderer> remote_renderer_;
  MainWndCallback* callback_;
  std::shared_ptr<rtc::AutoSocketServerThread> socket_thread_;
  const webrtc::AlphaCCConfig* config_;
  int close_time_;

 public:
  MainWindowMock(std::shared_ptr<rtc::AutoSocketServerThread> socket_thread)
      : callback_(NULL),
        socket_thread_(socket_thread),
        config_(webrtc::GetAlphaCCConfig()),
        close_time_(rtc::Thread::kForever) {}
  void RegisterObserver(MainWndCallback* callback) override {
    callback_ = callback;
  }

  bool IsWindow() override { return true; }

  void MessageBox(const char* caption,
                  const char* text,
                  bool is_error) override {
    RTC_LOG(LS_INFO) << caption << ": " << text;
  }

  UI current_ui() override { return WAIT_FOR_CONNECTION; }

  void SwitchToConnectUI() override {}
  void SwitchToStreamingUI() override {}

  void StartLocalRenderer(webrtc::VideoTrackInterface* local_video) override {}

  void StopLocalRenderer() override {}

  void StartRemoteRenderer(webrtc::VideoTrackInterface* remote_video) override {
    remote_renderer_.reset(new VideoRenderer(remote_video, callback_));
  }

  void StopRemoteRenderer() override { remote_renderer_.reset(); }

  void QueueUIThreadCallback(int msg_id, void* data) override {
    callback_->UIThreadCallback(msg_id, data);
  }

  void Run() {
    if (config_->conn_autoclose != kAutoCloseDisableValue) {
      while (close_time_ == rtc::Thread::kForever) {
        RTC_CHECK(socket_thread_->ProcessMessages(0));
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
      RTC_CHECK(socket_thread_->ProcessMessages(close_time_));
    } else {
      socket_thread_->Run();
    }
    StopRemoteRenderer();
    socket_thread_->Stop();
    callback_->Close();
  }

  void StartAutoCloseTimer(int close_time) override {
    close_time_ = close_time;
  }
};

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s config_file\n", argv[0]);
    exit(EINVAL);
  }

  const auto json_file_path = argv[1];
  if (!webrtc::ParseAlphaCCConfig(json_file_path)) {
    std::cerr << "bad config file" << std::endl;
    exit(EINVAL);
  }

  rtc::LogMessage::LogToDebug(rtc::LS_INFO);

  auto config = webrtc::GetAlphaCCConfig();
  std::unique_ptr<FileLogSink> sink;

  if (config->save_log_to_file) {
    sink = std::make_unique<FileLogSink>(config->log_output_path);
  }

  webrtc::field_trial::InitFieldTrialsFromString(
      "WebRTC-KeepAbsSendTimeExtension/Enabled/");  //  Config for
                                                    //  hasAbsSendTimestamp in
                                                    //  RTP Header extension

#ifdef WIN32
  rtc::WinsockInitializer win_sock_init;
  rtc::Win32SocketServer socket_server;
#else
  rtc::PhysicalSocketServer socket_server;
#endif

  std::shared_ptr<rtc::AutoSocketServerThread> thread(
      new rtc::AutoSocketServerThread(&socket_server));

  MainWindowMock wnd(thread);

  rtc::InitializeSSL();
  PeerConnectionClient client;
  rtc::scoped_refptr<Conductor> conductor(
      new rtc::RefCountedObject<Conductor>(&client, &wnd));

  if (config->is_receiver) {
    client.StartListen(config->listening_ip, config->listening_port);
  } else if (config->is_sender) {
    client.StartConnect(config->dest_ip, config->dest_port);
  }

  wnd.Run();

  rtc::CleanupSSL();
  return 0;
}