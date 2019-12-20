# Serverless Peerconnection Application Tutorial

## Overview

### **Source code**

* The source code of serverless peerconnection application is under `rtc/src/examples/peerconnection/serverless`.
### **Build peerconnection**
* Please refer to `rtc/README.md` for compiling, running and debugging the peerconnection application.
### **Running the peerconnection application**
* Make sure you have compiled the executable file, `peerconnection_serverless.exe` and get all the required `.dll` files in the same folder.
* Start the receiver with a file path as command line argument:  
    ```cmd
    peerconnection_serverless.exe ./webrtc_config_receiver.json
    ```
    Note: The config file should be in `json-formant`. An exmaple configuration file can be found [here](./webrtc_config_receiver.json).  Below we explain the configurations in detail. *Note: All fields in the configuration file are required*  
* Start the sender with a file path as command line argument:  
    ```cmd
    peerconnection_serverless.exe ./webrtc_config_sender.json
    ```

## Configurations for *peerconnection_serverless*

### **server_connection**
- **ip**: The IP of peerconnection_server.
- **port**: The port of peerconnection_server.
- **autoconnect**: Connect to the server without user intervention.
- **autocall**: Call the first available other client without user intervention. *Note: Only one of two clients could set it to true, otherwise both two clients will block*.
- **autoclose**: The time *in seconds* before close automatically (always run if autoclose=0)
  
Note that this part is kept for backward compatibility and not used in serverless version except for the autoclose option.

### **serverless_connection**
- **sender**
  - **enabled**: If set to `true`, the client will act as sender and automatically connect to receiver when launched.
  - **send_to_ip**: The IP of serverless peerconnection receiver 
  - **send_to_port**: The port of serverless peerconnection receiver
- **receiver**
  - **enabled**: If set to `true`, the client will act as receiver and wait for sender to connect.
  - **listening_ip**: The IP address that the socket in receiver binds and listends to
  - **listening_port**: The port number that the socket in receiver binds and listends to

### **bwe_feedback_duration**
The duration the receiver sends its estimated target rate every time(*in millisecond*).

### **onnx**
- **onnx_model_path**: The path of the onnx model.

### **redis**
- **ip**: The ip of Redis Service.
- **port**: The port of Redis Service.
- **session_id**: The session id of Redis Service
- **redis_update_duration**: The duration that the client collects status and push into redis(*in millisecond*).

### **video_source**
- **video_disabled**:
    - enabled: If set to `true`, the client will not take any video source as input.
- **webcam**:
    - enabled: If set to `true`, then the client will use the web camera as the video source
