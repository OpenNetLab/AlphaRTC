#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
import subprocess
import traceback
import json
import time

import rl_training.cmdtrain as cmdtrain

# `peerconnection_serverless` python program for running e2e call without docker.

def main():
    # `peerconnection_serverless.origin` is an e2e WebRTC app built by examples/BUILD.gn
    # Run this e2e app on a separate process,
    # with AlphaCC config file as an argument (e.g. receiver_pyinfer.json)
    e2e_app = os.path.join(os.path.dirname(__file__), 'peerconnection_serverless.origin')
    app = subprocess.Popen(
        [e2e_app] + sys.argv[1:],
        bufsize=1,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT)
    try:
        # TODO: Move this measurement inside conductor.cc
        print("Starting the training loop (cmdtrain.main)")
        start = time.time()
        # This e2e app's stdout and stdin are piped to cmdtrain.main
        cmdtrain.main(app.stdout, app.stdin)
        print("Finished the training loop (cmdtrain.main)")
        end = time.time()
        print(f'Completion time: {end - start} sec')
        app.wait()
    except:
        app.terminate()
        app.wait()
        error_message = traceback.format_exc()
        error_message = "\n{}".format(error_message)
        sys.stderr.write(error_message)
        if len(sys.argv[1:]) == 0:
            return
        config_file = sys.argv[1]
        config_file = json.load(open(config_file, "r"))
        if "logging" not in config_file:
            return
        if "enabled" not in config_file["logging"] or not config_file["logging"]["enabled"]:
            return
        with open(config_file["logging"]["log_output_path"], "a") as log_file:
            log_file.write(error_message)


if __name__ == "__main__":
    main()
