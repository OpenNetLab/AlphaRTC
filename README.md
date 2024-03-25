# AlphaRTC

<table>
    <tr>
        <td align="center">main</td>
        <td align="center">
            <a href="https://dev.azure.com/OpenNetLab/ONL-github/_build/latest?definitionId=3&branchName=main">
                <img src="https://dev.azure.com/OpenNetLab/ONL-github/_apis/build/status/OpenNetLab.AlphaRTC?branchName=main">
            </a>
        </td>
    </tr>
    <tr>
        <td align="center">dev</td>
        <td align="center">
            <a href="https://dev.azure.com/OpenNetLab/ONL-github/_build/latest?definitionId=3&branchName=dev">
                <img src="https://dev.azure.com/OpenNetLab/ONL-github/_apis/build/status/OpenNetLab.AlphaRTC?branchName=dev">
            </a>
        </td>
    </tr>
    <tr>
        <td align="center">issues</td>
        <td align="center">
            <a href="https://github.com/OpenNetLab/AlphaRTC/issues">
                <img src="https://img.shields.io/github/issues-raw/OpenNetLab/AlphaRTC">
            </a>
        </td>
    </tr>
    <tr>
        <td align="center">commits</td>
        <td align="center">
            <a href="https://github.com/OpenNetLab/AlphaRTC/graphs/commit-activity">
                <img src="https://img.shields.io/github/commit-activity/m/OpenNetLab/AlphaRTC">
            </a>
        </td>
    </tr>
</table>


## Motivation

AlphaRTC is a fork of Google's WebRTC project using ML-based bandwidth estimation, delivered by the OpenNetLab team. By equipping WebRTC with a more accurate bandwidth estimator, our mission is to eventually increase the quality of transmission.

AlphaRTC replaces Google Congestion Control (GCC) with two customized congestion control interfaces, PyInfer and ONNXInfer. The PyInfer provides an opportunity to load external bandwidth estimator written by Python. The external bandwidth estimator could be based on ML framework, like PyTorch or TensorFlow, or a pure Python algorithm without any dependencies. And the ONNXInfer is an ML-powered bandwidth estimator, which takes in an ONNX model to make bandwidth estimation more accurate. ONNXInfer is proudly powered by Microsoft's [ONNXRuntime](https://github.com/microsoft/onnxruntime).

If you are preparing a publication and need to introduce OpenNetLab or AlphaRTC, kindly consider citing the following paper:

```latex
@inproceedings{eo2022opennetlab,
  title={Opennetlab: Open platform for rl-based congestion control for real-time communications},
  author={Eo, Jeongyoon and Niu, Zhixiong and Cheng, Wenxue and Yan, Francis Y and Gao, Rui and Kardhashi, Jorina and Inglis, Scott and Revow, Michael and Chun, Byung-Gon and Cheng, Peng and Xiong, Yongqiang},
  booktitle={Proceedings of the 6th Asia-Pacific Workshop on Networking},
  pages={70--75},
  year={2022}
}
```



## Environment

