#ifndef MODULES_THIRD_PARTY_CMDTRAIN_H_
#define MODULES_THIRD_PARTY_CMDTRAIN_H_

#include <cinttypes>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include "modules/third_party/statcollect/json.hpp"

using namespace std;
using namespace nlohmann;

namespace cmdtrain {
    void SendRTT(int64_t rtt);
    void SendLossRate(float loss_rate);
    void SendReceiverSideThp(float receiver_side_thp);
    int32_t GetBwe(void);
}

#endif
