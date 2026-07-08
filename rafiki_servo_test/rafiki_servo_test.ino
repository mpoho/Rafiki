/**
 * @file rafiki_servo_test.ino
 * @brief Script de Test Unitaire des Servomoteurs - Version Arduino Uno
 * @author Antigravity (Principal Robotics Engineer)
 * 
 * Ce script permet de tester individuellement chaque servomoteur pour valider
 * le câblage, le sens de rotation et l'alignement mécanique.
 * Contrôle par chiffres via le Moniteur Série (115200 bauds).
 * 
 * BROCHAGE ARDUINO UNO :
 *  - Pin 4 -> Servo Jambe Gauche (YL)
 *  - Pin 5 -> Servo Jambe Droite (YR)
 *  - Pin 6 -> Servo Pied Gauche (RL)
 *  - Pin 7 -> Servo Pied Droite (RR)
 */

#include <Servo.h>

// --- Configuration des Broches ---
const int PIN_YL = 4;   // Servo Jambe Gauche
const int PIN_YR = 5;   // Servo Jambe Droite
const int PIN_RL = 6;   // Servo Pied Gauche
const int PIN_RR = 7;   // Servo Pied Droite

// --- Calibrage Matériel ---
const int BASE_ANGLE = 0;
const int TRIM_YL = 80; // Trim de la jambe gauche
const int TRIM_YR = 0;
const int TRIM_RL = 0;
const int TRIM_RR = 0;

// Servomoteurs
Servo servoYL;
Servo servoYR;
Servo servoRL;
Servo servoRR;

// Limiteur de vitesse angulaire maximale (degrés par seconde)
const float SERVO_SPEED_LIMIT = 100.0f; 

// Constante de temps pour le filtre de lissage exponentiel (en secondes)
const float SMOOTHING_TIME_CONSTANT = 0.15f; 

// --- États du Test Unitaire ---
int activeTest = 0; // 0 = Repos, 1 = YL, 2 = YR, 3 = RL, 4 = RR, 5 = Séquentiel

// --- Variables de Contrôle Temporel ---
unsigned long lastServoUpdateTime = 0;
float currentAngles[4] = {0.0f, 0.0f, 0.0f, 0.0f}; // Positions réelles des servos

void setup() {
  Serial.begin(115200);
  Serial.println(F("=== KIN OPERE - SERVO UNIT TEST ==="));

  // Attachement des Servos
  servoYL.attach(PIN_YL);
  servoYR.attach(PIN_YR);
  servoRL.attach(PIN_RL);
  servoRR.attach(PIN_RR);

  // Initialisation à l'angle de départ 0° + trims
  servoYL.write(BASE_ANGLE + TRIM_YL);
  servoYR.write(BASE_ANGLE + TRIM_YR);
  servoRL.write(BASE_ANGLE + TRIM_RL);
  servoRR.write(BASE_ANGLE + TRIM_RR);

  for (int i = 0; i < 4; i++) {
    currentAngles[i] = (float)BASE_ANGLE;
  }

  lastServoUpdateTime = millis();

  // Affichage du menu d'aide
  printTestMenu();
}

