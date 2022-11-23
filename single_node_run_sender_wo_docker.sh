#!/bin/bash

echo -e "\n\nRunning the sender...\n\n"
python peerconnection_serverless_wo_docker.py sender_pyinfer.json 2>&1 | tee sender_stdout.log

