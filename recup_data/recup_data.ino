#include "Wire.h"
#include "I2Cdev.h"
#include "MPU6050.h"

MPU6050 mpu1(0x68); //premier mpu adresse 0x68 car AD0 connecté au GND
MPU6050 mpu2(0x69); //second mpu adresse 0x69 car AD0 connecté a Vcc

void setup() {
    Wire.begin(); //démarre la communication I2C
    Serial.begin(9600); //démarre communication série
    mpu1.initialize(); //configuration basique
    if (mpu1.testConnection()) { //verif mpu1 connecté
        Serial.println("MPU6050 #1 connecté avec succès !");
    } else {
        Serial.println("Connexion #1 au MPU6050 échouée !");
    }
    mpu1.initialize();
    if (mpu2.testConnection()) { //verif mpu2 connecté
        Serial.println("MPU6050 #2 connecté avec succès !");
    } else {
        Serial.println("Connexion #2 au MPU6050 échouée !");
    }
}

void loop() {
    int16_t ax1, ay1, az1, gx1, gy1, gz1;
    int16_t ax2, ay2, az2, gx2, gy2, gz2;
    mpu1.getMotion6(&ax1, &ay1, &az1, &gx1, &gy1, &gz1); //récupère les données du capteur
    delay(10);
    mpu2.getMotion6(&ax2, &ay2, &az2, &gx2, &gy2, &gz2); //récupère les données du capteur

// Affichage des données du premier capteur
    Serial.print("MPU1 - Acc: ");
    Serial.print(ax1); Serial.print(", ");
    Serial.print(ay1); Serial.print(", ");
    Serial.print(az1); Serial.print(" | Gyro: ");
    Serial.print(gx1); Serial.print(", ");
    Serial.print(gy1); Serial.print(", ");
    Serial.println(gz1);
  
    // Affichage des données du deuxième capteur
    Serial.print("MPU2 - Acc: ");
    Serial.print(ax2); Serial.print(", ");
    Serial.print(ay2); Serial.print(", ");
    Serial.print(az2); Serial.print(" | Gyro: ");
    Serial.print(gx2); Serial.print(", ");
    Serial.print(gy2); Serial.print(", ");
    Serial.println(gz2);
  
    delay(1000);
}