void loop() {
  // 1. Lecture de la commande série (chiffre de 0 à 5)
  readSerialCommand();

  // 2. Calcul des consignes cibles pour le test actif
  float targetAngles[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  calculateTargetAngles(targetAngles);

  // 3. Application du lissage et positionnement des servos
  applyServoSmoothness(targetAngles);
}

void printTestMenu() {
  Serial.println(F("\n--- MENU DE TEST UNITAIRE DES SERVO-MOTEURS ---"));
  Serial.println(F("  0 : Arrêt (Tous les moteurs retournent à 0° + trim)"));
  Serial.println(F("  1 : Tester la Jambe Gauche (YL - Pin 4, Trim: 80°)"));
  Serial.println(F("  2 : Tester la Jambe Droite (YR - Pin 5, Trim: 0°)"));
  Serial.println(F("  3 : Tester le Pied Gauche (RL - Pin 6, Trim: 0°)"));
  Serial.println(F("  4 : Tester le Pied Droite (RR - Pin 7, Trim: 0°)"));
  Serial.println(F("  5 : Tester tous les moteurs à la suite (Séquentiel)"));
  Serial.println(F("-----------------------------------------------"));
}

void readSerialCommand() {
  if (Serial.available() > 0) {
    char ch = Serial.read();

    // Filtre les retours à la ligne ou espaces
    if (ch == '\r' || ch == '\n' || ch == ' ') return;

    if (ch >= '0' && ch <= '5') {
      activeTest = ch - '0';
      Serial.print(F("ACK: TEST ACTIF -> "));
      if (activeTest == 0) Serial.println(F("REPOS"));
      else if (activeTest == 1) Serial.println(F("JAMBE GAUCHE (YL)"));
      else if (activeTest == 2) Serial.println(F("JAMBE DROITE (YR)"));
      else if (activeTest == 3) Serial.println(F("PIED GAUCHE (RL)"));
      else if (activeTest == 4) Serial.println(F("PIED DROITE (RR)"));
      else if (activeTest == 5) Serial.println(F("SEQUENTIEL"));
    } else {
      Serial.print(F("Entrée invalide : "));
      Serial.println(ch);
      printTestMenu();
    }
  }
}

void calculateTargetAngles(float targets[4]) {
  unsigned long now = millis();
  
  // Par défaut, tous les servos retournent à BASE_ANGLE (0°)
  for (int i = 0; i < 4; i++) {
    targets[i] = (float)BASE_ANGLE;
  }

  // Calcul d'un mouvement d'oscillation fluide (balayage doux de 0 à 30 degrés)
  float phase = (float)(now % 2000) / 2000.0f * 2.0f * M_PI;
  float sweepOffset = 15.0f + 15.0f * sin(phase); // Varie de 0° à 30°

  if (activeTest == 1) {
    targets[0] = (float)BASE_ANGLE + sweepOffset;
  } 
  else if (activeTest == 2) {
    targets[1] = (float)BASE_ANGLE + sweepOffset;
  } 
  else if (activeTest == 3) {
    targets[2] = (float)BASE_ANGLE + sweepOffset;
  } 
  else if (activeTest == 4) {
    targets[3] = (float)BASE_ANGLE + sweepOffset;
  } 
  else if (activeTest == 5) {
    // Séquence : chaque moteur est testé pendant 3 secondes
    int servoIndex = (now / 3000) % 4;
    targets[servoIndex] = (float)BASE_ANGLE + sweepOffset;
  }
}

void applyServoSmoothness(float targets[4]) {
  unsigned long now = millis();
  float dt = (now - lastServoUpdateTime) / 1000.0f;
  lastServoUpdateTime = now;

  if (dt <= 0.0f) return;
  if (dt > 0.1f) dt = 0.1f;

  float alpha = dt / (dt + SMOOTHING_TIME_CONSTANT);
  float maxAngleChange = SERVO_SPEED_LIMIT * dt;

  for (int i = 0; i < 4; i++) {
    // Lissage exponentiel passe-bas
    float smoothedTarget = currentAngles[i] + alpha * (targets[i] - currentAngles[i]);
    float angleDifference = smoothedTarget - currentAngles[i];

    // Limitation de la vitesse
    if (abs(angleDifference) > maxAngleChange) {
      if (angleDifference > 0) {
        currentAngles[i] += maxAngleChange;
      } else {
        currentAngles[i] -= maxAngleChange;
      }
    } else {
      currentAngles[i] = smoothedTarget;
    }

    // Calcul de l'angle physique final + Trim
    int physicalAngle = round(currentAngles[i]);
    int finalAngle = 0;

    if (i == 0) {
      finalAngle = physicalAngle + TRIM_YL;
      servoYL.write(constrain(finalAngle, 0, 180));
    } 
    else if (i == 1) {
      finalAngle = physicalAngle + TRIM_YR;
      servoYR.write(constrain(finalAngle, 0, 180));
    } 
    else if (i == 2) {
      finalAngle = physicalAngle + TRIM_RL;
      servoRL.write(constrain(finalAngle, 0, 180));
    } 
    else if (i == 3) {
      finalAngle = physicalAngle + TRIM_RR;
      servoRR.write(constrain(finalAngle, 0, 180));
    }
  }
}
