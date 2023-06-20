#ifndef MODULES_THIRD_PARTY_CMDINFER_H_
#define MODULES_THIRD_PARTY_CMDINFER_H_

#include <cinttypes>
#include <cstddef>

namespace cmdinfer {
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
