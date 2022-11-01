#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import json
import time
from statistics import mean

def fetch_stats(line: str)->dict:
    line = line.strip()
    try:
        stats = json.loads(line)
        return stats
    except json.decoder.JSONDecodeError:
        return None

def find_estimator_class():
    import rl_training.BandwidthEstimator as BandwidthEstimator
    return BandwidthEstimator.Estimator


def main(ifd = sys.stdin, ofd = sys.stdout):
    step_times = []
    start_ts = time.time()
    end_ts = 0
    report_states_cnt = 0
    # Instantiate RL agent to train:
    estimator_class = find_estimator_class()
    estimator = estimator_class()
    while True:
        # Read a line from app.stdout, which is packet statistics
        line = ifd.readline()
        if not line:
            break
        if isinstance(line, bytes):
            line = line.decode("utf-8")

        stats = fetch_stats(line)
        if stats:
            # Send per-packet stats to the RL agent and receive latest BWE
            bwe = estimator.report_states(stats)
            report_states_cnt += 1
            if report_states_cnt % 600 == 0:
                end_ts = time.time()
                step_time = end_ts - start_ts
                print(f'start_ts {start_ts} end_ts {end_ts} report_states_cnt {report_states_cnt}\taggregate step time (s, 600 steps) : {step_time}')
                start_ts = end_ts

            ofd.write("{}\n".format(int(bwe)).encode("utf-8"))
            ofd.flush()
            continue

        sys.stdout.write(line)
        sys.stdout.flush()


if __name__ == '__main__':
    main()
