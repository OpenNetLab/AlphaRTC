#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import glob
import json
import numpy as np
import os
import sys
import time
from stable_baselines3 import PPO, A2C, DQN, TD3, SAC
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


def fetch_stats(line: str) -> dict:
    line = line.strip()
    try:
        stats = json.loads(line)
        return stats
    except json.decoder.JSONDecodeError:
        return None

def load_checkpoint(rl_algo, ckpt_dir):
    # Load the most recent checkpoint of the RL algorithm
    ckpt_files = glob.glob(f'{ckpt_dir}/{rl_algo}*')
    if (len(ckpt_files) == 0):
        with open("ckpt_loading.log", "a+") as out:
            out.write(f'No checkpoint for {rl_algo} yet\n')
        return None
    else:
        ckpt_file = max(ckpt_files)
        with open("ckpt_loading.log", "a+") as out:
            out.write(f'Loading checkpoint {ckpt_file} (the most recent one among {ckpt_files})\n')
        return ckpt_file

def create_or_load_policy(rl_algo, ckpt_dir):

    ckpt_file = load_checkpoint(rl_algo, ckpt_dir)

    # Instantiate gym environment
    if rl_algo == 'PPO':
        env = GymEnv()
        if (ckpt_file is None):
            policy = PPO("MlpPolicy", env, n_steps=1, device='cpu', verbose=1)
        else:
            policy = PPO.load(path=ckpt_file, env=env, device='cpu', verbose=1)

    elif rl_algo == 'A2C':
        env = GymEnv()
        if (ckpt_file is None):
            policy = A2C("MlpPolicy", env, n_steps=1, device='cpu', verbose=1)
        else:
            policy = A2C.load(path=ckpt_file, env=env, device='cpu', verbose=1)

    elif rl_algo == 'DQN':
        env = GymEnv(action_space_type='discrete')
        if (ckpt_file is None):
            policy = DQN("MlpPolicy", env, device='cpu', verbose=1)
        else:
            policy = DQN.load(path=ckpt_file, env=env, device='cpu', verbose=1)

    elif rl_algo == 'TD3':
        env = GymEnv()
        if (ckpt_file is None):
            policy = TD3("MlpPolicy", env, device='cpu', verbose=1)
        else:
            policy = TD3.load(path=ckpt_file, env=env, device='cpu', verbose=1)

    elif rl_algo == 'SAC':
        env = GymEnv()
        if (ckpt_file is None):
            policy = SAC("MlpPolicy", env, device='cpu', verbose=1)
        else:
            policy = SAC.load(path=ckpt_file, env=env, device='cpu', verbose=1)

    else:
        raise Exception(f'Unsupported RL algorithm {rl_algo}')

    return env, policy



# Rather an environment which provides real network statistics from end-to-end call
def main(ifd = sys.stdin, ofd = sys.stdout):
    num_steps = 0

    rl_algo = 'SAC'

    # Path to save model ckpt
    ckpt_dir = './rl_model/ckpt'
    if not os.path.exists(ckpt_dir):
        os.makedirs(ckpt_dir)

    env, policy = create_or_load_policy(rl_algo, ckpt_dir)

    while True:
        # Read a line from app.stdout, which is packet statistics
        line = ifd.readline()
        if not line:
            break
        if isinstance(line, bytes):
            line = line.decode("utf-8")

        stats = fetch_stats(line)
        if stats:
            # Send per-packet stats to the RL agent and receive latest BWE
            env.packet_record.add_receiver_side_thp(stats['receiver_side_thp'])
            env.packet_record.add_rtt(stats['rtt'])
            env.packet_record.add_loss_rate(stats['loss_rate'])

            # Train the RL policy with the state.
            # Calls collect_rollouts() and train() in SB3.
            # - collect_rollouts():    For n_rollout_steps, call ppo.policy(obs) to get action
            #                               and call env.step(clipped_actions) to get new_obs, rewards, dones, infos
            # - train():               Update the policy.
            policy.learn(total_timesteps=1)

            # return latest action (bwe) produced by the policy network
            bwe = env.get_latest_bwe()
            # For debugging purposes only (remove the file I/O for actual training)
            with open('rl_agent_stats_action.log', mode='a+') as f:
                f.write(f'rl_agent ({rl_algo}): stats {stats} bwe {bwe}\n')
            num_steps += 1

            # if num_steps % 500 == 0:
            #     end_ts = time.time()
            #     step_time = end_ts - start_ts
            #     print(f'start_ts {start_ts} end_ts {end_ts} num_steps {num_steps}\taggregate step time (s, 500 steps) : {step_time}')
            #     start_ts = end_ts

            # truncate-then-write the bwe
            with open('bwe.txt', mode='w') as f:
                f.write(f'{bwe}')
            continue

        sys.stdout.write(line)
        sys.stdout.flush()

    # the checkpoint file will be saved & and loaded as a .zip file
    # (its prefix is determined as .zip by the SB3 APIs)
    ckpt_file = f'{ckpt_dir}/{rl_algo}_{time.strftime("%Y_%m_%d_%H_%M_%S", time.localtime())}'
    print(f'Saving checkpoint file {ckpt_file}')
    policy.save(ckpt_file)

    # Path to save reward curves
    # reward_curve_dir = './rl_model/reward_curves/'
    # if not os.path.exists(reward_curve_dir):
    #     os.makedirs(reward_curve_dir)

    # plt.plot(range(len(record_episode_reward)), record_episode_reward)
    # plt.plot(record_episode_reward, c = '#bbbcb8') # gray
    # plt.xlabel('Steps')
    # plt.ylabel('Averaged Training Reward')
    # plt.savefig(f'{reward_curve_dir}/training_reward_curve_steps{num_steps}.pdf')

    return num_steps


if __name__ == '__main__':
    main()


