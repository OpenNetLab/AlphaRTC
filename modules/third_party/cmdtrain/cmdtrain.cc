#include "cmdtrain.h"
#include "modules/third_party/statcollect/json.hpp"

#include <iostream>
#include <fstream>
#include <unistd.h>

void cmdtrain::SendRTT(int64_t rtt) {
    nlohmann::json j;
    j["rtt"] = rtt;

    std::cout << j.dump() << std::endl;
}

void cmdtrain::SendLossRate(float loss_rate) {
    nlohmann::json j;
    j["loss_rate"] = loss_rate;

    std::cout << j.dump() << std::endl;
}

void cmdtrain::SendReceiverSideThp(float receiver_side_thp) {
    nlohmann::json j;
    j["receiver_side_thp"] = receiver_side_thp;

    std::cout << j.dump() << std::endl;
}
