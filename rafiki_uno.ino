/**
 * @file rafiki_uno.ino
 * @brief Robot Humanoïde Bipède Interactif - Version Arduino Uno (Sans capteur de vide)
 * @author Antigravity (Principal Robotics Engineer)
 * 
 * Ce script implémente les mouvements cinématiques lissés pour Arduino Uno.
 * Le capteur de distance HC-SR04 et la sécurité anti-chute ont été retirés.
 * Contrôle par le Moniteur Série (115200 bauds).
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
// Angles de départ (calibrage) fixés à 0 degré
const int BASE_ANGLE = 0;
const int TRIM_YL = 0;
const int TRIM_YR = 0;
const int TRIM_RL = 0;
const int TRIM_RR = 0;

// Servomoteurs
Servo servoYL;
Servo servoYR;
Servo servoRL;
Servo servoRR;

// Limiteur de vitesse angulaire maximale (degrés par seconde)
// Évite les mouvements trop brusques et protège l'alimentation électrique
const float SERVO_SPEED_LIMIT = 120.0f; 

// Constante de temps pour le filtre de lissage exponentiel (en secondes)
// Plus la valeur est élevée (ex: 0.15f ou 0.2f), plus les mouvements seront fluides et doux
const float SMOOTHING_TIME_CONSTANT = 0.18f; 

// --- États du Robot ---
enum RobotState {
  STATE_STOP,
  STATE_WALK_FORWARD,
  STATE_DANCE_HAPPY
};

RobotState currentState = STATE_STOP;
unsigned long stateStartTime = 0;

// --- Structure pour les Paramètres Oscillatoires ---
struct MotionParams {
  unsigned int period;       // Période du cycle en millisecondes
  int amplitude[4];          // Amplitudes (YL, YR, RL, RR)
  int offset[4];             // Offsets de centrage (YL, YR, RL, RR)
  float phase_offset[4];     // Déphasages initiaux en radians
};

// Paramètres de marche (les mouvements oscillent autour de la position de départ BASE_ANGLE)
// Note: Les angles résultants négatifs seront bridés à 0 par la fonction constrain.
const MotionParams WALK_FORWARD_PARAMS = {
  1200,                            // Période (1200ms)
  {30, 30, 20, 20},                // Amplitudes
  {0, 0, 4, -4},                   // Offsets
  {0.0f, 0.0f, -M_PI/2.0f, -M_PI/2.0f} // Déphasages
};

// Paramètres de danse
const MotionParams DANCE_STEPS[] = {
  { 1000, {0, 0, 20, 20}, {0, 0, 10, -10}, {0.0f, 0.0f, 0.0f, 0.0f} },
  { 600, {20, 20, 0, 0}, {0, 0, 0, 0}, {-M_PI/2.0f, M_PI/2.0f, 0.0f, 0.0f} }
};
const int NUM_DANCE_STEPS = sizeof(DANCE_STEPS) / sizeof(MotionParams);
const unsigned long DANCE_STEP_DURATION = 3000; 

// --- Variables de Contrôle Temporel ---
unsigned long lastServoUpdateTime = 0;
float currentAngles[4] = {0.0f, 0.0f, 0.0f, 0.0f}; // Positions réelles des servos (démarrent à 0)

void setup() {
  Serial.begin(115200);
  Serial.println(F("=== KIN OPERE - ARDUINO UNO INITIALIZATION (NO SENSOR) ==="));

  // Attachement des Servos sur Arduino Uno
  servoYL.attach(PIN_YL);
  servoYR.attach(PIN_YR);
  servoRL.attach(PIN_RL);
  servoRR.attach(PIN_RR);

  // Initialisation à l'angle de départ 0° + trim
  servoYL.write(BASE_ANGLE + TRIM_YL);
  servoYR.write(BASE_ANGLE + TRIM_YR);
  servoRL.write(BASE_ANGLE + TRIM_RL);
  servoRR.write(BASE_ANGLE + TRIM_RR);

  for (int i = 0; i < 4; i++) {
    currentAngles[i] = (float)BASE_ANGLE;
  }

  lastServoUpdateTime = millis();
  stateStartTime = millis();

  Serial.println(F("Robot pret sur Arduino Uno. Envoyez l'une des commandes suivantes :"));
  Serial.println(F("  WALK_FORWARD -> Faire avancer le robot"));
  Serial.println(F("  DANCE_HAPPY  -> Faire danser le robot"));
  Serial.println(F("  STOP         -> Arreter les moteurs"));
}

void loop() {
  // 1. Lecture des commandes du moniteur série
  readSerialCommand();

  // 2. Calcul des angles cibles
  float targetAngles[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  calculateTargetAngles(targetAngles);

  // 3. Application du lissage et envoi aux servos
  applyServoSmoothness(targetAngles);
}

void readSerialCommand() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.length() > 0) {
      if (input == "WALK_FORWARD") {
        currentState = STATE_WALK_FORWARD;
        stateStartTime = millis();
        Serial.println(F("ACK: WALK_FORWARD"));
      } 
      else if (input == "DANCE_HAPPY") {
        currentState = STATE_DANCE_HAPPY;
        stateStartTime = millis();
        Serial.println(F("ACK: DANCE_HAPPY"));
      } 
      else if (input == "STOP") {
        currentState = STATE_STOP;
        stateStartTime = millis();
        Serial.println(F("ACK: STOP"));
      } 
      else {
        Serial.print(F("Commande inconnue : "));
        Serial.println(input);
      }
    }
  }
}

void calculateTargetAngles(float targets[4]) {
  unsigned long now = millis();
  unsigned long elapsed = now - stateStartTime;

  if (currentState == STATE_WALK_FORWARD) {
    float phase = (float)(elapsed % WALK_FORWARD_PARAMS.period) / WALK_FORWARD_PARAMS.period * 2.0f * M_PI;
    for (int i = 0; i < 4; i++) {
      targets[i] = (float)BASE_ANGLE + WALK_FORWARD_PARAMS.offset[i] + WALK_FORWARD_PARAMS.amplitude[i] * sin(phase + WALK_FORWARD_PARAMS.phase_offset[i]);
    }
  } 
  else if (currentState == STATE_DANCE_HAPPY) {
    int danceIndex = (elapsed / DANCE_STEP_DURATION) % NUM_DANCE_STEPS;
    unsigned long stepStartTime = stateStartTime + (elapsed / DANCE_STEP_DURATION) * DANCE_STEP_DURATION;
    unsigned long elapsedInStep = now - stepStartTime;

    const MotionParams& currentStep = DANCE_STEPS[danceIndex];
    float phase = (float)(elapsedInStep % currentStep.period) / currentStep.period * 2.0f * M_PI;

    for (int i = 0; i < 4; i++) {
      targets[i] = (float)BASE_ANGLE + currentStep.offset[i] + currentStep.amplitude[i] * sin(phase + currentStep.phase_offset[i]);
    }
  } 
  else { // STATE_STOP
    targets[0] = (float)BASE_ANGLE;
    targets[1] = (float)BASE_ANGLE;
    targets[2] = (float)BASE_ANGLE;
    targets[3] = (float)BASE_ANGLE;
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
    // 1. Lissage exponentiel (filtre passe-bas)
    float smoothedTarget = currentAngles[i] + alpha * (targets[i] - currentAngles[i]);
    float angleDifference = smoothedTarget - currentAngles[i];

    // 2. Limitation de la vitesse angulaire
    if (abs(angleDifference) > maxAngleChange) {
      if (angleDifference > 0) {
        currentAngles[i] += maxAngleChange;
      } else {
        currentAngles[i] -= maxAngleChange;
      }
    } else {
      currentAngles[i] = smoothedTarget;
    }

    // 3. Contrainte physique d'angle (0 à 180 degrés)
    int physicalAngle = round(currentAngles[i]);
    int constrainedAngle = constrain(physicalAngle, 0, 180);

    if (i == 0) {
      servoYL.write(constrainedAngle + TRIM_YL);
    } 
    else if (i == 1) {
      servoYR.write(constrainedAngle + TRIM_YR);
    } 
    else if (i == 2) {
      servoRL.write(constrainedAngle + TRIM_RL);
    } 
    else if (i == 3) {
      servoRR.write(constrainedAngle + TRIM_RR);
    }
  }
}
