import paho.mqtt.client as mqtt

# Define the MQTT broker details
broker = "broker.emqx.io"
port = 1883
topic = "/gilbert/iot"
message = "branch/feature/1.0.0"

# Create a new MQTT client instance
client = mqtt.Client()

# Connect to the MQTT broker
client.connect(broker, port, 60)

# Publish a message to the specified topic
client.publish(topic, message)

# Disconnect from the broker
client.disconnect()

print(f"Message '{message}' sent to topic '{topic}'")