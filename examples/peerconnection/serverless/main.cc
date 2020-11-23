/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <memory>

#include "examples/peerconnection/serverless/conductor.h"
#include "examples/peerconnection/serverless/peer_connection_client.h"
#include "rtc_base/ssl_adapter.h"
#include "rtc_base/string_utils.h"  // For ToUtf8
#include "system_wrappers/include/field_trial.h"

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

class Timer {
 private:
  std::vector<std::shared_ptr<std::thread>> timers_;

 public:
  template <typename Function>
  void AddTimer(int ms, Function f) {
    std::shared_ptr<std::thread> t = std::make_shared<std::thread>([=] {
      std::this_thread::sleep_for(std::chrono::milliseconds(ms));
      f();
    });
    timers_.push_back(t);
  }

  ~Timer() {
    for (auto thread : timers_) {
      thread.get()->join();
    }
  }
};

class MainWindowMock : public MainWindow {
 private:
  std::unique_ptr<VideoRenderer> remote_renderer_;
  MainWndCallback* callback_;
  std::shared_ptr<rtc::AutoSocketServerThread> socket_thread_;
  Timer t;

 public:
  MainWindowMock(std::shared_ptr<rtc::AutoSocketServerThread> socket_thread)
      : callback_(NULL), socket_thread_(socket_thread) {}
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

  void Close() {
    RTC_LOG(INFO) << "Cleaning up";
    callback_->Close();
    socket_thread_.get()->Stop();
  }

  void StartAutoCloseTimer(int interval_ms) override {
    t.AddTimer(interval_ms, std::bind(&MainWindowMock::Close, this));
  }
};

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s config_file\n", argv[0]);
    exit(EINVAL);
  }

  webrtc::field_trial::InitFieldTrialsFromString(
      "WebRTC-KeepAbsSendTimeExtension/Enabled/");  //  Config for
                                                    //  hasAbsSendTimestamp in
                                                    //  RTP Header extension

  const auto json_file_path = argv[1];
  if (!webrtc::ParseAlphaCCConfig(json_file_path)) {
    perror("bad config file");
    exit(EINVAL);
  }

  rtc::PhysicalSocketServer socket_server;

  std::shared_ptr<rtc::AutoSocketServerThread> thread(
      new rtc::AutoSocketServerThread(&socket_server));

  MainWindowMock wnd(thread);

  rtc::InitializeSSL();
  PeerConnectionClient client;
  rtc::scoped_refptr<Conductor> conductor(
      new rtc::RefCountedObject<Conductor>(&client, &wnd));

  auto config = webrtc::GetAlphaCCConfig();
  if (config->is_receiver) {
    client.StartListen(config->listening_ip, config->listening_port);
  }
  if (config->is_sender) {
    client.StartConnect(config->dest_ip, config->dest_port);
  }

  thread.get()->Run();

  rtc::CleanupSSL();
  return 0;
}