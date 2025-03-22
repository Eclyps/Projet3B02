#include <Wire.h>
#include <Adafruit_ICM20948.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "Fusion.h"

// Id du protege tibia
int id = 0;

// Informations WiFi
const char* ssid = "Lfo*";
const char* password = "motdepasse";

// Adresse du serveur Flask
const char* serverAddress = "http://10.221.29.254:5000";

Adafruit_ICM20948 icm1, icm2;  // Instance du capteur
FusionAhrs ahrs1, ahrs2;       // Instance de l'algorithme Fusion

// Pin moteur
int pin = 18;
#define LED_PIN 13

// Déclarer les évènement des capteurs
sensors_event_t accel1, gyro1, temp1, mag1;
sensors_event_t accel2, gyro2, temp2, mag2;

//freq d'échantillonage
const float sampleRate = 100.0f;  // 100 Hz
unsigned long lastUpdate = 0;

//Var pour calibration magnéto
float mag_min1[3] = { 1000, 1000, 1000 }, mag_max1[3] = { -1000, -1000, -1000 };
float mag_min2[3] = { 1000, 1000, 1000 }, mag_max2[3] = { -1000, -1000, -1000 };
float mag_offset1[3], mag_scale1[3];
float mag_offset2[3], mag_scale2[3];

// calibration gyro
float gyro_offset1[3] = { 0, 0, 0 };
float gyro_offset2[3] = { 0, 0, 0 };

// data magneto
float mag1_x, mag1_y, mag1_z;
float mag2_x, mag2_y, mag2_z;
float filtMag1_x, filtMag1_y, filtMag1_z;
float filtMag2_x, filtMag2_y, filtMag2_z;

// Covariances pour Kalman
float P1_x = 1.0, P1_y = 1.0, P1_z = 1.0;
float P2_x = 1.0, P2_y = 1.0, P2_z = 1.0;

// gain Kalman
float K1_x = 1.0, K1_y = 1.0, K1_z = 1.0;
float K2_x = 1.0, K2_y = 1.0, K2_z = 1.0;

// estimation Kalman
float U1_x = 1.0, U1_y = 1.0, U1_z = 1.0;
float U2_x = 1.0, U2_y = 1.0, U2_z = 1.0;

//filtre PB acc
float filtax, filtay, filtaz;

// Variable des angles
float pitch_diff, roll_diff, yaw_diff;
float pitchOffset = 0;
float rollOffset = 0.0;
float yawOffset = 0.0;

// Variables pour les calculs de vitesse et distance
float distance = 0, vitesse = 0, distanceSprint = 0, vitesseMax = 0;

//Compteur pour post
int i = 1;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  initIMU();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  postMessage(id);

  // Initialiser la pin moteur en tant que sortie
  pinMode(pin, OUTPUT);

  //calibration des gyroscope
  calibrateGyro();

  // Calibration des magnétomètres
  calibrateMagnetometer();

  // Initialiser Fusion AHRS
  FusionAhrsInitialise(&ahrs1);
  FusionAhrsInitialise(&ahrs2);

  delay(2000);
  Serial.println("Bouge plus!");
  calculateInitialOffsets();
  lastUpdate = millis();
}

