import paho.mqtt.client as mqtt
import time

# Define the MQTT broker details
broker = "broker.emqx.io"
port = 1883
topic = "/gilbert/iot"
message = "branch/feature/1.0.0"

# Flag to indicate message receipt
message_received = False

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT Broker!")
    else:
        print(f"Failed to connect, return code {rc}")

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    global message_received
    print(f"Received message: {msg.payload.decode()} on topic: {msg.topic}")
    if msg.payload.decode() == message:
        message_received = True

# Create a new MQTT client instance
client = mqtt.Client()

# Attach the on_connect and on_message callback functions
client.on_connect = on_connect
client.on_message = on_message

# Connect to the MQTT broker
client.connect(broker, port, 60)

# Start the loop to process network traffic and dispatch callbacks
client.loop_start()

# Subscribe to the topic to listen for messages
client.subscribe(topic)
print(f"Subscribed to topic: {topic}")

# Publish a message to the specified topic
client.publish(topic, message)
print(f"Published message: {message} to topic: {topic}")

# Wait for a longer period to receive the message
time.sleep(5)

# Stop the loop
client.loop_stop()

# Disconnect from the broker
client.disconnect()

# Check if the message was received to confirm pairing
if message_received:
    print("Pairing test successful: Message received.")
else:
    print("Pairing test failed: Message not received.")
