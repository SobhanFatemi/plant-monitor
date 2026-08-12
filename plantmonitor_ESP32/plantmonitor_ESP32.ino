#include <WiFi.h>
#include <PubSubClient.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_AHTX0.h>

#include <math.h>

// =====================================================
// OLED
// =====================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

#define SDA_PIN 21
#define SCL_PIN 22

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  -1
);

// =====================================================
// AHT10
// =====================================================

#define AHT_ADDR 0x38

Adafruit_AHTX0 aht;

bool ahtConnected = false;

// =====================================================
// YL-69
// =====================================================

#define MOISTURE_PIN 34

// Calibration:
// 4095 = 0%
// 600  = 70%
// 200  = 100%

const int DRY_VALUE = 4095;
const int SEVENTY_PERCENT_VALUE = 600;
const int WET_VALUE = 200;

// =====================================================
// Wi-Fi
// =====================================================

const char* WIFI_SSID = "Beef";
const char* WIFI_PASSWORD = "Bee811024";

// =====================================================
// MQTT
// =====================================================

const char* MQTT_SERVER = "87.248.155.166";
const int MQTT_PORT = 1883;

const char* MQTT_USER = "django";
const char* MQTT_PASS = "Bee811024";

const char* MQTT_TOPIC = "plant/1/reading";

WiFiClient espClient;
PubSubClient mqtt(espClient);

// =====================================================
// Lopaka icons
// =====================================================

static const unsigned char PROGMEM image_paint_5_bits[] = {
  0xe0, 0xa0, 0xe0
};

static const unsigned char PROGMEM image_plant_bits[] = {
  0x00, 0x03,
  0x00, 0x0f,
  0x20, 0x1f,
  0x70, 0x3f,
  0xf8, 0x3f,
  0xfc, 0x7e,
  0xfc, 0x7e,
  0xfc, 0x7c,
  0x78, 0xf0,
  0x30, 0xc0,
  0x10, 0x80,
  0x08, 0x80,
  0x05, 0x00,
  0x03, 0x00,
  0x01, 0x00,
  0x01, 0x00
};

static const unsigned char PROGMEM image_weather_humidity_bits[] = {
  0x04, 0x00,
  0x04, 0x00,
  0x0c, 0x00,
  0x0e, 0x00,
  0x1e, 0x00,
  0x1f, 0x00,
  0x3f, 0x80,
  0x3f, 0x80,
  0x7e, 0xc0,
  0x7f, 0x40,
  0xff, 0x60,
  0xff, 0xe0,
  0x7f, 0xc0,
  0x7f, 0xc0,
  0x3f, 0x80,
  0x0f, 0x00
};

static const unsigned char PROGMEM image_weather_temperature_bits[] = {
  0x1c, 0x00,
  0x22, 0x02,
  0x2b, 0x05,
  0x2a, 0x02,
  0x2b, 0x38,
  0x2a, 0x60,
  0x2b, 0x40,
  0x2a, 0x40,
  0x2a, 0x60,
  0x49, 0x38,
  0x9c, 0x80,
  0xae, 0x80,
  0xbe, 0x80,
  0x9c, 0x80,
  0x41, 0x00,
  0x3e, 0x00
};

// =====================================================
// Moisture percentage
// =====================================================

float moisturePercent(int raw)
{
  // 4095 = 0%
  if (raw >= DRY_VALUE)
    return 0.0;

  // 200 = 100%
  if (raw <= WET_VALUE)
    return 100.0;

  // 600 to 4095 = 70% to 0%
  if (raw >= SEVENTY_PERCENT_VALUE)
  {
    return (4095.0 - raw) *
           70.0 /
           (4095.0 - 600.0);
  }

  // 200 to 600 = 100% to 70%
  return 70.0 +
         (600.0 - raw) *
         30.0 /
         (600.0 - 200.0);
}

// =====================================================
// Draw Lopaka-style screen
// =====================================================

