#include <DHT.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";
const char* MQTT_BROKER = "3.120.74.109";
const int MQTT_PORT = 1883;
const char* MQTT_CLIENT_ID = "ChickCare_ESP32";

const char* TOPIC_SENSOR_DATA = "chickcare/sensor/data";
const char* TOPIC_STATUS = "chickcare/status";
const char* TOPIC_EVENT = "chickcare/event";
const char* TOPIC_ONLINE = "chickcare/online";
const char* TOPIC_CONTROL_HEATER = "chickcare/control/heater";
const char* TOPIC_CONTROL_SPRAY = "chickcare/control/spray";
const char* TOPIC_CONTROL_MODE = "chickcare/control/mode";
const char* TOPIC_SETTINGS_TEMP = "chickcare/settings/temperature";
const char* TOPIC_SETTINGS_HUMIDITY = "chickcare/settings/humidity";
const char* TOPIC_SETTINGS_SPRAY_DURATION = "chickcare/settings/spray_duration";
const char* TOPIC_COMMAND_RESTART = "chickcare/command/restart";
const char* TOPIC_COMMAND_RESET = "chickcare/command/reset";
const char* TOPIC_COMMAND_REQUEST_STATUS = "chickcare/command/request_status";

// Pin definitions
#define DHT_PIN 4
#define DHT_TYPE DHT22
#define HEATER_LED_PIN 15
#define SPRAY_LED_PIN 2   // LED/Relay untuk Dehumidifier Spray

// Temperature settings
#define TARGET_TEMP_MIN 32.0
#define TARGET_TEMP_MAX 33.0
#define TEMP_HYSTERESIS 0.5    

// Humidity settings (histeresis)
#define HUMIDITY_ON_THRESHOLD 60.0   // Spray nyala kalau < 40%
#define HUMIDITY_OFF_THRESHOLD 62.0  // Spray mati kalau > 45%
#define SPRAY_DURATION_MS 3000       // Spray aktif selama 3 detik

// System variables
DHT dht(DHT_PIN, DHT_TYPE);
float currentTemp = 0;
float currentHumidity = 0;

int pwmValue = 0;
bool heaterEnabled = true;

// Dehumidifier variables
bool sprayActive = false;
unsigned long sprayStartTime = 0;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
bool mqttConnected = false;
unsigned long lastHeartbeat = 0;
float tempMin = TARGET_TEMP_MIN;
float tempMax = TARGET_TEMP_MAX;
float humidityOnThreshold = HUMIDITY_ON_THRESHOLD;
float humidityOffThreshold = HUMIDITY_OFF_THRESHOLD;
unsigned long sprayDuration = SPRAY_DURATION_MS;

TaskHandle_t tempTaskHandle;
TaskHandle_t controlTaskHandle;
TaskHandle_t mqttTaskHandle;
SemaphoreHandle_t tempMutex;

void mqttCallback(char* topic, byte* payload, unsigned int length);
void connectWiFi();
void connectMQTT();
void publishSensorData();
void publishStatus();
void publishEvent(const char* eventType, const char* value);
void mqttTask(void *parameter);
void temperatureTask(void *parameter);
void timeManagementTask(void *parameter);
void controlTask(void *parameter);
void controlHeaterLED(float temperature);
void controlSpray(float humidity);
void displaySystemStatus();

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  
  if (error) return;

  String topicStr = String(topic);

  if (topicStr == TOPIC_CONTROL_HEATER) {
    if (doc.containsKey("enabled")) {
      heaterEnabled = doc["enabled"];
    }
  }
  else if (topicStr == TOPIC_CONTROL_SPRAY) {
    if (doc.containsKey("trigger") && doc["trigger"] == true) {
      if (!sprayActive) {
        sprayActive = true;
        sprayStartTime = millis();
        digitalWrite(SPRAY_LED_PIN, HIGH);
        publishEvent("spray_manual", "ON");
      }
    }
  }
  else if (topicStr == TOPIC_SETTINGS_TEMP) {
    if (doc.containsKey("min")) tempMin = doc["min"];
    if (doc.containsKey("max")) tempMax = doc["max"];
  }
  else if (topicStr == TOPIC_SETTINGS_HUMIDITY) {
    if (doc.containsKey("on")) humidityOnThreshold = doc["on"];
    if (doc.containsKey("off")) humidityOffThreshold = doc["off"];
  }
  else if (topicStr == TOPIC_SETTINGS_SPRAY_DURATION) {
    if (doc.containsKey("duration")) sprayDuration = doc["duration"];
  }
  else if (topicStr == TOPIC_COMMAND_RESTART) {
    ESP.restart();
  }
  else if (topicStr == TOPIC_COMMAND_RESET) {
    tempMin = TARGET_TEMP_MIN;
    tempMax = TARGET_TEMP_MAX;
    humidityOnThreshold = HUMIDITY_ON_THRESHOLD;
    humidityOffThreshold = HUMIDITY_OFF_THRESHOLD;
    sprayDuration = SPRAY_DURATION_MS;
    heaterEnabled = true;
  }
  else if (topicStr == TOPIC_COMMAND_REQUEST_STATUS) {
    publishStatus();
  }
}