void loop() {
  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - lastUpdate;

  // Attendre l'intervalle de l’échantillonnage
  if (elapsedTime < (1000 / sampleRate)) {
    delay(1000 / sampleRate - elapsedTime);
    currentTime = millis();  // Mettre à jour le temps actuel après l'attente
  }

  float dt = (currentTime - lastUpdate) / 1000.0f;  // dt en secondes
  lastUpdate = currentTime;
  // Lire les données des capteurs
  readIMU();

  // calibration gyro
  gyro1.gyro.x -= gyro_offset1[0];
  gyro1.gyro.y -= gyro_offset1[1];
  gyro1.gyro.z -= gyro_offset1[2];
  gyro2.gyro.x -= gyro_offset2[0];
  gyro2.gyro.y -= gyro_offset2[1];
  gyro2.gyro.z -= gyro_offset2[2];

  // calibration mag
  mag1_x = (mag1.magnetic.x - mag_offset1[0]) / mag_scale1[0];
  mag1_y = (mag1.magnetic.y - mag_offset1[1]) / mag_scale1[1];
  mag1_z = (mag1.magnetic.z - mag_offset1[2]) / mag_scale1[2];
  mag2_x = (mag2.magnetic.x - mag_offset2[0]) / mag_scale2[0];
  mag2_y = (mag2.magnetic.y - mag_offset2[1]) / mag_scale2[1];
  mag2_z = (mag2.magnetic.z - mag_offset2[2]) / mag_scale2[2];

  // filtrage kalman
  filtMag1_x = Kalman(mag1_x, &P1_x, &K1_x, &U1_x);
  filtMag1_y = Kalman(mag1_y, &P1_y, &K1_y, &U1_y);
  filtMag1_z = Kalman(mag1_z, &P1_z, &K1_z, &U1_z);
  filtMag2_x = Kalman(mag2_x, &P2_x, &K2_x, &U2_x);
  filtMag2_y = Kalman(mag2_y, &P2_y, &K2_y, &U2_y);
  filtMag2_z = Kalman(mag2_z, &P2_z, &K2_z, &U2_z);

  // Permuter et inverser les axes pour correspondre à la nouvelle orientation
  FusionVector gyroscope1 = {
    gyro1.gyro.z * 57.2958f,   // Nouvel axe X (ancien Z)
    -gyro1.gyro.y * 57.2958f,  // Nouvel axe Y (ancien Y, inversé)
    gyro1.gyro.x * 57.2958f    // Nouvel axe Z (ancien X)
  };

  FusionVector gyroscope2 = {
    gyro2.gyro.z * 57.2958f,   // Nouvel axe X (ancien Z)
    -gyro2.gyro.y * 57.2958f,  // Nouvel axe Y (ancien Y, inversé)
    gyro2.gyro.x * 57.2958f    // Nouvel axe Z (ancien X)
  };

  FusionVector accelerometer1 = {
    accel1.acceleration.z / 9.81f,   // Nouvel axe X (ancien Z)
    -accel1.acceleration.y / 9.81f,  // Nouvel axe Y (ancien Y, inversé)
    accel1.acceleration.x / 9.81f    // Nouvel axe Z (ancien X)
  };

  FusionVector accelerometer2 = {
    accel2.acceleration.z / 9.81f,   // Nouvel axe X (ancien Z)
    -accel2.acceleration.y / 9.81f,  // Nouvel axe Y (ancien Y, inversé)
    accel2.acceleration.x / 9.81f    // Nouvel axe Z (ancien X)
  };

  FusionVector magnetometer1 = {
    filtMag1_z,   // Nouvel axe X (ancien Z)
    -filtMag1_y,  // Nouvel axe Y (ancien Y, inversé)
    filtMag1_x    // Nouvel axe Z (ancien X)
  };

  FusionVector magnetometer2 = {
    filtMag2_z,   // Nouvel axe X (ancien Z)
    -filtMag2_y,  // Nouvel axe Y (ancien Y, inversé)
    filtMag2_x    // Nouvel axe Z (ancien X)
  };

  // Mettre à jour l'AHRS
  FusionAhrsUpdate(&ahrs1, gyroscope1, accelerometer1, magnetometer1, dt);
  FusionAhrsUpdate(&ahrs2, gyroscope2, accelerometer2, magnetometer2, dt);

  // Récupérer l'orientation
  FusionEuler euler1 = FusionQuaternionToEuler(FusionAhrsGetQuaternion(&ahrs1));
  FusionEuler euler2 = FusionQuaternionToEuler(FusionAhrsGetQuaternion(&ahrs2));

  //  Calcul différence angle
  pitch_diff = (euler1.angle.pitch - euler2.angle.pitch) - pitchOffset;  //rotation
  roll_diff = (euler1.angle.roll - euler2.angle.roll) - rollOffset;      // flex
  yaw_diff = euler1.angle.yaw - euler2.angle.yaw;
  Serial.print("pitch: ");
  Serial.println(pitch_diff);
  Serial.print("roll: ");
  Serial.println(roll_diff);
  Serial.print("yaw: ");
  Serial.println(yaw_diff);
  Serial.print(pitch_diff);
  Serial.print("\t");
  Serial.print(roll_diff);
  Serial.print("\t");
  Serial.println(yaw_diff);



  // 0 pour pitch c'est jambe droite tendu apres en pliant c'est val positive
  // rotation interne négative (roll) roation externe positive (roll)
  // celui sur la cuisse doit etre connecté à Vcc selui sur le tibia à GND pour gérer les adresses
  // voir photo pour voir sens capteur

  //Pour une jambe droite
  // if(pitch_diff < 30 && roll_diff < -15){
  //   digitalWrite(pin, HIGH);
  //   postAlerte(id);
  //   digitalWrite(pin, LOW);
  // } else if(roll_diff < -15){
  //   digitalWrite(pin, HIGH);
  //   postAlerte(id);
  //   digitalWrite(pin, LOW);
  // } else if(roll_diff > 30 && pitch_diff < 110){
  //   digitalWrite(pin, HIGH);
  //   postAlerte(id);
  //   digitalWrite(pin, LOW);
  // } else if (roll_diff > 35 && pitch_diff > 110){
  //   digitalWrite(pin, HIGH);
  //   postAlerte(id);
  //   digitalWrite(pin, LOW);
  // }



  // Calculs des métriques
  calculsVitesseDistance(dt, vitesse, distance, vitesseMax, distanceSprint);

  Serial.print("Vitesse: ");
  Serial.print(vitesse);
  Serial.print(" m/s, ");
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.print(" m, ");
  Serial.print("Vitesse Max: ");
  Serial.print(vitesseMax);
  Serial.print(" m/s, ");
  Serial.print("Distance Sprint: ");
  Serial.print(distanceSprint);
  Serial.println(" m");

  //post toutes les métriques tout les 500 ms
  switch (i) {
    case 1:
      postVitesse(id, vitesse);
      break;
    case 2:
      postDistance(id, distance);
      break;
    case 3:
      postVitesseMax(id, vitesseMax);
      break;
    case 4:
      postDistanceSprint(id, distanceSprint);
      i = 0;  // reset compteur
      break;
  }
  i++;
}


