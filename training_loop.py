#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import glob
import os
import random
import subprocess
from statistics import mean, median
import matplotlib.pyplot as plt
import numpy as np

RL_ALGO='SAC'
LINK_BW='1mbps'

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

# Deprecated
def plot_training_curve():
    rewards = []
    with open(f'state_reward_action_{RL_ALGO}_{LINK_BW}', 'r') as f:
        for line in f:
            line = line.rstrip()
            l = line.split()
            rewards.append(float(l[8]))

    plt.ylim(0, max(rewards))
    plt.plot(rewards)
    plt.title(f'RL-based CC ({RL_ALGO})')
    plt.xlabel('Step')
    plt.ylabel('Training Reward')
    plt.savefig(f'learning-curve-{RL_ALGO}-{LINK_BW}.pdf')

def save_webrtc_logs():
    # Path of sender, receiver WebRTC internal logs
    sender_log_name = os.environ["ALPHARTC_HOME"] + 'webrtc-sender-log'
    receiver_log_name = os.environ["ALPHARTC_HOME"] + 'webrtc-receiver-log'
    sender_log_rename = os.environ["ALPHARTC_HOME"] + f'{RL_ALGO}/sender-logs/webrtc-sender-{RL_ALGO}-{LINK_BW}'
    receiver_log_rename = os.environ["ALPHARTC_HOME"] + f'{RL_ALGO}/receiver-logs/webrtc-receiver-{RL_ALGO}-{LINK_BW}'

    # Rename log files
    os.rename(sender_log_name, sender_log_rename)
    os.rename(receiver_log_name, receiver_log_rename)

def cleanup():
    for f in glob.glob("*.log"):
        os.remove(f)

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


def main():
    # `peerconnection_serverless.origin` is an e2e WebRTC app built by examples/BUILD.gn
    # Run this e2e app on separate processes in parallel
    # with AlphaCC config file as an argument (e.g. receiver_pyinfer.json)
    receiver_cmd = f"$ALPHARTC_HOME/peerconnection_serverless.origin receiver_pyinfer.json"
    # encapsulate `peerconnection_serverless.origin sender_pyinfer.json` with a python training code
    # sender_cmd = f"sleep 5; mm-loss uplink 0.2 mm-link traces/1mbps traces/1mbps python $ALPHARTC_HOME/rl_agent_wrapper.py"
    sender_cmd = f"sleep 5; mm-link traces/{LINK_BW} traces/{LINK_BW} python $ALPHARTC_HOME/rl_agent_wrapper.py"

    # Training environments
    # [1, 4, 10] Mbps x [10, 20, 30] ms = 9 environments
    # sender_cmd = f"sleep 5; mm-delay 10 mm-link traces/1mbps traces/1mbps python $ALPHARTC_HOME/rl_agent_wrapper.py"
    # sender_cmd = f"sleep 5; mm-delay 10 mm-link traces/4mbps traces/4mbps python $ALPHARTC_HOME/rl_agent_wrapper.py"
    # sender_cmd = f"sleep 5; mm-delay 10 mm-link traces/12mbps traces/12mbps python $ALPHARTC_HOME/rl_agent_wrapper.py"
    # sender_cmd = f"sleep 5; mm-delay 20 mm-link traces/1mbps traces/1mbps python $ALPHARTC_HOME/rl_agent_wrapper.py"
    # sender_cmd = f"sleep 5; mm-delay 20 mm-link traces/4mbps traces/4mbps python $ALPHARTC_HOME/rl_agent_wrapper.py"
    # sender_cmd = f"sleep 5; mm-delay 20 mm-link traces/12mbps traces/12mbps python $ALPHARTC_HOME/rl_agent_wrapper.py"
    # sender_cmd = f"sleep 5; mm-delay 30 mm-link traces/1mbps traces/1mbps python $ALPHARTC_HOME/rl_agent_wrapper.py"
    # sender_cmd = f"sleep 5; mm-delay 30 mm-link traces/4mbps traces/4mbps python $ALPHARTC_HOME/rl_agent_wrapper.py"
    # sender_cmd = f"sleep 5; mm-delay 30 mm-link traces/12mbps traces/12mbps python $ALPHARTC_HOME/rl_agent_wrapper.py"

    # Testing environments
    # [2, 3, 6] Mbps x [15, 25] ms = 6 environments
    # sender_cmd = f"sleep 5; mm-delay 15 mm-link traces/2mbps traces/2mbps python $ALPHARTC_HOME/rl_agent_wrapper.py"
    # sender_cmd = f"sleep 5; mm-delay 15 mm-link traces/3mbps traces/3mbps python $ALPHARTC_HOME/rl_agent_wrapper.py"
    # sender_cmd = f"sleep 5; mm-delay 15 mm-link traces/6mbps traces/6mbps python $ALPHARTC_HOME/rl_agent_wrapper.py"
    # sender_cmd = f"sleep 5; mm-delay 25 mm-link traces/2mbps traces/2mbps python $ALPHARTC_HOME/rl_agent_wrapper.py"
    # sender_cmd = f"sleep 5; mm-delay 25 mm-link traces/3mbps traces/3mbps python $ALPHARTC_HOME/rl_agent_wrapper.py"
    # sender_cmd = f"sleep 5; mm-delay 25 mm-link traces/6mbps traces/6mbps python $ALPHARTC_HOME/rl_agent_wrapper.py"


    cleanup()

    # Randomly assign different port per e2e call
    generate_random_port()

    # Run an e2e call
    receiver_app = subprocess.Popen(receiver_cmd, shell=True)
    sender_app = subprocess.Popen(sender_cmd, shell=True)

    receiver_app.wait()
    sender_app.wait()

    check_call_result(receiver_app, sender_app)

    # Save WebRTC logs and plot results
    save_webrtc_logs()
    plot_recv_thp()
    plot_training_curve()


if __name__ == "__main__":
    main()