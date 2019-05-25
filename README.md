# Project for the course LINGI2146
This repository contains the source code for the project of Mobile and Embedded computing, in april 2019.

#### Contributors :
Everarts de Velp Guillaume, Ody Lucas, Rousseaux Tom

## How to run?
### Install
the gateway python script requires the paho-mqtt library to run.
install it with
```bash
pip install paho-mqtt
```

### Compile
You can simply run this command, which will call the makefile :
```bash
make
```
This will generate 2 executable files, which you will need to burn into your embedded devices: the /root directory contains the program of the node which is connected to the gateway, while the /node directory contains code for the other motes.

### Execute
Note: the gateway softare uses the serialdump tool of Contiki, make sure you are able to run this tool to ensure the correctness of the gateway software.

You can specify inside the gateway.py file the various parameters of the network : 
broker : the ip address of the mqtt broker server
port : port used by said server
serialDevice = path to the file descriptor of the serial device, i.e. the root
channel_prefix = topic name of sensor data in mqtt
measures = Set of measures produced by your sensor network

run
```bash
python gateway.py
```
you should see a few info messages appear. At this point, you can type in any command to configure your sensor network

have fun

