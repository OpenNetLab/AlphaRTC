#include "cmdtrain.h"
#include "modules/third_party/statcollect/json.hpp"

#include <iostream>
#include <fstream>
#include <unistd.h>

float cmdtrain::ReportStatsAndGetBWE(
    std::uint64_t sendTimeMs,
    std::uint64_t receiveTimeMs,
    std::size_t payloadSize,
    std::uint8_t payloadType,
    std::uint16_t sequenceNumber,
    std::uint32_t ssrc,
    std::size_t paddingLength,
    std::size_t headerLength) {

    nlohmann::json j;
    j["send_time_ms"] = sendTimeMs;
    j["arrival_time_ms"] = receiveTimeMs;
    j["payload_type"] = payloadType;
    j["sequence_number"] = sequenceNumber;
    j["ssrc"] = ssrc;
    j["padding_length"] = paddingLength;
    j["header_length"] = headerLength;
    j["payload_size"] = payloadSize;

    // Receive packet statistics from WebRTC receiver
    std::cout << j.dump() << std::endl;

    // Send BWE to the WebRTC receiver
    std::uint64_t bandwidth = 0;
    std::cin >> bandwidth;
    return static_cast<float>(bandwidth);
}