void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed");
  }
}

void connectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT...");
    
    if (mqttClient.connect(MQTT_CLIENT_ID, TOPIC_ONLINE, 1, true, "offline")) {
      Serial.println("connected");
      mqttConnected = true;
      
      mqttClient.publish(TOPIC_ONLINE, "online", true);
      
      mqttClient.subscribe(TOPIC_CONTROL_HEATER, 1);
      mqttClient.subscribe(TOPIC_CONTROL_SPRAY, 1);
      mqttClient.subscribe(TOPIC_CONTROL_MODE, 1);
      mqttClient.subscribe(TOPIC_SETTINGS_TEMP, 1);
      mqttClient.subscribe(TOPIC_SETTINGS_HUMIDITY, 1);
      mqttClient.subscribe(TOPIC_SETTINGS_SPRAY_DURATION, 1);
      mqttClient.subscribe(TOPIC_COMMAND_RESTART, 1);
      mqttClient.subscribe(TOPIC_COMMAND_RESET, 1);
      mqttClient.subscribe(TOPIC_COMMAND_REQUEST_STATUS, 1);
      
      publishStatus();
    } else {
      Serial.print("failed, rc=");
      Serial.println(mqttClient.state());
      mqttConnected = false;
      vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
  }
}

void publishSensorData() {
  if (!mqttClient.connected()) return;
  
  float temp, hum;
  if (xSemaphoreTake(tempMutex, portMAX_DELAY) == pdTRUE) {
    temp = currentTemp;
    hum = currentHumidity;
    xSemaphoreGive(tempMutex);
  }
  
  StaticJsonDocument<128> doc;
  doc["temperature"] = temp;
  doc["humidity"] = hum;
  
  char buffer[128];
  serializeJson(doc, buffer);
  mqttClient.publish(TOPIC_SENSOR_DATA, buffer, false);
}

void publishStatus() {
  if (!mqttClient.connected()) return;
  
  float temp, hum;
  if (xSemaphoreTake(tempMutex, portMAX_DELAY) == pdTRUE) {
    temp = currentTemp;
    hum = currentHumidity;
    xSemaphoreGive(tempMutex);
  }
  
  StaticJsonDocument<512> doc;
  doc["temperature"] = temp;
  doc["humidity"] = hum;
  doc["heater"]["enabled"] = heaterEnabled;
  doc["heater"]["pwm"] = pwmValue;
  doc["spray"]["active"] = sprayActive;
  doc["settings"]["tempMin"] = tempMin;
  doc["settings"]["tempMax"] = tempMax;
  doc["settings"]["humidityOn"] = humidityOnThreshold;
  doc["settings"]["humidityOff"] = humidityOffThreshold;
  doc["settings"]["sprayDuration"] = sprayDuration;
  doc["uptime"] = millis() / 1000;
  
  char buffer[512];
  serializeJson(doc, buffer);
  mqttClient.publish(TOPIC_STATUS, buffer, true);
}

void publishEvent(const char* eventType, const char* value) {
  if (!mqttClient.connected()) return;
  
  StaticJsonDocument<128> doc;
  doc["type"] = eventType;
  doc["value"] = value;
  doc["timestamp"] = millis() / 1000;
  
  char buffer[128];
  serializeJson(doc, buffer);
  mqttClient.publish(TOPIC_EVENT, buffer, false);
}

