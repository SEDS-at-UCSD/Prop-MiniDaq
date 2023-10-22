import paho.mqtt.client as mqtt
import time

# MQTT Broker Settings
mqtt_broker = "localhost"  # Use the hostname or IP where Mosquitto is running
mqtt_port = 1883

# Topic and Message
mqtt_topic = "test/topic"
mqtt_message = "Hello, MQTT!"

# Callback when a message is published
def on_publish(client, userdata, mid):
    print("Message Published")

# Callback when a message is received
def on_message(client, userdata, message):
    print(f"Received message on topic {message.topic}: {message.payload.decode()}")

# Create an MQTT client
client = mqtt.Client()

# Set up callbacks
client.on_publish = on_publish
client.on_message = on_message

# Connect to the MQTT broker
client.connect(mqtt_broker, mqtt_port)

# Subscribe to the topic
client.subscribe(mqtt_topic)

# Publish a message
client.publish(mqtt_topic, mqtt_message)

# Loop to wait for incoming messages
client.loop_start()

# Wait for a few seconds to receive messages
time.sleep(5)

# Disconnect from the MQTT broker
client.loop_stop()
client.disconnect()
