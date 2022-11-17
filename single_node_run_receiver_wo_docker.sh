echo -e "Removing webrtc logs from the previous run\n\n"
rm *.log
echo -e "\n\nCompiling (make all)...\n\n"
make all
echo -e "\n\nRunning the receiver...\n\n"
./setup_run_wo_docker.sh && ./peerconnection_serverless.origin receiver_pyinfer.json 2>&1 | tee receiver_stdout.log

