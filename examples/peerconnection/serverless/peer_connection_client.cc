/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "examples/peerconnection/serverless/peer_connection_client.h"

#include "examples/peerconnection/serverless/defaults.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/net_helpers.h"

#ifdef WIN32
#include "rtc_base/win32_socket_server.h"
#endif

namespace {

// This is our magical hangup signal.
const char kByeMessage[] = "BYE";

rtc::AsyncSocket* CreateClientSocket(int family) {
#ifdef WIN32
  rtc::Win32Socket* sock = new rtc::Win32Socket();
  sock->CreateT(family, SOCK_STREAM);
  return sock;
#elif defined(WEBRTC_POSIX)
  rtc::Thread* thread = rtc::Thread::Current();
  RTC_DCHECK(thread != NULL);
  return thread->socketserver()->CreateAsyncSocket(family, SOCK_STREAM);
#else
#error Platform not supported.
#endif
}

}  // namespace

PeerConnectionClient::PeerConnectionClient() : callback_(NULL) {}

PeerConnectionClient::~PeerConnectionClient() {}

void PeerConnectionClient::RegisterObserver(
    PeerConnectionClientObserver* callback) {
  RTC_DCHECK(!callback_);
  callback_ = callback;
}

void PeerConnectionClient::StartListen(const std::string& ip, int port) {
  rtc::SocketAddress listening_addr(ip, port);
  listen_socket_.reset(CreateClientSocket(listening_addr.ipaddr().family()));

  int err = listen_socket_->Bind(listening_addr);
  if (err == SOCKET_ERROR) {
    listen_socket_->Close();
    RTC_LOG(LS_ERROR) << "Failed to bind listen socket to port " << port;
    RTC_NOTREACHED();
  }
  listen_socket_->Listen(1);
  listen_socket_->SignalReadEvent.connect(
      this, &PeerConnectionClient::OnSenderConnect);
}

void PeerConnectionClient::StartConnect(const std::string& ip, int port) {
  rtc::SocketAddress send_to_addr(ip, port);
  message_socket_.reset(CreateClientSocket(send_to_addr.ipaddr().family()));
  message_socket_->SignalReadEvent.connect(this,
                                           &PeerConnectionClient::OnGetMessage);
  int err = message_socket_->Connect(send_to_addr);
  if (err == SOCKET_ERROR) {
    message_socket_->Close();
    RTC_LOG(LS_ERROR) << "Failed to connect to receiver";
    RTC_NOTREACHED();
  } else {
    callback_->ConnectToPeer();
  }
}

void PeerConnectionClient::SendClientMessage(const std::string& message) {
  size_t sent = message_socket_->Send(message.c_str(), message.length());
  RTC_DCHECK(sent == message.length());
}

void PeerConnectionClient::SignOut() {
  SendClientMessage(kByeMessage);
}

void PeerConnectionClient::OnSenderConnect(rtc::AsyncSocket* socket) {
  message_socket_.reset(socket->Accept(nullptr));
  message_socket_->SignalReadEvent.connect(this,
                                           &PeerConnectionClient::OnGetMessage);
}

void PeerConnectionClient::OnGetMessage(rtc::AsyncSocket* socket) {
  std::string msg;
  ReadIntoBuffer(socket, &msg);

  if (msg.length() == (sizeof(kByeMessage) - 1) &&
      msg.compare(kByeMessage) == 0) {
    callback_->OnPeerDisconnected();
  } else {
    callback_->OnGetMessage(msg);
  }
}

void PeerConnectionClient::ReadIntoBuffer(rtc::AsyncSocket* socket,
                                          std::string* data) {
  char buffer[0xffff];
  do {
    int bytes = socket->Recv(buffer, sizeof(buffer), nullptr);
    if (bytes <= 0)
      break;
    data->append(buffer, bytes);
  } while (true);
}