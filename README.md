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

## AlphaRTC

AlphaRTC is an evaluation framework for ML-based bandwidth estimation in real-time communications based on [WebRTC](https://webrtc.googlesource.com/), delivered by the OpenNetLab team. Our mission is to facilitate data-driven, ML-based bandwidth estimation research that learns to improve the quality of experience (QoE) in diverse use cases of real-time communications, e.g. video conferencing.

Users can plug in custom bandwidth estimators with AlphaRTC's interfaces for Python-based and [ONNX](https://github.com/microsoft/onnxruntime)-based model checkpoints. By using the interfaces, the trained model checkpoint is used for bandwidth estimation instead of [GCC](https://dl.acm.org/doi/abs/10.1145/2910017.2910605)(Google Congestion Control), a default bandwidth estimation module of WebRTC.


## Installation


``` shell
# Install depot tools
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export DEPOT_TOOLS_HOME=/path/to/depot_tools
# NOTE: DEPOT_TOOLS_HOME should be located at the front of the PATH
export PATH=:$DEPOT_TOOLS_HOME:$PATH

# Fetch chromium
git config --global core.autocrlf false
git config --global core.filemode false
cd $DEPOT_TOOLS_HOME/src && sudo ./build/install-build-deps.sh

# Get the AlphaRTC code
git clone https://github.com/OpenNetLab/AlphaRTC.git
export ALPHARTC_HOME=/path/to/AlphaRTC
export PATH=:$PATH:$ALPHARTC_HOME
export ONNX_HOME=$ALPHARTC_HOME/modules/third_party/onnxinfer
export LD_LIBRARY_PATH=$ONNX_HOME/lib:$LD_LIBRARY_PATH
```

## Compilation

``` shell
# Compile AlphaRTC and peerconnection_serverless,
# the video call app that uses the custom bandwidth estimator
./onl-build.sh

# Compile AlphaRTC and peerconnection_serverless_gcc,
# the video call app that uses GCC, the default bandwidth estimator of WebRTC
git checkout gcc
./gcc-build.sh
```

## Evaluation

- Evaluate the bandwidth estimation by running end-to-end video call app, `peerconnection_serverless`. It makes quick and easy testing of the bandwidth estimator, as it establishes RTC communication with a peer without the need of a signaling server.
- In case of running an app with GCC, use `peerconnection_serverless_gcc` instead of `peerconnection_serverless` following the [compilation guideline](#compilation)
- Results will be saved as `receiver-*.log` and `sender-*.log`.

``` shell
## Running a call with custom bandwidth estimator:
# Receiver side (Note: receiver should be running first)
python peerconnection_serverless.py receiver_360p.json
# Sender side
python peerconnection_serverless.py sender_360p.json

## Running a call with GCC:
# Receiver side (Note: receiver should be running first)
./peerconnection_serverless_gcc receiver_360p.json
# Sender side
./peerconnection_serverless_gcc sender_360p.json
```

- Check the call quality with receiver- and sender-side QoE statistics in the logs. Below are example statistics from `receiver-360p.log` and `sender-360p.log` when running a video call app with 360p video at fps=25:

``` shell
# Example receiver-side statistics
WebRTC.Video.ReceivedWidthInPixels 575
WebRTC.Video.ReceivedHeightInPixels 323
WebRTC.Video.MediaBitrateReceivedInKbps 1350
WebRTC.Video.HarmonicFrameRate 24

# Example sender-side statistics
WebRTC.Video.SentWidthInPixels 584
WebRTC.Video.SentHeightInPixels 329
WebRTC.Video.MediaBitrateSentInBps periodic_samples:14, {min:434864, avg:1301272, max:1535160}
WebRTC.Video.SentFramesPerSecond periodic_samples:14, {min:25, avg:25, max:26}
```

## Using Docker

### Getting the docker image

- We recommend you directly fetch the pre-provided Docker images from `opennetlab.azurecr.io/alphartc` or [Github release](https://github.com/OpenNetLab/AlphaRTC/releases/latest/download/alphartc.tar.gz)
- Supported distros: Ubuntu 18.04 or 20.04


``` shell
# From docker registry:
docker pull opennetlab.azurecr.io/alphartc
docker image tag opennetlab.azurecr.io/alphartc alphartc

# From github release:
wget https://github.com/OpenNetLab/AlphaRTC/releases/latest/download/alphartc.tar.gz
docker load -i alphartc.tar.gz
```

### Compilation

``` shell
# Install Docker
curl -fsSL get.docker.com -o get-docker.sh
sudo sh get-docker.sh
sudo usermod -aG docker ${USER}

# Get the AlphaRTC code
git clone https://github.com/OpenNetLab/AlphaRTC.git

# Build Docker images
cd AlphaRTC
make all

# Should show `alphartc` and `alphartc-compile`
docker images
```

### Evaluation

- Since the `peerconnection_serverless` needs two peers, one sender and another receiver, spawn two instances (a receiver and a sender) in the same network and make them talk to each other. For more information on Docker networking, check [Docker Networking](https://docs.docker.com/network/network-tutorial-standalone/)

``` shell
sudo docker run -v config_files:/app/config_files alphartc peerconnection_serverless /app/config_files/config.json
```

### Using PyInfer and ONNXInfer

- **PyInfer**: Implement a Python `Estimator` class with required methods `report_states` and `get_estimated_bandwidth` in `BandwidthEstimator.py` and put this file in your workspace.
- Below is a sample skeleton `BandwidthEstimator.py` with 1Mbps fixed estimated bandwidth.

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

- **ONNXInfer**: Specify the path of onnx model in the config file. Here is an example receiver-side configuration [receiver.json](examples/peerconnection/serverless/corpus/receiver.json)


### Evaluation
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

## Receiver- and Sender-side Configurations

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


## OpenNetLab Team

The OpenNetLab is an open-networking research community. Our members are from Microsoft Research Asia, Tsinghua Univeristy, Peking University, Nanjing University, KAIST, Seoul National University, National University of Singapore, SUSTech, Shanghai Jiaotong Univerisity.