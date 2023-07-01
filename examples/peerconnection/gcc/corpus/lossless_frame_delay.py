#!/usr/bin/env python3

import sys
import argparse
import numpy as np


def main(args):
    generated_frames = []
    with open(args.s, 'r') as send_log:
        for line in send_log:
            if 'FRAME GENERATION' in line:
                generated_frames.append(int(line.split()[-1]))

    rendered_frames = []
    with open(args.r, 'r') as recv_log:
        for line in recv_log:
            if 'FRAME RENDERING' in line:
                rendered_frames.append(int(line.split()[-1]))

    print(f'Generated {len(generated_frames)} frames')
    print(f'Rendered {len(rendered_frames)} frames')

    delays = []
    i = 0  # index of generated_frames
    for j in range(len(rendered_frames)):
        if i > len(generated_frames):
            sys.exit('ERROR: rendered more frames than generated')

        delay = rendered_frames[j] - generated_frames[i]
        delays.append(delay)
        i += 1

    print('Median per-frame delay (ms):', np.median(delays))
    print('P95 per-frame delay (ms):', np.percentile(delays, 95))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-s', metavar='SENDER_LOG',
                        required=True, help='sender log')
    parser.add_argument('-r', metavar='RECEIVER_LOG',
                        required=True, help='receiver log')
    args = parser.parse_args()

    main(args)
