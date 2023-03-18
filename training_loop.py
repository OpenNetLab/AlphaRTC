#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import emoji
import glob
import os
import subprocess
import time


def cleanup_logs():
    for f in glob.glob("*.log"):
        os.remove(f)

def record_call_result(receiver_app, sender_app, ith_call):
    # Log whether the call ended successfully
    call_result = ''
    if receiver_app.returncode == 0 and sender_app.returncode == 0:
        call_result = f'Call {ith_call} finished successfully! :smile:\n'
    else:
        call_result = f'Call {ith_call} finished with errors! :sob: \
            receiver\'s return code {receiver_app.returncode} \
            sender\'s return code {sender_app.returncode}\n'

    with open("call_result.log", "a+") as out:
            out.write(emoji.emojize(call_result, language='alias'))


# TODO: Training until convergence
# (train sufficiently long, and measure the test rewards to see whether it converged)
def main():
    # `peerconnection_serverless.origin` is an e2e WebRTC app built by examples/BUILD.gn
    # Run this e2e app on separate processes in parallel
    # with AlphaCC config file as an argument (e.g. receiver_pyinfer.json)
    receiver_cmd = f"$ALPHARTC_HOME/peerconnection_serverless.origin receiver_pyinfer.json"
    # sender_cmd = f"sleep 5; mm-loss uplink 0.2 mm-link 12mbps 12mbps $ALPHARTC_HOME/peerconnection_serverless.origin sender_pyinfer.json"
    # encapsulate `peerconnection_serverless.origin sender_pyinfer.json` with a python training code
    sender_cmd = f"sleep 5; mm-loss uplink 0.2 mm-link traces/12mbps traces/12mbps python $ALPHARTC_HOME/rl_agent_wrapper.py"
    num_calls = 1

    cleanup_logs()

    for i in range(0, num_calls):
        # Run a single e2e call
        receiver_app = subprocess.Popen(receiver_cmd, shell=True)
        sender_app = subprocess.Popen(sender_cmd, shell=True)

        receiver_app.wait()
        sender_app.wait()

        record_call_result(receiver_app, sender_app, i)

        time.sleep(5)



if __name__ == "__main__":
    main()