void drawScreen(
  float temperature,
  float humidity,
  float moisture,
  bool ahtOK,
  bool moistureOK
)
{
  display.clearDisplay();

  // ---------------------------------------------------
  // Temperature icon
  // ---------------------------------------------------

  display.drawBitmap(
    15,
    2,
    image_weather_temperature_bits,
    16,
    16,
    1
  );

  // ---------------------------------------------------
  // Humidity icon
  // ---------------------------------------------------

  display.drawBitmap(
    15,
    24,
    image_weather_humidity_bits,
    11,
    16,
    1
  );

  // ---------------------------------------------------
  // Plant icon
  // ---------------------------------------------------

  display.drawBitmap(
    14,
    45,
    image_plant_bits,
    16,
    16,
    1
  );

  // ---------------------------------------------------
  // Text settings
  // ---------------------------------------------------

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setTextWrap(false);

  // ---------------------------------------------------
  // Temperature
  // ---------------------------------------------------

  char temperatureText[20];

  if (ahtOK)
  {
    snprintf(
      temperatureText,
      sizeof(temperatureText),
      "%.1f C",
      temperature
    );
  }
  else
  {
    snprintf(
      temperatureText,
      sizeof(temperatureText),
      "--- C"
    );
  }

  display.setCursor(43, 3);
  display.print(temperatureText);

  // ---------------------------------------------------
  // Tiny paint/degree-style icon
  // ---------------------------------------------------

  display.drawBitmap(
    93,
    2,
    image_paint_5_bits,
    3,
    3,
    1
  );

  // ---------------------------------------------------
  // Humidity
  // ---------------------------------------------------

  char humidityText[20];

  if (ahtOK)
  {
    snprintf(
      humidityText,
      sizeof(humidityText),
      "%.1f %%",
      humidity
    );
  }
  else
  {
    snprintf(
      humidityText,
      sizeof(humidityText),
      "--- %%"
    );
  }

  display.setCursor(43, 26);
  display.print(humidityText);

  // ---------------------------------------------------
  // Soil moisture
  // ---------------------------------------------------

  char moistureText[20];

  if (moistureOK)
  {
    snprintf(
      moistureText,
      sizeof(moistureText),
      "%.1f %%",
      moisture
    );
  }
  else
  {
    snprintf(
      moistureText,
      sizeof(moistureText),
      "--- %%"
    );
  }

  display.setCursor(43, 47);
  display.print(moistureText);

  // ---------------------------------------------------
  // Show screen
  // ---------------------------------------------------

  display.display();
}

// =====================================================
// Wi-Fi connection
// =====================================================

void connectWiFi()
{
  if (WiFi.status() == WL_CONNECTED)
    return;

  Serial.print("Connecting to Wi-Fi");

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );

  int attempts = 0;

  while (
    WiFi.status() != WL_CONNECTED &&
    attempts < 30
  )
  {
    delay(500);

    Serial.print(".");

    attempts++;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Wi-Fi connected!");

    Serial.print("ESP32 IP: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("Wi-Fi connection failed.");
  }
}

// =====================================================
// MQTT connection
// =====================================================

void connectMQTT()
{
  if (mqtt.connected())
    return;

  Serial.print("Connecting to MQTT...");

  String clientID =
    "ESP32-Plant-" +
    String(
      (uint32_t)ESP.getEfuseMac(),
      HEX
    );

  if (mqtt.connect(clientID.c_str(), MQTT_USER, MQTT_PASS))
  {
    Serial.println("connected!");
  }
  else
  {
    Serial.print("MQTT failed, state = ");
    Serial.println(mqtt.state());
  }
}

// =====================================================
// Setup
// =====================================================

