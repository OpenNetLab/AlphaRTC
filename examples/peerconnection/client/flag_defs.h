/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef EXAMPLES_PEERCONNECTION_CLIENT_FLAG_DEFS_H_
#define EXAMPLES_PEERCONNECTION_CLIENT_FLAG_DEFS_H_

#include "rtc_base/flags.h"

extern const uint16_t kDefaultServerPort;  // From defaults.[h|cc]
extern const int kAutoCloseDisableValue;   // From defaults.[h|cc], The value of auto close time for disabling auto close 
extern const std::string kDefaultRedisIP;  // From defaults.[h|cc], The default ip of Redis Service
extern const int kDefaultRedisPort;        // From defaults.[h|cc], The default port of Redis Service
extern const std::string kDefaultRedisSID; // From defaults.[h|cc], The default session id of Redis service
extern const int kDefaultRedisUpdate;      // From defaults.[h|cc], The default time of collecting states in milliseconds
extern const int kDefaultRateUpdate;       // From defaults.[h|cc], The default rate update time in milliseconds

// Define flags for the peerconnect_client testing tool, in a separate
// header file so that they can be shared across the different main.cc's
// for each platform.

WEBRTC_DEFINE_bool(help, false, "Prints this message");
WEBRTC_DEFINE_bool(autoconnect,
                   false,
                   "Connect to the server without user "
                   "intervention.");
WEBRTC_DEFINE_string(server, "localhost", "The server to connect to.");
WEBRTC_DEFINE_int(port,
                  kDefaultServerPort,
                  "The port on which the server is listening.");
WEBRTC_DEFINE_bool(
    autocall,
    false,
    "Call the first available other client on "
    "the server without user intervention.  Note: this flag should only be set "
    "to true on one of the two clients.");

WEBRTC_DEFINE_string(
    force_fieldtrials,
    "",
    "Field trials control experimental features. This flag specifies the field "
    "trials in effect. E.g. running with "
    "--force_fieldtrials=WebRTC-FooFeature/Enabled/ "
    "will assign the group Enabled to field trial WebRTC-FooFeature. Multiple "
    "trials are separated by \"/\"");

WEBRTC_DEFINE_bool(
    webcam_disabled,
    false,
    "Disable the webcam.");

WEBRTC_DEFINE_int(
	autoclose, 
	kAutoCloseDisableValue,
	"The time in seconds before close automatically (always run if autoclose=0).");

WEBRTC_DEFINE_string(
	video_path,
	"",
	"The path of the input video file");

WEBRTC_DEFINE_string(
	redis_ip,
	kDefaultRedisIP.c_str(),
	"The ip of Redis Service");

WEBRTC_DEFINE_int(
	redis_port,
	kDefaultRedisPort,
	"The port of Redis Service");

WEBRTC_DEFINE_string(
	redis_sid,
    kDefaultRedisSID.c_str(),
	"The session id of Redis Service");

WEBRTC_DEFINE_int(
	redis_update,
    kDefaultRedisUpdate,
    "The time period of collecting states in milliseconds​​​");

WEBRTC_DEFINE_string(
	onnx_model_path,
    "",
    "The path of the onnx model​");

WEBRTC_DEFINE_int(
	rate_update,
    kDefaultRateUpdate,
    "The rate update time in milliseconds​​​");

#endif  // EXAMPLES_PEERCONNECTION_CLIENT_FLAG_DEFS_H_
