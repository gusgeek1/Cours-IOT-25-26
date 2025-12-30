#include <Arduino.h>
#include <M5TimerCAM.h>

//Test pir et led

// --- CONFIGURATION DES PINS (GROVE) ---
// Sur le TimerCam, le port Grove utilise G4 et G13.
// Si ça ne marche pas, inverse juste ces deux chiffres :
#define PIN_JAUNE 4   // Ton fil Jaune (PIR Sensor Out ?)
#define PIN_BLANC 13  // Ton fil Blanc (LED IR ?)

void setup() {
  Serial.begin(115200);
  delay(2000); // Pause pour laisser le temps d'ouvrir le moniteur

  Serial.println("\n--- MODE DIAGNOSTIC MATÉRIEL ---");
  Serial.print("PIN PIR (Jaune) : GPIO "); Serial.println(PIN_JAUNE);
  Serial.print("PIN LED (Blanc) : GPIO "); Serial.println(PIN_BLANC);

  // Configuration des Entrées/Sorties
  pinMode(PIN_JAUNE, INPUT);  // Le PIR envoie un signal (on écoute)
  pinMode(PIN_BLANC, OUTPUT); // La LED reçoit un ordre (on commande)

  // Test rapide de la LED au démarrage (Clignotement 3x)
  Serial.println("Test de la LED...");
  for(int i=0; i<3; i++) {
    digitalWrite(PIN_BLANC, HIGH); // Allume
    delay(200);
    digitalWrite(PIN_BLANC, LOW);  // Eteint
    delay(200);
  }
  Serial.println("Début du test PIR (Passe ta main devant)...");
}

void loop() {
  // Lecture de l'état du PIR (0 ou 1)
  int etatPIR = digitalRead(PIN_JAUNE);

  if (etatPIR == HIGH) {
    // Mouvement détecté !
    Serial.println("MOUVEMENT DÉTECTÉ ! (PIR = 1) -> LED ON");
    digitalWrite(PIN_BLANC, HIGH); // On allume la LED
  } else {
    // Calme plat
    Serial.println("Aucun mouvement (PIR = 0) -> LED OFF");
    digitalWrite(PIN_BLANC, LOW);  // On éteint la LED
  }

  // Petite pause pour ne pas spammer l'écran trop vite
  delay(500);
}