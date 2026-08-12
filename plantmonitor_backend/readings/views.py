from rest_framework import generics
from rest_framework.pagination import PageNumberPagination
from django.http import Http404

from .models import SensorReading
from .serializers import SensorReadingSerializer


class SensorReadingPagination(PageNumberPagination):
    page_size = 50
    page_size_query_param = "page_size"
    max_page_size = 200


class SensorReadingListView(generics.ListAPIView):
    serializer_class = SensorReadingSerializer
    pagination_class = SensorReadingPagination

    def get_queryset(self):
        plant_id = self.request.query_params.get("plant_id", 1)

        return (
            SensorReading.objects
            .filter(plant_id=plant_id)
            .order_by("-recorded_at")
        )


class LatestReadingView(generics.RetrieveAPIView):
    serializer_class = SensorReadingSerializer

    def get_object(self):
        plant_id = self.request.query_params.get("plant_id", 1)
        obj = (
            SensorReading.objects
            .filter(plant_id=plant_id)
            .order_by("-recorded_at")
            .first()
        )
        if obj is None:
            raise Http404("No readings yet for this plant.")
        return obj