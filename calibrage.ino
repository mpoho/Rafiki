#include <ESP32Servo.h>

// --- Configuration Matérielle (Pins correspondants à ton schéma Cirkit) ---
#define PIN_YL 4  // Jambe Gauche
#define PIN_YR 5  // Jambe Droite
#define PIN_RL 6  // Pied Gauche
#define PIN_RR 7  // Pied Droite

Servo servo_yl; 
Servo servo_yr; 
Servo servo_rl; 
Servo servo_rr;

void setup() {
  // Initialisation de la console de simulation
  Serial.begin(115200);
  delay(1000);
  Serial.println("--- SCRIPT DE CALIBRAGE ESP32-S3 ACTIF ---");
  Serial.println("[INFO] Alignement force de tous les servomoteurs a 0 degre.");

  // Allocation des Timers PWM nécessaires pour l'ESP32-S3
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  
  // Configuration de la fréquence standard des servos (50Hz)
  servo_yl.setPeriodHertz(50);
  servo_yr.setPeriodHertz(50);
  servo_rl.setPeriodHertz(50);
  servo_rr.setPeriodHertz(50);

  // Attachement des broches physiques (Pins 4, 5, 6, 7)
  servo_yl.attach(PIN_YL, 500, 2400);
  servo_yr.attach(PIN_YR, 500, 2400);
  servo_rl.attach(PIN_RL, 500, 2400);
  servo_rr.attach(PIN_RR, 500, 2400);

  // Écriture de la position de départ absolue (0°)
  servo_yl.write(0);
  servo_yr.write(0);
  servo_rl.write(0);
  servo_rr.write(0);

  Serial.println("[STATUT] Tous les moteurs sont positionnes et bloques a 0°.");
}

void loop() {
  // On laisse le loop vide pour maintenir le signal à 0° actif
}
