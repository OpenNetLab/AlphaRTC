import glob
import os

from rl_training.policy_factory import PolicyFactory


RL_ALGO='PPO'
LINK_BW='1mbps'


def cleanup():
    for f in glob.glob("*.log"):
        os.remove(f)

def main():
    cleanup()

    env = PolicyFactory().create_env_and_policy(RL_ALGO)
    # env.set_calls(link_bandwidths=['1mbps', '2mbps', '4mbps', '12mbps'], delays=[10, 20, 30])
    env.set_calls(link_bandwidths=['1mbps'], delays=[0])
    env.setup_learn(total_timesteps=4096)
    env.start_calls()


if __name__ == "__main__":
    main()
