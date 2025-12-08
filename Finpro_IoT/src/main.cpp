#include <DHT.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

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

// Time settings (demo mode: 1 jam = 1 detik)
#define DAY_HOURS 18
#define NIGHT_HOURS 6

// System variables
DHT dht(DHT_PIN, DHT_TYPE);
float currentTemp = 0;
float currentHumidity = 0;
bool isDayTime = true;

unsigned long systemStartTime;
int pwmValue = 0;
bool heaterEnabled = true;

// Dehumidifier variables
bool sprayActive = false;
unsigned long sprayStartTime = 0;

// FreeRTOS handles
TaskHandle_t tempTaskHandle;
TaskHandle_t timeTaskHandle;
TaskHandle_t controlTaskHandle;
SemaphoreHandle_t tempMutex;

void setup() {
  Serial.begin(115200);
  
  pinMode(HEATER_LED_PIN, OUTPUT);
  pinMode(SPRAY_LED_PIN, OUTPUT);
  
  dht.begin();
  systemStartTime = millis();
  
  tempMutex = xSemaphoreCreateMutex();
  
  Serial.println("=== ChickCare System Initialized ===");

  xTaskCreatePinnedToCore(temperatureTask, "TempTask", 4096, NULL, 1, &tempTaskHandle, 0);
  xTaskCreatePinnedToCore(timeManagementTask, "TimeTask", 2048, NULL, 1, &timeTaskHandle, 0);
  xTaskCreatePinnedToCore(controlTask, "ControlTask", 4096, NULL, 2, &controlTaskHandle, 1);
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

// TASK: Mengatur pergantian mode siang/malam 
void timeManagementTask(void *parameter) {
  while (1) {
    unsigned long elapsedSeconds = (millis() - systemStartTime) / 1000;
    unsigned long cycleTime = elapsedSeconds % (DAY_HOURS + NIGHT_HOURS);
    bool newIsDay = (cycleTime < DAY_HOURS);

    if (newIsDay != isDayTime) {
      isDayTime = newIsDay;
      Serial.print("Mode changed to: ");
      Serial.println(isDayTime ? "DAY" : "NIGHT");
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
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

    // Kontrol heater hanya aktif saat siang
    if (heaterEnabled && isDayTime) {
      controlHeaterLED(temp);
    } else {
      analogWrite(HEATER_LED_PIN, 0);
      pwmValue = 0;
    }

    // Kontrol spray dehumidifier
    controlSpray(hum);

    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

// FUNCTION: Kontrol Heater dengan PWM 
void controlHeaterLED(float temperature) {
  int newPwmValue;
  if (temperature < TARGET_TEMP_MIN - TEMP_HYSTERESIS) newPwmValue = 255;
  else if (temperature > TARGET_TEMP_MAX + TEMP_HYSTERESIS) newPwmValue = 0;
  else if (temperature < TARGET_TEMP_MIN) newPwmValue = 200;
  else if (temperature > TARGET_TEMP_MAX) newPwmValue = 50;
  else newPwmValue = 128;

  analogWrite(HEATER_LED_PIN, newPwmValue);
  pwmValue = newPwmValue;
}

// FUNCTION: Kontrol Dehumidifier Spray (non-blocking + histeresis) 
void controlSpray(float humidity) {
  unsigned long now = millis();

  // Nyalakan spray kalau kelembapan rendah
  if (!sprayActive && humidity < HUMIDITY_ON_THRESHOLD) {
    sprayActive = true;
    sprayStartTime = now;
    digitalWrite(SPRAY_LED_PIN, HIGH);
    Serial.println(">>> Dehumidifier Spray ON <<<");
  }

  // Matikan spray setelah durasi tertentu atau jika kelembapan sudah cukup tinggi
  if (sprayActive && ((now - sprayStartTime > SPRAY_DURATION_MS) || humidity > HUMIDITY_OFF_THRESHOLD)) {
    digitalWrite(SPRAY_LED_PIN, LOW);
    sprayActive = false;
    Serial.println(">>> Dehumidifier Spray OFF <<<");
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

  Serial.print(" | Mode: ");
  Serial.print(isDayTime ? "DAY" : "NIGHT");
  Serial.print(" | Temp: ");
  Serial.print(temp);
  Serial.print("°C | Humidity: ");
  Serial.print(hum);
  Serial.print("% | Heater PWM: ");
  Serial.print(pwmValue);
  Serial.print(" | Spray: ");
  Serial.println(sprayActive ? "ON" : "OFF");
}