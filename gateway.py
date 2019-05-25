#!/usr/bin/env python3

import threading
import paho.mqtt.client as mqtt
import subprocess
import sys
import re
from sets import Set

broker = "127.0.0.1"
port = 1234
serialDevice = "/dev/pts/21"
channel_prefix = "ME_34_DATA"
measures = Set(["humidity", "temperature"])


topic_pattern = re.compile("/[\d,.]+|#/[\w]+|#")
nodes = Set()
nodes_lock = threading.Lock() 

subscribers = {}

p = subprocess.Popen(["/home/user/contiki/tools/sky/serialdump-linux", "-b115200", serialDevice], stdout=subprocess.PIPE, stdin=subprocess.PIPE, bufsize=1, universal_newlines=True)


#This thread class is in charge of the command-line interface
class CommandTool (threading.Thread):
	def __init__(self, threadID):
		threading.Thread.__init__(self)
		self.threadID = threadID
	def run(self):
		while True:
			try:
				line = sys.stdin.readline()
			except KeyboardInterrupt:
				break
			if not line:
				break
			p.stdin.write(line)
			
#subscribe to all topics of interest whenever we connect to broker
def on_connect_callback(client, userdata, flags, rc):
	print("Connected with result code "+str(rc))
	client.subscribe("$SYS/broker/log/M/subscribe")
	client.subscribe("$SYS/broker/log/M/unsubscribe")
	client.subscribe(channel_prefix+"/subscription/#")

def add_channel(channel):
	if topic_pattern.search(channel) is not None:
		if channel in subscribers :
			subscribers[channel]+=1
		else:
			parts = channel.split('/')
			if len(parts) != 3:
				return
			if parts[1] == "#":
				nodes_lock.acquire()
				for n in nodes:
					add_channel("/"+n+"/"+parts[2])
				nodes_lock.release()
				
			elif parts[2] == "#":
				for m in measures :
					add_channel("/"+parts[1]+"/"+m)
			else:
				print parts[1]
				subscribers[channel]=1
				p.stdin.write("DATA-PER "+ parts[1] + "\n")

def remove_channel(channel):
	if topic_pattern.search(channel) is not None:
		if channel in subscribers:
			subscribers[channel]-=1
			if (subscribers[channel]) == 0:
				parts = channel.split('/')
				if len(parts) != 3:
					return
				if parts[1] == "#":
					nodes_lock.acquire()
					for n in nodes :
						remove_channel(n+"/"+parts[2])
					nodes_lock.release()
				
				elif parts[2] == "#":
					for m in measures :
						remove_channel(parts[1]+"/"+m)
				else:
					del subscribers[channel]
					p.stdin.write("DATA-NEV "+ parts[1]+ "\n")

#here we process the arrival/departure of subscribers to limit network usage
def on_message_callback(client, userdata, message):
	msg = str(message.payload.decode("utf-8")).split()

	#automatic detection of subsciptions
	if message.topic == "$SYS/broker/log/M/subscribe":
		channel = msg[3]
		add_channel(channel)
	elif message.topic == "$SYS/broker/log/M/unsubscribe":
		channel = msg[3]
		remove_channel(channel)

	#this is an active way of joining/leaving
	elif channel_prefix+"/subscription" in message.topic:
		action = message.topic[len( channel_prefix+"/subscription"):]

		if topic_pattern.search(msg[0]) is not None:
			if(action == "/subscribe"):
				add_channel(msg[0])
			elif(action == "/unsubscribe"):
				remove_channel(msg[0])

def main():

	#start mqtt connection
	client = mqtt.Client("Gateway")
	client.on_connect = on_connect_callback
	client.on_message = on_message_callback

	client.connect(broker, port=port, keepalive=60, bind_address="")

	client.loop_start()

	#start command-line tool
	ct = CommandTool(0)
	ct.start()

	#forward measurements and update list of nodes
	while (True):
		line = p.stdout.readline()
 		data = line.split()
		if (len(data) == 2 and topic_pattern.search(data[0]) is not None):
			client.publish(channel_prefix+data[0],data[1])
		elif "new route " in line:
			nodes_lock.acquire()
			nodes.add(line[len("new route "):].strip("\n"))
			nodes_lock.release()
		elif "del route " in line:
			nodes_lock.acquire()
			nodes.discard(line[len("del route "):].strip("\n"))
			nodes_lock.release()
		else:
			print "[Info]: "+line

	client.loop_stop()

if __name__ == '__main__':
	main()