void calculateInitialOffsets() {
  const int numSamples = 100;  // Nombre de mesures à prendre
  float pitchSum = 0.0;
  float rollSum = 0.0;

  for (int j = 0; j < numSamples; j++) {
    // Lire les données des capteurs
    readIMU();

    // Convertir les données pour Fusion
    FusionVector gyroscope1 = {
      gyro1.gyro.x * 57.2958f,  // rad/s → °/s
      gyro1.gyro.y * 57.2958f,
      gyro1.gyro.z * 57.2958f
    };

    FusionVector gyroscope2 = {
      gyro2.gyro.x * 57.2958f,  // rad/s → °/s
      gyro2.gyro.y * 57.2958f,
      gyro2.gyro.z * 57.2958f
    };

    FusionVector accelerometer1 = {
      accel1.acceleration.x / 9.81f,  // m/s² → g
      accel1.acceleration.y / 9.81f,
      accel1.acceleration.z / 9.81f
    };

    FusionVector accelerometer2 = {
      accel2.acceleration.x / 9.81f,  // m/s² → g
      accel2.acceleration.y / 9.81f,
      accel2.acceleration.z / 9.81f
    };

    FusionVector magnetometer1 = {
      filtMag1_x,
      filtMag1_y,
      filtMag1_z
    };

    FusionVector magnetometer2 = {
      filtMag2_x,
      filtMag2_y,
      filtMag2_z
    };

    // Mettre à jour l'AHRS
    FusionAhrsUpdate(&ahrs1, gyroscope1, accelerometer1, magnetometer1, 0.01f);
    FusionAhrsUpdate(&ahrs2, gyroscope2, accelerometer2, magnetometer2, 0.01f);

    if (j > 50) {
      // Récupérer l'orientation initiale
      FusionEuler initialEuler1 = FusionQuaternionToEuler(FusionAhrsGetQuaternion(&ahrs1));
      FusionEuler initialEuler2 = FusionQuaternionToEuler(FusionAhrsGetQuaternion(&ahrs2));

      // Accumuler les différences d'angles
      pitchSum += (initialEuler1.angle.pitch - initialEuler2.angle.pitch);
      rollSum += (initialEuler1.angle.roll - initialEuler2.angle.roll);
      Serial.print(initialEuler1.angle.pitch - initialEuler2.angle.pitch);
      Serial.print("\t");
      Serial.println(initialEuler1.angle.roll - initialEuler2.angle.roll);
    }
  }

  // Calculer les offsets moyens
  pitchOffset = pitchSum / (numSamples / 2);
  rollOffset = rollSum / (numSamples / 2);

  Serial.print("pitchOffset: ");
  Serial.println(pitchOffset);
  Serial.print("rollOffset: ");
  Serial.println(rollOffset);
  Serial.print("yawOffset: ");
  delay(3000);
}

void lowPassFilter(float currentValue, float& filteredValue, float alpha = 0.8) {
  filteredValue = alpha * filteredValue + (1 - alpha) * currentValue;
}

