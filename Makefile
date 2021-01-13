dockers_dir := dockers
build_dockerfile := $(dockers_dir)/Dockerfile.compile
output_dir := out/Default

compile_docker := alphartc-compile
release_docker := alphartc

host_workdir := `pwd`
docker_homedir := /app/AlphaRTC
docker_workdir := $(docker_homedir)

docker_flags := --rm -v $(host_workdir):$(docker_homedir) -w $(docker_workdir)
gn_flags := --args='is_debug=false is_component_build=false rtc_include_tests=false rtc_use_h264=true rtc_enable_protobuf=false use_rtti=true use_custom_libcxx=false use_ozone=true'

all: init sync lib

init:
	docker build dockers --build-arg UID=$(shell id -u) --build-arg GUID=$(shell id -g) -f $(build_dockerfile) -t $(compile_docker)

sync:
	docker run $(docker_flags) $(compile_docker) \
		make docker-$@ \
		output_dir=$(output_dir)

lib:
	docker run $(docker_flags) $(compile_docker) \
		make docker-$@ \
		gn_flags="$(gn_flags)" \
		output_dir=$(output_dir) \
		target_lib_dir=$(target_lib_dir) \
		target_bin_dir=$(target_bin_dir)

# Docker internal command

docker-sync:
	gclient sync
	mv -fvn src/* .
	rm -rf src

docker-lib:
	gn gen $(output_dir) $(gn_flags)
	ninja -C $(output_dir)
