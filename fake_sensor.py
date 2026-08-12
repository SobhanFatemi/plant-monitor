import paho.mqtt.client as mqtt
import json
import random
import time

BROKER = "localhost"
PORT = 1883
TOPIC = "plant/1/reading"

client = mqtt.Client()
client.connect(BROKER, PORT)

while True:
    reading = {
        "moisture": round(random.uniform(20, 80), 1),
        "temperature": round(random.uniform(18, 26), 1),
        "humidity": round(random.uniform(30, 60), 1),
    }
    client.publish(TOPIC, json.dumps(reading))
    print(f"Published: {reading}")
    time.sleep(5)