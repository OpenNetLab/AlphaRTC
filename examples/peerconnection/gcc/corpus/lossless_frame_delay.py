#!/usr/bin/env python3

import sys
import argparse
import numpy as np


def main(args):
    transmitted_frames = []
    sender_dropped_frames = 0
    with open(args.s, 'r') as sender_log:
        for line in sender_log:
            if 'FRAME GENERATED' in line:
                transmitted_frames.append(int(line.split()[-1]))
            elif 'FRAME DROPPED' in line:
                transmitted_frames.pop()
                sender_dropped_frames += 1

    rendered_frames = []
    with open(args.r, 'r') as receiver_log:
        for line in receiver_log:
            if 'FRAME RENDERED' in line:
                rendered_frames.append(int(line.split()[-1]))

    print(f'Transmitted {len(transmitted_frames)} frames (and dropped {sender_dropped_frames} frames)')
    print(f'Rendered {len(rendered_frames)} frames')

    delays = []
    for i in range(len(rendered_frames)):
        if i >= len(transmitted_frames):
            sys.exit('ERROR: rendered more frames than transmitted')

        delay = rendered_frames[i] - transmitted_frames[i]
        if delay < 0:
            sys.exit('ERROR: delay cannot be negative')

        delays.append(delay)

    print('Median per-frame delay (us):', np.median(delays))
    print('P95 per-frame delay (us):', np.percentile(delays, 95))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-s', metavar='SENDER_LOG',
                        required=True, help='sender log')
    parser.add_argument('-r', metavar='RECEIVER_LOG',
                        required=True, help='receiver log')
    args = parser.parse_args()

    main(args)
