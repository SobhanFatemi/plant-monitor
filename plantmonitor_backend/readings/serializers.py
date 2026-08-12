from rest_framework import serializers
from .models import SensorReading

class SensorReadingSerializer(serializers.ModelSerializer):
    class Meta:
        model = SensorReading
        fields = ['id', 'plant_id', 'moisture', 'raw_moisture', 'temperature', 'humidity', 'recorded_at']
