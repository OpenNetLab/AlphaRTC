import json
import sys
import random
import numpy as np
import logging
logging.basicConfig(
    filename='example.log', encoding='utf-8',
    format='%(asctime)s,%(msecs)d %(levelname)-8s [%(pathname)s:%(lineno)d in ' \
           'function %(funcName)s] %(message)s',
    datefmt='%Y-%m-%d:%H:%M:%S',
    level=logging.DEBUG
)
import subprocess

from gym import Env
from gym import spaces

from rl_training.packet_record import PacketRecord
from rl_training.call import Call

def fetch_stats(line):
    line = line.strip()
    if 'AlphaCC: SendState' in line:
        lrdr = line.split(',')
        stats_dict = {}
        stats_dict['loss_rate'] = float(lrdr[1].split(' ')[1])
        stats_dict['rtt'] = float(lrdr[2].split(' ')[1])
        stats_dict['delay_interval'] = float(lrdr[3].split(' ')[1])
        stats_dict['recv_thp'] = float(lrdr[4].split(' ')[1])
        return stats_dict
    else:
        return None

"""
Custom Environment that follows OpenAI gym interface.
Must inherit from OpenAI Gym Class
and implement the following methods: step(), reset(), render(), close()
RTCEnv is a single environment that plays multiple video calls in parallel.
"""
class RTCEnv(Env):
    metadata = {'render.modes': ['human']}

    def __init__(self, rl_algo, action_space_type='continuous'):
        super(RTCEnv, self).__init__()
        self.metadata = None
        self.action_space_type = action_space_type
        self.history_len = 10
        # end of episode signal
        self.dones = np.zeros(1) # single env (with multiple calls)
        self.infos = [{'episode': 640}] # List[Dict[str, Any]]
        self.logger = logging.getLogger(__name__)
        self.policy = None
        # number of steps in one rollout collection loop.
        # HP of an RL algorithm: 2048 for PPO, 5 for A2C
        self.total_rollout_steps = 0
        # number of env.step() performed in a single rollout
        # reset to 0 when a single rollout is finished
        self.num_rollout_steps = 0
        # k-th rollout loops we're in
        self.kth_rollout_loop = 0
        # the total number of samples (env steps) to train on
        self.total_timesteps = 0
        self.num_timesteps = 0
        assert rl_algo == 'PPO' or rl_algo == 'A2C' \
            or rl_algo == 'DQN' or rl_algo == 'SAC' or rl_algo == 'TD3'
        self.rl_algo = rl_algo
        self.on_or_off_policy = ''
        if self.rl_algo == 'PPO' or self.rl_algo == 'A2C':
            self.on_or_off_policy = 'On-Policy'
        else:
            self.on_or_off_policy = 'Off-Policy'
        self.training_completed = False

        # Define an action space (must be gym.spaces objects)
        if self.action_space_type == 'discrete':
            self.action_space = spaces.Discrete(10)

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

        self.call = None

        # Per-call data structures
        self.packet_records = {}
        self.num_stats = {}

    def set_calls(self, link_bandwidths, delays):
        self.call = Call(link_bandwidths, delays)
        num_calls = self.call.num_calls
        for call_idx in range(0, num_calls):
            self.packet_records[call_idx] = PacketRecord()
            self.num_stats[call_idx] = 0

    def setup_learn(self, total_timesteps):
        # Set total rollout steps
        if self.on_or_off_policy == 'On-Policy':
            self.total_rollout_steps = self.policy.n_rollout_steps
            print(f'total_rollout_steps {self.total_rollout_steps}')
        else:
            self.total_rollout_steps = self.policy.train_freq.frequency

        # Set total timesteps
        self.total_timesteps = total_timesteps
        # self.policy._total_timesteps is set in _setup_learn
        self.policy._setup_learn(total_timesteps=self.total_timesteps, num_calls=self.call.num_calls)

    def collect_packet_stats(self, call_idx, stats_dict):
        self.packet_records[call_idx].add_loss_rate(stats_dict['loss_rate'])
        self.packet_records[call_idx].add_rtt(stats_dict['rtt'])
        self.packet_records[call_idx].add_delay_interval()
        self.packet_records[call_idx].add_receiver_side_thp(stats_dict['recv_thp'])
        self.num_stats[call_idx] += 1
        print(f'CALL collect_packet_stats: call {call_idx} {stats_dict} num_stats {self.num_stats[call_idx]}')


    '''
    One env step implemented in two parts:
    - Part 1. policy.compute_actions() computes actions based on the latest previous obs
    - Part 2. policy.add_new_obs_and trajectory() sends a trajectory related to the latest previous obs
      and the new obs.
    '''
    # TODO: Rewrite policy#add_to_rollout_buffer and policy#compute_action such that:
    # obs: observation obtained as a result of applying the current action
    # rewards: how good the current action was
    # compute_actions() returns
    def on_policy_env_step(self, call_idx):
        # (1) Compute action based on the latest obs
        # and send it to the video call sender
        actions, values, log_probs = self.policy.compute_actions(call_idx)
        bwe = self.packet_records[call_idx].rescale_action_continuous(actions)
        if type(bwe) is list or isinstance(bwe, np.ndarray):
            bwe = bwe[0]
        print(f'Call {call_idx} sending BWE {bwe}')
        # truncate-then-write the bwe for this call
        with open(f'bwe{call_idx}.txt', mode='w') as f:
            f.write(f'{bwe}')

        # (2) Sends a trajectory (the latest previous obs and the new obs to the policy)
        # as a result of applying the action.
        # - obs: observation obtained as a result of applying the current action
        # - rewards: how good the current action was
        new_obs = self.packet_records[call_idx].calculate_obs(self.history_len)
        rewards, recv_thp, loss_rate, rtt, recv_thp_fluct = self.packet_records[call_idx].calculate_reward(self.history_len)
        self.policy.add_to_rollout_buffer(call_idx, new_obs, actions, rewards, values, log_probs, self.dones, self.infos)

        # New Obs: a new obs collected, as a result of the previously computed action
        # Reward: how good the previously computed action was
        # Action: a new action computed based on the previous obs
        print(f'[{self.on_or_off_policy} {self.rl_algo}] Step {self.num_timesteps} traj. of call {call_idx} ({self.kth_rollout_loop}th rc loop): New Obs {new_obs} ([loss_rate, norm_rtt, norm_delay_interval, norm_recv_thp]) Reward {rewards} (50 * {recv_thp} - 50 * {loss_rate} - 10 * {rtt} - 30 * {recv_thp_fluct}) Action (1Kbps-1Mbps) {bwe} action (-1~1) {actions}')

        with open('reward-curve.log', mode='a+') as f:
            f.write(f'{self.num_timesteps} {rewards}\n')

    '''
    One env step implemented in two parts:
    - Part 1. policy.compute_actions() computes actions based on the latest previous obs
    - Part 2. policy.add_new_obs_and trajectory() sends a trajectory related to the latest previous obs
      and the new obs.
    '''
    # TODO: Rewrite policy#add_to_replay_buffer and policy#sample_action such that:
    # obs: observation obtained as a result of applying the current action
    # rewards: how good the current action was
    # compute_actions() returns
    def off_policy_env_step(self, call_idx):
        # Part 2. Sample an action according to the exploration policy
        # and send it to the video call sender
        actions, buffer_actions = self.policy.sample_action()
        if (self.action_space_type == 'continuous'):
            bwe = self.packet_records[call_idx].rescale_action_continuous(actions)
        else:
            bwe = self.packet_records[call_idx].rescale_action_discrete(actions)
        if type(bwe) is list or isinstance(bwe, np.ndarray):
            bwe = bwe[0]
        # truncate-then-write the bwe
        with open('bwe.txt', mode='w') as f:
            f.write(f'{bwe}')

        # Part 1. Sends a trajectory (the latest previous obs and the new obs to the policy)
        # obs: observation obtained as a result of applying the current action
        # rewards: how good the current action was
        new_obs = self.packet_records[call_idx].calculate_obs(self.history_len)
        rewards, recv_thp, loss_rate, rtt, recv_thp_fluct = self.packet_records[call_idx].calculate_reward(self.history_len)
        self.policy.add_to_replay_buffer(new_obs, buffer_actions, rewards, self.dones, self.infos)

        # New Obs: a new obs collected, as a result of the previously computed action
        # Reward: how good the previously computed action was
        # Action: a new action computed based on the previous obs
        print(f'[{self.on_or_off_policy} {self.rl_algo}] Step {self.num_timesteps} ({self.kth_rollout_loop}th rollout_collection loop): New Obs {new_obs} ([loss_rate, norm_rtt, norm_delay_interval, norm_recv_thp]) Reward {rewards} (50 * {recv_thp} - 50 * {loss_rate} - 10 * {rtt} - 30 * {recv_thp_fluct}) Action (1Kbps-1Mbps) {bwe} action (-1~1) {actions}')

    def rollout_collection(self, call_idx):
        # (1) Rollout collection.
        # One rollout collection consists of total_rollout_steps number of env.step()s.
        # Do env.step() when history_len number of stats are collected.
        if self.num_rollout_steps < self.total_rollout_steps:
            # We just started the rollout collection.
            if self.num_rollout_steps == 0:
                self.policy.init_rollout_collection()
            # Execute logic that corresponds to env.step()
            if self.on_or_off_policy == 'On-Policy':
                self.on_policy_env_step(call_idx)
            else:
                self.off_policy_env_step(call_idx)
            self.num_rollout_steps += 1
            self.num_timesteps += 1

    def model_update(self, call_idx):
        # (2) Model update.
        # After finishing one rollout collection loop, do model update.
        # print(f'model_update: self.num_rollout_steps {self.num_rollout_steps} self.total_rollout_steps {self.total_rollout_steps} ')
        if self.num_rollout_steps % self.total_rollout_steps == 0:
            self.num_rollout_steps = 0
            self.kth_rollout_loop += 1
            self.policy.collection_loop_fin(True)
            print(f'[{self.on_or_off_policy} {self.rl_algo}] Step {self.num_timesteps} traj. of call {call_idx}: Model Update (rc len {self.total_rollout_steps})')
            # Do model update
            self.policy.train()
            # One policy.learn loop finished

        # Training finished (total_timesteps number of steps reached)!
        if self.num_timesteps == self.total_timesteps:
            print(f'Training finished!')
            self.training_completed = True

    '''
    Adaptation of SB3's On/OffPolicyAlgorithm for an environment that runs in real-time.
    '''
    def learn(self, call_idx):
        self.rollout_collection(call_idx)
        # TODO: Model update should be called after one episode from each env have finished
        self.model_update(call_idx)

    def start_calls(self):
        receiver_apps, sender_apps = self.call.run_calls()
        num_calls = len(sender_apps)
        # A loop that monitors sender-side packet stats in real time for training.
        # TODO: Better way of collecting parallel packet stats?
        lines = {}
        while True:
            if self.training_completed:
                break

            for call_idx in range(0, num_calls):
                lines[call_idx] = sender_apps[call_idx].stdout.readline()

            for call_idx in range(0, num_calls):
                line = lines[call_idx]
                if not line:
                    continue
                if isinstance(line, bytes):
                    line = line.decode("utf-8")
                    stats_dict = fetch_stats(line)

                    if stats_dict:
                        self.collect_packet_stats(call_idx, stats_dict)
                        # For every HISTORY_LEN number of received packets in each call,
                        # run (1) rollout collection, (2) model update, or both.
                        if self.num_stats[call_idx] % self.history_len == 0:
                            self.learn(call_idx)

            lines.clear()

        for call_idx in range(0, num_calls):
            receiver_apps[call_idx].wait()
            sender_apps[call_idx].wait()
            print(f'CALL wait() called for receiver_apps[{call_idx}], sender_apps[{call_idx}]')

        self.call.check_result()
        print(f'CALL Video calls ended')

    '''
    Deprecated.
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
        logging.error(f'env.step() is no more used!')

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
        for call_idx in range(0, self.call.num_calls):
            self.packet_records[call_idx].reset()
            self.num_stats[call_idx] = 0
        self.num_rollout_steps = 0
        self.num_timesteps = 0

        # Produce initial observation
        obs = self.packet_records[0].calculate_obs(self.history_len)

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
    Nothing to do.
    '''
    def close (self):
        pass
