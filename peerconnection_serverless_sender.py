#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import subprocess

# `peerconnection_serverless` python program for running e2e call without docker.

def main():
    # `peerconnection_serverless.origin` is an e2e WebRTC app built by examples/BUILD.gn
    # Run this e2e app on a separate process,
    # with AlphaCC config file as an argument (e.g. receiver_pyinfer.json)
    script_path = os.path.join(os.path.dirname(__file__), 'single_node_run_sender_wo_docker.sh')
    subprocess.run(script_path, shell=True)


if __name__ == "__main__":
    main()
