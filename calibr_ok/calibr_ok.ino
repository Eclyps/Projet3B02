#include <Wire.h>
#include <Adafruit_ICM20948.h>
#include "MadgwickAHRS.h"

// Déclaration des IMUs
Adafruit_ICM20948 icm1, icm2;
Madgwick filter1, filter2;

// Stockage des valeurs min/max pour chaque axe (X, Y, Z)
float mag_min1[3] = {1000, 1000, 1000}, mag_max1[3] = {-1000, -1000, -1000};
float mag_min2[3] = {1000, 1000, 1000}, mag_max2[3] = {-1000, -1000, -1000};

// Offsets et échelle pour correction après calibration
float mag_offset1[3], mag_scale1[3];
float mag_offset2[3], mag_scale2[3];
float gyro_offset1[3] = {0, 0, 0};
float gyro_offset2[3] = {0, 0, 0};

unsigned long previousMillis = 0;  // Variable pour stocker le temps précédent
const long interval = 2000;  // Intervalle de 1000 millisecondes (1 seconde)



void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  Serial.println("Initialisation des IMUs...");

  if (!icm1.begin_I2C(0x68)) { 
    Serial.println("IMU 1 non détecté !");
    while (1) delay(10);
  }
  if (!icm2.begin_I2C(0x69)) { 
    Serial.println("IMU 2 non détecté !");
    while (1) delay(10);
  }

  //calibration des gyroscope
  calibrateGyro();

  // Calibration des magnétomètres
  calibrateMagnetometer();

  Serial.println("Les deux IMUs sont détectés !");
  filter1.begin(100.0f); // 100 Hz
  filter2.begin(100.0f);
}

void loop() {
  sensors_event_t accel1, gyro1, mag1, temp1;
  sensors_event_t accel2, gyro2, mag2, temp2;

  icm1.getEvent(&accel1, &gyro1, &temp1, &mag1);
  icm2.getEvent(&accel2, &gyro2, &temp2, &mag2);

  // Convertir les valeurs d'accélération de m/s² à g
  accel1.acceleration.x = accel1.acceleration.x / 9.81;
  accel1.acceleration.y = accel1.acceleration.y / 9.81;
  accel1.acceleration.z = accel1.acceleration.z / 9.81;
  accel2.acceleration.x = accel2.acceleration.x / 9.81;
  accel2.acceleration.y = accel2.acceleration.y / 9.81;
  accel2.acceleration.z = accel2.acceleration.z / 9.81;

  //Correction valeur magnétomètre
  mag1.magnetic.x = (mag1.magnetic.x - mag_offset1[0]) / mag_scale1[0];
  mag1.magnetic.y = (mag1.magnetic.y - mag_offset1[1]) / mag_scale1[1];
  mag1.magnetic.z = (mag1.magnetic.z - mag_offset1[2]) / mag_scale1[2];
  mag2.magnetic.x = (mag2.magnetic.x - mag_offset2[0]) / mag_scale2[0];
  mag2.magnetic.y = (mag2.magnetic.y - mag_offset2[1]) / mag_scale2[1];
  mag2.magnetic.z = (mag2.magnetic.z - mag_offset2[2]) / mag_scale2[2];


  //Correction gyroscope
  gyro1.gyro.x -= gyro_offset1[0];
  gyro1.gyro.y -= gyro_offset1[1];
  gyro1.gyro.z -= gyro_offset1[2];
  gyro2.gyro.x -= gyro_offset2[0];
  gyro2.gyro.y -= gyro_offset2[1];
  gyro2.gyro.z -= gyro_offset2[2];


  filter1.updateIMU(gyro1.gyro.x, gyro1.gyro.y, gyro1.gyro.z, accel1.acceleration.x, accel1.acceleration.y, accel1.acceleration.z);//, mag1.magnetic.x, mag1.magnetic.y, mag1.magnetic.z);
  filter2.updateIMU(gyro2.gyro.x, gyro2.gyro.y, gyro2.gyro.z, accel2.acceleration.x, accel2.acceleration.y, accel2.acceleration.z);//, mag2.magnetic.x, mag2.magnetic.y, mag2.magnetic.z);
  

  float q1[4] = { filter1.getQ0(), filter1.getQ1(), filter1.getQ2(), filter1.getQ3() };
  float q2[4] = { filter2.getQ0(), filter2.getQ1(), filter2.getQ2(), filter2.getQ3() };

  normalizeQuaternion(q1);
  normalizeQuaternion(q2);
  delay(10);

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // Sauvegarder le temps actuel pour le prochain affichage
    previousMillis = currentMillis;
    Serial.print("Q0:");
    Serial.print(filter1.getQ0());
    Serial.print("   Q1:");
    Serial.print(filter1.getQ1());
    Serial.print("   Q2:");
    Serial.println(filter1.getQ2());
    Serial.print("   Q3:");
    Serial.println(filter1.getQ3());
    Serial.println("------------------------------");

    Serial.print("Q0:");
    Serial.print(filter2.getQ0());
    Serial.print("   Q1:");
    Serial.print(filter2.getQ1());
    Serial.print("   Q2:");
    Serial.println(filter2.getQ2());
    Serial.print("   Q3:");
    Serial.println(filter2.getQ3());
    Serial.println("------------------------------");
  }
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


void normalizeQuaternion(float q[4]) {
  float norm = sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
  if (norm > 0.0f) {
    q[0] /= norm;
    q[1] /= norm;
    q[2] /= norm;
    q[3] /= norm;
  }

}

void calibrateGyro() {
  Serial.println("=== Calibration du Gyroscope ===");
  
  const int numSamples = 500;
  float sum1[3] = {0, 0, 0};
  float sum2[3] = {0, 0, 0};

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
