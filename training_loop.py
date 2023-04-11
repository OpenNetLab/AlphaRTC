import glob
import os
from statistics import mean, median
import matplotlib.pyplot as plt
import numpy as np
import random
import subprocess
import sys

from rl_training.rl_policy import PolicyFactory
from multiprocessing import Process

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


def fetch_stats(line):
    line = line.strip()
    if 'AlphaCC: SendState' in line:
        lrdr = line.split(',')
        stats_dict = {}
        stats_dict['loss_rate'] = float(lrdr[1].split(' ')[1])
        stats_dict['rtt'] = float(lrdr[2].split(' ')[1])
        stats_dict['delay_interval'] = float(lrdr[3].split(' ')[1])
        stats_dict['recv_thp'] = float(lrdr[4].split(' ')[1])
        print(f'fetch_stats:<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< {stats_dict}')
        return stats_dict
    else:
        return None

def relay_packet_stats(ifd, ofd, env):
    while True:
        # Read from sender_app.stdout
        line = ifd.readline()
        if not line:
            break
        if isinstance(line, bytes):
            line = line.decode("utf-8")

        # Extract packet statistics
        stats_dict = fetch_stats(line)
        if stats_dict:
            print(f'relay_packet_stats:================================= {stats_dict}')
            # env.packet_record.add_loss_rate(stats_dict['loss_rate'])
            # env.packet_record.add_rtt(stats_dict['rtt'])
            # env.packet_record.add_delay_interval()
            # env.packet_record.add_receiver_side_thp(stats_dict['recv_thp'])
            continue

        sys.stdout.write(f'{line}')
        sys.stdout.flush()

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

    link_bandwidth='300kbps'
    delay=0

    env, policy = PolicyFactory(episode_len=64).create_policy(rl_algo=RL_ALGO)

    # Step 1. Run the video call (env) in separate processes
    # - sender runs in a dedicated process
    # - receiver runs in a dedicated process
    # - packet stats from sender are relayed to the `env`
    # call_wrapper.run_call(env, link_bandwidth, delay)
    receiver_cmd = f"$ALPHARTC_HOME/peerconnection_serverless.origin receiver_pyinfer.json"
    sender_cmd = f"sleep 5; mm-link traces/{link_bandwidth} traces/{link_bandwidth} $ALPHARTC_HOME/peerconnection_serverless.origin sender_pyinfer.json"

    # Randomly assign different port for this video call
    generate_random_port()

    # Run the video call via subprocesses
    receiver_app = subprocess.Popen(receiver_cmd, shell=True)
    sender_app = subprocess.Popen(sender_cmd, bufsize=1, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
    print(f'Video call env started: link BW {link_bandwidth}, one-way delay {delay}ms')

    # Run the policy training (`policy.learn(total_timesteps=640)`) via multiprocessing
    p1 = Process(target=policy.learn, args=(64000, ))
    # Runs in parallel with the above video call and policy training
    # relay_packet_stats(sender_app.stdout, sender_app.stdin, env)
    p2 = Process(target=relay_packet_stats, args=(sender_app.stdout, sender_app.stdin, env))

    p1.start()
    print(f'Policy training started')
    p2.start()
    print(f'relay_packet_stats started')

    receiver_app.wait()
    sender_app.wait()

    p1.join() # this blocks until the process terminates
    p2.join()

    check_call_result(receiver_app, sender_app)
    print('run_call: ended')

    # Save WebRTC logs and plot results
    save_webrtc_logs()
    # plot_recv_thp()


if __name__ == "__main__":
    main()
