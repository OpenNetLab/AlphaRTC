#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import glob
import json
import matplotlib.pyplot as plt
import numpy as np
import os
import sys
from stable_baselines3 import PPO, A2C, DQN, TD3, SAC
# from stable_baselines.common.env_checker import check_env
from rl_training.rtc_env import RTCEnv
import logging
logging.basicConfig(filename='step_obs_reward_action.log', encoding='utf-8', level=logging.INFO)


UNIT_M = 1000000
MAX_BANDWIDTH_MBPS = 8      # Max: 8 Mbps
MIN_BANDWIDTH_MBPS = 0.01   # Min: 10 Kbps
LOG_MAX_BANDWIDTH_MBPS = np.log(MAX_BANDWIDTH_MBPS)
LOG_MIN_BANDWIDTH_MBPS = np.log(MIN_BANDWIDTH_MBPS)

RL_ALGO='SAC'
LINK_BW='1mbps'
EPISODE_LEN=600


def fetch_stats(line: str) -> dict:
    line = line.strip()
    try:
        stats = json.loads(line)
        return stats
    except json.decoder.JSONDecodeError:
        return None

def plot_training_curve(num_episodes, rewards):
    training_curve_dir = './rl_model/training_curves/'
    if not os.path.exists(training_curve_dir):
        os.makedirs(training_curve_dir)

    plt.ylim(0, max(rewards))
    plt.plot(rewards, c = '#bbbcb8') # gray
    plt.title(f'RL-based CC ({RL_ALGO})')
    plt.xlabel('Step')
    plt.ylabel('Average Training Reward')
    plt.savefig(f'{training_curve_dir}/training-curve-{RL_ALGO}-{LINK_BW}-{num_episodes}episodes.pdf')

def load_checkpoint(ckpt_dir):
    # Load the most recent checkpoint of the RL algorithm
    ckpt_files = glob.glob(f'{ckpt_dir}/{RL_ALGO}*')
    if (len(ckpt_files) == 0):
        with open("ckpt_loading.log", "a+") as out:
            out.write(f'No checkpoint for {RL_ALGO} yet\n')
        return None
    else:
        ckpt_file = max(ckpt_files)
        with open("ckpt_loading.log", "a+") as out:
            out.write(f'Loading checkpoint {ckpt_file} (the most recent one among {ckpt_files})\n')
        return ckpt_file

def create_or_load_policy(ckpt_dir):
    # ckpt_file = load_checkpoint(ckpt_dir)
    ckpt_file = None

    # Instantiate gym environment
    if RL_ALGO == 'PPO':
        env = RTCEnv(rl_algo=RL_ALGO, link_bw=LINK_BW)
        # It will check your custom environment and output additional warnings if needed
        # check_env(env, warn=True)
        if (ckpt_file is None):
            policy = PPO("MlpPolicy", env, n_steps=EPISODE_LEN, device='cpu', verbose=1)
        else:
            policy = PPO.load(path=ckpt_file, env=env, device='cpu', verbose=1)

    elif RL_ALGO == 'A2C':
        env = RTCEnv(rl_algo=RL_ALGO, link_bw=LINK_BW)
        # It will check your custom environment and output additional warnings if needed
        # check_env(env, warn=True)
        if (ckpt_file is None):
            policy = A2C("MlpPolicy", env, n_steps=EPISODE_LEN, device='cpu', verbose=1)
        else:
            policy = A2C.load(path=ckpt_file, env=env, device='cpu', verbose=1)

    elif RL_ALGO == 'DQN':
        env = RTCEnv(rl_algo=RL_ALGO, link_bw=LINK_BW, action_space_type='discrete')
        # It will check your custom environment and output additional warnings if needed
        # check_env(env, warn=True)
        if (ckpt_file is None):
            policy = DQN("MlpPolicy", env, n_steps=EPISODE_LEN, device='cpu', verbose=1)
        else:
            policy = DQN.load(path=ckpt_file, env=env, device='cpu', verbose=1)

    elif RL_ALGO == 'TD3':
        env = RTCEnv(rl_algo=RL_ALGO, link_bw=LINK_BW)
        # It will check your custom environment and output additional warnings if needed
        # check_env(env, warn=True)
        if (ckpt_file is None):
            policy = TD3("MlpPolicy", env, n_steps=EPISODE_LEN, device='cpu', verbose=1)
        else:
            policy = TD3.load(path=ckpt_file, env=env, device='cpu', verbose=1)

    elif RL_ALGO == 'SAC':
        env = RTCEnv(rl_algo=RL_ALGO, link_bw=LINK_BW)
        # It will check your custom environment and output additional warnings if needed
        # check_env(env, warn=True)
        if (ckpt_file is None):
            policy = SAC("MlpPolicy", env, device='cpu', verbose=1)
        else:
            policy = SAC.load(path=ckpt_file, env=env, device='cpu', verbose=1)

    else:
        raise Exception(f'Unsupported RL algorithm {RL_ALGO}')

    return env, policy

def random_action():
    # Quickly trying a random agent on the custom environment
    env = RTCEnv()
    obs = env.reset()
    n_steps = 10
    for _ in range(n_steps):
        # Random action
        action = env.action_space.sample()
        obs, reward, done, info = env.step(action)
        print(f'Random action: obs {obs} reward {reward} produced by using action {action}')


def main(ifd, ofd):
    ckpt_dir = './rl_model/ckpt'
    if not os.path.exists(ckpt_dir):
        os.makedirs(ckpt_dir)

    env, policy = create_or_load_policy(ckpt_dir)

    while True:
        # Read a line from app.stdout, which is packet statistics
        # line = ifd.readline()
        # if not line:
        #     break
        # if isinstance(line, bytes):
        #     line = line.decode("utf-8")

        # stats = fetch_stats(line)
        # logging.info(f'SendState stats {line}')
        # if stats:

        with open('packet_stats.json', 'r') as packet_stats_json:
            # Opening JSON file
            stats = json.loads(packet_stats_json)

            # Send per-packet stats to the RL agent and receive latest BWE
            env.packet_record.add_loss_rate(stats['loss_rate'])
            env.packet_record.add_rtt(stats['rtt'])
            env.packet_record.add_delay_interval()
            env.packet_record.add_receiver_side_thp(stats['receiver_side_thp'])

            # Train the RL policy with the state.
            # Calls collect_rollouts() and train() in SB3.
            # - collect_rollouts():    For n_rollout_steps, call ppo.policy(obs) to get action
            #                          and call env.step(clipped_actions) to get new_obs, rewards, dones, infos
            # - train():               Update the policy.

            # 1 episode = 600 timesteps
            # Let's start with 10 episodes (i.e. 5 min)
            policy.learn(total_timesteps=600)

            bwe = env.get_latest_bwe()
            # truncate-then-write the bwe
            with open('bwe.txt', mode='w') as f:
                f.write(f'{bwe}')
            continue

        sys.stdout.write(line)
        sys.stdout.flush()

    # Checkpoint file will be saved and loaded as a .zip file by the SB3
    # ckpt_file = f'{ckpt_dir}/{RL_ALGO}_{time.strftime("%Y_%m_%d_%H_%M_%S", time.localtime())}'
    # print(f'Saving checkpoint file {ckpt_file}')
    # policy.save(ckpt_file)


if __name__ == '__main__':
    main()


