#include "rl_agent.h"

using namespace std;
using namespace nlohmann;

void rl_agent::SendState(float loss_rate, int64_t rtt, int64_t delay_interval, float avg_receiver_side_thp) {
    json j;
    j["loss_rate"] = loss_rate;
    j["rtt"] = rtt;
    j["delay_interval"] = delay_interval;
    j["receiver_side_thp"] = avg_receiver_side_thp;

    cout << j.dump() << endl;
}

int32_t rl_agent::GetBwe() {
    int32_t bwe = 300000;
    std::string name = "bwe.txt";
    std::ifstream f(name.c_str());
    std::string line;
    // File exists
    if (f.good()) {
        std::ifstream bwe_file(name);
        std::getline(bwe_file, line);
        std::stringstream sss(line);
        if( !( sss >> bwe ) ) {
            std::cout << "failed to convert " << line << " to int32" << std::endl;
        } else {
            // std::cout << "rl_agent::GetBwe: received latest bwe " << bwe << std::endl;
        }
        bwe_file.close();
    } else {
        std::cout << "rl_agent::GetBwe: Cannot open bwe.txt - use 300Kbps as an initial bwe" << std::endl;
    }

    // Send BWE to the WebRTC receiver
    return bwe;
}
