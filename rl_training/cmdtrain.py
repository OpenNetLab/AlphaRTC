#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import json

def fetch_stats(line: str)->dict:
    line = line.strip()
    try:
        stats = json.loads(line)
        return stats
    except json.decoder.JSONDecodeError:
        return None

def find_estimator_class():
    #import rl_training.BandwidthEstimator as BandwidthEstimator
    import Concerto.BandwidthEstimator as BandwidthEstimator
    return BandwidthEstimator.Estimator


def main(ifd = sys.stdin, ofd = sys.stdout):
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
            ofd.write("{}\n".format(int(bwe)).encode("utf-8"))
            ofd.flush()
            continue

        sys.stdout.write(line)
        sys.stdout.flush()


if __name__ == '__main__':
    main()
