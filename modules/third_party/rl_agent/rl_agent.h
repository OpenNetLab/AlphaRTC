#ifndef MODULES_THIRD_PARTY_RLAGENT_H_
#define MODULES_THIRD_PARTY_RLAGENT_H_

#include <cinttypes>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include "modules/third_party/statcollect/json.hpp"

namespace rl_agent {
    void SendState(float avg_receiver_side_thp, int64_t rtt, float loss_rate, bool* estimated_bitrate_bps_updated);
    int32_t GetBwe(bool* estimated_bitrate_bps_updated);
}

#endif
