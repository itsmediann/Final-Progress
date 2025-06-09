#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "IMDAR";
const char* password = "imdar123";

const int ldrPin = 35;
const int ledPin = 13;
const int trigPin = 25;  // Changed to safer pins
const int echoPin = 27;

float lumenThreshold = 20.0;
float distanceThreshold = 100.0;

WebServer server(80);

volatile float latestDistance = 0.0;

float measureDistanceCM() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000);  // timeout 30ms to avoid blocking too long
  if (duration == 0) return -1; // no echo received
  float distance = duration * 0.034 / 2.0;
  return distance;
}

void handleStatus() {
  int analogValue = analogRead(ldrPin);
  float voltage = analogValue * (3.3 / 4095.0);
  float resistance = (3.3 - voltage) * 10000.0 / voltage;
  float lux = 500 / (resistance / 1000.0);

  float correctedDistance = 0.95 * latestDistance - 0.5;

  bool ledOn = (lux < lumenThreshold && correctedDistance <= distanceThreshold && correctedDistance > 0);
  digitalWrite(ledPin, ledOn ? HIGH : LOW);

  String json = "{";
  json += "\"lux\":" + String(lux, 2) + ",";
  json += "\"distance\":" + String(correctedDistance, 2) + ",";
  json += "\"led\":" + String(ledOn ? "true" : "false");
  json += "}";

  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  WiFi.begin(ssid, password);

  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connect, restarting...");
    ESP.restart();
  }

  Serial.println("");
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/status", handleStatus);
  server.begin();
}

void loop() {
  latestDistance = measureDistanceCM();
  server.handleClient();
  delay(50); // small delay to prevent watchdog reset
}