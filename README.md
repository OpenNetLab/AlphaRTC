# AlphaRTC

## Motivation
AlphaRTC is a fork of Google's WebRTC project using ML-based bandwidth estimation, delivered by the OpenNetLab team. By equipping WebRTC with a more accurate bandwidth estimator, our mission is to eventually increase the quality of transmission.

## Demo
You can compile your own copy of `peerconnection_serverless` as a quick demo.
### **Source code**

* The source code of peerconnection application is under `alpha_rtc/src/examples/peerconnection`.
1. Get the AlphaRTC repo:

    ```
    git clone https://github.com/OpenNetLab/AlphaRTC
    ```

2. Enter rtc directory and get the src code from AlphaRTC-src and third party:

    __Compiling on Linux__: you may want change the HTTPS `url` in `alphartc/.gclient` to an SSH endpoint

    ```
    cd AlphaRTC/alpha_rtc
    gclient sync
    ```

    **Note:** It may take >10 mins to download all dependencies.

### **Build peerconnection_serverless**
Make sure to install `depot_tools`, the compilation toolkit from [here](https://dev.chromium.org/developers/how-tos/install-depot-tools).

```
cd src
gn gen out/Default
ninja -C out/Default peerconnection_serverless
```
You should then be able to find your `peerconnection_serverless` binary at `out/Default`

### **Run peerconnection_serverless**
You will need a config file (in json format), a ONNX model and a few dynamic libraries to run the binary successfully.
- `libonnxinfer.so`
- `libonnxinfercore.so`
- `libonnxruntime.so`
- `libonnxruntime.so.\d+`

You can grab them at `modules/third_party/onnxinfer/lib`. Make sure to add them to `LD_LIBRARY_PATH`.

### Configurations for *peerconnection_serverless*

#### **server_connection**
- **ip**: The IP of peerconnection_server.
- **port**: The port of peerconnection_server.
- **autoconnect**: Connect to the server without user intervention.
- **autocall**: Call the first available other client without user intervention. *Note: Only one of two clients could set it to true, otherwise both two clients will block*.
- **autoclose**: The time *in seconds* before close automatically (always run if autoclose=0)

#### **bwe_feedback_duration**
The duration the receiver sends its estimated target rate every time(*in millisecond*).

#### **onnx**
  - **onnx_model_path**: The path of the onnx model.

#### **redis**
  - **ip**: The ip of Redis Service.
  - **port**: The port of Redis Service.
  - **session_id**: The session id of Redis Service
  - **redis_update_duration**: The duration that the client collects status and push into redis(*in millisecond*).

#### **video_source**
 - **video_disabled**:
     - enabled: If set to `true`, the client will not take any video source as input.
 - **webcam**:
     - enabled: If set to `true`, then the client will use the web camera as the video source

#### **save_to_file**
  - enabled: Whether to enable file saving or not
  - **audio**:
      - file_path: The file path of the output audio file
  - **video**
      - width: The width of the output video file
      - height: The height of the output video file
      - fps: Frames per second of the output video file
      - file_path: The file path of the output video file

## Who Are We
The OpenNetLab team is an inter-academia research team, initiated by the Networking Reasearch Group at Microsoft Research Asia. Our team members are from different research institudes, including Peking University, Nanjing University and Nanyang Technological University.

## Point of Contact
In case you have any questions, you are welcomed to send an email at `zegan#microsoft.com`, `pengc#microsoft.com` or `v-zhonxia#microsoft.com`.

## WebRTC
You can find the Readme of the original WebRTC project [here](./README.webrtc.md)
