#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import json
import time
import rl_training.BandwidthEstimator as BandwidthEstimator
from rl_training.rtc_env import GymEnv
from stable_baselines3 import PPO

def fetch_stats(line: str)->dict:
    line = line.strip()
    try:
        stats = json.loads(line)
        return stats
    except json.decoder.JSONDecodeError:
        return None

# def find_estimator_class():
#     import rl_training.BandwidthEstimator as BandwidthEstimator
#     return BandwidthEstimator.Estimator

# Rather an environment which provides real network statistics from end-to-end call
def main(ifd = sys.stdin, ofd = sys.stdout):
    start_ts = time.time()
    end_ts = 0
    report_states_cnt = 0
    # Instantiate RL agent to train:
    estimator = BandwidthEstimator.Estimator()

    while True:
        # Read a line from app.stdout, which is packet statistics
        line = ifd.readline()
        if not line:
            break
        if isinstance(line, bytes):
            line = line.decode("utf-8")

        per_packet_stats = fetch_stats(line)
        if per_packet_stats:
            # Send per-packet stats to the RL agent and receive latest BWE
            # bwe = estimator.relay_packet_statistics(per_packet_stats)
            bwe = 1e6
            report_states_cnt += 1
            if report_states_cnt % 500 == 0:
                end_ts = time.time()
                step_time = end_ts - start_ts
                print(f'start_ts {start_ts} end_ts {end_ts} report_states_cnt {report_states_cnt}\taggregate step time (s, 500 steps) : {step_time}')
                start_ts = end_ts
            ofd.write("{}\n".format(int(bwe)).encode("utf-8"))
            ofd.flush()
            continue

        sys.stdout.write(line)
        sys.stdout.flush()


if __name__ == '__main__':
    main()
