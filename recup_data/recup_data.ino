#include <Wire.h>
#include <MPU9250.h>

// Déclaration des objets pour les capteurs
MPU9250 mpu1; // Adresse I2C du premier capteur
MPU9250 mpu2; // Adresse I2C du deuxième capteur

float roll1, pitch1, yaw1;
float roll2, pitch2, yaw2;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  //Initialisation des capteurs
  if(!mpu1.setup(0x68)) {
    Serial.println("Failed to connect mpu1");
    while(1);
  }
  if(!mpu2.setup(0x69)) {
    Serial.println("Failed to connect mpu2");
    while(1);
  }

  //calibration à chaque boot
  mpu1.calibrateAccelGyro(); //rester immobile
  mpu2.calibrateAccelGyro(); 
  mpu1.calibrateMag(); //faire tourner
  mpu2.calibrateMag();

  mpu1.setFilterIterations(10); //permet une meilleure estimation de yaw que 1 par défaut
  mpu2.setFilterIterations(10);
}

void loop() {
  mpu1.update();
  mpu2.update();

  roll1 = mpu1.getEulerX();
  pitch1 = mpu1.getEulerY();
  yaw1 = mpu1.getEulerZ();
  roll2 = mpu2.getEulerX();
  pitch2 = mpu2.getEulerY();
  yaw2 = mpu2.getEulerZ();

  float kneeAngle = pitch1 - pitch2; //pour flexion/extention
  float kneeRotation = yaw1 - yaw2; //pour rotation interne/externe

  // Afficher les angles sur le moniteur série
  Serial.print("Roll1: "); Serial.print(roll1);
  Serial.print(" Pitch1: "); Serial.print(pitch1);
  Serial.print(" Yaw1: "); Serial.print(yaw1);
  Serial.print(" Roll2: "); Serial.print(roll2);
  Serial.print(" Pitch2: "); Serial.print(pitch2);
  Serial.print(" Yaw2: "); Serial.print(yaw2);
  Serial.print(" Knee Angle: "); Serial.print(kneeAngle);
  Serial.print(" Internal/External Rotation: "); Serial.println(kneeRotation);
}