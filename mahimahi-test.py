#!/usr/bin/env python3
import os
import time
import signal
import subprocess

def main():
    # enable IP forwarding
    subprocess.run('sudo sysctl -w net.ipv4.ip_forward=1', shell=True)
    # run sender
    sender_cmd = 'iperf3 -s -p 12345'
    sender_proc = subprocess.Popen(sender_cmd, shell=True, preexec_fn=os.setsid)
    print(f'Running: {sender_cmd}')
    # wait for the sender to start up
    # time.sleep(1)

    # # run receiver inside Mahimahi shells
    # "mm-delay 10 mm-link traces/12mbps traces/12mbps -- sh -c 'iperf3 -c $MAHIMAHI_BASE -p 12345 -t 10'""
    receiver_cmd = ("mm-link traces/400kbps traces/400kbps -- sh -c " "'iperf3 -c $MAHIMAHI_BASE -p 12345 -t 10'")
    receiver_proc = subprocess.Popen(receiver_cmd, shell=True, preexec_fn=os.setsid)
    print(f'Running: {receiver_cmd}')
    # run 10 seconds
    time.sleep(10)

    # force clean up
    sender_proc.kill()
    receiver_proc.kill()
    os.killpg(os.getpgid(sender_proc.pid), signal.SIGKILL)
    os.killpg(os.getpgid(receiver_proc.pid), signal.SIGKILL)

if __name__ == '__main__':
    try:
        main()
    finally:
        subprocess.run('pkill iperf3', shell=True)
