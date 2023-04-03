#include <fstream>
#include "rl_agent.h"

using namespace std;
using namespace nlohmann;

void rl_agent::SendState(float loss_rate, int64_t rtt, int64_t delay_interval, float avg_receiver_side_thp) {
    json packet_stats_json;
    packet_stats_json["loss_rate"] = loss_rate;
    packet_stats_json["rtt"] = rtt;
    packet_stats_json["delay_interval"] = delay_interval;
    packet_stats_json["receiver_side_thp"] = avg_receiver_side_thp;

    // cout << j.dump() << endl;

    // ofstream ofs("packet_stats.json");
    // ofs << packet_stats_json;
    // ofs.close();

    ofstream ofs;
    ofs.open("packet_stats.json", fstream::out | fstream::app);
    ofs << packet_stats_json;
    ofs.close();
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
            cout << "failed to convert " << line << " to int32" << endl;
        } else {
            // cout << "rl_agent::GetBwe: received latest bwe " << bwe << endl;
        }
        bwe_file.close();
    } else {
        cout << "rl_agent::GetBwe: Cannot open bwe.txt - use 300Kbps as an initial bwe" << endl;
    }

    // Send BWE to the WebRTC receiver
    return bwe;
}
