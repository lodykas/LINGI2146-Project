#!/usr/bin/env python3

import threading
import paho.mqtt.client as mqtt
import subprocess
import sys

broker = "127.0.0.1"
port = 1234
serialDevice = "/dev/pts/7"
channel_prefix = "ME_34_DATA"

subscribers = {}

p = subprocess.Popen(["/home/user/contiki/tools/sky/serialdump-linux", "-b115200", serialDevice], stdout=subprocess.PIPE, stdin=subprocess.PIPE, bufsize=1, universal_newlines=True)


#This thread class is in charge of the command-line interface
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
			
#subscribe to all topics of interest whenever we connect to broker
def on_connect_callback(client, userdata, flags, rc):
	print("Connected with result code "+str(rc))
	client.subscribe("$SYS/broker/log/M/subscribe")
	client.subscribe("$SYS/broker/log/M/unsubscribe")
	client.subscribe(channel_prefix+"subscription/#")

#here we process the arrival/departure of subscribers to limit network usage
def on_message_callback(client, userdata, message):
	print(" msg : "+str(message.payload.decode("utf-8")))
	msg = str(message.payload.decode("utf-8")).split()
	if message.topic == "$SYS/broker/log/M/subscribe":
		channel = msg[3]
		if channel in subscribers:
			subscribers[channel]+=1
		else:
			subscribers[channel]=1
			parts = channel.split('/')
			p.stdin.write("DATA-PER "+ parts[0])
	elif message.topic == "$SYS/broker/log/M/unsubscribe":
		channel = msg[3]
		if channel in subscribers:
			subscribers[channel]-=1
			if (subscribers[channel]) == 0:
				del subscribers[channel]
				parts = channel.split('/')
				p.stdin.write("DATA-NEV "+ parts[0])

	#this is an active way of joining/leaving
	elif channel_prefix+"subscription/" in message.topic:
		channel = message.topic[len( channel_prefix+"subscription/"):]
		if(msg[0] == "subscribe"):
			if channel in subscribers:
				subscribers[channel]+=1
			else:
				subscribers[channel]=1
			parts = channel.split('/')
			p.stdin.write("DATA-PER "+ parts[0])
		elif(msg[0] == "unsubscribe"):
			if channel in subscribers:
				subscribers[channel]-=1
				if (subscribers[channel]) == 0:
					del subscribers[channel]
					parts = channel.split('/')
					p.stdin.write("DATA-NEV "+ parts[0])

def main():
	client = mqtt.Client("Gateway")
	client.on_connect = on_connect_callback
	client.on_message = on_message_callback

	client.connect(broker, port=port, keepalive=60, bind_address="")

	ct = CommandTool(0,p)
	ct.start()

	client.loop_start()

	while (True):
		line = p.stdout.readline()
 		data = line.split()
		if (len(data) == 2):
			client.publish(channel_prefix+data[0],data[1])
			print(channel_prefix+data[0]+" - "+data[1])
		else:
			print "response: "+line

	client.loop_stop()

if __name__ == '__main__':
	main()

