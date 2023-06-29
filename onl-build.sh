#!/bin/bash

if [ ! -d "out" ]
then
	mkdir -p out/Default
fi


# Build AlphaRTC.
gn gen out/Default --args='is_debug=false'

# Build AlphaRTC e2e app (peerconnection_serverless).
# (check ninja project files for peerconnection_serverless executable under ./examples/BUILD.gn)
ninja -C out/Default peerconnection_serverless
cp out/Default/peerconnection_serverless peerconnection_serverless