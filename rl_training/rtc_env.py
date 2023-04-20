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

def check_call_result(receiver_app, sender_app):
    # Log whether the call ended successfully
    call_result = ''
    if receiver_app.returncode == 0 and sender_app.returncode == 0:
        call_result = f'Call finished successfully!\n'
    else:
        call_result = f'Call finished with errors! \
            receiver\'s return code {receiver_app.returncode} \
            sender\'s return code {sender_app.returncode}\n'

    with open("call_result.log", "a+") as out:
        out.write(call_result)

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

'''
Generate a random free tcp6 port.
Goal: dynamically binding an unused port for e2e call
'''
def generate_random_port(config_file):
    MIN_PORT = 1024
    MAX_PORT = 65535

    used_ports = []
    free_port = -1

    out = subprocess.check_output('netstat -tnlp | grep tcp6', shell=True)
    lines = out.decode("utf-8").split("\n")
    # Figure out all the used ports
    for line in lines:
        # Proto    Recv-Q    Send-Q    Local Address    Foreign Address    State    PID/Program name
        line_elements = line.split()
        if len(line_elements) > 4:
            local_address = line.split()[3] # e.g., ::1:39673 :::22
            port = int(local_address.split(':')[-1])
            used_ports.append(port)

    while(free_port < 0 or free_port in used_ports):
        free_port = random.randint(MIN_PORT, MAX_PORT)

    # Write the free port to the designated port file
    port_path = ''
    with open(config_file) as json_file:
        data = json.load(json_file)
        port_path = data['serverless_connection']['receiver']['listening_port']
    with open(port_path, "w") as out:
        out.write(str(free_port))

    with open('port_assignment.log', "a+") as out:
        out.write(f'Call with config {config_file}: assigned {free_port} written to {port_path}\n')

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
        # end of episode signal
        self.dones = np.zeros(1) # single env (with multiple calls)
        self.infos = [{'episode': 640}] # List[Dict[str, Any]]
        self.logger = logging.getLogger(__name__)
        self.policy = None
        self.num_stats = 0
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

        self.packet_record = PacketRecord()

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
        self.policy._setup_learn(total_timesteps=self.total_timesteps)

    def collect_packet_stats(self, call_idx, stats_dict):
        self.packet_record.add_loss_rate(stats_dict['loss_rate'])
        self.packet_record.add_rtt(stats_dict['rtt'])
        self.packet_record.add_delay_interval()
        self.packet_record.add_receiver_side_thp(stats_dict['recv_thp'])
        # print(f'collect_packet_stats: call {call_idx} {stats_dict}')
        self.num_stats += 1

    def is_done(self):
        episode_len = self.infos[0]['episode']
        return self.num_timesteps == episode_len

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
    def on_policy_env_step(self):
        # (1) Compute action based on the latest obs
        # and send it to the video call sender
        actions, values, log_probs = self.policy.compute_actions()
        bwe = self.packet_record.rescale_action_continuous(actions)
        if type(bwe) is list or isinstance(bwe, np.ndarray):
            bwe = bwe[0]
        # truncate-then-write the bwe
        with open('bwe.txt', mode='w') as f:
            f.write(f'{bwe}')

        # (2) Sends a trajectory (the latest previous obs and the new obs to the policy)
        # as a result of applying the action.
        # - obs: observation obtained as a result of applying the current action
        # - rewards: how good the current action was
        new_obs = self.packet_record.calculate_obs()
        rewards, recv_thp, loss_rate, rtt, recv_thp_fluct = self.packet_record.calculate_reward()
        if self.is_done():
            self.dones = np.ones(1)
        print(f'DONES self.timesteps {self.num_timesteps} self.dones {self.dones}')
        self.policy.add_to_rollout_buffer(new_obs, actions, rewards, values, log_probs, self.dones, self.infos)

        # New Obs: a new obs collected, as a result of the previously computed action
        # Reward: how good the previously computed action was
        # Action: a new action computed based on the previous obs
        print(f'''\n[{self.on_or_off_policy} {self.rl_algo}] Step {self.num_timesteps} ({self.kth_rollout_loop}th rollout_collection loop):
            New Obs {new_obs} ([loss_rate, norm_rtt, norm_delay_interval, norm_recv_thp])
            Reward {rewards} (50 * {recv_thp} - 50 * {loss_rate} - 10 * {rtt} - 30 * {recv_thp_fluct})
            Action (1Kbps-1Mbps) {bwe} action (-1~1) {actions}''')

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
    def off_policy_env_step(self):
        # Part 2. Sample an action according to the exploration policy
        # and send it to the video call sender
        actions, buffer_actions = self.policy.sample_action()
        if (self.action_space_type == 'continuous'):
            bwe = self.packet_record.rescale_action_continuous(actions)
        else:
            bwe = self.packet_record.rescale_action_discrete(actions)
        if type(bwe) is list or isinstance(bwe, np.ndarray):
            bwe = bwe[0]
        # truncate-then-write the bwe
        with open('bwe.txt', mode='w') as f:
            f.write(f'{bwe}')

        # Part 1. Sends a trajectory (the latest previous obs and the new obs to the policy)
        # obs: observation obtained as a result of applying the current action
        # rewards: how good the current action was
        new_obs = self.packet_record.calculate_obs()
        rewards, recv_thp, loss_rate, rtt, recv_thp_fluct = self.packet_record.calculate_reward()
        self.policy.add_to_replay_buffer(new_obs, buffer_actions, rewards, self.dones, self.infos)

        # New Obs: a new obs collected, as a result of the previously computed action
        # Reward: how good the previously computed action was
        # Action: a new action computed based on the previous obs
        print(f'''\n[{self.on_or_off_policy} {self.rl_algo}] Step {self.num_timesteps} ({self.kth_rollout_loop}th rollout_collection loop):
            New Obs {new_obs} ([loss_rate, norm_rtt, norm_delay_interval, norm_recv_thp])
            Reward {rewards} (50 * {recv_thp} - 50 * {loss_rate} - 10 * {rtt} - 30 * {recv_thp_fluct})
            Action (1Kbps-1Mbps) {bwe} action (-1~1) {actions}''')

    def rollout_collection(self):
        # (1) Rollout collection.
        # One rollout collection consists of total_rollout_steps number of env.step()s.
        # Do env.step() when history_len number of stats are collected.
        if self.num_rollout_steps < self.total_rollout_steps:
            # We just started the rollout collection.
            if self.num_rollout_steps == 0:
                self.policy.init_rollout_collection()
            # Execute logic that corresponds to env.step()
            if self.on_or_off_policy == 'On-Policy':
                self.on_policy_env_step()
            else:
                self.off_policy_env_step()
            self.num_rollout_steps += 1
            self.num_timesteps += 1

    def model_update(self):
        # (2) Model update.
        # After finishing one rollout collection loop, do model update.
        # print(f'model_update: self.num_rollout_steps {self.num_rollout_steps} self.total_rollout_steps {self.total_rollout_steps} ')
        if self.num_rollout_steps % self.total_rollout_steps == 0:
            self.num_rollout_steps = 0
            self.kth_rollout_loop += 1
            self.policy.collection_loop_fin(True)
            print(f'\n[{self.on_or_off_policy} {self.rl_algo}] Step {self.num_timesteps}: Finished rollout_collection, starting model_update ({self.total_rollout_steps} steps for a single rollout collection)')
            # Do model update
            self.policy.train()
            # One policy.learn loop finished

        # Training finished (total_timesteps number of steps reached)!
        if self.num_timesteps == self.total_timesteps:
            print(f'Training finished!')
            self.training_completed = True

    '''
    Adaptation of SB3's OnPolicyAlgorithm for an environment that runs in real-time.
    '''
    def train(self):
        self.rollout_collection()
        # TODO: Model update should be called after one episode from each env have finished
        self.model_update()

    def start_calls(self, link_bandwidth, delay):
        # Randomly assign different port for this video call
        generate_random_port('receiver_pyinfer.json')
        generate_random_port('receiver_pyinfer2.json')
        # Run the video call (env) in separate processes
        receiver_cmd = f"$ALPHARTC_HOME/peerconnection_serverless.origin receiver_pyinfer.json"
        sender_cmd = f"sleep 5; mm-link traces/{link_bandwidth} traces/{link_bandwidth} $ALPHARTC_HOME/peerconnection_serverless.origin sender_pyinfer.json"
        receiver_app = subprocess.Popen(receiver_cmd, shell=True)
        sender_app = subprocess.Popen(sender_cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
        print(f'Video call started: link BW {link_bandwidth}') # , one-way delay {delay}ms')

        receiver_cmd2 = f"$ALPHARTC_HOME/peerconnection_serverless.origin receiver_pyinfer2.json"
        sender_cmd2 = f"sleep 5; mm-link traces/2mbps traces/2mbps $ALPHARTC_HOME/peerconnection_serverless.origin sender_pyinfer2.json"
        receiver_app2 = subprocess.Popen(receiver_cmd2, shell=True)
        sender_app2 = subprocess.Popen(sender_cmd2, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
        print(f'Video call started: link BW 2mbps') # , one-way delay {delay}ms')

        # A loop that monitors sender-side packet stats in real time for training.
        # TODO: cleanup
        while True:
            if self.training_completed:
                break

            line = sender_app.stdout.readline()
            line2 = sender_app2.stdout.readline()
            if not line and not line2:
                # sender_app completed.
                break
            if isinstance(line, bytes):
                line = line.decode("utf-8")
                stats_dict = fetch_stats(line)
            if isinstance(line2, bytes):
                line2 = line2.decode("utf-8")
                stats_dict2 = fetch_stats(line2)

            # Add newly fetched packet stats
            if stats_dict or stats_dict2:
                if stats_dict:
                    self.collect_packet_stats(0, stats_dict)
                if stats_dict2:
                    self.collect_packet_stats(1, stats_dict2)
                # For every HISTORY_LEN number of received packets,
                # run (1) rollout collection, (2) model update, or both.
                if self.num_stats % self.packet_record.history_len == 0:
                    self.train()

            sys.stdout.write(f'{line}')
            sys.stdout.flush()

        receiver_app.wait()
        sender_app.wait()
        receiver_app2.wait()
        sender_app2.wait()
        check_call_result(receiver_app, sender_app)
        check_call_result(receiver_app2, sender_app2)
        print(f'Video call ended: link BW {link_bandwidth}, 2mbps') # , one-way delay {delay}ms')

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
        self.packet_record.reset()
        self.num_rollout_steps = 0
        self.num_timesteps = 0
        self.num_stats = 0

        # Produce initial observation
        obs = self.packet_record.calculate_obs()
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
