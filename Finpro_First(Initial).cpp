#include <DHT.h>

// Pin definitions
#define DHT_PIN 4
#define DHT_TYPE DHT11
#define HEATER_LED_PIN 15
#define SPRAY_LED_PIN 2

DHT dht(DHT_PIN, DHT_TYPE);

void setup() {
  Serial.begin(115200);
  pinMode(HEATER_LED_PIN, OUTPUT);
  pinMode(SPRAY_LED_PIN, OUTPUT);
  dht.begin();
  Serial.println("=== ChickCare System Initialized (Day 1) ===");
}

void loop() {
  float newTemp = dht.readTemperature();
  float newHumidity = dht.readHumidity();

  if (!isnan(newTemp) && !isnan(newHumidity)) {
    Serial.print("Temp: ");
    Serial.print(newTemp);
    Serial.print("°C | Humidity: ");
    Serial.print(newHumidity);
    Serial.println("%");
  } else {
    Serial.println("Failed to read from DHT sensor!");
  }
  delay(2000);
}