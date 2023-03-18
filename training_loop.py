#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import emoji
import glob
import json
import os
import subprocess
import sys
import time
import traceback


import rl_training.rl_agent as rl_agent

# `peerconnection_serverless` python program for running e2e call without docker.
# Run only on the sender side. Refer to `single_node_run_sender_wo_docker.sh`.

def run_e2e_call():
     # Receiver command (should start first)
    receiver_stdout_path = "receiver_stdout.log"
    receiver_stderr_path = "receiver_stderr.log"
    receiver_cmd = f"$ALPHARTC_HOME/peerconnection_serverless.origin receiver_pyinfer.json"
    print(f'Receiver command: {receiver_cmd}')

    # Sender command
    sender_stdout_path = "mahimahi_sender_stdout.log"
    sender_stderr_path = "mahimahi_sender_stderr.log"
    sender_cmd = f"mm-loss uplink 0.2 mm-link 12mbps 12mbps $ALPHARTC_HOME/peerconnection_serverless.origin sender_pyinfer.json"
    print(f'Sender command: {sender_cmd}')

    receiver_app = subprocess.Popen(receiver_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    sender_app = subprocess.Popen(sender_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)

    # receiver_app.wait()
    # sender_app.wait()

    print(f'Running the receiver...')
    (recv_stdout, recv_stderr) = receiver_app.communicate()
    print(f'Running the sender...')
    (stdout, stderr) = sender_app.communicate()

    print(recv_stdout)
    print(recv_stderr)
    print(stdout)
    print(stderr)

    with open(sender_stdout_path, "w") as out, open(sender_stderr_path, "w") as err:
        out.write(stdout)
        err.write(stderr)

    with open(receiver_stdout_path, "w") as out, open(receiver_stderr_path, "w") as err:
        out.write(recv_stdout)
        err.write(recv_stderr)

    print(emoji.emojize('Finished the call! :grin:', language='alias', variant="emoji_type"))

def cleanup_logs():
    for f in glob.glob("*.log"):
        os.remove(f)

# TODO: Training until convergence
# (train sufficiently long, and measure the test rewards to see whether it converged)

def main():
    # `peerconnection_serverless.origin` is an e2e WebRTC app built by examples/BUILD.gn
    # Run this e2e app on a separate process,
    # with AlphaCC config file as an argument (e.g. receiver_pyinfer.json)

    cleanup_logs()

    receiver_cmd = f"$ALPHARTC_HOME/peerconnection_serverless.origin receiver_pyinfer.json"
    # sender_cmd = f"sleep 5; mm-loss uplink 0.2 mm-link 12mbps 12mbps $ALPHARTC_HOME/peerconnection_serverless.origin sender_pyinfer.json"
    # encapsulate `peerconnection_serverless.origin sender_pyinfer.json` with a python training code
    sender_cmd = f"sleep 5; mm-loss uplink 0.2 mm-link 12mbps 12mbps $ALPHARTC_HOME/rl_training/rl_agent.py"
    num_calls = 5

    for i in range(0, num_calls):
        # Run a single e2e call
        receiver_app = subprocess.Popen(receiver_cmd, shell=True)
        sender_app = subprocess.Popen(sender_cmd, shell=True)

        receiver_app.wait()
        sender_app.wait()

        # Log whether the call ended successfully
        call_result = ''
        if receiver_app.returncode == 0 and sender_app.returncode == 0:
            call_result = f'Call {i} finished successfully! :smile:\n'
        else:
            call_result = f'Call {i} finished with errors! :sob: \
                receiver\'s return code {receiver_app.returncode} \
                sender\'s return code {sender_app.returncode}\n'

        with open("call_result.log", "a+") as out:
                out.write(emoji.emojize(call_result, language='alias'))

        time.sleep(5)

    # e2e_app = os.path.join(os.path.dirname(__file__), 'peerconnection_serverless.origin')
    # args = 'sender_pyinfer.json'
    # app = subprocess.Popen(
    #     [e2e_app] + [args],
    #     bufsize=1,
    #     stdin=subprocess.PIPE,
    #     stdout=subprocess.PIPE,
    #     stderr=subprocess.STDOUT)
    # try:
    #     print("Starting the training loop (rl_agent.main)")
    #     start = time.time()
    #     # This e2e app's stdout and stdin are piped to rl_agent.main
    #     num_steps += rl_agent.main(app.stdout, app.stdin)
    #     print(f'\n\n\nFinished the training loop (rl_agent.main). Total trained steps {num_steps}\n\n\n')
    #     end = time.time()
    #     print(f'Completion time: {end - start} sec')
    #     app.wait()
    # except:
    #     app.terminate()
    #     app.wait()
    #     error_message = traceback.format_exc()
    #     error_message = "\n{}".format(error_message)
    #     sys.stderr.write(error_message)
    #     if len(sys.argv[1:]) == 0:
    #         return
    #     config_file = sys.argv[1]
    #     config_file = json.load(open(config_file, "r"))
    #     if "logging" not in config_file:
    #         return
    #     if "enabled" not in config_file["logging"] or not config_file["logging"]["enabled"]:
    #         return
    #     with open(config_file["logging"]["log_output_path"], "a") as log_file:
    #         log_file.write(error_message)
    # time.sleep(10)


if __name__ == "__main__":
    main()
