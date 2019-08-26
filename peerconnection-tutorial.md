# Peerconnection Application Tutorial

## **Source code**

* The source code of peerconnection application is under `rtc/src/examples/peerconnection`.

## **Build peerconnection**

* Please refer to `rtc/README.md` for compiling, running and debugging the peerconnection application.

## **Running the peerconnection application**

* Make sure you have two executable files, `peerconnection_client.exe` and `peerconnection_server.exe`.
* Run the server with a specified port. The port is option, with the default value `8888`.

    ```cmd
    >peerconnection_server.exe --port=8888
    ```

* The options of peerconnection_client are shown blow.

Options           | Description         | Default Value
------------------|---------------------|-----------------------
`server`          | `The ip of peerconnection_client`| "localhost"
`port`            | `The port of peerconnection_port`| 8888
`autoconnect`     | `Connect to the server without user intervention`| false
`autocall`        | `Call the first available other client without user intervention`  | false
`webcam_disabled` | `Disable the client's webcam. This is useful for running two clients on one machine. See more details below` | false
`autoclose`       | `The time in seconds before close automatically (always run if autoclose=0)` | 0
`video_path`      | `The path of the input video file` | ""
`redis_ip`        | `The ip of Redis Service` | "127.0.0.1"
`redis_port`      | `The port of Redis Service` | 6379
`redis_sid`       | `The session id of Redis Service` | "test_sid_00"
`redis_update`    | `The time period of collecting states in milliseconds` | 500
`onnx_model_path` | `The path of the onnx model` | ""
`rate_update`     | `The rate update time in milliseconds` | 200

## **The `autocall` option**

* Run two peerconnection_clients. *Note: Only one of the clients can set the `autocall` option*

    ```cmd
    >peerconnection_client.exe --server=[server_ip] --port=[server_port] --autoconnect --autocall
    >peerconnection_client.exe --server=[server_ip] --port=[server_port] --autoconnect
    ```

* Now two clients will connect to each other automatically. You can try to kill the second client and restart it.

## **The `webcam_disabled` option**

* `Peerconnection_client` uses your webcam by default. This makes trouble when two clients run on same machine and contend for the only webcam physical device. 
* Using the `webcam_disabled` option, `peerconnection_client` disables its webcam. Then, two clients can run with only one webcam available. The client with the `webcam_disabled` option will not send its video to the other client, but is still able to receive the video from the other client.  
* Now you can run two clients on one machine. More exactly, you can run two clients with only one webcam available. Run the cmd below to have a try.

    ```cmd
    >peerconnection_client.exe --server=[server_ip] --port=[server_port] --autoconnect --autocall --webcam_disabled
    >peerconnection_client.exe --server=[server_ip] --port=[server_port] --autoconnect
    ```

## **The `autoclose` option**

* `autoclose` can control the total communication time for peerconnection application. The application will be closed automatically after the timeout.
* `autoclose`=0 means that the communication will never be closed automatically.
* The unit of `autoclose` is second.

    ```cmd
    >peerconnection_client.exe --server=[server_ip] --port=[server_port] --autoconnect --autocall --webcam_disabled
    >peerconnection_client.exe --server=[server_ip] --port=[server_port] --autoconnect --autoclose=60
    ```

## **The `video_path` option** (Will Support)

* `video_path` is the path of input video file that application will replay. The application will be closed automatically after the video finishes.
* `video_path`="" means that the application will use camera for the RTP communication.

    ```cmd
    >peerconnection_client.exe --video_path=video_sample.flv
    ```

## **The `redis_ip`, `redis_port`, `redis_sid`, and `redis_update` option**

* All these options are used for connecting the redis service.
* `redis_ip` and `redis_port` are the ip and port of Redis Service. The default ip is 127.0.0.1, and default port is 6379.
* `redis_sid` is the session id of Redis Service. Each test should have independent session. The default value is "test_sid_00".
* `redis_update` is the time that application collects states and push into redis. The unit of `redis_update` is millisecond and the default value is 500ms.

    ```cmd
    >peerconnection_client.exe --redis_ip=[redis_ip] --redis_port=[redis_port] --redis_sid=[redis_sid] --redis_update=500
    ```

## **The `onnx_model_path` and `rate_update` option**

* `onnx_model_path` and `rate_update` are used for running the onnx model to control the WebRTC rate.
* `onnx_model_path` is used to specify the path of the onnx model.
* `rate_update` is the rate update time by onnx model. The unit of `rate_update` is millisecond and the default value is 200ms.

    ```cmd
    >peerconnection_client.exe --onnx_model_path="webrtc_rate_control.onnx" --redis_update=200
    ```
