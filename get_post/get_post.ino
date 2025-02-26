#include <WiFi.h>
#include <HTTPClient.h>

// Informations WiFi
const char* ssid = "Lfo*";
const char* password = "motdepasse";

// Adresse du serveur Flask
const char* serverAddress = "http://10.43.49.254:5000";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
  }
  delay(10000);
}

void postAlerte(int postId) {
  HTTPClient http;
  String url = String(serverAddress) + "/post/alerte/" + String(postId);
  http.begin(url);
  int httpCode = http.POST("");
  if (httpCode > 0) {
    Serial.println("Alerte envoyée : " + String(postId));
  } else {
    Serial.println("Erreur lors de l'envoi de l'alerte");
  }
  http.end();
}

void postVitesse(int postId, float vitesse) {
  HTTPClient http;
  String url = String(serverAddress) + "/post/vitesse/" + String(postId) + "/" + String(vitesse);
  http.begin(url);
  int httpCode = http.POST("");
  if (httpCode > 0) {
    Serial.println("Vitesse envoyée : " + String(postId) + " - " + String(vitesse));
  } else {
    Serial.println("Erreur lors de l'envoi de la vitesse");
  }
  http.end();
}

void postDistance(int postId, float distance) {
  HTTPClient http;
  String url = String(serverAddress) + "/post/distance/" + String(postId) + "/" + String(distance);
  http.begin(url);
  int httpCode = http.POST("");
  if (httpCode > 0) {
    Serial.println("Distance envoyée : " + String(postId) + " - " + String(distance));
  } else {
    Serial.println("Erreur lors de l'envoi de la distance");
  }
  http.end();
}

void getAlerte() {
  HTTPClient http;
  String url = String(serverAddress) + "/get/alerte";
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("Alertes reçues : " + payload);
  } else {
    Serial.println("Erreur lors de la réception des alertes");
  }
  http.end();
}

void getVitesse() {
  HTTPClient http;
  String url = String(serverAddress) + "/get/vitesse";
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("Vitesses reçues : " + payload);
  } else {
    Serial.println("Erreur lors de la réception des vitesses");
  }
  http.end();
}

void getDistance() {
  HTTPClient http;
  String url = String(serverAddress) + "/get/distance";
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("Distances reçues : " + payload);
  } else {
    Serial.println("Erreur lors de la réception des distances");
  }
  http.end();
}
