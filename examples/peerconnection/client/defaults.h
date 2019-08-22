/*
 *  Copyright 2011 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef EXAMPLES_PEERCONNECTION_CLIENT_DEFAULTS_H_
#define EXAMPLES_PEERCONNECTION_CLIENT_DEFAULTS_H_

#include <stdint.h>

#include <string>

extern const char kAudioLabel[];
extern const char kVideoLabel[];
extern const char kStreamId[];
extern const uint16_t kDefaultServerPort;

// The value of auto close time for disabling auto close 
extern const int kAutoCloseDisableValue;
// The default ip of Redis Service
extern const std::string kDefaultRedisIP;
// The default port of Redis Service
extern const int kDefaultRedisPort;
// The default session id of Redis service
extern const std::string kDefaultRedisSID;
// The default time of collecting states in milliseconds
extern const int kDefaultRedisUpdate;
// The default rate update time in milliseconds
extern const int kDefaultRateUpdate;

std::string GetEnvVarOrDefault(const char* env_var_name,
                               const char* default_value);
std::string GetPeerConnectionString();
std::string GetDefaultServerName();
std::string GetPeerName();

#endif  // EXAMPLES_PEERCONNECTION_CLIENT_DEFAULTS_H_
