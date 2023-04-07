import glob
import time
import logging
logging.basicConfig(filename='step_obs_reward_action.log', encoding='utf-8', level=logging.INFO)

from stable_baselines3 import PPO, A2C, DQN, TD3, SAC
from rl_training.rtc_env import RTCEnv


class Policy:
    def __init__(self, rl_algo, action_space_type, episode_len):
        self.rl_algo = rl_algo
        self.action_space_type = action_space_type
        self.episode_len = episode_len
        self.env = RTCEnv(rl_algo=self.rl_algo, action_space_type=self.action_space_type)
        self.policy = None

    def save_checkpoint(self, ckpt_dir):
        # Checkpoint file will be saved and loaded as a .zip file by the SB3
        ckpt_file = f'{ckpt_dir}/{self.rl_algo}_{time.strftime("%Y_%m_%d_%H_%M_%S", time.localtime())}'
        print(f'Saving checkpoint file {ckpt_file}')
        self.policy.save(ckpt_file)

    def load_checkpoint(self, ckpt_dir):
        # Load the most recent checkpoint of the RL algorithm
        ckpt_files = glob.glob(f'{ckpt_dir}/{self.rl_algo}*')
        if (len(ckpt_files) == 0):
            with open("ckpt_loading.log", "a+") as out:
                out.write(f'No checkpoint for {self.rl_algo} yet\n')
            return None
        else:
            ckpt_file = max(ckpt_files)
            with open("ckpt_loading.log", "a+") as out:
                out.write(f'Loading checkpoint {ckpt_file} (the most recent one among {ckpt_files})\n')
            return ckpt_file

    def _create_a2c_policy(self):
        # It will check your custom environment and output additional warnings if needed
        # check_env(self.env, warn=True)
        self.policy = A2C("MlpPolicy", self.env, n_steps=self.episode_len, device='cpu', verbose=1)

    def _create_ppo_policy(self):
        # check_env(self.env, warn=True)
        self.policy = PPO("MlpPolicy", self.env, n_steps=self.episode_len, device='cpu', verbose=1)

    def _create_dqn_policy(self):
        # check_env(self.env, warn=True)
        self.policy = DQN("MlpPolicy", self.env, device='cpu', verbose=1)

    def _create_td3_policy(self):
        # check_env(self.env, warn=True)
        self.policy = TD3("MlpPolicy", self.env, device='cpu', verbose=1)

    def _create_sac_policy(self):
        # check_env(self.env, warn=True)
        self.policy = SAC("MlpPolicy", self.env, device='cpu', verbose=1)

    # Factory method that returns a requested RL algorithm-based policy
    def _create_policy(self, rl_algo='PPO'):
        if rl_algo == 'A2C':
            self._create_a2c_policy()
        elif rl_algo == 'PPO':
            self._create_ppo_policy()
        elif rl_algo == 'DQN':
            self._create_dqn_policy()
        elif rl_algo == 'TD3':
            self._create_td3_policy()
        elif rl_algo == 'SAC':
            self._create_sac_policy()
        else:
            raise ValueError(f'{rl_algo} is not supported')
        return self.policy

    # A wrapper of the factory method that adds exception handling
    def create_policy(self, rl_algo):
        try:
            return self._create_policy(rl_algo)
        except ValueError as e:
            logging.error(e)
        return None
