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
def generate_random_port():
    MIN_PORT = 1024
    MAX_PORT = 65535

    used_ports = []
    ret_port = -1

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

    while(ret_port < 0 or ret_port in used_ports):
        ret_port = random.randint(MIN_PORT, MAX_PORT)

    # Write the free port as a file
    with open("free_port", "w") as out:
        out.write(str(ret_port))

    with open("port_assignment.log", "a+") as out:
        out.write(str(ret_port)+"\n")

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
        self.logger = logging.getLogger(__name__)
        self.policy = None
        self.link_bandwidth = None
        self.delay = None
        self.num_stats = 0
        # number of steps in one rollout collection loop.
        # 2048 for PPO, 5 for A2C
        self.total_rollout_steps = 0
        # number of env.step() performed in a single rollout
        self.num_rollout_steps = 0
        # number of steps in one training loop
        self.total_timesteps = 0
        self.num_timesteps = 0
        self.rl_algo = rl_algo
        assert self.rl_algo == 'PPO' or self.rl_algo == 'A2C' \
            or self.rl_algo == 'DQN' or self.rl_algo == 'SAC'
        self.on_or_off_policy = ''
        if self.rl_algo == 'PPO' or self.rl_algo == 'A2C':
            self.on_or_off_policy = 'On-Policy'
            self.policy._setup_learn(total_timesteps=self.total_timesteps)
        else:
            self.on_or_off_policy = 'Off-Policy'
        self.training_completed = False


        self.packet_record = PacketRecord()

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


    def set_policy(self, policy):
        self.policy = policy
        self.total_rollout_steps = policy.n_steps
        self.total_timesteps = policy._total_timesteps

    def set_bw_delay(self, link_bandwidth, delay):
        self.link_bandwidth = link_bandwidth
        self.delay = delay

    def collect_packet_stats(self, stats_dict):
        self.packet_record.add_loss_rate(stats_dict['loss_rate'])
        self.packet_record.add_rtt(stats_dict['rtt'])
        self.packet_record.add_delay_interval()
        self.packet_record.add_receiver_side_thp(stats_dict['recv_thp'])
        self.num_stats += 1

    '''
    Adaptation of SB3's OnPolicyAlgorithm for an environment that runs in real-time.
    '''
    def on_policy_training(self, stats_dict):
        print(f'env.on_policy_training:===================== {stats_dict}')
        # (1) Rollout collection.
        # One rollout collection consists of total_rollout_steps number of env.step()s.
        # Do env.step() when history_len number of stats are collected.
        if self.num_rollout_steps < self.total_rollout_steps:
            # We just started the rollout collection.
            if self.num_rollout_steps == 0:
                self.rollout_collection_init()
            self.do_env_step()
            self.num_rollout_steps += 1
            self.num_timesteps += 1

        # (2) Model update.
        # After finishing one rollout collection loop, do model update.
        if self.num_rollout_steps % self.total_rollout_steps == 0:
            self.num_rollout_steps = 0
            # Used in PPO
            self.policy._update_current_progress_remaining(self.num_timesteps, self.total_timesteps)
            print(f'\n[{self.on_or_off_policy} {self.rl_algo}] Step {self.num_timesteps}: Starting model update ({self.total_rollout_steps} steps for a single rollout collection)')
            # Do model update
            self.policy.train()
            # One policy.learn loop finished

        # Training finished (total_timesteps number of steps reached)!
        if self.num_timesteps == self.total_timesteps:
            print(f'Training finished!')
            self.training_completed = True

    '''
    Init rollout collection in on-policy training.
    '''
    def rollout_collection_init(self):
        self.policy.policy.set_training_mode(False)
        self.policy.rollout_buffer.reset()

    def do_env_step(self):
        # Calculate obs, reward
        new_obs = self.packet_record.calculate_obs()
        rewards, recv_thp, loss_rate, rtt, recv_thp_fluct \
            = self.packet_record.calculate_reward()
        # Compute action based on the obs, reward
        action = self.policy.env_step(new_obs, rewards, None, self.info)
        # Apply the action to the video call
        bwe = self.packet_record.rescale_action(action)
        # truncate-then-write the bwe
        with open('bwe.txt', mode='w') as f:
            f.write(f'{bwe}')

        print(f'''\n[{self.on_or_off_policy} {self.rl_algo}] Step {self.num_timesteps}:
            Obs {new_obs} ([loss_rate, norm_rtt, norm_delay_interval, norm_recv_thp])
            Reward {rewards} (50 * {recv_thp} - 50 * {loss_rate} - 10 * {rtt} - 30 * {recv_thp_fluct})
            Action (1Kbps-1Mbps) {bwe} action (-1~1) {action}''')

    def off_policy_training(self, stats_dict):
        pass

    def start_call(self):
        # Randomly assign different port for this video call
        generate_random_port()
        # Run the video call (env) in separate processes
        receiver_cmd = f"$ALPHARTC_HOME/peerconnection_serverless.origin receiver_pyinfer.json"
        sender_cmd = f"sleep 5; mm-link traces/{self.link_bandwidth} traces/{self.link_bandwidth} $ALPHARTC_HOME/peerconnection_serverless.origin sender_pyinfer.json"
        receiver_app = subprocess.Popen(receiver_cmd, shell=True)
        sender_app = subprocess.Popen(sender_cmd, bufsize=1, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
        print(f'Video call started. link BW {self.link_bandwidth}, one-way delay {self.delay}ms')

        # A loop that monitors sender-side packet stats in real time for training.
        while True:
            if self.training_completed:
                break

            line = sender_app.stdout.readline()
            if not line:
                # sender_app completed.
                break
            if isinstance(line, bytes):
                line = line.decode("utf-8")

            # Extract packet statistics
            stats_dict = fetch_stats(line)
            if stats_dict:
                self.collect_packet_stats(stats_dict)
                # For every HISTORY_LEN number of received packets,
                # run (1) rollout collection, (2) model update, or both.
                if self.num_stats % self.packet_record.history_len == 0:
                    if self.on_or_off_policy == 'On-Policy':
                        self.on_policy_training(stats_dict)
                    else:
                        self.off_policy_training(stats_dict)

            sys.stdout.write(f'{line}')
            sys.stdout.flush()

        receiver_app.wait()
        sender_app.wait()
        check_call_result(receiver_app, sender_app)
        print('Video call ended')

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
        print(f'env.step() not used!')
        logging.error(f'env.step() not used!')
        # if type(action) is list or isinstance(action, np.ndarray):
        #     action_val = action[0]
        # elif type(action) is None:
        #     action_val = -0.40140140140140135 # default BWE = 300Kbps
        # else:
        #     action_val = action

        # bwe = self.packet_record.rescale_action(action_val)
        # # truncate-then-write the bwe
        # with open('bwe.txt', mode='w') as f:
        #     f.write(f'{bwe}')

        # # TODO: obs, reward should be obtained after applying the action
        # # TODO: correct delay interval
        # obs = self.packet_record.calculate_obs()
        # reward, recv_thp, loss_rate, rtt, recv_thp_fluct = self.packet_record.calculate_reward()

        # print(f'''\n[{self.rl_algo}] Step {self.num_timesteps}:
        # Obs {obs} ([loss_rate, norm_rtt, norm_delay_interval, norm_recv_thp])
        # Reward {reward} (50 * {recv_thp} - 50 * {loss_rate} - 10 * {rtt} - 30 * {recv_thp_fluct})
        # Action (1Kbps-1Mbps) {bwe} action (-1~1) {action_val}''')

        # self.num_timesteps += 1

        # return obs, reward, None, self.info

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
        action = -0.4014 # default BWE = 300Kbps
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
    Nothing to do.
    '''
    def close (self):
        pass
