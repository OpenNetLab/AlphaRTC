# AlphaRTC

## Motivation
AlphaRTC is a fork of Google's WebRTC project using ML-based bandwidth estimation, delivered by the OpenNetLab team. By equipping WebRTC with a more accurate bandwidth estimator, our mission is to eventually increase the quality of transmission.

AlphaRTC replaces Google Congestion Control (GCC) with ONNXInfer, an ML-powered bandwidth estimator, which takes in an ONNX model to make bandwidth estimation more accurate. ONNXInfer is proudly powered by Microsoft's [ONNXRuntime](https://github.com/microsoft/onnxruntime).

## Environment
Ubuntu 18.04 is the only officially supported distro at this moment. For other distros, you may be able to compile your own binary, or use our pre-provided Docker images.

## Compilation

To compile AlphaRTC, please refer to the following steps

1. Prerequisites

   Make sure Docker is installed on your system.

   ```
   # Install Docker
   curl -fsSL get.docker.com -o get-docker.sh
   sudo sh get-docker.sh
   ```

2. Clone the code
   ```
   git clone https://github.com/OpenNetLab/AlphaRTC.git
   ```

3. Build Docker images
   ```
   cd AlphaRTC
   sudo make all
   ```
 
   You should then be able to see two Docker images, `alphartc` and `alphartc-compile` using `sudo docker images`
 
## Demo
AlphaRTC consists of many different components. `peerconnection_serverless` is an application for demo purposes that comes with AlphaRTC. It establishes RTC communication with another peer without the need of a server.

In order to run the application, you will need a configuration file in json format. The details are explained in the next chapter.

In addition to the config file, you will also need other files, such as video/audio source files and an ONNX model.

To run an AlphaRTC instance, put the config files in a directory, e.g., `config_files`, then mount it to an endpoint inside `alphartc` container

```
sudo docker run -v config_files:/app/config_files alphartc peerconnection_serverless /app/config_files/config.json
```

Since `peerconnection_serverless` needs two peers, you may spawn two instances (a receiver and a sender) in the same network and make them talk to each other. For more information on Docker networking, check [Docker Networking](https://docs.docker.com/network/network-tutorial-standalone/)

### Configurations for *peerconnection_serverless*
This section describes required fields for the json configuration file.

- **server_connection**
  - **ip**: The IP of peerconnection_server.
  - **port**: The port of peerconnection_server.
  - **autoconnect**: Connect to the server without user intervention.
  - **autocall**: Call the first available other client without user intervention. *Note: Only one of two clients could set it to true, otherwise both two clients will block*.
  - **autoclose**: The time *in seconds* before close automatically (always run if autoclose=0)

- **bwe_feedback_duration**
The duration the receiver sends its estimated target rate every time(*in millisecond*).

- **onnx**
  - **onnx_model_path**: The path of the onnx model.

- **video_source**
  - **video_disabled**:
    - enabled: If set to `true`, the client will not take any video source as input.
  - **webcam**:
    - enabled: If set to `true`, then the client will use the web camera as the video source

- **save_to_file**
  - enabled: Whether to enable file saving or not
  - **audio**:
      - file_path: The file path of the output audio file
  - **video**
      - width: The width of the output video file
      - height: The height of the output video file
      - fps: Frames per second of the output video file
      - file_path: The file path of the output video file

#### Example

```
{
    "server_connection": {
        "ip": "127.0.0.1",
        "port": 8888,
        "autoconnect": true,
        "autocall": false,
        "autoclose": 20
    },

    "serverless_connection": {
        "sender": {
            "enabled": false,
            "dest_ip": "127.0.0.1",
            "dest_port": 8888
        },
        "receiver": {
            "enabled": true,
            "listening_ip": "0.0.0.0",
            "listening_port": 8888
        }
    },

    "bwe_feedback_duration": 200,

    "onnx": {
        "onnx_model_path": "onnx-model.onnx"
    },

    "video_source":{
        "video_disabled": {
            "enabled": true
        },
        "webcam": {
            "enabled": false
        },
        "video_file": {
            "enabled": true,
            "height": 480,
            "width": 640,
            "fps": 24,
            "file_path": "testmedia/test.yuv"
        }
    },

    "audio_source": {
        "microphone": {
            "enabled": false
        },
        "audio_file": {
            "enabled": true,
            "file_path": "testmedia/test.wav"
        }
    },
    "save_to_file": {
        "enabled": true,
        "audio": {
            "file_path": "outaudio.wav"
        },
        "video": {
            "width": 640,
            "height": 480,
            "fps": 24,
            "file_path": "outvideo.yuv"
        }
    }
}
```
## Who Are We
The OpenNetLab team is an inter-academia research team, initiated by the Networking Reasearch Group at Microsoft Research Asia. Our team members are from different research institudes, including Peking University, Nanjing University and Nanyang Technological University.

## WebRTC
You can find the Readme of the original WebRTC project [here](./README.webrtc.md)