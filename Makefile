dockers_dir := dockers
build_dockerfile := $(dockers_dir)/Dockerfile.compile
release_dockerfile := $(dockers_dir)/Dockerfile.release
output_dir := out/Default
target_dir := target
target_lib_dir := $(target_dir)/lib
target_bin_dir := $(target_dir)/bin
target_pylib_dir := $(target_dir)/pylib

compile_docker := alphartc-compile
release_docker := alphartc

host_workdir := `pwd`
docker_homedir := /app/AlphaRTC/
docker_workdir := $(docker_homedir)

docker_flags := --rm -v $(host_workdir):$(docker_homedir) -w $(docker_workdir)
gn_flags := --args='is_debug=false'

all: init sync app release

init:
	docker build dockers --build-arg UID=$(shell id -u) --build-arg GUID=$(shell id -g) -f $(build_dockerfile) -t $(compile_docker)

release:
	docker build $(target_dir) -f $(release_dockerfile) -t $(release_docker)

sync:
	docker run $(docker_flags) $(compile_docker) \
		make docker-$@ \
		output_dir=$(output_dir) \
		gn_flags=$(gn_flags)

app: peerconnection_serverless

peerconnection_serverless:
	docker run $(docker_flags) $(compile_docker) \
		make docker-$@ \
		output_dir=$(output_dir) \
		target_lib_dir=$(target_lib_dir) \
		target_bin_dir=$(target_bin_dir) \
		target_pylib_dir=$(target_pylib_dir)

# Docker internal command

docker-sync:
	gclient sync
	mv -fvn src/* .
	rm -rf src
	gn gen $(output_dir) $(gn_flags)

docker-app: docker-peerconnection_serverless

docker-peerconnection_serverless:
	ninja -C $(output_dir) peerconnection_serverless

	mkdir -p $(target_lib_dir)
	cp modules/third_party/onnxinfer/lib/*.so $(target_lib_dir)
	cp modules/third_party/onnxinfer/lib/*.so.* $(target_lib_dir)

	mkdir -p $(target_bin_dir)
	cp $(output_dir)/peerconnection_serverless $(target_bin_dir)/peerconnection_serverless.origin
	cp examples/peerconnection/serverless/peerconnection_serverless $(target_bin_dir)

	mkdir -p $(target_pylib_dir)
	cp modules/third_party/cmdinfer/*.py $(target_pylib_dir)/
