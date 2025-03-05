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

// data magneto
float mag1_x, mag1_y, mag1_z;
float mag2_x, mag2_y, mag2_z;
float filtMag1_x, filtMag1_y, filtMag1_z;
float filtMag2_x, filtMag2_y, filtMag2_z;

//Var pour calibration magnéto
float mag_min1[3] = {1000, 1000, 1000}, mag_max1[3] = {-1000, -1000, -1000};
float mag_min2[3] = {1000, 1000, 1000}, mag_max2[3] = {-1000, -1000, -1000};
float mag_offset1[3], mag_scale1[3];
float mag_offset2[3], mag_scale2[3];

// calibration gyro
float gyro_offset1[3] = {0, 0, 0};
float gyro_offset2[3] = {0, 0, 0};

// Covariances pour Kalman
float P1_X = 1.0, P1_Y = 1.0, P1_Z = 1.0;  
float P2_X = 1.0, P2_Y = 1.0, P2_Z = 1.0;  

// Var pour affichage toutes les 2sec
unsigned long previousMillis = 0;  
const long interval = 2000;  

//def quaternions
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

  //calibration des gyroscope
  calibrateGyro();

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
  float dt = 1.0f / sampleRate;  // dt = période d'échantillonnage


  // Lire les données des capteurs
  readIMU();


  // calibration
  mag1_x = (mag1.magnetic.x - mag_offset1[0]) / mag_scale1[0];
  mag1_y = (mag1.magnetic.y - mag_offset1[1]) / mag_scale1[1];
  mag1_z = (mag1.magnetic.z - mag_offset1[2]) / mag_scale1[2];
  mag2_x = (mag2.magnetic.x - mag_offset2[0]) / mag_scale2[0];
  mag2_y = (mag2.magnetic.y - mag_offset2[1]) / mag_scale2[1];
  mag2_z = (mag2.magnetic.z - mag_offset2[2]) / mag_scale2[2];


  // filtrage kalman
  kalman(mag1_x, &filtMag1_x, &P1_X, dt);
  kalman(mag1_y, &filtMag1_y, &P1_Y, dt);
  kalman(mag1_z, &filtMag1_z, &P1_Z, dt);
  kalman(mag2_x, &filtMag2_x, &P2_X, dt);
  kalman(mag2_y, &filtMag2_y, &P2_Y, dt);
  kalman(mag2_z, &filtMag2_z, &P2_Z, dt);


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
    filtMag1_z};

  FusionVector magnetometer2 = { 
    filtMag2_x,  
    filtMag2_y,
    filtMag2_z};

  // Mettre à jour l'AHRS
  FusionAhrsUpdate(&ahrs1, gyroscope1, accelerometer1, magnetometer1, 1.0f / sampleRate);
  FusionAhrsUpdate(&ahrs2, gyroscope2, accelerometer2, magnetometer2, 1.0f / sampleRate);

  // Récupérer l'orientation
  FusionEuler euler1 = FusionQuaternionToEuler(FusionAhrsGetQuaternion(&ahrs1));
  FusionEuler euler2 = FusionQuaternionToEuler(FusionAhrsGetQuaternion(&ahrs2));

  FusionQuaternion q1 = FusionAhrsGetQuaternion(&ahrs1);
  FusionQuaternion q2 = FusionAhrsGetQuaternion(&ahrs2);
  float q1_w = q1.element.w;
  float q1_x = q1.element.x;
  float q1_y = q1.element.y;
  float q1_z = q1.element.z;
  float q2_w = q2.element.w;
  float q2_x = q2.element.x;
  float q2_y = q2.element.y;
  float q2_z = q2.element.z;
  // Convertir q1 et q2 en tableaux de floats
  float q1Array[4] = {q1_w, q1_x, q1_y, q1_z};
  float q2Array[4] = {q2_w, q2_x, q2_y, q2_z};

  // Maintenant, utilisez q1Array au lieu de q1
  quaternionToEulerDegree(q1Array, roll1, pitch1, yaw1);
  quaternionToEulerDegree(q2Array, roll2, pitch2, yaw2);

  //calcul quaternion relatif
  quaternionInverse(q1Array, q1Inv);
  multiplicationQuaternion(q2Array, q1Inv, qr);

  //calcul angle relatif
  quaternionToEulerDegree(qr,roll,pitch,yaw);

  unsigned long currentMillis = millis(); // Récupérer le temps actuel
  if (currentMillis - previousMillis >= interval) { // Vérifie si 2 secondes se sont écoulées
    previousMillis = currentMillis; // Met à jour le dernier temps enregistré
    
    // Afficher les résultats
    Serial.println("Avec librairie:");
    float pitch_diff = euler1.angle.pitch - euler2.angle.pitch;
    Serial.print("Pitch Capteur 1: "); Serial.print(euler1.angle.pitch);
    Serial.print(" | Pitch Capteur 2: "); Serial.print(euler2.angle.pitch);
    Serial.print(" | Différence de Pitch: "); Serial.println(pitch_diff);

    float roll_diff = euler1.angle.roll - euler2.angle.roll;
    Serial.print("Roll Capteur 1: "); Serial.print(euler1.angle.roll);
    Serial.print(" | Roll Capteur 2: "); Serial.print(euler2.angle.roll);
    Serial.print(" | Différence de Roll: "); Serial.println(roll_diff);

    float yaw_diff = euler1.angle.yaw - euler2.angle.yaw;
    if (yaw_diff > 180.0f) yaw_diff -= 360.0f;
    if (yaw_diff < -180.0f) yaw_diff += 360.0f;
    Serial.print("Yaw Capteur 1: "); Serial.print(euler1.angle.yaw);
    Serial.print(" | Yaw Capteur 2: "); Serial.print(euler2.angle.yaw);
    Serial.print(" | Différence de Yaw: "); Serial.println(yaw_diff);

    Serial.println("A la main");
    Serial.print("Pitch Capteur 1: "); Serial.print(pitch1);
    Serial.print(" | Pitch Capteur 2: "); Serial.print(pitch2);
    Serial.print(" | Différence de Pitch: "); Serial.println(pitch);

    Serial.print("Roll Capteur 1: "); Serial.print(roll1);
    Serial.print(" | Roll Capteur 2: "); Serial.print(roll2);
    Serial.print(" | Différence de Roll: "); Serial.println(roll);

    Serial.print("Yaw Capteur 1: "); Serial.print(yaw1);
    Serial.print(" | Yaw Capteur 2: "); Serial.print(yaw2);
    Serial.print(" | Différence de Yaw: "); Serial.println(yaw);


    Serial.println("--------------");

  delay(100);
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

void kalman(float newValue, float *state, float *P, float dt){
  float Q = 0.001;  // Bruit du modèle
  float R = 0.03;   // Bruit de la mesure 
  // mise à jour covariance
  *P += Q * dt;
  // calcul gain de Kalman
  float K = *P / (*P + R);
  //mise à jour val filtré
  *state += K * (newValue - *state);
  // mise à jour covariance
  *P = (1 -K)* *P;
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
