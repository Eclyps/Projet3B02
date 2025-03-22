#include <Wire.h>
#include <MPU9250_asukiaaa.h>
#include <MadgwickAHRS.h>  // Bibliothèque pour calculer les quaternions
#include <WiFi.h>
#include <PubSubClient.h>

// Paramètres WiFi
const char* ssid = "Lfo*";      // Remplacez par le nom de votre réseau WiFi
const char* password = "motdepasse"; // Remplacez par votre mot de passe WiFi

// Paramètres MQTT
const char* mqtt_server = "192.168.152.254"; // Adresse du broker MQTT
const int mqtt_port = 1883; // Port MQTT standard (non-sécurisé)
const char* mqtt_topic = "accx"; // Sujet pour publier et souscrire (non utilisé ici directement)

// Déclaration de la connexion WiFi et du client MQTT
WiFiClient espClient;
PubSubClient client(espClient);

// Instances des deux capteurs
MPU9250_asukiaaa mpu1(0x68); // Capteur 1
MPU9250_asukiaaa mpu2(0x69); // Capteur 2

// Instances Madgwick pour chaque capteur
Madgwick filter1;
Madgwick filter2;

// Fréquence d'échantillonnage (en Hz)
const float sampleFreq = 50.0; // 50 Hz (ajustez selon vos besoins)

// Fonction pour connecter au WiFi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connexion au réseau WiFi ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connecté");
  Serial.print("Adresse IP : ");
  Serial.println(WiFi.localIP());
}

// Fonction appelée lors de la réception d'un message MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message reçu sur le sujet : ");
  Serial.println(topic);

  Serial.print("Message : ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

// Fonction pour reconnecter le client MQTT si nécessaire
void reconnect() {
  while (!client.connected()) {
    Serial.print("Connexion au broker MQTT...");
    if (client.connect("ESP32Client")) { // Nom du client MQTT
      Serial.println("Connecté");
    } else {
      Serial.print("Echec, code erreur : ");
      Serial.print(client.state());
      Serial.println(". Nouvelle tentative dans 5 secondes...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  // Initialiser les capteurs
  mpu1.beginAccel();
  mpu1.beginGyro();
  mpu1.beginMag();
  
  mpu2.beginAccel();
  mpu2.beginGyro();
  mpu2.beginMag();
  
  // Initialiser les filtres Madgwick
  filter1.begin(sampleFreq);
  filter2.begin(sampleFreq);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Mise à jour des données des capteurs
  mpu1.accelUpdate();
  mpu1.gyroUpdate();
  mpu1.magUpdate();

  mpu2.accelUpdate();
  mpu2.gyroUpdate();
  mpu2.magUpdate();

  // Mise à jour des filtres Madgwick
  filter1.update(mpu1.gyroX(), mpu1.gyroY(), mpu1.gyroZ(), 
                 mpu1.accelX(), mpu1.accelY(), mpu1.accelZ(), 
                 mpu1.magX(), mpu1.magY(), mpu1.magZ());
                 
  filter2.update(mpu2.gyroX(), mpu2.gyroY(), mpu2.gyroZ(), 
                 mpu2.accelX(), mpu2.accelY(), mpu2.accelZ(), 
                 mpu2.magX(), mpu2.magY(), mpu2.magZ());

  // Obtenir les angles d'Euler (roll, pitch, yaw)
  float roll1 = filter1.getRoll();
  float pitch1 = filter1.getPitch();
  float yaw1 = filter1.getYaw();

  float roll2 = filter2.getRoll();
  float pitch2 = filter2.getPitch();
  float yaw2 = filter2.getYaw();

  Serial.println("envoie des données");
  client.publish("acc1X", String(mpu1.accelX()).c_str());
  client.publish("acc1Y", String(mpu1.accelY()).c_str());
  client.publish("acc1Z", String(mpu1.accelZ()).c_str());
  client.publish("gyro1X", String(mpu1.gyroX()).c_str());
  client.publish("gyro1Y", String(mpu1.gyroY()).c_str());
  client.publish("gyro1Z", String(mpu1.gyroZ()).c_str());
  client.publish("m1X", String(mpu1.magX()).c_str());
  client.publish("m1Y", String(mpu1.magY()).c_str());
  client.publish("m1Z", String(mpu1.magZ()).c_str());

  client.publish("acc2X", String(mpu2.accelX()).c_str());
  client.publish("acc2Y", String(mpu2.accelY()).c_str());
  client.publish("acc2Z", String(mpu2.accelZ()).c_str());
  client.publish("gyro2X", String(mpu2.gyroX()).c_str());
  client.publish("gyro2Y", String(mpu2.gyroY()).c_str());
  client.publish("gyro2Z", String(mpu2.gyroZ()).c_str());
  client.publish("m2X", String(mpu2.magX()).c_str());
  client.publish("m2Y", String(mpu2.magY()).c_str());
  client.publish("m2Z", String(mpu2.magZ()).c_str());

  client.publish("roll", String(roll1).c_str());
  client.publish("pitch", String(pitch1).c_str());
  client.publish("yaw", String(yaw1).c_str());

  delay(20); // Attendre pour correspondre à la fréquence d'échantillonnage (50 Hz)
}
