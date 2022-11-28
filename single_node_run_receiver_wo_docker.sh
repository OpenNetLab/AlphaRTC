#!/bin/bash

#echo -e "Removing logs and intermediate files from the previous run\n"
rm -rf bwe.txt

#echo -e "Compiling (without docker)\n"
#gn gen out/Default --args='is_debug=false'
#ninja -C out/Default peerconnection_serverless

#echo -e "\n\nRunning the receiver...\n\n"
# Executing the C++ WebRTC app directly,
# as the receiver doesn't need a python program to interact with OpenAI Baselines.
#cp out/Default/peerconnection_serverless peerconnection_serverless.origin

./peerconnection_serverless.origin receiver_pyinfer.json 2>&1 | tee receiver_stdout.log

