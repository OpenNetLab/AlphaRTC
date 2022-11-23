#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
import random
import numpy as np

from gym import spaces

sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), "gym"))
from rl_training.packet_record import PacketRecord

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
        # Using normalized state, action, reward spaces
        if self.action_space_type == 'discrete':
            '''
            Using 10Kbps ~ 10Mbps as the range of action space
            10Kbps = 10,000 bps
            10Mbps = 10,000,000 bps
            (10Mbps - 10Kbps) / 10 = 111e4
            '''
            space = np.array([1e4, 112e4, 223e4, 334e4, 445e4, 556e4, 667e4, 778e4, 889e4, 1000e4])
            # discrete_action_space = [1e4, 112e4, 223e4, 334e4, 445e4, 556e4, 667e4, 778e4, 889e4, 1000e4]
            # normalized_discrete_action_space = list(map(lambda x: self.packet_record.normalize_bps(x), discrete_action_space))
            # print(f'normalized_discrete_action_space {normalized_discrete_action_space}')
            # space = np.array(normalized_discrete_action_space)
            self.action_space = DiscreteSpace(10, space)
        else: # continuous action space
            '''
            Using 10Kbps ~ 10Mbps as the range of action space
            '''
            self.action_space = spaces.Box(low=1e4, high=1e7, shape=(1,), dtype=np.float64)
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
        # Calculate state for the latest `self.packet_record.history_len`
        normalized_receiver_side_thp, normalized_rtt, loss_rate = self.packet_record.calculate_state()

        # Calculate reward.
        # Incentivize increase in throughput, penalize increase in RTT and loss rate.
        # TODO: Tune normalizing coefficients
        # - receiver-side thp: 10Kbps~100Kbps (empirically) -> 0-1
        # - RTT: 1-100ms -> 0-1
        # - loss rate: 0-1
        state = [normalized_receiver_side_thp, normalized_rtt, loss_rate]
        reward = normalized_receiver_side_thp - normalized_rtt - loss_rate
        print(f'State: receiver-side thp\t{normalized_receiver_side_thp}, rtt\t{normalized_rtt}, loss rate\t{loss_rate}')
        print(f'Reward: {reward}')

        return state, reward, {}, {}

    def get_latest_bwe(self):
        bwe_l = self.packet_record.get_bwe()
        # print(f'bwe_l {bwe_l}')
        # default target bitrate in GCC: 300000 bps = 300 kbps
        # return 1e6
        return bwe_l[-1] if len(bwe_l) else 300000

    # Returns `action` to the cmdtrain
    # and calculate new_obs, rewards for latest history_len stats
    def step(self, action):
        # this latest_bwe is sent to cmdtrain
        # by BandwidthEstimator.relay_packet_statistics()
        if type(action) is list or isinstance(action, np.ndarray):
            latest_bwe = action[0]
        elif type(action) is None:
            300000 # default 300Kbps
        else:
            latest_bwe = action

        self.packet_record.add_bwe(latest_bwe)
        new_obs, rewards, dones, infos = self.calculate_state_reward()
        print(f'Step {self.num_steps} policy produced action {latest_bwe} from the new obs {new_obs}')
        self.num_steps += 1

        return new_obs, rewards, dones, infos


