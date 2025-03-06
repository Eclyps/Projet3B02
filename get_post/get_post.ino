#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_ICM20X.h>
#include <Adafruit_ICM20948.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

// Déclaration des deux capteurs
Adafruit_ICM20948 icm1;
Adafruit_ICM20948 icm2;

// Délais entre les mesures pour le test
uint16_t measurement_delay_us = 65535;

// Adresses I2C des capteurs (si différents)
#define ICM1_ADDR 0x68  // Adresse I2C pour le premier capteur
#define ICM2_ADDR 0x69  // Adresse I2C pour le deuxième capteur (si modifiée)


// Informations WiFi
const char* ssid = "Lfo*";
const char* password = "motdepasse";

// Adresse du serveur Flask
const char* serverAddress = "http://10.43.49.254:5000";

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);  // Attente de l'ouverture du moniteur série

  Serial.println("Adafruit ICM20948 test!");

  // Initialisation du premier capteur (I2C)
  if (!icm1.begin_I2C(ICM1_ADDR)) {
    Serial.println("Failed to find ICM20948 chip 1");
    while (1) {
      delay(10);
    }
  }
  Serial.println("ICM20948 chip 1 Found!");

  // Initialisation du deuxième capteur (I2C)
  if (!icm2.begin_I2C(ICM2_ADDR)) {
    Serial.println("Failed to find ICM20948 chip 2");
    while (1) {
      delay(10);
    }
  }
  Serial.println("ICM20948 chip 2 Found!");
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
  sensors_event_t accel1, gyro1, mag1, temp1;
  icm1.getEvent(&accel1, &gyro1, &temp1, &mag1);

  // Calculer la norme du vecteur d'accélération comme approximation de la vitesse
  float vitesse = sqrt(accel1.acceleration.x * accel1.acceleration.x + accel1.acceleration.y * accel1.acceleration.y + accel1.acceleration.z * accel1.acceleration.z);

  // Envoyer la vitesse calculée
  postVitesse(1, vitesse);
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
  vitesse = round(vitesse*10)/10;
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
  distance = round(distance*10)/10;
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
