#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import json
import time

RequestBandwidthCommand = "RequestBandwidth"

def fetch_stats(line: str)->dict:
    line = line.strip()
    try:
        stats = json.loads(line)
        return stats
    except json.decoder.JSONDecodeError:
        return None

def is_bwe_requested(line: str)->bool:
    line = line.strip()
    if RequestBandwidthCommand == line:
        return True
    return False

def find_estimator_class():
    import rl_training.BandwidthEstimator as BandwidthEstimator
    return BandwidthEstimator.Estimator


def main(ifd = sys.stdin, ofd = sys.stdout):
    # Instantiate RL agent to train:
    estimator_class = find_estimator_class()
    estimator = estimator_class()
    while True:
        # Read a line from app.stdout, which is packet statistics
        line = ifd.readline()
        if not line:
            print(f'peerconnection_serverless.origin produced nothing!')
            break
        if isinstance(line, bytes):
            line = line.decode("utf-8")

        # Receive per-packet stats
        stats = fetch_stats(line)
        if stats:
            print(f'peerconnection_serverless.origin produced {stats}')
            # deliver this to the RL agent
            estimator.report_states(stats)
            continue

        # Send latest action
        bwe_request = is_bwe_requested(line)
        if bwe_request:
            bandwidth = estimator.get_estimated_bandwidth()
            # bandwidth = 1000
            print(f'BWE requested: latest action {bandwidth}')
            ofd.write("{}\n".format(int(bandwidth)).encode("utf-8"))
            ofd.flush()
            continue

        sys.stdout.write(line)
        sys.stdout.flush()


if __name__ == '__main__':
    main()
