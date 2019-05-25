import paho.mqtt.client as mqtt
broker = "127.0.0.1"
port = 1234
channel_prefix = "ME_34_DATA"
client = mqtt.Client("client1")

def on_message_callback(client, userdata, message):
   print(str(message.payload.decode("utf-8")))

client.connect(broker, port=port, keepalive=60, bind_address="")
client.on_message = on_message_callback
client.connect(broker, port=port, keepalive=60, bind_address="")
client.subscribe(channel_prefix+"/5.0/temperature")
client.subscribe(channel_prefix+"/6.0/temperature")
client.subscribe(channel_prefix+"/7.0/temperature")
client.subscribe(channel_prefix+"/8.0/temperature")
client.subscribe(channel_prefix+"/9.0/temperature")

client.publish(channel_prefix+"/subscription/subscribe","/#/temperature") 
client.loop_forever()
