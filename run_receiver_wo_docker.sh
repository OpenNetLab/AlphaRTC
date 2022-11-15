echo "Removing webrtc logs from the previous run"
rm *.log
echo "Recompiling (make all)"
make all
echo "Running the receiver..."
./setup_run_wo_docker.sh && python peerconnection_serverless_wo_docker.py receiver_pyinfer.json

