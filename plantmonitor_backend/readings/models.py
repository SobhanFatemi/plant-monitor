from django.db import models


class SensorReading(models.Model):
    plant_id = models.IntegerField()
    moisture = models.FloatField()
    raw_moisture = models.IntegerField()
    temperature = models.FloatField()
    humidity = models.FloatField()
    recorded_at = models.DateTimeField(auto_now_add=True)

    class Meta:
        indexes = [
            models.Index(
                fields=["plant_id", "-recorded_at"],
                name="plant_recorded_idx",
            ),
        ]