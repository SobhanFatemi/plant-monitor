import json

import paho.mqtt.client as mqtt

from django.conf import settings
from django.core.management.base import BaseCommand

from readings.models import SensorReading


TOPIC = "plant/1/reading"


class Command(BaseCommand):
    help = "Subscribes to MQTT sensor topic and saves readings via Django ORM"

    def handle(self, *args, **options):

        def on_message(client, userdata, msg):
            try:
                data = json.loads(msg.payload.decode())

                SensorReading.objects.create(
                    plant_id=1,
                    moisture=data["moisture"],
                    raw_moisture=data["raw_moisture"],
                    temperature=data["temperature"],
                    humidity=data["humidity"],
                )

                self.stdout.write(
                    self.style.SUCCESS(f"Saved: {data}")
                )

            except Exception as e:
                self.stderr.write(
                    self.style.ERROR(
                        f"Error processing message: {e}"
                    )
                )

        client = mqtt.Client()

        client.username_pw_set(
            settings.MQTT_USERNAME,
            settings.MQTT_PASSWORD,
        )

        client.on_message = on_message

        client.connect(
            settings.MQTT_HOST,
            settings.MQTT_PORT,
        )

        client.subscribe(TOPIC)

        self.stdout.write(
            self.style.SUCCESS(
                f"Listening for sensor data on {TOPIC}..."
            )
        )

        client.loop_forever()