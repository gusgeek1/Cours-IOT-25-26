#include <Arduino.h>

// --- CONFIGURATION ---
#define PIN_PIR 13  // Entrée (Capteur)
#define PIN_LED 4   // Sortie (Lumière)

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n--- TEST FINAL : PIR + LED ---");
  Serial.println("1. Initialisation des broches...");
  
  pinMode(PIN_PIR, INPUT);
  pinMode(PIN_LED, OUTPUT);

  // --- ÉTAPE 1 : Test de la LED au démarrage ---
  // Permet de vérifier instantanément ta soudure sans attendre le PIR
  Serial.println("2. Test LED (3 clignotements rapides)...");
  for(int i=0; i<3; i++) {
    digitalWrite(PIN_LED, HIGH); // Allume
    delay(200);
    digitalWrite(PIN_LED, LOW);  // Éteint
    delay(200);
  }
  Serial.println("✅ Test LED terminé. Prêt pour la détection !");
  Serial.println(">>> PASSE TA MAIN DEVANT LE CAPTEUR <<<");
}

void loop() {
  // --- ÉTAPE 2 : Lecture du Capteur ---
  if (digitalRead(PIN_PIR) == HIGH) {
    
    // Mouvement détecté !
    Serial.println("🔥 MOUVEMENT DÉTECTÉ ! -> Allumage LED");
    
    // On allume la LED (Simulation prise de photo)
    digitalWrite(PIN_LED, HIGH);
    
    // On maintient la lumière pendant 3 secondes
    // (C'est le temps qu'il faudra pour le WiFi et la photo plus tard)
    delay(3000);
    
    // On éteint et on signale la fin
    digitalWrite(PIN_LED, LOW);
    Serial.println("💤 Fin de l'alerte. Retour en surveillance.");
    
    // Petite pause pour éviter de ré-déclencher immédiatement si le capteur est lent
    delay(1000);
    
  } else {
    // Si rien ne se passe, on ne fait rien (ou un petit point pour dire qu'on est vivant)
    // Serial.print("."); // Décommente si tu veux voir de l'activité
    delay(100);
  }
}