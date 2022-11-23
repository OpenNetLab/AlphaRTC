import numpy as np
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


'''
Estimator class for emulator or in-situ training.
'''
class Estimator(object):
    def __init__(self):
        self.rl_algo = 'DQN'
        print(f'RL algorithm used: {self.rl_algo}')

        # Instantiate gym environment
        if self.rl_algo == 'PPO':
            self.env = GymEnv()
            self.policy = PPO("MlpPolicy", self.env, n_steps=1, device='cpu', verbose=1)
        elif self.rl_algo == 'A2C':
            self.env = GymEnv()
            self.policy = A2C("MlpPolicy", self.env, n_steps=1, device='cpu', verbose=1)
        elif self.rl_algo == 'DQN':
            self.env = GymEnv(action_space_type='discrete')
            self.policy = DQN("MlpPolicy", self.env, device='cpu', verbose=1)
        elif self.rl_algo == 'SAC':
            self.env = GymEnv()
            self.policy = SAC("MlpPolicy", self.env, device='cpu', verbose=1)
        else:
            print(f'Unsupported algorithm: {self.rl_algo}')

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
        # Collect the statistics
        self.env.packet_record.add_receiver_side_thp(state['receiver_side_thp'])
        self.env.packet_record.add_rtt(state['rtt'])
        self.env.packet_record.add_loss_rate(state['loss_rate'])

        # Train the RL policy with the state.
        # Calls self.collect_rollouts() and self.train() in SB3.
        # - self.collect_rollouts():    For n_rollout_steps, call ppo.policy(obs) to get action
        #                               and call env.step(clipped_actions) to get new_obs, rewards, dones, infos
        # - self.train():               Update the policy.
        self.policy.learn(total_timesteps=1)

        # return latest action (bwe) produced by the policy network
        return self.env.get_latest_bwe()