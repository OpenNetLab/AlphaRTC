dockers_dir := dockers
dockerfile_compile := $(dockers_dir)/Dockerfile.compile
dockerfile_release := $(dockers_dir)/Dockerfile.release
output_dir := out/Default
target_dir := target
target_lib_dir := $(target_dir)/lib
target_bin_dir := $(target_dir)/bin
target_pylib_dir := $(target_dir)/pylib

# docker image name
image_compile := alphartc-compile
image_release := alphartc

host_workdir := `pwd`
docker_homedir := /app/AlphaRTC/
docker_workdir := $(docker_homedir)

docker_flags := --rm -v $(host_workdir):$(docker_homedir) -w $(docker_workdir)
gn_flags := --args='is_debug=false'


all: init sync app release

# Create a docker image with name $(image_compile)
init:
	docker build dockers --build-arg UID=$(shell id -u) --build-arg GUID=$(shell id -g) -f $(dockerfile_compile) -t $(image_compile)

# Build AlphaRTC inside the image
# After modifying and re-compiling the AlphaRTC, skip `make init`.
sync:
	docker run $(docker_flags) $(image_compile) \
		make docker-$@ \
		output_dir=$(output_dir) \
		gn_flags=$(gn_flags)

# Build peerconnection_serverless binary.
app:
	docker run $(docker_flags) $(image_compile) \
		make docker-$@ \
		output_dir=$(output_dir) \
		target_lib_dir=$(target_lib_dir) \
		target_bin_dir=$(target_bin_dir) \
		target_pylib_dir=$(target_pylib_dir)

release:
	cp requirements.txt $(target_dir)
	docker build $(target_dir) -f $(dockerfile_release) -t $(image_release)


clean:
	rm -rf *.log
	rm -rf outvideo.yuv outaudio.wav
	docker rmi alphartc-compile alphartc

# Docker internal command
# Removed the following three lines:
# gclient sync
# mv -fvn src/* .
# rm -rf src
docker-sync:
	gn gen $(output_dir) $(gn_flags)

# Build peerconnection_serverless AlphaRTC e2e example.
# (check ninja project files for peerconnection_serverless executable under ./examples/BUILD.gn)
docker-app:
	ninja -C $(output_dir) peerconnection_serverless
	mkdir -p $(target_lib_dir)
	mkdir -p $(target_bin_dir)
	cp $(output_dir)/peerconnection_serverless $(target_bin_dir)/peerconnection_serverless.origin
	cp examples/peerconnection/serverless/peerconnection_serverless $(target_bin_dir)
	mkdir -p $(target_pylib_dir)
	cp modules/third_party/rl_agent/*.py $(target_pylib_dir)/

