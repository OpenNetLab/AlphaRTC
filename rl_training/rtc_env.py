#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
import random
import numpy as np

from gym import spaces

sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), "gym"))
from rl_training.packet_record import PacketRecord

UNIT_M = 1000000
MAX_BANDWIDTH_MBPS = 8
MIN_BANDWIDTH_MBPS = 0.01
LOG_MAX_BANDWIDTH_MBPS = np.log(MAX_BANDWIDTH_MBPS)
LOG_MIN_BANDWIDTH_MBPS = np.log(MIN_BANDWIDTH_MBPS)


def liner_to_log(value):
    # from 10kbps~8Mbps to 0~1
    value = np.clip(value / UNIT_M, MIN_BANDWIDTH_MBPS, MAX_BANDWIDTH_MBPS)
    log_value = np.log(value)
    return (log_value - LOG_MIN_BANDWIDTH_MBPS) / (LOG_MAX_BANDWIDTH_MBPS - LOG_MIN_BANDWIDTH_MBPS)

def log_to_linear(value):
    # from 0~1 to 10kbps to 8Mbps
    value = np.clip(value, 0, 1)
    log_bwe = value * (LOG_MAX_BANDWIDTH_MBPS - LOG_MIN_BANDWIDTH_MBPS) + LOG_MIN_BANDWIDTH_MBPS
    return np.exp(log_bwe) * UNIT_M

class DiscreteSpace(spaces.discrete.Discrete):
    def __init__(self, n, space):
        assert n >= 0
        self.n = n
        self.space = space
        super(DiscreteSpace, self).__init__(n)

    def sample(self):
        ret = random.choice(self.space)
        print(f'{ret} sampled from the discrete action space')
        return ret


class GymEnv:
    def __init__(self, action_space_type='continuous'):
        self.num_steps = 0
        self.packet_record = PacketRecord()
        self.metadata = None
        self.action_space_type = action_space_type
        # TODO: Make the range of actions consistent: 10Kbps to 10Mbps
        # for both discrete/continuous action spaces
        if self.action_space_type == 'discrete':
            '''
            Adopting discrete action space in MobiCom'21 Loki:
            "Following this common practice, the action at of Loki, is chosen from
            a bitrate space with 10 discrete actions empirically set as follows:
            A : {0.7Mbps, 0.83Mbps, . . . , 1.87Mbps, 2.0Mbps}.
            The two extreme boundary values, e.g., 0.7 Mbps and 2.0 Mbps,
            are consistent with those in our commercial real-time video system.
            Besides, other 8 levels are roughly equally separated in between."
            '''
            space = np.array([0.7, 0.83, 0.97, 1.11, 1.25, 1.39, 1.53, 1.67, 1.87, 2.0])
            self.action_space = DiscreteSpace(10, space)
        else: # continuous action space
            self.action_space = spaces.Box(low=0.0, high=1.0, shape=(1,), dtype=np.float64)
        self.observation_space = spaces.Box(
            low=np.array([0.0, 0.0, 0.0]),
            high=np.array([1.0, 1.0, 1.0]),
            dtype=np.float64)
        # To calculate average reward per step in each episode
        self.accum_reward  = 0
        self.avg_reward_per_step_list = []

    def close(self):
        return

    def reset(self):
        print(f'env reset')
        # self.packet_record = PacketRecord()
        # self.packet_record.reset()
        obs, _, _, _ = self.calculate_state_reward()
        return obs

    '''
    Received packet statistics are the result of running the latest bwe.
    From this, calculate state and reward.
    '''
    def calculate_state_reward(self):
        # Calculate state for the latest `self.packet_record.history_len` number of RTCP packets
        states = self.packet_record.calculate_state()

        # Calculate reward.
        # Incentivize increase in throughput, penalize increase in RTT and loss rate.
        # TODO: Add normalizing coefficients
        reward = states[0] - states[1]/1000 - states[2]

        print(f'State: receiver-side thp\t{states[0]}, rtt\t{states[1]}, loss rate\t{states[2]}')
        print(f'Reward: {reward}')

        return states, reward, {}, {}

    def get_latest_bwe(self):
        bwe_l = self.packet_record.get_bwe()
        # default target bitrate in GCC: 300000 bps = 300 kbps
        return bwe_l[-1] if len(bwe_l) else 300000

    # Returns `action` to the cmdtrain
    # and calculate new_obs, rewards for latest history_len stats
    def step(self, action):
        # this latest_bwe is sent to cmdtrain
        # by BandwidthEstimator.relay_packet_statistics()
        latest_bwe = log_to_linear(action)[0]
        self.packet_record.add_bwe(latest_bwe)
        new_obs, rewards, dones, infos = self.calculate_state_reward()
        print(f'Step {self.num_steps} sending action {latest_bwe} to the cmdtrain, new obs {new_obs}')
        self.num_steps += 1
        return new_obs, rewards, dones, infos


