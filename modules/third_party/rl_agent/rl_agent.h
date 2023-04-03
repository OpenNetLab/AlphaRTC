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
    void SendState(float loss_rate, int64_t rtt, int64_t delay_interval, float avg_receiver_side_thp);
    int32_t GetBwe();
}

#endif
