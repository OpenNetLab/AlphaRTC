/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef API_ALPHACC_CONFIG_H_
#define API_ALPHACC_CONFIG_H_

#include <string>

namespace webrtc {

struct AlphaCCConfig {
  AlphaCCConfig() = default;
  ~AlphaCCConfig() = default;

  // The server to connect
  std::string conn_server_ip;
  int conn_server_port = 0;
  // Connect to the server without user intervention.
  bool conn_autoconnect = false;
  // Call the first available other client on
  // the server without user intervention. Note: this flag should be set
  // to true on ONLY one of the two clients.
  bool conn_autocall = false;
  // The time in seconds before close automatically (always run
  // if autoclose=0)"
  int conn_autoclose = 0;

  bool is_sender = false;
  bool is_receiver = false;

  // The address to connect to
  std::string dest_ip;
  int dest_port = 0;
  std::string listening_ip;
  int listening_port = 0;

  int bwe_feedback_duration_ms = 0;
  std::string onnx_model_path;

  enum class VideoSourceOption {
    kVideoDisabled,
    kWebcam,
    kVideoFile,
  } video_source_option;
  int video_height = 0;
  int video_width = 0;
  int video_fps = 0;
  std::string video_file_path;

  enum class AudioSourceOption { kMicrophone, kAudioFile } audio_source_option;
  std::string audio_file_path;

  bool save_to_file = false;
  std::string video_output_path;
  std::string audio_output_path;
  int video_output_height = 0;
  int video_output_width = 0;
  int video_output_fps = 0;

  bool save_log_to_file;
  std::string log_output_path;
};

// Get alphaCC global configurations
const AlphaCCConfig* GetAlphaCCConfig();

// Parse configurations files from |file_path|
bool ParseAlphaCCConfig(const std::string& file_path);

}  // namespace webrtc

#endif  // API_ALPHACC_CONFIG_H_