void calculsVitesseDistance(float deltaTime, float& vitesse, float& distance, float& vitesseMax, float& distanceSprint) {
  //Serial.println(deltaTime);
  FusionVector linearAcc = FusionAhrsGetLinearAcceleration(&ahrs1);
  float ax = linearAcc.array[0] * 9.81;
  float ay = linearAcc.array[1] * 9.81;
  float az = linearAcc.array[2] * 9.81;

  // Appliquer un filtre passe-bas à l'accélération pour réduire les bruits
  lowPassFilter(ax, filtax, 0.6);  // Utiliser un facteur plus élevé pour plus de filtrage
  lowPassFilter(ay, filtay, 0.6);  // Idem pour Y
  lowPassFilter(az, filtaz, 0.6);  // Idem pour Z

  // Calculer la magnitude de l'accélération (accélération totale)
  float accelMagnitude = sqrt(filtax * filtax + filtay * filtay + filtaz * filtaz);

  // Définir un seuil pour l'accélération afin de considérer l'objet comme immobile
  float seuilAcc = 0.7;  // Seuil d'accélération

  // Si l'accélération est inférieure au seuil, considérer l'objet immobile
  if (accelMagnitude < seuilAcc) {
    vitesse = 0;  // Pas de mouvement, vitesse nulle
  } else {
    if (sqrt(filtax * filtax) > 0.25) {
      vitesse += filtax * deltaTime;
    }
    if (sqrt(filtay * filtay) > 0.25) {
      vitesse += filtay * deltaTime;
    }
    if (sqrt(filtaz * filtaz) > 0.25) {
      vitesse += filtaz * deltaTime;
    }
  }
  // Limiter la vitesse à une valeur maximale réaliste
  float vitesseMaxLimite = 10.0;
  if (vitesse > vitesseMaxLimite) {
    vitesse = vitesseMaxLimite;
  }

  // Assurer que la vitesse ne soit pas inférieure à zéro
  if (vitesse < 0) {
    vitesse = 0;
  }

  // Calculer la distance parcourue
  distance += vitesse * deltaTime;

  // Suivi de la vitesse maximale et de la distance de sprint
  if (vitesse > vitesseMax) vitesseMax = vitesse;
  if (vitesse > 6.94) distanceSprint += vitesse * deltaTime;  // Distance parcourue en sprint (vitesse > 25 km/h)

  // Affichage de la vitesse pour débogage
  Serial.print(vitesse);
  Serial.println("\t");
}

float Kalman(float U, float* P, float* K, float* U_hat) {
  //def constantes
  static float R = 3;    //noise covariance
  static float H = 1.0;  //measurment map scalar
  static float Q = 1;    //initial estimated covariance

  //begin
  *K = (*P) * H / (H * (*P) * H + R);         //update gain
  *U_hat = *U_hat + *K * (U - H * (*U_hat));  //update estimation

  //update error covariance
  *P = (1 - *K * H) * (*P) * Q;

  return *U_hat;  //retourne la valeur estimé
}

void readIMU() {
  icm1.getEvent(&accel1, &gyro1, &temp1, &mag1);
  icm2.getEvent(&accel2, &gyro2, &temp2, &mag2);
}

void calibrateGyro() {
  Serial.println("=== Calibration du Gyroscope ===");

  const int numSamples = 500;
  float sum1[3] = { 0, 0, 0 };
  float sum2[3] = { 0, 0, 0 };

  for (int i = 0; i < numSamples; i++) {
    sensors_event_t accel1, gyro1, mag1, temp1;
    sensors_event_t accel2, gyro2, mag2, temp2;

    icm1.getEvent(&accel1, &gyro1, &temp1, &mag1);
    icm2.getEvent(&accel2, &gyro2, &temp2, &mag2);

    sum1[0] += gyro1.gyro.x;
    sum1[1] += gyro1.gyro.y;
    sum1[2] += gyro1.gyro.z;

    sum2[0] += gyro2.gyro.x;
    sum2[1] += gyro2.gyro.y;
    sum2[2] += gyro2.gyro.z;

    delay(5);
  }

  for (int i = 0; i < 3; i++) {
    gyro_offset1[i] = sum1[i] / numSamples;
    gyro_offset2[i] = sum2[i] / numSamples;
    Serial.println(gyro_offset1[i]);
    Serial.println(gyro_offset2[i]);
  }

  Serial.println("Gyroscope calibré !");
}

