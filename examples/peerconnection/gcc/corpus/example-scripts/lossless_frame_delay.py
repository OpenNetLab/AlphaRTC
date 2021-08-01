#!/usr/bin/env python3

import sys
import argparse
import numpy as np


def main(args):
    read_frames = []
    with open(args.s, 'r') as sender_log:
        for line in sender_log:
            if 'FRAME READ' in line:
                read_frames.append(int(line.split()[-1]))

    write_frames = []
    with open(args.r, 'r') as receiver_log:
        for line in receiver_log:
            if 'FRAME WRITE' in line:
                write_frames.append(int(line.split()[-1]))

    print(f'Read {len(read_frames)} frames')
    print(f'Wrote {len(write_frames)} frames')

    delays = []
    for i in range(len(write_frames)):
        if i >= len(read_frames):
            sys.exit('ERROR: wrote more frames than read')

        delay = write_frames[i] - read_frames[i]
        if delay < 0:
            sys.exit('ERROR: delay cannot be negative')

        delays.append(delay)

    print('Median per-frame delay (ms): {:.2f}'.format(np.median(delays) / 1000))
    print('P95 per-frame delay (ms): {:.2f}'.format(np.percentile(delays, 95) / 1000))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-s', metavar='SENDER_LOG',
                        required=True, help='sender log')
    parser.add_argument('-r', metavar='RECEIVER_LOG',
                        required=True, help='receiver log')
    args = parser.parse_args()

    main(args)
