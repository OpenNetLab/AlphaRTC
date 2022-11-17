#ifndef MODULES_THIRD_PARTY_CMDTRAIN_H_
#define MODULES_THIRD_PARTY_CMDTRAIN_H_

#include <cinttypes>
#include <cstddef>

namespace cmdtrain {
    void SendRTT(int64_t rtt);
    void SendLossRate(float loss_rate);
    void SendReceiverSideThp(float receiver_side_thp);
}

#endif
