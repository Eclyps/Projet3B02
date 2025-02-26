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

void setup(void) {
  Serial.begin(115200);
  while (!Serial)
    delay(10); // Attente de l'ouverture du moniteur série

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

  // Configuration des plages de mesure pour le premier capteur
  setupIMU(icm1);

  // Configuration des plages de mesure pour le deuxième capteur
  setupIMU(icm2);
}

void loop() {
  // Lecture des données du premier capteur
  sensors_event_t accel1, gyro1, mag1, temp1;
  icm1.getEvent(&accel1, &gyro1, &temp1, &mag1);

  // Affichage des données du premier capteur
  displayIMUData("IMU 1", accel1, gyro1, mag1, temp1);

  // Lecture des données du deuxième capteur
  sensors_event_t accel2, gyro2, mag2, temp2;
  icm2.getEvent(&accel2, &gyro2, &temp2, &mag2);

  // Affichage des données du deuxième capteur
  displayIMUData("IMU 2", accel2, gyro2, mag2, temp2);

  delay(100);
}

// Fonction pour configurer l'IMU (plages de mesure et taux de données)
void setupIMU(Adafruit_ICM20948 &icm) {
  Serial.print("Accelerometer range set to: ");
  switch (icm.getAccelRange()) {
    case ICM20948_ACCEL_RANGE_2_G:
      Serial.println("+-2G");
      break;
    case ICM20948_ACCEL_RANGE_4_G:
      Serial.println("+-4G");
      break;
    case ICM20948_ACCEL_RANGE_8_G:
      Serial.println("+-8G");
      break;
    case ICM20948_ACCEL_RANGE_16_G:
      Serial.println("+-16G");
      break;
  }

  Serial.print("Gyro range set to: ");
  switch (icm.getGyroRange()) {
    case ICM20948_GYRO_RANGE_250_DPS:
      Serial.println("250 degrees/s");
      break;
    case ICM20948_GYRO_RANGE_500_DPS:
      Serial.println("500 degrees/s");
      break;
    case ICM20948_GYRO_RANGE_1000_DPS:
      Serial.println("1000 degrees/s");
      break;
    case ICM20948_GYRO_RANGE_2000_DPS:
      Serial.println("2000 degrees/s");
      break;
  }

  uint16_t accel_divisor = icm.getAccelRateDivisor();
  float accel_rate = 1125 / (1.0 + accel_divisor);
  Serial.print("Accelerometer data rate (Hz) is approximately: ");
  Serial.println(accel_rate);

  uint8_t gyro_divisor = icm.getGyroRateDivisor();
  float gyro_rate = 1100 / (1.0 + gyro_divisor);
  Serial.print("Gyro data rate (Hz) is approximately: ");
  Serial.println(gyro_rate);

  Serial.print("Magnetometer data rate set to: ");
  switch (icm.getMagDataRate()) {
    case AK09916_MAG_DATARATE_10_HZ:
      Serial.println("10 Hz");
      break;
    case AK09916_MAG_DATARATE_20_HZ:
      Serial.println("20 Hz");
      break;
    case AK09916_MAG_DATARATE_50_HZ:
      Serial.println("50 Hz");
      break;
    case AK09916_MAG_DATARATE_100_HZ:
      Serial.println("100 Hz");
      break;
  }
  Serial.println();
}

// Fonction pour afficher les données des capteurs
void displayIMUData(String label, sensors_event_t accel, sensors_event_t gyro, sensors_event_t mag, sensors_event_t temp) {
  Serial.print(label);
  Serial.println();

  Serial.print("\t\tAccel X: ");
  Serial.print(accel.acceleration.x);
  Serial.print(" \tY: ");
  Serial.print(accel.acceleration.y);
  Serial.print(" \tZ: ");
  Serial.print(accel.acceleration.z);
  Serial.println(" m/s^2");

  Serial.print("\t\tMag X: ");
  Serial.print(mag.magnetic.x);
  Serial.print(" \tY: ");
  Serial.print(mag.magnetic.y);
  Serial.print(" \tZ: ");
  Serial.print(mag.magnetic.z);
  Serial.println(" uT");

  Serial.print("\t\tGyro X: ");
  Serial.print(gyro.gyro.x);
  Serial.print(" \tY: ");
  Serial.print(gyro.gyro.y);
  Serial.print(" \tZ: ");
  Serial.print(gyro.gyro.z);
  Serial.println(" radians/s");

  Serial.println();
}
