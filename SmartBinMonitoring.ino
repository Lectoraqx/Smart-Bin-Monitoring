#include <WiFi.h>
#include <PubSubClient.h>

#define TRIG_PIN 5
#define ECHO_PIN 18
#define LED_GREEN 2
#define LED_RED 4

const char* ssid = "Wokwi-GUEST";
const char* password = "";

const char* mqttServer = "mqtt.netpie.io";
const int mqttPort = 1883;

const char* clientID = ".......";
const char* mqttUser = "......";
const char* mqttPass = "......";

WiFiClient wifiClient;
PubSubClient client(wifiClient);

unsigned long lastSend = 0;
float fullDistance = 100.0;  // ความลึกของถัง (cm)

void connectToMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect(clientID, mqttUser, mqttPass)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 2 seconds...");
      delay(2000);
    }
  }
}

float readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  int duration = pulseIn(ECHO_PIN, HIGH);
  return duration * 0.034 / 2;
}

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected!");

  client.setServer(mqttServer, mqttPort);
}

void loop() {
  if (!client.connected()) {
    connectToMQTT();
  }

  client.loop();

  if (millis() - lastSend > 5000) {
    float distance = readDistanceCM();
    float trashLevel = fullDistance - distance;

    String status;

    if (trashLevel > 80.0) {
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_GREEN, LOW);
      status = "FULL";
    } else {
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_GREEN, HIGH);
      status = "OK";
    }

    Serial.printf("Distance: %.2f cm | Trash Level: %.2f cm | Status: %s\n",
                  distance, trashLevel, status.c_str());

    client.publish("@msg/smartbin/level", String(trashLevel).c_str());
    client.publish("@msg/smartbin/status", status.c_str());

    String shadowData = "{";
    shadowData += "\"data\":{";
    shadowData += "\"level\":" + String(trashLevel) + ",";
    shadowData += "\"status\":\"" + status + "\"";
    shadowData += "}}";

    client.publish("@shadow/data/update", shadowData.c_str());

    lastSend = millis();
  }
}
