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
        # print(f'{ret} sampled from the discrete action space')
        return ret


"""
Custom Environment that follows OpenAI gym interface.
Must inherit from OpenAI Gym Class
and implement the following methods: step(), reset(), render(), close()
"""
class GymEnv(gym.Env):
    metadata = {'render.modes': ['human']}

    def __init__(self, rl_algo, link_bw, action_space_type='continuous'):
        super(GymEnv, self).__init__()
        self.metadata = None
        self.action_space_type = action_space_type
        self.episode_len = 600

        # Define an action space (must be gym.spaces objects)
        if self.action_space_type == 'discrete':
            '''
            Using 10Kbps ~ 10Mbps as the range of action space
            10Kbps = 10,000 bps
            10Mbps = 10,000,000 bps
            (10Mbps - 10Kbps) / 10 = 111e4
            '''
            space = np.array([1e4, 112e4, 223e4, 334e4, 445e4, 556e4, 667e4, 778e4, 889e4, 1000e4])
            self.action_space = DiscreteSpace(10, space)

        # Using 10Kbps ~ 10Mbps as the range of action space.
        # Best practice: action space normalized to [-1, 1], i.e. symmetric and has an interval range of 2,
        # which is usually the same magnitude as the initial stdev of the Gaussian used to define the policy
        # (e.g. unit initial stdev in SB3)
        # Most RL algorithms (except DDPG or TD3) rely on a Gaussian distribution
        # (initially centered at 0 with std 1) for continuous actions.
        # So, if you forget to normalize the action space when using a custom environment,
        # this can harm learning and be difficult to debug (cf attached image and issue #473).
        else: # continuous action space
            self.action_space = spaces.Box(low=-1, high=1, shape=(1,), dtype=np.float32)

        # Define an observation space (must be gym.spaces objects)
        # Example for using image as input:
        # self.observation_space = spaces.Box(low=0, high=255,
        #                                 shape=(HEIGHT, WIDTH, N_CHANNELS), dtype=np.uint8)
        self.observation_space = spaces.Box(
            low=np.array([0.0, 0.0, 0.0]),
            high=np.array([1.0, 1.0, 1.0]),
            dtype=np.float64)

        # Custom
        self.rl_algo = rl_algo
        self.link_bw = link_bw
        self.packet_record = PacketRecord()
        self.num_steps = 0
        self.episode_reward  = []

    '''
    Received packet statistics are the result of running the latest action.
    From this, calculate state and reward.
    '''
    def calculate_state_reward(self):
        # Calculate state for the latest `self.packet_record.history_len`
        norm_recv_thp, norm_rtt, loss_rate = self.packet_record.calculate_state()

        # Calculate reward.
        # Incentivize increase in throughput, penalize increase in RTT and loss rate.
        # TODO: Tune normalizing coefficients
        # - receiver-side thp: 1Kbps~200Kbps (empirically) -> 0-1
        # - RTT: 1-100ms -> 0-1
        # - loss rate: 0-1
        state = [norm_recv_thp, norm_rtt, loss_rate]
        reward = 30 * norm_recv_thp - 5 * norm_rtt - 50 * loss_rate

        return state, reward, {}, {}

    def get_latest_bwe(self):
        bwe_l = self.packet_record.get_bwe()
        # print(f'bwe_l {bwe_l}')
        # default target bitrate in GCC: 300000 bps = 300 kbps
        # return 1e6
        return bwe_l[-1] if len(bwe_l) else 300000

    '''
    Run one timestep of the environment's dynamics.
    When end of episode is reached, you are responsible for calling `reset()`
    to reset this environment's state.

    Args:
        action (object): an action provided by the agent

    Returns:
        observation (object): agent's observation of the current environment
        reward (float) : amount of reward returned after previous action
        done (bool): whether the episode has ended, in which case further step() calls will return undefined results
        info (dict): contains auxiliary diagnostic information (helpful for debugging, and sometimes learning)
    '''
    def step(self, action):
        if type(action) is list or isinstance(action, np.ndarray):
            latest_bwe = action[0]
        elif type(action) is None:
            300000 # default 300Kbps
        else:
            latest_bwe = action

        self.packet_record.add_bwe(latest_bwe)

        # Packet-level statistics for the latest `self.packet_record.history_len`
        norm_recv_thp, loss_rate, norm_rtt, norm_recv_thp_fluct = self.packet_record.calculate_statistics()

        # State.
        # TODO: add delay interval
        obs = [norm_recv_thp, norm_rtt, loss_rate]

        # Reward.
        # Incentivize increase in receiver-side thp,
        # penalize increase in loss, RTT, and thp fluctuation.
        # TODO: Check whether to use normalized values or actual values
        reward = 50 * norm_recv_thp - 50 * loss_rate - 10 * norm_rtt - 30 * norm_recv_thp_fluct

        with open(f'state_reward_action_{self.rl_algo}_{self.link_bw}', mode='a+') as f:
            f.write(f'{self.rl_algo} step {self.num_steps} state {obs} reward {reward} action {latest_bwe}\n')

        self.num_steps += 1
        if self.num_steps > self.episode_len:
            done = True
        else:
            done = False

        return obs, reward, done, None

    '''
    Resets the environment to an initial state and returns an initial observation.

    Note that this function should not reset the environment's random number generator(s);
    random variables in the environment's state should be sampled independently
    between multiple calls to `reset()`.
    In other words, each call of `reset()` should yield an environment suitable for
    a new episode, independent of previous episodes.

    Returns:
        observation (object): the initial observation.
    '''
    def reset(self):
        # Reset internal states of the environment
        self.packet_record = PacketRecord()
        self.num_steps = 0
        self.episode_reward = []

        # Produce initial observation
        action = 300000 # default BWE = 300Kbps
        obs, _, _, _ = self.step(action)
        print(f'env reset done: an environment for a new episode is set')
        return obs

    '''
    Renders the environment.
    Nothing to do.
    '''
    def render(self, mode='human'):
        pass

    '''
    Perform any necessary cleanup.
    Environments will automatically close() themselves
    when garbage collected or the program exits.
    '''
    def close (self):
        pass