**We recommend you directly fetch the pre-provided Docker images from `opennetlab.azurecr.io/alphartc` or [Github release](https://github.com/OpenNetLab/AlphaRTC/releases/latest/download/alphartc.tar.gz)**

### From docker registry

``` bash
docker pull opennetlab.azurecr.io/alphartc
docker image tag opennetlab.azurecr.io/alphartc alphartc
```

### From github release

``` bash
wget https://github.com/OpenNetLab/AlphaRTC/releases/latest/download/alphartc.tar.gz
docker load -i alphartc.tar.gz
```

Ubuntu 18.04 or 20.04 is the only officially supported distro at this moment. For other distros, you may be able to compile your own binary, or use our pre-provided Docker images.

## Compilation

### Option 1: Docker (recommended)

To compile AlphaRTC, please refer to the following steps

1. Prerequisites

   Make sure Docker is installed on your system and add user to docker group.

   ``` shell
   # Install Docker
   curl -fsSL get.docker.com -o get-docker.sh
   sudo sh get-docker.sh
   sudo usermod -aG docker ${USER}
   ```

2. Clone the code

   ``` shell
   git clone https://github.com/OpenNetLab/AlphaRTC.git
   ```

3. Build Docker images

   ``` shell
   cd AlphaRTC
   make all
   ```

   You should then be able to see two Docker images, `alphartc` and `alphartc-compile` using `sudo docker images`

### Option 2: Compile from Scratch

If you don't want to use Docker, or have other reasons to compile from scratch (e.g., you want a native Windows build), you may use this method.

Note: all commands below work for both Linux (sh) and Windows (pwsh), unless otherwise specified

1. Grab essential tools

    You may follow the guide [here](https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html#_setting_up) to obtain a copy of `depot_tools`

2. Clone the repo

    ```shell
    git clone https://github.com/OpenNetLab/AlphaRTC.git
    ```

3. Sync the dependencies

    ```shell
    cd AlphaRTC
    gclient sync
    mv src/* .
    ```

4. Generate build rules

    _Windows users_: Please use __x64 Native Tools Command Prompt for VS2017__. The clang version comes with the project is 9.0.0, hence incompatible with VS2019. In addition, environmental variable `DEPOT_TOOLS_WIN_TOOLSCHAIN` has to be set to `0` and `GYP_MSVS_VERSION` has to be set to `2017`.

    ```shell
    gn gen out/Default
    ```

5. Compile

    ```shell
    ninja -C out/Default peerconnection_serverless
    ```

    For Windows users, we also provide a GUI version. You may compile it via

    ```shell
    ninja -C out/Default peerconnection_serverless_win_gui
    ```

## Demo

AlphaRTC consists of many different components. `peerconnection_serverless` is an application for demo purposes that comes with AlphaRTC. It establishes RTC communication with another peer without the need of a server.

In order to run the application, you will need a configuration file in json format. The details are explained in the next chapter.

In addition to the config file, you will also need other files, such as video/audio source files and an ONNX model.

To run an AlphaRTC instance, put the config files in a directory, e.g., `config_files`, then mount it to an endpoint inside `alphartc` container

``` shell
sudo docker run -v config_files:/app/config_files alphartc peerconnection_serverless /app/config_files/config.json
```

Since `peerconnection_serverless` needs two peers, you may spawn two instances (a receiver and a sender) in the same network and make them talk to each other. For more information on Docker networking, check [Docker Networking](https://docs.docker.com/network/network-tutorial-standalone/)

### Configurations for *peerconnection_serverless*

This section describes required fields for the json configuration file.

- **serverless_connection**
  - **sender**
    - **enabled**: If set to `true`, the client will act as sender and automatically connect to receiver when launched
    - **send_to_ip**: The IP of serverless peerconnection receiver 
    - **send_to_port**: The port of serverless peerconnection receiver
  - **receiver**
    - **enabled**: If set to `true`, the client will act as receiver and wait for sender to connect.
    - **listening_ip**: The IP address that the socket in receiver binds and listends to
    - **listening_port**: The port number that the socket in receiver binds and listends to
  - **autoclose**: The time *in seconds* before close automatically (always run if autoclose=0)

  ***Note: one and only one of `sender.enabled` and `receiver.enabled` has to be `true`. I.e., `sender.enabled` XOR `receiver.enabled`***

- **bwe_feedback_duration**: The duration the receiver sends its estimated target rate every time(*in millisecond*)

- **video_source**
  - **video_disabled**:
    - **enabled**: If set to `true`, the client will not take any video source as input
  - **webcam**:
    - **enabled**: __Windows-only__. If set to `true`, then the client will use the web camera as the video source. For Linux, please set to `false`
  - **video_file**:
    - **enabled**: If set to `true`, then the client will use a video file as the video source
    - **height**: The height of the input video
    - **width**: The width of the input video
    - **fps**: The frames per second (FPS) of the input video
    - **file_path**: The file path of the input video in [YUV](https://en.wikipedia.org/wiki/YUV) format
  - **logging**:
    - **enabled**: If set to `true`, the client will write log to the file specified
    - **log_output_path**: The out path of the log file

  ***Note: one and only one of `video_source.webcam.enabled` and `video_source.video_file.enabled` has to be `true`. I.e., `video_source.webcam.enabled` XOR `video_source.video_file.enabled`***

- **audio_source**
  - **microphone**:
    - **enabled**: Whether to enable microphone output or not
  - **audio_file**:
    - **enabled**: Whether to enable audio file input or not
    - **file_path**: The file path of the input audio file in WAV format

- **save_to_file**
  - **enabled**: Whether to enable file saving or not
  - **audio**:
    - **file_path**: The file path of the output audio file in WAV format
  - **video**
    - **width**: The width of the output video file
    - **height**: The height of the output video file
    - **fps**: Frames per second of the output video file
    - **file_path**: The file path of the output video file in YUV format

#### Use PyInfer or ONNXInfer

##### PyInfer

The default bandwidth estimator is PyInfer, You should implement your Python class named `Estimator` with required methods `report_states` and `get_estimated_bandwidth` in Python file `BandwidthEstimator.py` and put this file in your workspace.
There is an example of Estimator with fixed estimated bandwidth 1Mbps. Here is an example [BandwidthEstimator.py](examples/peerconnection/serverless/corpus/BandwidthEstimator.py).

```python
class Estimator(object):
    def report_states(self, stats: dict):
        '''
        stats is a dict with the following items
        {
            "send_time_ms": uint,
            "arrival_time_ms": uint,
            "payload_type": int,
            "sequence_number": uint,
            "ssrc": int,
            "padding_length": uint,
            "header_length": uint,
            "payload_size": uint
        }
        '''
        pass

    def get_estimated_bandwidth(self)->int:
        return int(1e6) # 1Mbps

```

##### ONNXInfer

If you want to use the ONNXInfer as the bandwidth estimator, you should specify the path of onnx model in the config file. Here is an example configuration [receiver.json](examples/peerconnection/serverless/corpus/receiver.json)

- **onnx**
  - **onnx_model_path**: The path of the [onnx](https://www.onnxruntime.ai/) model

#### Run peerconnection_serverless

- Dockerized environment

    To better demonstrate the usage of peerconnection_serverless, we provide an all-inclusive corpus in `examples/peerconnection/serverless/corpus`. You can use the following commands to execute a tiny example. After these commands terminates, you will get `outvideo.yuv` and `outaudio.wav`.

    PyInfer:

    ```shell
    sudo docker run -d --rm -v `pwd`/examples/peerconnection/serverless/corpus:/app -w /app --name alphartc alphartc peerconnection_serverless receiver_pyinfer.json
    sudo docker exec alphartc peerconnection_serverless sender_pyinfer.json
    ```

    ONNXInfer:

    ``` shell
    sudo docker run -d --rm -v `pwd`/examples/peerconnection/serverless/corpus:/app -w /app --name alphartc alphartc peerconnection_serverless receiver.json
    sudo docker exec alphartc peerconnection_serverless sender.json
    ```

- Bare metal

    If you compiled your own binary, you can also run it on your bare-metal machine.

    - Linux users:
        1. Copy the provided corpus to a new directory

            ```shell
            cp -r examples/peerconnection/serverless/corpus/* /path/to/your/runtime
            ```

        2. Copy the essential dynanmic libraries and add them to searching directory

            ```shell
            cp modules/third_party/onnxinfer/lib/*.so /path/to/your/dll
            export LD_LIBRARY_PATH=/path/to/your/dll:$LD_LIBRARY_PATH
            ```

        3. Start the receiver and the sender

            ```shell
            cd /path/to/your/runtime
            /path/to/alphartc/out/Default/peerconnection ./receiver.json
            /path/to/alphartc/out/Default/peerconnection ./sender.json
            ```

    - Windows users:
        1. Copy the provided corpus to a new directory

            ```shell
            cp -Recursive examples/peerconnection/serverless/corpus/* /path/to/your/runtime
            ```

        2. Copy the essential dynanmic libraries and add them to searching directory

            ```shell
            cp modules/third_party/onnxinfer/bin/*.dll /path/to/your/dll
            set PATH=/path/to/your/dll;%PATH%
            ```

        3. Start the receiver and the sender

            ```shell
            cd /path/to/your/runtime
            /path/to/alphartc/out/Default/peerconnection ./receiver.json
            /path/to/alphartc/out/Default/peerconnection ./sender.json
            ```

## Who Are We

The OpenNetLab is an open-networking research community. Our members are from Microsoft Research Asia, Tsinghua Univeristy, Peking University, Nanjing University, KAIST, Seoul National University, National University of Singapore, SUSTech, Shanghai Jiaotong Univerisity. 

## WebRTC

You can find the Readme of the original WebRTC project [here](./README.webrtc.md)
