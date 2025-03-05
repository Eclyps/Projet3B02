#include <Wire.h>
#include <Adafruit_ICM20948.h>
#include "Fusion.h"  // Bibliothèque Fusion

Adafruit_ICM20948 icm1, icm2;  // Instance du capteur
FusionAhrs ahrs1,ahrs2;        // Instance de l'algorithme Fusion

// Déclarer les évènement des capteurs
sensors_event_t accel1, gyro1, temp1, mag1;
sensors_event_t accel2, gyro2, temp2, mag2;

//freq d'échantillonage
const float sampleRate = 100.0f;  // 100 Hz
unsigned long lastUpdate = 0;

//Var pour calibration magnéto
float mag_min1[3] = {1000, 1000, 1000}, mag_max1[3] = {-1000, -1000, -1000};
float mag_min2[3] = {1000, 1000, 1000}, mag_max2[3] = {-1000, -1000, -1000};
float mag_offset1[3], mag_scale1[3];
float mag_offset2[3], mag_scale2[3];

// Var pour affichage toutes les 2sec
unsigned long previousMillis = 0;  
const long interval = 2000;  

void setup() {
  Serial.begin(115200);
  while (!Serial); 

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

  // Configurer les capteurs
  // icm1.setAccelRange(ICM20948_ACCEL_RANGE_4_G);
  // icm1.setGyroRange(ICM20948_GYRO_RANGE_250_DPS);
  // icm1.setMagDataRate(AK09916_MAG_DATARATE_100_HZ);
  // icm2.setAccelRange(ICM20948_ACCEL_RANGE_4_G);
  // icm2.setGyroRange(ICM20948_GYRO_RANGE_250_DPS);
  // icm2.setMagDataRate(AK09916_MAG_DATARATE_100_HZ);


  // Calibration des magnétomètres
  calibrateMagnetometer();

  // Initialiser Fusion AHRS
  FusionAhrsInitialise(&ahrs1);
  FusionAhrsInitialise(&ahrs2);
}

void loop() {
  // Attendre l'intervalle de l’échantillonnage
  if (millis() - lastUpdate < (1000 / sampleRate)) return;
  lastUpdate = millis();

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
    (mag1.magnetic.x - mag_offset1[0]) / mag_scale1[0],  //calibration
    (mag1.magnetic.y - mag_offset1[1]) / mag_scale1[1],
    (mag1.magnetic.z - mag_offset1[2]) / mag_scale1[2]
  };

  FusionVector magnetometer2 = { 
    (mag2.magnetic.x - mag_offset2[0]) / mag_scale2[0],  //calibration
    (mag2.magnetic.y - mag_offset2[1]) / mag_scale2[1],
    (mag2.magnetic.z - mag_offset2[2]) / mag_scale2[2]
  };

  // Mettre à jour l'AHRS
  FusionAhrsUpdate(&ahrs1, gyroscope1, accelerometer1, magnetometer1, 1.0f / sampleRate);
  FusionAhrsUpdate(&ahrs2, gyroscope2, accelerometer2, magnetometer2, 1.0f / sampleRate);

  // Récupérer l'orientation
  FusionEuler euler1 = FusionQuaternionToEuler(FusionAhrsGetQuaternion(&ahrs1));
  FusionEuler euler2 = FusionQuaternionToEuler(FusionAhrsGetQuaternion(&ahrs2));

  unsigned long currentMillis = millis(); // Récupérer le temps actuel
  if (currentMillis - previousMillis >= interval) { // Vérifie si 2 secondes se sont écoulées
    previousMillis = currentMillis; // Met à jour le dernier temps enregistré
    // Afficher les résultats
    float pitch_diff = euler1.angle.pitch - euler2.angle.pitch;
    Serial.print("Pitch Capteur 1: "); Serial.print(euler1.angle.pitch);
    Serial.print(" | Pitch Capteur 2: "); Serial.print(euler2.angle.pitch);
    Serial.print(" | Différence de Pitch: "); Serial.println(pitch_diff);

    float roll_diff = euler1.angle.roll - euler2.angle.roll;
    Serial.print("Roll Capteur 1: "); Serial.print(euler1.angle.roll);
    Serial.print(" | Roll Capteur 2: "); Serial.print(euler2.angle.roll);
    Serial.print(" | Différence de Roll: "); Serial.println(roll_diff);

    float yaw_diff = euler1.angle.yaw - euler2.angle.yaw;
    Serial.print("Yaw Capteur 1: "); Serial.print(euler1.angle.yaw);
    Serial.print(" | Yaw Capteur 2: "); Serial.print(euler2.angle.yaw);
    Serial.print(" | Différence de Yaw: "); Serial.println(yaw_diff);

    Serial.println("--------------");

  delay(100);
  }
}


void readIMU(){
  icm1.getEvent(&accel1, &gyro1, &temp1, &mag1);
  icm2.getEvent(&accel2, &gyro2, &temp2, &mag2);
}

void calibrateMagnetometer() {
  Serial.println("=== DÉBUT CALIBRATION DES MAGNÉTOMÈTRES ===");
  Serial.println("Tournez les IMUs dans toutes les directions pendant 30s...");

  unsigned long startTime = millis();
  const unsigned long calibrationTime = 30000; // 30 secondes

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

    float mag1_values[3] = {mag1.magnetic.x, mag1.magnetic.y, mag1.magnetic.z};
    float mag2_values[3] = {mag2.magnetic.x, mag2.magnetic.y, mag2.magnetic.z};

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
