/*
 *  Copyright 2011 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef EXAMPLES_PEERCONNECTION_CLIENT_PEER_CONNECTION_CLIENT_H_
#define EXAMPLES_PEERCONNECTION_CLIENT_PEER_CONNECTION_CLIENT_H_

#include <map>
#include <memory>
#include <string>

#include "rtc_base/net_helpers.h"
#include "rtc_base/physical_socket_server.h"
#include "rtc_base/signal_thread.h"
#include "rtc_base/third_party/sigslot/sigslot.h"

typedef std::map<int, std::string> Peers;
const char messageTerminate[] = "[EOF]";

struct PeerConnectionClientObserver {
  virtual void OnGetMessage(const std::string& message) = 0;
  virtual void OnPeerDisconnected() = 0;
  virtual void ConnectToPeer() = 0;

 protected:
  virtual ~PeerConnectionClientObserver() {}
};

class PeerConnectionClient : public sigslot::has_slots<> {
 public:
  PeerConnectionClient();
  ~PeerConnectionClient();

  void RegisterObserver(PeerConnectionClientObserver* callback);
  void StartListen(const std::string& ip, int port);
  void StartConnect(const std::string& ip, int port);
  void SendClientMessage(const std::string& message);
  void SendAClientMessage(const std::string& message);
  void SignOut();

 protected:
  void OnSenderConnect(rtc::AsyncSocket* socket);
  void OnGetMessage(rtc::AsyncSocket* socket);

  // Returns true if the whole response has been read.
  void ReadIntoBuffer(rtc::AsyncSocket* socket,
                      std::string* data);

  PeerConnectionClientObserver* callback_;
  std::unique_ptr<rtc::AsyncSocket> listen_socket_;
  std::unique_ptr<rtc::AsyncSocket> message_socket_;
  rtc::CriticalSection cs_;
};

#endif  // EXAMPLES_PEERCONNECTION_CLIENT_SERVERLESS_PEER_CONNECTION_CLIENT_H_
