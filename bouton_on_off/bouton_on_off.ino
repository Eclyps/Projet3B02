#define BUTTON_PIN 6 // ou une autre pin si besoin

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // On affiche l'état au démarrage
  Serial.print("Etat initial du bouton (HIGH=non appuyé): ");
  Serial.println(digitalRead(BUTTON_PIN));

  // Wakeup sur niveau bas : bouton appuyé = 0 = réveil
  esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, 0);

  // On conserve la pull-up pendant le deep sleep
  gpio_hold_en((gpio_num_t)BUTTON_PIN);

  Serial.println("ENTREE EN DEEP SLEEP");
  delay(100);  // Petit délai avant de dormir
  esp_deep_sleep_start();
}

void loop() {
  // Normalement jamais atteint après deep sleep
  Serial.println("Boucle loop...");
  delay(1000);
}
