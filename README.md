# Plant Monitor

A self-hosted plant monitoring system built around an ESP32. A soil moisture, temperature, and humidity sensor rig publishes readings over MQTT, a Django backend stores and serves them through a REST API, and a small web dashboard visualizes the data in real time.

```
ESP32 (sensors) --MQTT--> Django subscriber --DB--> REST API --> Web dashboard
```

## Features

- **ESP32 firmware** reads soil moisture (capacitive/resistive YL-69 sensor), temperature, and humidity (AHT10) every 60 seconds, shows live readings on an attached OLED display, and publishes them as JSON over MQTT.
- **Django + DRF backend** subscribes to the MQTT topic via a management command, persists readings to PostgreSQL, and exposes a paginated REST API.
- **Web dashboard** (vanilla HTML/CSS/JS + Chart.js) shows the latest readings and a live-updating moisture chart, with a low-moisture watering alert and a full history page.

## Project structure

```
.
├── plantmonitor_backend/       # Django project
│   ├── config/                 # Settings, URLs, WSGI/ASGI
│   └── readings/               # App: models, API views, MQTT subscriber
│       └── management/commands/subscribe.py
├── plantmonitor_frontend/      # Static dashboard (served by Django)
│   ├── dashboard.html / .js    # Live view + chart
│   ├── history.html / .js      # Full history table + chart
│   └── style.css
├── plantmonitor_ESP32/
│   └── plantmonitor_ESP32.ino  # Firmware for the sensor unit
└── requirements.txt
```

## How it works

1. The ESP32 reads the soil moisture sensor and AHT10 temperature/humidity sensor, converts the raw moisture value to a percentage using a calibrated curve, and draws the readings on its OLED.
2. Every 60 seconds it publishes a JSON payload (`moisture`, `raw_moisture`, `temperature`, `humidity`) to the MQTT topic `plant/1/reading`.
3. A Django management command (`subscribe`) runs continuously, listens on that topic, and saves each incoming reading as a `SensorReading` row in PostgreSQL.
4. The DRF API exposes `GET /api/readings/` (paginated, newest first) and `GET /api/readings/latest/`.
5. The dashboard polls the API every 30 seconds and renders the current stats, a watering alert if moisture drops below 30%, and a live chart. The history page shows the complete reading log.

## Hardware

| Component | Notes |
|---|---|
| ESP32 dev board | Wi-Fi + MQTT client |
| YL-69 soil moisture sensor | Analog, connected to GPIO 34 |
| AHT10 temperature/humidity sensor | I2C, address `0x38` |
| SSD1306 OLED display | 128x64, I2C, address `0x3C` |

Wiring: SDA → GPIO 21, SCL → GPIO 22 (shared I2C bus for OLED and AHT10); moisture sensor analog output → GPIO 34.

## Getting started

### Prerequisites

- Python 3.11+
- PostgreSQL
- An MQTT broker (e.g. [Mosquitto](https://mosquitto.org/)), reachable by both the Django backend and the ESP32
- Arduino IDE (or PlatformIO) for flashing the ESP32

### 1. Backend setup

```bash
cd plantmonitor_backend
python -m venv venv
source venv/bin/activate      # Windows: venv\Scripts\activate
pip install -r ../requirements.txt
```

Create a `.env` file in `plantmonitor_backend/` based on `.env.example`:

```env
# Django
SECRET_KEY=replace-with-a-long-random-secret-key
DEBUG=False
ALLOWED_HOSTS=localhost,127.0.0.1,your-server-ip
CSRF_TRUSTED_ORIGINS=https://your-server-ip
SECURE_SSL_REDIRECT=False
SESSION_COOKIE_SECURE=False
CSRF_COOKIE_SECURE=False

# PostgreSQL
DB_NAME=plantmonitor
DB_USER=plantmonitor
DB_PASSWORD=your-postgres-password
DB_HOST=localhost
DB_PORT=5432

# MQTT
MQTT_HOST=localhost
MQTT_PORT=1883
MQTT_USERNAME=django
MQTT_PASSWORD=your-mqtt-password
```

Create the database, run migrations, and start the server:

```bash
createdb plantmonitor   # or create it in psql / your Postgres client
python manage.py migrate
python manage.py runserver
```

The dashboard will be available at `http://localhost:8000/` and the history page at `http://localhost:8000/history/`.

### 2. Start the MQTT subscriber

In a separate terminal (with the virtualenv activated), run the command that listens for sensor readings and writes them to the database:

```bash
python manage.py subscribe
```

Keep this running alongside the web server — it's what turns incoming MQTT messages into database rows. In production, run it as a background service (e.g. systemd, supervisord, or a Docker container).

### 3. Flash the ESP32

Open `plantmonitor_ESP32/plantmonitor_ESP32.ino` in the Arduino IDE.

Install the required libraries via the Library Manager:
- `PubSubClient`
- `Adafruit GFX Library`
- `Adafruit SSD1306`
- `Adafruit AHTX0`

Update the configuration constants at the top of the file:

```cpp
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

const char* MQTT_SERVER = "your-server-ip-or-hostname";
const int MQTT_PORT = 1883;
const char* MQTT_USER = "django";
const char* MQTT_PASS = "YOUR_MQTT_PASSWORD";
```

If your soil sensor's dry/wet analog readings differ from the defaults, recalibrate the constants:

```cpp
const int DRY_VALUE = 4095;             // fully dry
const int SEVENTY_PERCENT_VALUE = 600;  // reference point
const int WET_VALUE = 200;              // fully saturated
```

Select your ESP32 board, then compile and upload.

## API reference

| Endpoint | Method | Description |
|---|---|---|
| `/api/readings/` | GET | Paginated list of readings, newest first. Supports `?plant_id=` (default `1`), `?page=`, `?page_size=` (max 200). |
| `/api/readings/latest/` | GET | Most recent reading for a plant. Supports `?plant_id=`. Returns 404 if none exist. |

Both endpoints are rate-limited to 60 requests/minute per client. Example response for a reading:

```json
{
  "id": 42,
  "plant_id": 1,
  "moisture": 54.3,
  "raw_moisture": 2140,
  "temperature": 22.1,
  "humidity": 47.8,
  "recorded_at": "2026-08-12T09:15:00Z"
}
```

## Deployment notes

- Static files are served with [WhiteNoise](https://whitenoise.readthedocs.io/); run `python manage.py collectstatic` before deploying.
- `gunicorn` is included in `requirements.txt` as the production WSGI server.
- Set `DEBUG=False`, a real `SECRET_KEY`, and your actual `ALLOWED_HOSTS`/`CSRF_TRUSTED_ORIGINS` in production. Enable `SECURE_SSL_REDIRECT`, `SESSION_COOKIE_SECURE`, and `CSRF_COOKIE_SECURE` once you're behind HTTPS.
- The `subscribe` management command needs to run as a long-lived background process independent of the web server.

## Tech stack

**Backend:** Django, Django REST Framework, django-environ, paho-mqtt, PostgreSQL, WhiteNoise, Gunicorn
**Frontend:** HTML/CSS/JavaScript, Chart.js, Lucide icons
**Firmware:** Arduino (ESP32), PubSubClient, Adafruit AHTX0, Adafruit SSD1306

## License

Add a license of your choice (e.g. MIT) here.
