# Serverless Peerconnection Application Tutorial

## Overview

### **Source code**

* The source code of serverless peerconnection application is under `rtc/src/examples/peerconnection/serverless`.
### **Build peerconnection**
* Please refer to `rtc/README.md` for compiling, running and debugging the peerconnection application.

  1. First, we need to generate ninja files with gn. 

     Add the following argument “--ide=vs” (ref: https://gn.googlesource.com/gn/+/master/docs/reference.md#IDE-options):

     ```
     gn gen out/default --ide=vs
     ```

  2. Now, compile it with ninja, we target `peerconnection_serverless`

     ```
     ninja -C out/default peerconnection_serverless
     ```
### **Running the peerconnection application**
1. Make sure you have compiled the executable file, `peerconnection_serverless.exe` and get all the required `.dll` files in the same folder.

    1)Integrate ONNX runtime into WebRTC

    ​	To run the peerconnection_client.exe successfully, you have to include onnxinfer.dll, ONNXInferCore.dll, onnxruntime.dll, and onnx_model.onnx (the name of your model) into the same directory as peerconnection_serverless.exe file.	

    2)Add some missing dlls

    ​	There are some missing dlls are needed for running webrtc, such as vcruntime140_1.dll,  vcruntime140_1d.dll and vcomp140.dll. 
    ​	For these files, you can install the latest supported Visual C++ redistributable packages for Visual Studio 2015, 2017 and 2019. Here is the link: https://support.microsoft.com/en-ca/help/2977003/the-latest-supported-visual-c-downloads.
    ​	You can also download them from the internet directly and put them into the same directory as peerconnection_serverless.exe file.


2. Start the receiver with a file path as command line argument:  
    ```cmd
    peerconnection_serverless.exe ./webrtc_config_receiver.json
    ```
    Note: The config file should be in `json-formant`. An exmaple configuration file can be found [here](./webrtc_config_receiver.json).  Below we explain the configurations in detail. *Note: All fields in the configuration file are required*  

3. Start the sender with a file path as command line argument:  
    ```cmd
    peerconnection_serverless.exe ./webrtc_config_sender.json
    ```

## Configurations for *peerconnection_serverless*

### **server_connection**
- **ip**: The IP of peerconnection_server. **NO NEEDED** in peerconnection_serverless.
- **port**: The port of peerconnection_server.**NO NEEDED** in peerconnection_serverless.
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

Redis is **NO NEEDED** in peerconnection_serverless.

- **ip**: The ip of Redis Service.
- **port**: The port of Redis Service.
- **session_id**: The session id of Redis Service
- **redis_update_duration**: The duration that the client collects status and push into redis(*in millisecond*).

### **video_source**
- **video_disabled**:
    - enabled: If set to `true`, the client will not take any video source as input.
- **webcam**:
    - enabled: If set to `true`, then the client will use the web camera as the video source

## Extra features of peerconnection_serverless

### **Running on WAN**

If you want to run the peerconnection_serverless on WAN, you should use a stun server at least.

To use stun server in AlphaRTC, You should change the peerconnection string in [GetPeerConnectionString() ](https://msrasia.visualstudio.com/NetworkingResearchGroup/_git/AlphaRTC-src?path=%2Fexamples%2Fpeerconnection%2Fserverless%2Fdefaults.cc&version=GBmaster&line=45&lineEnd=45&lineStartColumn=47&lineEndColumn=77&lineStyle=plain)of the defaults.cc to your stun server address, such as "**stun:ip:port**".

If you don't have any available stun server, you can follow the steps below to build your own stun server on ubuntu.

we recommend you use coturn to build the stun server.

first, run these command to install coturn and dependency:

```
$sudo apt-get install libssl-dev 

$sudo apt-get install libevent-dev 

$sudo apt-get install coturn

$sudo apt-get install libpq-dev 

$sudo apt-get install mysql-client

$sudo apt-get install libmysqlclient-dev

$sudo apt-get install libhiredis-dev 
```

View the installation location of turnserver and you may need to add it to the PATH

```
$which turnserver
```

Generate a self-signed certificate for the turnserver

```
$openssl req -x509 -newkey rsa:2048 -keyout /etc/turn_server_pkey.pem -out /etc/turn_server_cert.pem -days 99999 -nodes 
```

Add a coturn account for app to login in

```
$turnadmin -k -u USERNAME -r REALM -p PASSWORD
```

Modify the configuration

```
$vi etc/turnserver.conf
```

```
relay-device=eth0 #be consistent with "ifconfig"

listening-ip= #private ip

listening-port=3478 #port

relay-ip=#private ip

external-ip= #public ip

relay-threads=50 

lt-cred-mech 

cert=/etc/turn_server_cert.pem 

pkey=/etc/turn_server_pkey.pem 

pidfile=”/var/run/turnserver.pid” 

min-port=49152

max-port=65535 

user=USERNAME:PASSWORD

cli-password=qwerty 

realm=REALM

Fingerprint
```

Start the turnserver service

```
sudo turnserver -o -a -f -b /etc/turnserver.conf 
```

Test the turnserver 

You could access this address to test the turnserver, https://webrtc.github.io/samples/src/content/peerconnection/trickle-ice/.

For we only use the function of stun server,  you don't need to fill in the coturn username and password. After you click the "gather candidates", if you can see "rtp srflx" candidates, you are successful!

### **Out log to file**

To enable log to file, add these 2 lines in the [main.cc](https://msrasia.visualstudio.com/NetworkingResearchGroup/_git/AlphaRTC-src?path=%2Fexamples%2Fpeerconnection%2Fserverless%2Fmain.cc&version=GBmaster&line=42&lineEnd=42&lineStartColumn=3&lineEndColumn=80&lineStyle=plain) , and to ensure collect logs correctly, you may should create the logs directory manually in the the same directory as peerconnection_serverless.exe file.

```
  rtc::LogMessage::SetLogFileName("logs/webrtc.log");

  rtc::LogMessage::SetIfLogToFile(true);
```

