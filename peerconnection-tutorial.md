# Peerconnection Application Tutorial

## **Source code**
* The source code of peerconnection application is under `rtc/src/examples/peerconnection`.

## **Build peerconnection**
* Please refer to `rtc/README.md` for compiling, running and debugging the peerconnection application.

## **Running the peerconnection application**
* Make sure you have two executables, `peerconnection_client.exe` and `peerconnection_server.exe`.
* Run the server with a specified port. The port is option, with the default value `8888`.
```
>peerconnection_server.exe --port=8888
````
* The options of peerconnection_client are shown blow.

Options         | Description         | Default Value
----------------|---------------------|-----------------------
`server`        | `The ip of peerconnection_client`| "localhost"
`port`          | `The port of peerconnection_port`| 8888
`autoconnect`   | `Connect to the server without user intervention`| false
`autocall`      | `Call the first available other client without user intervention`  | false

* Run two peerconnection_clients. *Note: Only one of the clients can set the `autocall` option*
```
 >peerconnection_client.exe --server=[server_ip] --port=[server_port] --autoconnect --autocall
 >peerconnection_client.exe --server=[server_ip] --port=[server_port] --autoconnect
 ````
 * Now two clients will connect to each other automatically. You can try to kill the second client and restart it. 
