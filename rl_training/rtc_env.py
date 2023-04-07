#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
import random
import numpy as np
import logging
logging.basicConfig(filename='step_obs_reward_action.log', encoding='utf-8', level=logging.INFO)

from gym import Env
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
class RTCEnv(Env):
    metadata = {'render.modes': ['human']}

    def __init__(self, rl_algo, action_space_type='continuous'):
        super(RTCEnv, self).__init__()
        self.metadata = None
        self.action_space_type = action_space_type
        self.info = {'episode': 600} # Dict[str, Any]

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
            low=np.array([0.0, 0.0, 0.0, 0.0]),
            high=np.array([1.0, 1.0, 1.0, 1.0]),
            dtype=np.float64)

        # Custom
        self.rl_algo = rl_algo
        self.packet_record = PacketRecord()
        self.num_steps = 0
        self.episode_reward  = []

    def get_latest_bwe(self):
        bwe_l = self.packet_record.get_bwe()
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
            action_val = action[0]
        elif type(action) is None:
            action_val = -0.40140140140140135 # default BWE = 300Kbps
        else:
            action_val = action

        # norm_action = self.packet_record.normalize_action(action_val)
        self.packet_record.add_bwe(self.packet_record.rescale_action(action_val))

        # Observation.
        # TODO: correct delay interval
        obs = self.packet_record.calculate_obs()

        # Reward.
        reward, recv_thp, loss_rate, rtt, recv_thp_fluct = self.packet_record.calculate_reward()

        logging.info(f'''\n[{self.rl_algo}] Step {self.num_steps}:
        Obs {obs} ([loss_rate, norm_rtt, norm_delay_interval, norm_recv_thp])
        Reward {reward} (50 * {recv_thp} - 50 * {loss_rate} - 10 * {rtt} - 30 * {recv_thp_fluct})
        Action (1Kbps-1Mbps) {self.get_latest_bwe()} action (-1~1) {action_val}''')

        self.num_steps += 1
        # TODO: Check whether it's okay to decide 'done' here
        # and not inside SB3 RL algorithms, e.g. OffPolicyAlgorithm
        # if self.num_steps > self.info['episode']:
        #     done = True
        # else:
        #     done = False

        return obs, reward, None, self.info

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
        self.packet_record.reset()
        self.num_steps = 0
        self.episode_reward = []

        # Produce initial observation
        action = -0.40140140140140135 # default BWE = 300Kbps
        obs, _, _, _ = self.step(action)
        logging.info(f'env reset done: an environment for a new episode is set')
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
