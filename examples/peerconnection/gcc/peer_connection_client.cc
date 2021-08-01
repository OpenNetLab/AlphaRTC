/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "examples/peerconnection/gcc/peer_connection_client.h"

#include "examples/peerconnection/gcc/defaults.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/net_helpers.h"

#ifdef WIN32
#include "rtc_base/win32_socket_server.h"
#endif

namespace {

// This is our magical hangup signal.
const char kByeMessage[] = "BYE";
// Delay between server connection retries, in milliseconds

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

PeerConnectionClient::PeerConnectionClient() : callback_(NULL) {
  cs_ = rtc::CriticalSection();
}

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

void PeerConnectionClient::SendAClientMessage(const std::string& message) {
  // enter the critical section
  rtc::CritScope cs(&cs_);
  // add terminal symbol in sending message
  std::string complete_message = message + messageTerminate;
  size_t sent = 0;
  do {
    sent = message_socket_->Send(complete_message.c_str(),
                                 complete_message.length());
  } while((sent != complete_message.length()));
}

void PeerConnectionClient::SendClientMessage(const std::string& message) {
  SendAClientMessage(message);
}

void PeerConnectionClient::SignOut() {
  if (message_socket_ != nullptr)
    SendClientMessage(kByeMessage);
}

// void PeerConnectionClient::InitSocketSignals() {
//   RTC_DCHECK(control_socket_.get() != NULL);
//   RTC_DCHECK(hanging_get_.get() != NULL);
//   control_socket_->SignalCloseEvent.connect(this,
//                                             &PeerConnectionClient::OnClose);
//   hanging_get_->SignalCloseEvent.connect(this, &PeerConnectionClient::OnClose);
//   control_socket_->SignalConnectEvent.connect(this,
//                                               &PeerConnectionClient::OnConnect);
//   hanging_get_->SignalConnectEvent.connect(
//       this, &PeerConnectionClient::OnHangingGetConnect);
//   control_socket_->SignalReadEvent.connect(this, &PeerConnectionClient::OnRead);
//   hanging_get_->SignalReadEvent.connect(
//       this, &PeerConnectionClient::OnHangingGetRead);
// }


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
