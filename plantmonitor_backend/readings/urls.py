from django.urls import path
from .views import SensorReadingListView, LatestReadingView

urlpatterns = [
    path('readings/', SensorReadingListView.as_view(), name='reading-list'),
    path('readings/latest/', LatestReadingView.as_view(), name='reading-latest'),
]