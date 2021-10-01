/*
Autour: Furkan Metin OĞUZ
Date:2021
*/

#include <ArduinoJson.h>

#include <PubSubClient.h>

#include <WiFi.h>

#include <stdio.h>

#include <stdlib.h>

#include <time.h>

#define WIFI_AP "Microzerr"
#define WIFI_PASSWORD "micro2017"

#include <Adafruit_MAX31856.h>

char thingsboardServer[] = "192.168.1.55";
#define TOKEN "sicaklikfmo1"
WiFiClient wifiClient;

PubSubClient client(wifiClient);

int status = WL_IDLE_STATUS;

Adafruit_MAX31856 maxthermo = Adafruit_MAX31856(17, 16, 5, 4);

double pressureSensorValue = 0;
void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.println("MAX31856 thermocouple test");

  if (!maxthermo.begin()) {
    Serial.println("Could not initialize thermocouple.");
    while (1) delay(10);
  }

  maxthermo.setThermocoupleType(MAX31856_TCTYPE_K);

  Serial.print("Thermocouple type: ");
  switch (maxthermo.getThermocoupleType()) {
  case MAX31856_TCTYPE_B:
    Serial.println("B Type");
    break;
  case MAX31856_TCTYPE_E:
    Serial.println("E Type");
    break;
  case MAX31856_TCTYPE_J:
    Serial.println("J Type");
    break;
  case MAX31856_TCTYPE_K:
    Serial.println("K Type");
    break;
  case MAX31856_TCTYPE_N:
    Serial.println("N Type");
    break;
  case MAX31856_TCTYPE_R:
    Serial.println("R Type");
    break;
  case MAX31856_TCTYPE_S:
    Serial.println("S Type");
    break;
  case MAX31856_TCTYPE_T:
    Serial.println("T Type");
    break;
  case MAX31856_VMODE_G8:
    Serial.println("Voltage x8 Gain mode");
    break;
  case MAX31856_VMODE_G32:
    Serial.println("Voltage x8 Gain mode");
    break;
  default:
    Serial.println("Unknown");
    break;
  }

  maxthermo.setConversionMode(MAX31856_ONESHOT_NOWAIT);
  delay(10);
  InitWiFi();
  client.setServer(thingsboardServer, 1883);
  client.setCallback(on_message);
}

void loop() {

  maxthermo.triggerOneShot();

  delay(500);

  Serial.println(maxthermo.readThermocoupleTemperature());
  if (!client.connected()) {
    reconnect();
  }

  client.loop();
  if (client.connect("ESP32 Device", TOKEN, NULL)) {
    Serial.println("[DONE]");
    StaticJsonDocument < 256 > JSONbuffer;
    JsonObject veri = JSONbuffer.createNestedObject();

    pressureSensorValue = maxthermo.readThermocoupleTemperature();
    pressureSensorValue = pressureSensorValue;

    veri["sensorValue"] = pressureSensorValue;

    char JSONmessageBuffer[200];
    serializeJsonPretty(JSONbuffer, JSONmessageBuffer);
    Serial.println("Sending message to MQTT topic..");
    Serial.println(JSONmessageBuffer);
    if (client.publish("v1/devices/me/telemetry", JSONmessageBuffer) == true) {
      Serial.println("Success sending message");
    } else {
      Serial.println("Error sending message");
    }
  }
}
void InitWiFi() {
  Serial.println("Connecting to AP ...");

  WiFi.begin(WIFI_AP, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to AP");
  Serial.print("MAC Address:  ");
  Serial.println(WiFi.macAddress());
  Serial.println("IP Address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {

  while (!client.connected()) {
    status = WiFi.status();
    if (status != WL_CONNECTED) {
      WiFi.begin(WIFI_AP, WIFI_PASSWORD);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("Connected to AP");
    }
    Serial.print("Connecting to ThingsBoard node ...");

    if (client.connect("ESP32 Device", TOKEN, NULL)) {
      Serial.println("[DONE]");

      client.subscribe("v1/devices/me/attributes");
    } else {
      Serial.print("[FAILED] [ rc = ");
      Serial.print(client.state());
      Serial.println(" : retrying in 5 seconds]");

      delay(5000);
    }
  }
}

void on_message(const char * topic, byte * payload, unsigned int length) {

  Serial.println("On message");
  StaticJsonDocument < 200 > doc;

  char json[length + 1];
  strncpy(json, (char * ) payload, length);
  json[length] = '\0';

  Serial.print("Topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  Serial.println(json);

  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

}