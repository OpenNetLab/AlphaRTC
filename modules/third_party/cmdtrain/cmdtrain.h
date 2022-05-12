#ifndef MODULES_THIRD_PARTY_CMDTRAIN_H_
#define MODULES_THIRD_PARTY_CMDTRAIN_H_

#include <cinttypes>
#include <cstddef>

struct TimeStampAndBwe {
    TimeStampAndBwe() : ts(0), bwe(0) {}
    int ts;
    int bwe;
};

namespace cmdtrain {
    void ReportStates(
        std::uint64_t sendTimeMs,
        std::uint64_t receiveTimeMs,
        std::size_t payloadSize,
        std::uint8_t payloadType,
        std::uint16_t sequenceNumber,
        std::uint32_t ssrc,
        std::size_t paddingLength,
        std::size_t headerLength);

    float GetEstimatedBandwidth();
}

#endif
