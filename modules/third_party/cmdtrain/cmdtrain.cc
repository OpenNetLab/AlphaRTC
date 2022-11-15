#include "cmdtrain.h"
#include "modules/third_party/statcollect/json.hpp"

#include <iostream>
#include <fstream>
#include <unistd.h>

float cmdtrain::ComputeReceiverSideThroughput(std::size_t payloadSize) {

    nlohmann::json j;
    j["payload_size"] = payloadSize;

    // Received payload info from the receiver
    std::cout << j.dump() << std::endl;

    // Send receiver-side throughput to the sender
    std::uint64_t receiver_side_thp = 0;
    std::cin >> receiver_side_thp;
    return static_cast<float>(receiver_side_thp);
}
