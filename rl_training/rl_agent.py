#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import json
import matplotlib.pyplot as plt
import numpy as np
import os
import sys

import logging
logging.basicConfig(filename='step_obs_reward_action.log', encoding='utf-8', level=logging.INFO)

RL_ALGO='SAC'
LINK_BW='1mbps'
EPISODE_LEN=600


def fetch_stats_orig(line: str) -> dict:
    line = line.strip()
    logging.info(f'fetch_stats: {line}')
    try:
        stats = json.loads(line)
        return stats
    except json.decoder.JSONDecodeError:
        return None

def fetch_stats(line:str):
    line = line.strip()
    logging.info(f'fetch_stats {line}')
    if 'AlphaCC: SendState' in line:
        lrdr = line.split(',')
        loss_rate = float(lrdr[1].split(' ')[1])
        rtt = float(lrdr[2].split(' ')[1])
        delay_interval = float(lrdr[3].split(' ')[1])
        recv_thp = float(lrdr[4].split(' ')[1])
        return [loss_rate, rtt, delay_interval, recv_thp]
    else:
        return None

def plot_training_curve(num_episodes, rewards):
    training_curve_dir = './rl_model/training_curves/'
    if not os.path.exists(training_curve_dir):
        os.makedirs(training_curve_dir)

    plt.ylim(0, max(rewards))
    plt.plot(rewards, c = '#bbbcb8') # gray
    plt.title(f'RL-based CC ({RL_ALGO})')
    plt.xlabel('Step')
    plt.ylabel('Average Training Reward')
    plt.savefig(f'{training_curve_dir}/training-curve-{RL_ALGO}-{LINK_BW}-{num_episodes}episodes.pdf')


def main(ifd = sys.stdin, ofd = sys.stdout):
    while True:
        # Read a line from app.stdout, which is packet statistics
        line = ifd.readline()
        if not line:
            break
        if isinstance(line, bytes):
            line = line.decode("utf-8")

        logging.info(f'line {line}')
        stats = fetch_stats(line)
        if stats:
            logging.info(f'fetch_stats returned {stats}')

            # Send per-packet stats to the RL agent and receive latest BWE
            env.packet_record.add_loss_rate(stats[0])
            env.packet_record.add_rtt(stats[1])
            env.packet_record.add_delay_interval()
            env.packet_record.add_receiver_side_thp(stats[3])

            bwe = env.get_latest_bwe()
            # truncate-then-write the bwe
            with open('bwe.txt', mode='w') as f:
                f.write(f'{bwe}')
            continue

        sys.stdout.write(line)
        sys.stdout.flush()


if __name__ == '__main__':
    main()
