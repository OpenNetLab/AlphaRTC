#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import json
import glob


RequestBandwidthCommand = "RequestBandwidth"


def fetch_stats(line: str)->dict:
    line = line.strip()
    try:
        stats = json.loads(line)
        return stats
    except json.decoder.JSONDecodeError:
        return None


def request_estimated_bandwidth(line: str)->bool:
    line = line.strip()
    if RequestBandwidthCommand == line:
        return True
    return False


def find_estimator_class():
    import BandwidthEstimator
    return BandwidthEstimator.Estimator


def main(ifd = sys.stdin, ofd = sys.stdout):
    estimator_class = find_estimator_class()
    estimator = estimator_class()
    while True:
        line = ifd.readline()
        if not line:
            break
        if isinstance(line, bytes):
            line = line.decode("utf-8")
        stats = fetch_stats(line)
        if stats:
            estimator.report_states(stats)
            continue
        request = request_estimated_bandwidth(line)
        if request:
            bandwidth = estimator.get_estimated_bandwidth()
            ofd.write("{}\n".format(int(bandwidth)).encode("utf-8"))
            ofd.flush()
            continue
        sys.stdout.write(line)
        sys.stdout.flush()


if __name__ == '__main__':
    main()