void calibrateMagnetometer() {
  Serial.println("=== DÉBUT CALIBRATION DES MAGNÉTOMÈTRES ===");
  Serial.println("Tournez les IMUs dans toutes les directions pendant 30s...");

  unsigned long startTime = millis();
  const unsigned long calibrationTime = 30000;  // 30 secondes

  // Initialisation des min/max pour chaque axe
  for (int i = 0; i < 3; i++) {
    mag_min1[i] = mag_min2[i] = 1000;
    mag_max1[i] = mag_max2[i] = -1000;
  }

  // Collecte des valeurs pendant 30s
  while (millis() - startTime < calibrationTime) {
    sensors_event_t accel1, gyro1, mag1, temp1;
    sensors_event_t accel2, gyro2, mag2, temp2;

    icm1.getEvent(&accel1, &gyro1, &temp1, &mag1);
    icm2.getEvent(&accel2, &gyro2, &temp2, &mag2);

    float mag1_values[3] = { mag1.magnetic.x, mag1.magnetic.y, mag1.magnetic.z };
    float mag2_values[3] = { mag2.magnetic.x, mag2.magnetic.y, mag2.magnetic.z };

    // Mise à jour des min/max pour les 3 axes
    for (int i = 0; i < 3; i++) {
      mag_min1[i] = min(mag_min1[i], mag1_values[i]);
      mag_max1[i] = max(mag_max1[i], mag1_values[i]);
      mag_min2[i] = min(mag_min2[i], mag2_values[i]);
      mag_max2[i] = max(mag_max2[i], mag2_values[i]);
    }

    delay(10);
  }

  Serial.println("=== FIN DE LA CALIBRATION ===");

  // Calcul des offsets et échelles
  for (int i = 0; i < 3; i++) {
    mag_offset1[i] = (mag_max1[i] + mag_min1[i]) / 2.0;
    mag_scale1[i] = (mag_max1[i] - mag_min1[i]) / 2.0;
    mag_offset2[i] = (mag_max2[i] + mag_min2[i]) / 2.0;
    mag_scale2[i] = (mag_max2[i] - mag_min2[i]) / 2.0;
  }

  // Affichage des résultats
  Serial.println("Calibration des magnétomètres terminée !");
  for (int i = 0; i < 3; i++) {
    Serial.print("IMU1 - Offset ");
    Serial.print((char)('X' + i));
    Serial.print(": ");
    Serial.print(mag_offset1[i]);
    Serial.print("  Scale: ");
    Serial.println(mag_scale1[i]);

    Serial.print("IMU2 - Offset ");
    Serial.print((char)('X' + i));
    Serial.print(": ");
    Serial.print(mag_offset2[i]);
    Serial.print("  Scale: ");
    Serial.println(mag_scale2[i]);
  }
}

void initIMU() {
  Serial.println("Initialisation des IMUs...");

  if (!icm1.begin_I2C(0x69)) {
    Serial.println("IMU 1 non détecté !");
    while (1) delay(10);
  }
  if (!icm2.begin_I2C(0x68)) {
    Serial.println("IMU 2 non détecté !");
    while (1) delay(10);
  }
  Serial.println("Les deux IMUs sont détectés !");
}

void postMessage(int postId) {
  HTTPClient http;
  String url = String(serverAddress) + "/post/message/" + String(postId);
  http.begin(url);
  int httpCode = http.POST("");
  if (httpCode > 0) {
    //Serial.println("Message envoyée : " + String(postId));
  } else {
    Serial.println("Erreur lors de l'envoi du message");
  }
  http.end();
}

void postAlerte(int postId) {
  HTTPClient http;
  String url = String(serverAddress) + "/post/alerte/" + String(postId);
  http.begin(url);
  int httpCode = http.POST("");
  if (httpCode > 0) {
    //Serial.println("Alerte envoyée : " + String(postId));
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
    //Serial.println("Vitesse envoyée : " + String(postId) + " - " + String(vitesse));
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
    //Serial.println("Distance envoyée : " + String(postId) + " - " + String(distance));
  } else {
    Serial.println("Erreur lors de l'envoi de la distance");
  }
  http.end();
}

void postVitesseMax(int postId, float vitesse) {
  HTTPClient http;
  String url = String(serverAddress) + "/post/vitesseMax/" + String(postId) + "/" + String(vitesse);
  http.begin(url);
  int httpCode = http.POST("");
  if (httpCode > 0) {
    //Serial.println("Vitesse max envoyée : " + String(postId) + " - " + String(vitesse));
  } else {
    Serial.println("Erreur lors de l'envoi de la vitesse max");
  }
  http.end();
}

void postDistanceSprint(int postId, float distance) {
  HTTPClient http;
  String url = String(serverAddress) + "/post/distanceSprint/" + String(postId) + "/" + String(distance);
  http.begin(url);
  int httpCode = http.POST("");
  if (httpCode > 0) {
    //Serial.println("Distance sprint envoyée : " + String(postId) + " - " + String(distance));
  } else {
    Serial.println("Erreur lors de l'envoi de la distance sprint");
  }
  http.end();
}
