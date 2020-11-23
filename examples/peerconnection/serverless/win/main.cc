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
#include <windows.h>
#include <shellapi.h>  // must come after windows.h
// clang-format on

#include <cstdlib>
#include <string>
#include <vector>

#include "examples/peerconnection/serverless/conductor.h"
#include "examples/peerconnection/serverless/main_wnd.h"
#include "examples/peerconnection/serverless/peer_connection_client.h"
#include "rtc_base/checks.h"
#include "rtc_base/constructor_magic.h"
#include "rtc_base/ssl_adapter.h"
#include "rtc_base/string_utils.h"  // For ToUtf8
#include "rtc_base/win32_socket_init.h"
#include "rtc_base/win32_socket_server.h"
#include "system_wrappers/include/field_trial.h"
#include "test/field_trial.h"

int PASCAL wWinMain(HINSTANCE instance,
                    HINSTANCE prev_instance,
                    wchar_t* cmd_line,
                    int cmd_show) {
  rtc::WinsockInitializer winsock_init;
  rtc::Win32SocketServer w32_ss;
  rtc::Win32Thread w32_thread(&w32_ss);
  rtc::ThreadManager::Instance()->SetCurrentThread(&w32_thread);

  // InitFieldTrialsFromString stores the char*, so the char array must outlive
  // the application.
  webrtc::field_trial::InitFieldTrialsFromString(
      "WebRTC-KeepAbsSendTimeExtension/Enabled/");  //  Config for
                                                    //  hasAbsSendTimestamp in
                                                    //  RTP Header extension

  // Read the json-format configuration file.
  // File path is passed through |cmd_line|
  char cmd_line_s[1024];
  wcstombs(cmd_line_s, cmd_line, 1024);
  if (!webrtc::ParseAlphaCCConfig(cmd_line_s)) {
    RTC_NOTREACHED();
    return -1;
  };
  MainWnd wnd;
  if (!wnd.Create()) {
    RTC_NOTREACHED();
    return -1;
  }
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
 
  // Main loop.
  MSG msg;
  BOOL gm;
  while ((gm = ::GetMessage(&msg, NULL, 0, 0)) != 0 && gm != -1) {
    if (!wnd.PreTranslateMessage(&msg)) {
      ::TranslateMessage(&msg);
      ::DispatchMessage(&msg);
    }
  }

  rtc::CleanupSSL();
  return 0;
}
