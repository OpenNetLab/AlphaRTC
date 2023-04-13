import glob
import os
from statistics import mean, median
import matplotlib.pyplot as plt
import numpy as np

from rl_training.policy_factory import PolicyFactory


RL_ALGO='PPO'
LINK_BW='1mbps'


def cleanup():
    for f in glob.glob("*.log"):
        os.remove(f)

def plot_recv_thp():
    recv_thp_l = []
    sender_log = os.environ["ALPHARTC_HOME"] + f'{RL_ALGO}/sender-logs/webrtc-sender-{RL_ALGO}-{LINK_BW}'
    with open(sender_log, 'r') as f:
        for line in f:
            if 'Sending avg receiver side thp' in line:
                l = line.split()
                recv_thp_l.append(float(l[8]))

    a = np.array(recv_thp_l)
    average = "{:.2f}".format(mean(recv_thp_l))
    med = "{:.2f}".format(median(recv_thp_l))
    p90 = "{:.2f}".format(np.percentile(a, 90)) # 90th percentile
    p99 = "{:.2f}".format(np.percentile(a, 99)) # 99th percentile

    plt.ylim(0, 300000)
    plt.plot(recv_thp_l)
    plt.title(f'Recv-Thp: Avg {average} Med {med} P90 {p90} P99 {p99} bps')
    plt.xlabel('Time')
    plt.ylabel('Receiver-side throughput (bps)')
    recv_thp_file = os.environ["ALPHARTC_HOME"] + f'{RL_ALGO}/recv-thp/recv-thp-{RL_ALGO}-{LINK_BW}.pdf'
    plt.savefig(recv_thp_file)


def save_webrtc_logs():
    # Path of sender, receiver WebRTC internal logs
    sender_log_name = os.environ["ALPHARTC_HOME"] + 'webrtc-sender-log'
    receiver_log_name = os.environ["ALPHARTC_HOME"] + 'webrtc-receiver-log'
    sender_log_rename = os.environ["ALPHARTC_HOME"] + f'{RL_ALGO}/sender-logs/webrtc-sender-{RL_ALGO}-{LINK_BW}'
    receiver_log_rename = os.environ["ALPHARTC_HOME"] + f'{RL_ALGO}/receiver-logs/webrtc-receiver-{RL_ALGO}-{LINK_BW}'

    # Rename log files
    os.rename(sender_log_name, sender_log_rename)
    os.rename(receiver_log_name, receiver_log_rename)


def main():
    cleanup()

    link_bandwidth='300kbps'
    delay=0

    env, policy = PolicyFactory(episode_len=64).create_policy(RL_ALGO)
    env.set_policy(policy)
    env.set_bw_delay(link_bandwidth, delay)
    env.start_call()

    # Save WebRTC logs and plot results
    save_webrtc_logs()
    # plot_recv_thp()


if __name__ == "__main__":
    main()
