#include "cmdtrain.h"

void cmdtrain::SendRTT(int64_t rtt) {
    json j;
    j["rtt"] = rtt;

    cout << j.dump() << endl;
}

void cmdtrain::SendLossRate(float loss_rate) {
    json j;
    j["loss_rate"] = loss_rate;

    cout << j.dump() << endl;
}

void cmdtrain::SendReceiverSideThp(float receiver_side_thp) {
    json j;
    j["receiver_side_thp"] = receiver_side_thp;

    cout << j.dump() << endl;
}

int32_t cmdtrain::GetBwe(void) {
    std::ifstream ifs("bwe.json");
    json bwe = json::parse(ifs);
    ifs.close();

    // Send BWE to the WebRTC receiver
    return bwe["bwe"];
}
