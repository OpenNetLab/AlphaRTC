import glob
import os

from rl_training.policy_factory import PolicyFactory


RL_ALGO='A2C'
LINK_BW='1mbps'


def cleanup():
    for f in glob.glob("*.log"):
        os.remove(f)

def main():
    cleanup()

    env = PolicyFactory().create_env_and_policy(RL_ALGO)
    env.set_calls(link_bandwidths=['1mbps', '2mbps', '4mbps', '12mbps'], delays=[10, 20, 30])
    env.setup_learn(total_timesteps=25600)
    env.start_calls()


if __name__ == "__main__":
    main()