void mqttTask(void *parameter) {
  while (1) {
    if (WiFi.status() != WL_CONNECTED) {
      connectWiFi();
      vTaskDelay(5000 / portTICK_PERIOD_MS);
      continue;
    }
    
    if (!mqttClient.connected()) {
      connectMQTT();
    }
    
    mqttClient.loop();
    
    unsigned long now = millis();
    if (now - lastHeartbeat > 5000) {
      publishSensorData();
      publishStatus();
      lastHeartbeat = now;
    }
    
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(HEATER_LED_PIN, OUTPUT);
  pinMode(SPRAY_LED_PIN, OUTPUT);
  
  dht.begin();
  
  tempMutex = xSemaphoreCreateMutex();
  
  Serial.println("=== ChickCare System Initialized ===");

  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(512);

  xTaskCreatePinnedToCore(temperatureTask, "TempTask", 4096, NULL, 1, &tempTaskHandle, 0);
  xTaskCreatePinnedToCore(controlTask, "ControlTask", 4096, NULL, 2, &controlTaskHandle, 1);
  xTaskCreatePinnedToCore(mqttTask, "MQTTTask", 8192, NULL, 1, &mqttTaskHandle, 1);
}

void loop() {
  displaySystemStatus();
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}

// TASK: Membaca suhu & kelembapan 
void temperatureTask(void *parameter) {
  while (1) {
    float newTemp = dht.readTemperature();
    float newHumidity = dht.readHumidity();

    if (!isnan(newTemp) && !isnan(newHumidity)) {
      if (xSemaphoreTake(tempMutex, portMAX_DELAY) == pdTRUE) {
        currentTemp = newTemp;
        currentHumidity = newHumidity;
        xSemaphoreGive(tempMutex);
      }
    }
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

// TASK: Mengontrol heater dan spray 
void controlTask(void *parameter) {
  while (1) {
    float temp, hum;
    if (xSemaphoreTake(tempMutex, portMAX_DELAY) == pdTRUE) {
      temp = currentTemp;
      hum = currentHumidity;
      xSemaphoreGive(tempMutex);
    }

    // Kontrol heater
    if (heaterEnabled) {
      controlHeaterLED(temp);
    } else {
      analogWrite(HEATER_LED_PIN, 0);
      pwmValue = 0;
    }
    
    if (temp > tempMax + 2.0) {
      publishEvent("alert", "temperature_high");
    } else if (temp < tempMin - 2.0) {
      publishEvent("alert", "temperature_low");
    }

    // Kontrol spray dehumidifier
    controlSpray(hum);

    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void controlHeaterLED(float temperature) {
  int newPwmValue;
  if (temperature < tempMin - TEMP_HYSTERESIS) {
    newPwmValue = 255;
    heaterEnabled = true;
  }
  else if (temperature > tempMax + TEMP_HYSTERESIS) {
    newPwmValue = 0;
    heaterEnabled = false;
  }
  else if (temperature < tempMin) {
    newPwmValue = 200;
    heaterEnabled = true;
  }
  else if (temperature > tempMax) {
    newPwmValue = 50;
    heaterEnabled = true;
  }
  else {
    newPwmValue = 128;
    heaterEnabled = true;
  }

  analogWrite(HEATER_LED_PIN, newPwmValue);
  pwmValue = newPwmValue;
}

void controlSpray(float humidity) {
  unsigned long now = millis();

  if (!sprayActive && humidity < humidityOnThreshold) {
    sprayActive = true;
    sprayStartTime = now;
    digitalWrite(SPRAY_LED_PIN, HIGH);
    Serial.println(">>> Dehumidifier Spray ON <<<");
    publishEvent("spray_change", "ON");
  }

  if (sprayActive && ((now - sprayStartTime > sprayDuration) || humidity > humidityOffThreshold)) {
    digitalWrite(SPRAY_LED_PIN, LOW);
    sprayActive = false;
    Serial.println(">>> Dehumidifier Spray OFF <<<");
    publishEvent("spray_change", "OFF");
  }
}

// FUNCTION: Menampilkan status sistem di Serial 
void displaySystemStatus() {
  float temp, hum;
  if (xSemaphoreTake(tempMutex, portMAX_DELAY) == pdTRUE) {
    temp = currentTemp;
    hum = currentHumidity;
    xSemaphoreGive(tempMutex);
  }

  Serial.print(" | Temp: ");
  Serial.print(temp);
  Serial.print("°C | Humidity: ");
  Serial.print(hum);
  Serial.print("% | Heater PWM: ");
  Serial.print(pwmValue);
  Serial.print(" | Spray: ");
  Serial.println(sprayActive ? "ON" : "OFF");
}