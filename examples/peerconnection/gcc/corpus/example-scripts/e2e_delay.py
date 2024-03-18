#!/usr/bin/env python3

import sys
import argparse
import numpy as np


def main(args):
    delays = []
    with open(args.r, 'r') as receiver_log:
        for line in receiver_log:
            if 'E2E FRAME DELAY' in line:
                delays.append(int(line.split()[-1]))

    print('Delay measurement count:', len(delays))
    print('Median per-frame delay (ms):', np.median(delays))
    print('P95 per-frame delay (ms):', np.percentile(delays, 95))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-r', metavar='RECEIVER_LOG',
                        required=True, help='receiver log')
    args = parser.parse_args()

    main(args)
