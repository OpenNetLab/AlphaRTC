/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// clang-format off
// clang formating would change include order.
// #include <windows.h>
// #include <shellapi.h>  // must come after windows.h
// clang-format on

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

namespace {
// A helper class to translate Windows command line arguments into UTF8,
// which then allows us to just pass them to the flags system.
// This encapsulates all the work of getting the command line and translating
// it to an array of 8-bit strings; all you have to do is create one of these,
// and then call argc() and argv().
class WindowsCommandLineArguments {
 public:
  WindowsCommandLineArguments();

  int argc() { return argv_.size(); }
  char** argv() { return argv_.data(); }

 private:
  // Owned argument strings.
  std::vector<std::string> args_;
  // Pointers, to get layout compatible with char** argv.
  std::vector<char*> argv_;

 private:
  RTC_DISALLOW_COPY_AND_ASSIGN(WindowsCommandLineArguments);
};

}
// WindowsCommandLineArguments::WindowsCommandLineArguments() {
//   // start by getting the command line.
//   LPCWSTR command_line = ::GetCommandLineW();
//   // now, convert it to a list of wide char strings.
//   int argc;
//   LPWSTR* wide_argv = ::CommandLineToArgvW(command_line, &argc);

//   // iterate over the returned wide strings;
//   for (int i = 0; i < argc; ++i) {
//     args_.push_back(rtc::ToUtf8(wide_argv[i], wcslen(wide_argv[i])));
//     // make sure the argv array points to the string data.
//     argv_.push_back(const_cast<char*>(args_.back().c_str()));
//   }
//   LocalFree(wide_argv);
// }

// }  // namespace
// int PASCAL wWinMain(HINSTANCE instance,
//                     HINSTANCE prev_instance,
//                     wchar_t* cmd_line,
//                     int cmd_show) {
//   rtc::WinsockInitializer winsock_init;
//   rtc::Win32SocketServer w32_ss;
//   rtc::Win32Thread w32_thread(&w32_ss);
//   rtc::ThreadManager::Instance()->SetCurrentThread(&w32_thread);

//   WindowsCommandLineArguments win_args;
//   int argc = win_args.argc();
//   char** argv = win_args.argv();

//   absl::ParseCommandLine(argc, argv);

//   // InitFieldTrialsFromString stores the char*, so the char array must outlive
//   // the application.
//   const std::string forced_field_trials =
//       absl::GetFlag(FLAGS_force_fieldtrials);
//   webrtc::field_trial::InitFieldTrialsFromString(forced_field_trials.c_str());

//   // Abort if the user specifies a port that is outside the allowed
//   // range [1, 65535].
//   if ((absl::GetFlag(FLAGS_port) < 1) || (absl::GetFlag(FLAGS_port) > 65535)) {
//     printf("Error: %i is not a valid port.\n", absl::GetFlag(FLAGS_port));
//     return -1;
//   }

//   const std::string server = absl::GetFlag(FLAGS_server);
//   MainWnd wnd(server.c_str(), absl::GetFlag(FLAGS_port),
//               absl::GetFlag(FLAGS_autoconnect), absl::GetFlag(FLAGS_autocall));
//   if (!wnd.Create()) {
//     RTC_NOTREACHED();
//     return -1;
//   }

//   rtc::InitializeSSL();
//   PeerConnectionClient client;
//   rtc::scoped_refptr<Conductor> conductor(
//       new rtc::RefCountedObject<Conductor>(&client, &wnd));

//   // Main loop.
//   MSG msg;
//   BOOL gm;
//   while ((gm = ::GetMessage(&msg, NULL, 0, 0)) != 0 && gm != -1) {
//     if (!wnd.PreTranslateMessage(&msg)) {
//       ::TranslateMessage(&msg);
//       ::DispatchMessage(&msg);
//     }
//   }

//   if (conductor->connection_active() || client.is_connected()) {
//     while ((conductor->connection_active() || client.is_connected()) &&
//            (gm = ::GetMessage(&msg, NULL, 0, 0)) != 0 && gm != -1) {
//       if (!wnd.PreTranslateMessage(&msg)) {
//         ::TranslateMessage(&msg);
//         ::DispatchMessage(&msg);
//       }
//     }
//   }

//   rtc::CleanupSSL();
//   return 0;
// }


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