void setup()
{
  Serial.begin(115200);

  // -----------------------------------------
  // I2C
  // -----------------------------------------

  Wire.begin(
    SDA_PIN,
    SCL_PIN
  );

  // -----------------------------------------
  // OLED
  // -----------------------------------------

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDR))
  {
    Serial.println("OLED not found!");

    while (true)
    {
      delay(1000);
    }
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setTextWrap(false);
  display.setCursor(25, 25);
  display.print("Starting");

  display.display();

  // -----------------------------------------
  // AHT10
  // -----------------------------------------

  if (aht.begin())
  {
    ahtConnected = true;

    Serial.println("AHT10 connected!");
  }
  else
  {
    ahtConnected = false;

    Serial.println("AHT10 not connected!");
  }

  // -----------------------------------------
  // YL-69
  // -----------------------------------------

  analogReadResolution(12);

  // -----------------------------------------
  // Wi-Fi
  // -----------------------------------------

  connectWiFi();

  // -----------------------------------------
  // MQTT
  // -----------------------------------------

  mqtt.setServer(
    MQTT_SERVER,
    MQTT_PORT
  );

  connectMQTT();

  delay(1000);
}

// =====================================================
// Loop
// =====================================================

void loop()
{
  // ===================================================
  // Maintain Wi-Fi
  // ===================================================

  if (WiFi.status() != WL_CONNECTED)
  {
    connectWiFi();
  }

  // ===================================================
  // Maintain MQTT
  // ===================================================

  if (WiFi.status() == WL_CONNECTED)
  {
    if (!mqtt.connected())
    {
      connectMQTT();
    }

    mqtt.loop();
  }

  // ===================================================
  // AHT10
  // ===================================================

  float temperature = NAN;
  float humidity = NAN;

  if (ahtConnected)
  {
    sensors_event_t humidityEvent;
    sensors_event_t temperatureEvent;

    aht.getEvent(
      &humidityEvent,
      &temperatureEvent
    );

    temperature =
      temperatureEvent.temperature;

    humidity =
      humidityEvent.relative_humidity;

    if (
      isnan(temperature) ||
      isnan(humidity)
    )
    {
      ahtConnected = false;

      Serial.println(
        "AHT10 read failed!"
      );
    }
  }

  // ===================================================
  // YL-69
  // ===================================================

  int rawMoisture =
    analogRead(MOISTURE_PIN);

  bool moistureConnected =
    rawMoisture > 5;

  float moisture = 0.0;

  if (moistureConnected)
  {
    moisture =
      moisturePercent(rawMoisture);
  }

  // ===================================================
  // SERIAL MONITOR
  // ===================================================

  Serial.println();
  Serial.println("--------------------");

  if (ahtConnected)
  {
    Serial.print("Temperature: ");
    Serial.print(temperature, 1);
    Serial.println(" C");

    Serial.print("Humidity: ");
    Serial.print(humidity, 1);
    Serial.println(" %");
  }
  else
  {
    Serial.println("Temperature: ---");
    Serial.println("Humidity: ---");
  }

  if (moistureConnected)
  {
    Serial.print("Raw moisture: ");
    Serial.println(rawMoisture);

    Serial.print("Moisture: ");
    Serial.print(moisture, 1);
    Serial.println(" %");
  }
  else
  {
    Serial.println("Raw moisture: ---");
    Serial.println("Moisture: ---");
  }

  // ===================================================
  // OLED
  // ===================================================

  drawScreen(
    temperature,
    humidity,
    moisture,
    ahtConnected,
    moistureConnected
  );

  // ===================================================
  // MQTT
  // ===================================================

  if (
    mqtt.connected() &&
    ahtConnected &&
    moistureConnected
  )
  {
    char payload[200];

    snprintf(
      payload,
      sizeof(payload),
      "{\"moisture\":%.1f,\"raw_moisture\":%d,\"temperature\":%.1f,\"humidity\":%.1f}",
      moisture,
      rawMoisture,
      temperature,
      humidity
    );

    if (
      mqtt.publish(
        MQTT_TOPIC,
        payload
      )
    )
    {
      Serial.print("MQTT sent: ");
      Serial.println(payload);
    }
    else
    {
      Serial.println(
        "MQTT publish failed!"
      );
    }
  }
  else
  {
    Serial.println(
      "MQTT: not sending - sensor/broker unavailable"
    );
  }

  // ===================================================
  // Wait
  // ===================================================

  delay(60000);
}
