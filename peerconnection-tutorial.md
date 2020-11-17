# Peerconnection Application Tutorial

## Overview

### **Source code**

* The source code of peerconnection application is under `rtc/src/examples/peerconnection`.
### **Build peerconnection**
* Please refer to `rtc/README.md` for compiling, running and debugging the peerconnection application.
### **Running the peerconnection application**
* Make sure you have two executable files, `peerconnection_client.exe` and `peerconnection_server.exe`.
* Run the server with a specified port. The port is optional, with the default value `8888`:
    ```cmd
    >peerconnection_server.exe --port=8888
    ```
* Run the client with a file path as command line argument:  
    ```cmd
    peerconnection_client.exe ./webrtc_config_example.json
    ```
    The file should a `json-formant` file, used as the configuration file for the client. An exmaple configuration file is [here](./examples/peerconnection/client/webrtc_config_example.json).  Below we explain the configurations in detail. *Note: All fields in the configuration file are required*  

## Configurations for *peerconnection_client*

### **server_connection**
- **ip**: The IP of peerconnection_server.
- **port**: The port of peerconnection_server.
- **autoconnect**: Connect to the server without user intervention.
- **autocall**: Call the first available other client without user intervention. *Note: Only one of two clients could set it to true, otherwise both two clients will block*.
- **autoclose**: The time *in seconds* before close automatically (always run if autoclose=0)

### **bwe_feedback_duration**
The duration the receiver sends its estimated target rate every time(*in millisecond*).

### **onnx**
- **onnx_model_path**: The path of the onnx model.

### **video_source**
- **video_disabled**:
    - enabled: If set to `true`, the client will not take any video source as input.
- **webcam**:
    - enabled: If set to `true`, then the client will use the web camera as the video source
