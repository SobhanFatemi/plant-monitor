from django.contrib import admin
from django.shortcuts import render
from django.urls import path, include

def dashboard_view(request):
    return render(request, 'dashboard.html')

def history_view(request):
    return render(request, 'history.html')

urlpatterns = [
    path('', dashboard_view, name='dashboard'),
    path('history/', history_view, name='history'),
    path('admin/', admin.site.urls),
    path('api/', include('readings.urls')),
]
