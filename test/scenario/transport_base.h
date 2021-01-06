#pragma once
#include "api/call/transport.h"
#include "call/call.h"
#include "system_wrappers/include/clock.h"

namespace webrtc {
namespace test {

class TransportBase : public Transport {
 public:
  TransportBase() {}

  virtual ~TransportBase() {}

  virtual void Construct(Clock* sender_clock, Call* sender_call) = 0;

  virtual bool SendRtp(const uint8_t* packet,
                       size_t length,
                       const PacketOptions& options) {
    return true;
  }

  virtual bool SendRtcp(const uint8_t* packet, size_t length) { return true; }
};
}  // namespace test
}  // namespace webrtc
