#!/usr/bin/env python3

import threading
import paho.mqtt.client as mqtt
import subprocess
import sys

broker = "test.mosquitto.org"
port = 1883
serialDevice = "/dev/pts/7"

class CommandTool (threading.Thread):
	def __init__(self, threadID, file):
		threading.Thread.__init__(self)
		self.threadID = threadID
		self.file = file
	def run(self):
		while True:
			try:
				line = sys.stdin.readline()
			except KeyboardInterrupt:
				break
			if not line:
				break
			self.file.stdin.write(line)
			

def on_connect_callback(client, userdata, flags, rc):
	print("Connected with result code "+str(rc))
	#subsribe here

def on_message_callback():
	pass

def main():
	client = mqtt.Client("Gateway")
	client.on_connect = on_connect_callback
	client.on_message = on_message_callback

	client.connect(broker, port=port, keepalive=60, bind_address="")


	p = subprocess.Popen(["/home/user/contiki/tools/sky/serialdump-linux", "-b115200", serialDevice], stdout=subprocess.PIPE, stdin=subprocess.PIPE, bufsize=1, universal_newlines=True)

	ct = CommandTool(0,p)
	ct.start()

	client.loop_start()

	while (True):
		line = p.stdout.readline()
 		data = line.split()
		if (len(data) == 2):
			#print("data received on topic : "+ data[0])
			client.publish("ME_34_DATA/"+data[0],data[1])
		else:
			print "response: "+line

	client.loop_stop()

if __name__ == '__main__':
	main()

