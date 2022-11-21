import numpy as np
# from rl_training.ppo_agent import PPO
from stable_baselines3 import PPO, A2C, DQN, SAC
from rl_training.rtc_env import GymEnv

UNIT_M = 1000000
MAX_BANDWIDTH_MBPS = 8      # Max: 8 Mbps
MIN_BANDWIDTH_MBPS = 0.01   # Min: 10 Kbps
LOG_MAX_BANDWIDTH_MBPS = np.log(MAX_BANDWIDTH_MBPS)
LOG_MIN_BANDWIDTH_MBPS = np.log(MIN_BANDWIDTH_MBPS)

def log_to_linear(value):
    # from 0~1 to 10kbps to 8Mbps
    value = value.detach().numpy()
    value = np.clip(value, 0, 1)
    log_bwe = value * (LOG_MAX_BANDWIDTH_MBPS - LOG_MIN_BANDWIDTH_MBPS) + LOG_MIN_BANDWIDTH_MBPS
    return np.exp(log_bwe) * UNIT_M

def liner_to_log(value):
    # from 10kbps~8Mbps to 0~1
    value = np.clip(value / UNIT_M, MIN_BANDWIDTH_MBPS, MAX_BANDWIDTH_MBPS)
    log_value = np.log(value)
    return (log_value - LOG_MIN_BANDWIDTH_MBPS) / (LOG_MAX_BANDWIDTH_MBPS - LOG_MIN_BANDWIDTH_MBPS)


'''
Estimator class for emulator or in-situ training.
'''
class Estimator(object):
    def __init__(self):
        # Hyperparameters for RL training
        # 1 episode = num_steps_per_episode steps. Policy is updated whenever an episode is finished.
        # 1 step = whenever a reward is generated, i.e. an action is applied and new state is observed as a result of it
        self.num_steps_per_episode = 10     # cf. set as 4000 in gym-example
        self.num_steps = 0
        self.num_episodes = 0

        # PPO agent for RL policy
        state_dim = 4
        action_dim = 1
        exploration_param = 0.05            # the std var of action distribution
        lr = 3e-5                           # 0.00003. for Adam
        betas = (0.9, 0.999)
        self.gamma = 0.99                   # discount factor
        K_epochs = 37                       # update policy for K_epochs epochs in one PPO update
        ppo_clip = 0.2                      # clip parameter

        self.rl_algo = 'PPO'
        print(f'Using {self.rl_algo}')

        # Instantiate gym environment
        if self.rl_algo == 'PPO':
            self.env = GymEnv()
            self.ppo = PPO("MlpPolicy", self.env, n_steps=1, device='cpu', verbose=1)
        elif self.rl_algo == 'A2C':
            self.env = GymEnv()
            self.ppo = A2C("MlpPolicy", self.env, n_steps=1, device='cpu', verbose=1)
        elif self.rl_algo == 'DQN':
            self.env = GymEnv(action_space_type='discrete')
            self.ppo = DQN("MlpPolicy", self.env, n_steps=1, device='cpu', verbose=1)
        elif self.rl_algo == 'SAC':
            self.env = GymEnv()
            self.ppo = SAC("MlpPolicy", self.env, device='cpu', verbose=1)
        else:
            print(f'Unsupported algorithm: {self.rl_algo}')

        # # To calculate average reward per step in each episode
        # self.accum_reward  = 0
        # self.avg_reward_per_step_list = []

        # # Path to save model ckpt
        # self.ckpt_dir = './rl_model/ckpt/'
        # if not os.path.exists(self.ckpt_dir):
        #     os.makedirs(self.ckpt_dir)
        # # Save ckpt at every 10 episodes
        # self.episodes_between_ckpt = 10

        # # Path to save reward curve
        # self.reward_curve_dir = './rl_model/reward_curves/'
        # if not os.path.exists(self.reward_curve_dir):
        #     os.makedirs(self.reward_curve_dir)


    # states obtained by the latest action
    def relay_packet_statistics(self, state: dict):
        # Add the statistics for this packet in env's data structure
        self.env.packet_record.add_receiver_side_thp(state['receiver_side_thp'])
        self.env.packet_record.add_rtt(state['rtt'])
        self.env.packet_record.add_loss_rate(state['loss_rate'])

        # Train the RL policy.
        # Calls self.collect_rollouts() and self.train() in SB3 PPO.
        # self.collect_rollouts():  for n_rollout_steps, call ppo.policy(obs) to get action
        #                           and call env.step(clipped_actions) to get new_obs, rewards, dones, infos
        # self.train():             policy update
        self.ppo.learn(total_timesteps=1)

        # return latest action (bwe) produced by the policy network
        return self.env.get_latest_bwe()