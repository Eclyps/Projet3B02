#include <Wire.h>
#include <Adafruit_ICM20948.h>
#include "MadgwickAHRS.h"
#include <math.h>


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

//Def quaternions
float q1[4];
float q2[4];
float q1Inv[4];
float qr[4];

//Def angles
float roll;
float pitch;
float yaw;
float roll1;
float pitch1;
float yaw1;
float roll2;
float pitch2;
float yaw2;

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
  
  Serial.println("Les deux IMUs sont détectés !");

  //calibration des gyroscope
  calibrateGyro();

  // Calibration des magnétomètres
  calibrateMagnetometer();

  //Init filtre Madgwick
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

  // Update val dans le filtre
  filter1.update(gyro1.gyro.x, gyro1.gyro.y, gyro1.gyro.z, accel1.acceleration.x, accel1.acceleration.y, accel1.acceleration.z, mag1.magnetic.x, mag1.magnetic.y, mag1.magnetic.z);
  filter2.update(gyro2.gyro.x, gyro2.gyro.y, gyro2.gyro.z, accel2.acceleration.x, accel2.acceleration.y, accel2.acceleration.z, mag2.magnetic.x, mag2.magnetic.y, mag2.magnetic.z);
  
  //Récupération des quaternions (ajout dans le code de la librairie)
  q1[0] = filter1.getQ0(); q1[1] = filter1.getQ1(); q1[2] = filter1.getQ2(); q1[3] = filter1.getQ3();
  q2[0] = filter2.getQ0(); q2[1] = filter2.getQ1(); q2[2] = filter2.getQ2(); q2[3] = filter2.getQ3();


  //Normalise si besoin
  normalizeQuaternion(q1);
  normalizeQuaternion(q2);

quaternionToEulerDegree(q1,roll1,pitch1,yaw1);
quaternionToEulerDegree(q2,roll2,pitch2,yaw2);

  //calcul quaternion relatif
  quaternionInverse(q1, q1Inv);
  multiplicationQuaternion(q2, q1Inv, qr);

  //calcul angle relatif
  quaternionToEulerDegree(qr,roll,pitch,yaw);

unsigned long currentMillis = millis(); // Récupérer le temps actuel
if (currentMillis - previousMillis >= interval) { // Vérifie si 2 secondes se sont écoulées
    previousMillis = currentMillis; // Met à jour le dernier temps enregistré

    // Affichage des résultats
    Serial.println("------------------------------");
    Serial.print("q1: ");
    for (int i = 0; i < 4; i++) {
        Serial.print(q1[i]); Serial.print(" ");
    }
    Serial.println();
    Serial.print("Roll: "); Serial.print(roll1); Serial.print("°, ");
    Serial.print("Pitch: "); Serial.print(pitch1); Serial.print("°, ");
    Serial.print("Yaw: "); Serial.print(yaw1); Serial.println("°");
    Serial.print("q2: ");
    for (int i = 0; i < 4; i++) {
        Serial.print(q2[i]); Serial.print(" ");
    }
    Serial.println();
    Serial.print("Roll: "); Serial.print(roll2); Serial.print("°, ");
    Serial.print("Pitch: "); Serial.print(pitch2); Serial.print("°, ");
    Serial.print("Yaw: "); Serial.print(yaw2); Serial.println("°");
    Serial.print("q1Inv: ");
    for (int i = 0; i < 4; i++) {
        Serial.print(q1Inv[i]); Serial.print(" ");
    }
    Serial.println();
    Serial.print("qr: ");
    for (int i = 0; i < 4; i++) {
        Serial.print(qr[i]); Serial.print(" ");
    }
    Serial.println("Angle globaux");
    Serial.print("Roll: "); Serial.print(roll); Serial.print("°, ");
    Serial.print("Pitch: "); Serial.print(pitch); Serial.print("°, ");
    Serial.print("Yaw: "); Serial.print(yaw); Serial.println("°");
    Serial.println();
}

}

void quaternionToEulerDegree(float q[4], float &roll, float &pitch, float &yaw){
  //calcul
  roll = atan2(2.0*(q[0]*q[1] + q[2]*q[3]) , 1.0 - 2.0*(q[0]*q[0] + q[1]*q[1]));
  pitch = asin(2.0 * (q[0] * q[2] - q[3] * q[1]));
  yaw = atan2(2.0*(q[0]*q[3] + q[1]*q[2]) , 1.0 - 2.0*(q[2]*q[2] + q[3]*q[3]));

  // Conversion en degrés
  roll  *= 180.0 / M_PI;
  pitch *= 180.0 / M_PI;
  yaw   *= 180.0 / M_PI;
}

void multiplicationQuaternion(float q1[4], float q2[4], float qm[4]){
  qm[0] = q1[0]*q2[0] - q1[1]*q2[1] - q1[2]*q2[2] - q1[3]*q2[3];
  qm[1] = q1[0]*q2[1] + q1[1]*q2[0] + q1[2]*q2[3] - q1[3]*q2[2];
  qm[2] = q1[0]*q2[2] - q1[1]*q2[3] + q1[2]*q2[0] + q1[3]*q2[1];
  qm[3] = q1[0]*q2[3] + q1[1]*q2[2] - q1[2]*q2[1] + q1[3]*q2[0];
}

void quaternionInverse(float q[4], float qInv[4]) {
  qInv[0] =  q[0];
  qInv[1] = -q[1];
  qInv[2] = -q[2];
  qInv[3] = -q[3];
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
