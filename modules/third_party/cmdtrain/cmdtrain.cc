#include "cmdtrain.h"

using namespace std;
using namespace nlohmann;

void cmdtrain::SendState(float avg_receiver_side_thp, int64_t rtt, float loss_rate, bool* estimated_bitrate_bps_updated) {
    json j;
    j["receiver_side_thp"] = avg_receiver_side_thp;
    j["rtt"] = rtt;
    j["loss_rate"] = loss_rate;

    // std::string name = "state.txt";
    // std::ifstream f(name.c_str());
    // std::string line;

    // std::ofstream StateFile("state.txt");
    // StateFile << j.dump() << endl;
    // StateFile.close();

    cout << j.dump() << endl;

    *estimated_bitrate_bps_updated = true;
}

int32_t cmdtrain::GetBwe(bool* estimated_bitrate_bps_updated) {
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
            std::cout << "cmdtrain::GetBwe: received latest bwe " << bwe << std::endl;
        }
        bwe_file.close();
        *estimated_bitrate_bps_updated = false;
    } else {
        std::cout << "cmdtrain::GetBwe: Cannot open bwe.txt - use 300Kbps as an initial bwe" << std::endl;
    }

    // Send BWE to the WebRTC receiver
    return bwe;
}
