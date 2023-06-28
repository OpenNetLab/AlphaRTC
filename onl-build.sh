#!/bin/bash

if [ ! -d "out" ]
then
	mkdir out
	mkdir out/Default
	# Do gclient sync only for initial build,
	# as it takes a long time to do for every build
	gclient sync
	mv -fvn src/* .
	rm -rf src
	sudo apt install ninja-build
fi


# Build AlphaRTC.
gn gen out/Default --args='is_debug=false'

# Build AlphaRTC e2e app (peerconnection_serverless).
# (check ninja project files for peerconnection_serverless executable under ./examples/BUILD.gn)
ninja -C out/Default peerconnection_serverless
cp out/Default/peerconnection_serverless peerconnection_serverless
