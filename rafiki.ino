/**
 * @file rafiki.ino
 * @brief Robot Humanoïde Bipède Interactif - Script de Mouvement Continu & Sécurité
 * @author Antigravity (Principal Robotics Engineer)
 * 
 * Ce script implémente les mouvements cinématiques du robot de manière 100% non-bloquante.
 * La logique cinématique est adaptée directement des formules d'oscillation du dépôt officiel
 * OttoDIYLib (https://github.com/OttoDIY/OttoDIYLib).
 * 
 * BROCHAGE MATÉRIEL (ESP32-S3 DevKitC-1) :
 *  - GPIO 4  -> Servo Jambe Gauche (YL)
 *  - GPIO 5  -> Servo Jambe Droite (YR)
 *  - GPIO 6  -> Servo Pied Gauche (RL)
 *  - GPIO 7  -> Servo Pied Droite (RR)
 *  - GPIO 15 -> TRIG Capteur HC-SR04
 *  - GPIO 16 -> ECHO Capteur HC-SR04
 * 
 * FONCTIONNALITÉS CLÉS :
 *  1. Architecture non-bloquante utilisant millis() et des calculs d'angles continus.
 *  2. Trajectoires sinusoïdales fluides avec limitation de vitesse (rate limiter) pour
 *     éviter les surintensités électriques et les mouvements saccadés.
 *  3. Sécurité anti-chute (cliff detection) prioritaire en tâche de fond : arrêt d'urgence 
 *     instantané et émission d'une alerte si la distance mesurée dépasse 30 cm.
 *  4. Contrôle interactif par liaison Série (console de simulation).
 */

#include <Arduino.h>
#include <ESP32Servo.h>
#include <math.h>

// --- Configuration des Broches ---
const int PIN_YL = 4;   // Servo Jambe Gauche (Yellow Left)
const int PIN_YR = 5;   // Servo Jambe Droite (Yellow Right)
const int PIN_RL = 6;   // Servo Pied Gauche (Red Left)
const int PIN_RR = 7;   // Servo Pied Droite (Red Right)

const int PIN_TRIG = 15; // HC-SR04 Trigger
const int PIN_ECHO = 16; // HC-SR04 Echo

// --- Calibrage Matériel (Trims) ---
// Ajustez ces valeurs pour aligner parfaitement les servos à 90°
const int TRIM_YL = 0;
const int TRIM_YR = 0;
const int TRIM_RL = 0;
const int TRIM_RR = 0;

// --- Paramètres des Servomoteurs ---
Servo servoYL;
Servo servoYR;
Servo servoRL;
Servo servoRR;

// Limiteur de vitesse angulaire maximale (en degrés par seconde)
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
  unsigned int period;       // Période du cycle en millisecondes (T)
  int amplitude[4];          // Amplitudes (YL, YR, RL, RR)
  int offset[4];             // Offsets de centrage (YL, YR, RL, RR)
  float phase_offset[4];     // Déphasages initiaux en radians (Phase0)
};

// --- Définition des Mouvements (Extraits et adaptés de Otto.cpp) ---

// Marche Avant : Jambes en phase, pieds en phase déphasés de -90° par rapport aux jambes
const MotionParams WALK_FORWARD_PARAMS = {
  1000,                            // Période (T = 1000ms)
  {30, 30, 20, 20},                // A[4] = {YL, YR, RL, RR}
  {0, 0, 4, -4},                   // O[4] = {YL, YR, RL, RR} (léger offset des pieds)
  {0.0f, 0.0f, -M_PI/2.0f, -M_PI/2.0f} // phase_diff en rad (pieds à -90°)
};

// Pas de Danse (Enchaînement cyclique sans delay)
const MotionParams DANCE_STEPS[] = {
  // Étape 1 : Swing (Balancement latéral - pieds en phase, jambes fixes)
  {
    1000,
    {0, 0, 20, 20},
    {0, 0, 10, -10},
    {0.0f, 0.0f, 0.0f, 0.0f}
  },
  // Étape 2 : Jitter (Tremblement des hanches déphasé, pieds fixes)
  {
    600,
    {20, 20, 0, 0},
    {0, 0, 0, 0},
    {-M_PI/2.0f, M_PI/2.0f, 0.0f, 0.0f}
  },
  // Étape 3 : Crusaito (Mélange de moonwalk et marche - jambes en phase, pieds déphasés)
  {
    900,
    {25, 25, 20, 20},
    {0, 0, 14, -14},
    {M_PI/2.0f, M_PI/2.0f, 0.0f, -M_PI/3.0f} // Pied droit déphasé de -60°
  },
  // Étape 4 : Tiptoe Swing (Balancement sur la pointe des pieds)
  {
    800,
    {0, 0, 20, 20},
    {0, 0, 20, -20},
    {0.0f, 0.0f, 0.0f, 0.0f}
  }
};
const int NUM_DANCE_STEPS = sizeof(DANCE_STEPS) / sizeof(MotionParams);
const unsigned long DANCE_STEP_DURATION = 3000; // Durée de chaque style de danse (3s)

// --- Variables de Contrôle Temporel ---
unsigned long lastServoUpdateTime = 0;
float currentAngles[4] = {90.0f, 90.0f, 90.0f, 90.0f}; // Positions réelles des servos

unsigned long lastUltrasonicTime = 0;
const unsigned long ULTRASONIC_INTERVAL = 80; // Fréquence de mesure (toutes les 80ms)
float currentDistance = 0.0f;
bool cliffDetected = false;

// --- Prototypes des fonctions ---
void readSerialCommand();
void checkCliffDistance();
void calculateTargetAngles(float targets[4]);
void applyServoSmoothness(float targets[4]);

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000); // Attente de la console (simulation)
  Serial.println("=== KIN OPERE - ESP32-S3 INITIALIZATION ===");

  // Configuration des E/S du capteur ultrasons
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  // Allocation explicite des Timers PWM de l'ESP32-S3 (Requis par ESP32Servo.h)
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  // Configuration des Servos (Hertz standard de 50Hz pour analogique)
  servoYL.setPeriodHertz(50);
  servoYR.setPeriodHertz(50);
  servoRL.setPeriodHertz(50);
  servoRR.setPeriodHertz(50);

  // Attachement physique des pins servos avec plage d'impulsion large
  servoYL.attach(PIN_YL, 500, 2400);
  servoYR.attach(PIN_YR, 500, 2400);
  servoRL.attach(PIN_RL, 500, 2400);
  servoRR.attach(PIN_RR, 500, 2400);

  // Envoi à la position neutre de départ
  servoYL.write(90 + TRIM_YL);
  servoYR.write(90 + TRIM_YR);
  servoRL.write(90 + TRIM_RL);
  servoRR.write(90 + TRIM_RR);

  lastServoUpdateTime = millis();
  stateStartTime = millis();

  Serial.println("Robot Pret. Envoyez: 'WALK_FORWARD', 'DANCE_HAPPY' ou 'STOP'");
}

void loop() {
  // 1. Surveillance constante de la sécurité anti-chute (Cliff Detection)
  checkCliffDistance();

  // 2. Lecture non-bloquante des ordres de la console de simulation
  readSerialCommand();

  // 3. Calcul des consignes angulaires théoriques (Trigonométrie Sinusoïdale)
  float targetAngles[4] = {90.0f, 90.0f, 90.0f, 90.0f};
  calculateTargetAngles(targetAngles);

  // 4. Asservissement en vitesse fluide (anti-saccade) vers les positions cibles
  applyServoSmoothness(targetAngles);
}

/**
 * @brief Lit et traite les commandes Série en provenance de la console de simulation.
 */
void readSerialCommand() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.length() > 0) {
      if (input == "WALK_FORWARD") {
        if (cliffDetected) {
          // Sécurité prioritaire : commande rejetée
          Serial.println("CLIFF_DETECTED");
        } else {
          currentState = STATE_WALK_FORWARD;
          stateStartTime = millis();
          Serial.println("ACK: WALK_FORWARD");
        }
      } 
      else if (input == "DANCE_HAPPY") {
        if (cliffDetected) {
          // Sécurité prioritaire : commande rejetée
          Serial.println("CLIFF_DETECTED");
        } else {
          currentState = STATE_DANCE_HAPPY;
          stateStartTime = millis();
          Serial.println("ACK: DANCE_HAPPY");
        }
      } 
      else if (input == "STOP") {
        currentState = STATE_STOP;
        stateStartTime = millis();
        Serial.println("ACK: STOP");
      } 
      else {
        Serial.print("Commande inconnue: ");
        Serial.println(input);
      }
    }
  }
}

/**
 * @brief Mesure la distance au sol de façon non-bloquante. 
 * Déclenche l'arrêt d'urgence instantané si le sol s'éloigne (vide > 30cm).
 */
void checkCliffDistance() {
  unsigned long now = millis();
  if (now - lastUltrasonicTime >= ULTRASONIC_INTERVAL) {
    lastUltrasonicTime = now;

    // Envoi d'une impulsion trigger de 10 microsecondes
    digitalWrite(PIN_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_TRIG, LOW);

    // Lecture de l'écho de manière sécurisée (timeout de 5000µs = ~85cm max pour éviter le blocage)
    long duration = pulseIn(PIN_ECHO, HIGH, 5000);

    if (duration == 0) {
      // Pas de retour d'écho dans les temps : vide potentiel ou capteur déconnecté
      currentDistance = 999.0f;
    } else {
      // Conversion en centimètres
      currentDistance = (duration * 0.0343f) / 2.0f;
    }

    // Analyse du seuil de vide (cliff limit > 30 cm)
    if (currentDistance > 30.0f) {
      if (!cliffDetected) {
        cliffDetected = true;
        currentState = STATE_STOP; // Écrase immédiatement le mouvement en cours
        Serial.println("CLIFF_DETECTED"); // Alerte critique sur console / MQTT
      }
    } else {
      cliffDetected = false;
    }
  }
}

/**
 * @brief Calcule les angles cibles théoriques en fonction du temps et du mouvement.
 */
void calculateTargetAngles(float targets[4]) {
  unsigned long now = millis();
  unsigned long elapsed = now - stateStartTime;

  if (currentState == STATE_WALK_FORWARD) {
    // Calcul de la phase courante pour le cycle de marche
    float phase = (float)(elapsed % WALK_FORWARD_PARAMS.period) / WALK_FORWARD_PARAMS.period * 2.0f * M_PI;
    
    for (int i = 0; i < 4; i++) {
      targets[i] = 90.0f + WALK_FORWARD_PARAMS.offset[i] + WALK_FORWARD_PARAMS.amplitude[i] * sin(phase + WALK_FORWARD_PARAMS.phase_offset[i]);
    }
  } 
  else if (currentState == STATE_DANCE_HAPPY) {
    // Enchaînement automatique des différentes danses toutes les 3 secondes
    int danceIndex = (elapsed / DANCE_STEP_DURATION) % NUM_DANCE_STEPS;
    unsigned long stepStartTime = stateStartTime + (elapsed / DANCE_STEP_DURATION) * DANCE_STEP_DURATION;
    unsigned long elapsedInStep = now - stepStartTime;

    const MotionParams& currentStep = DANCE_STEPS[danceIndex];
    float phase = (float)(elapsedInStep % currentStep.period) / currentStep.period * 2.0f * M_PI;

    for (int i = 0; i < 4; i++) {
      targets[i] = 90.0f + currentStep.offset[i] + currentStep.amplitude[i] * sin(phase + currentStep.phase_offset[i]);
    }
  } 
  else { // STATE_STOP
    // Retour à la position neutre (home)
    targets[0] = 90.0f;
    targets[1] = 90.0f;
    targets[2] = 90.0f;
    targets[3] = 90.0f;
  }
}

/**
 * @brief Filtre les sauts brusques d'angles et applique la consigne de vitesse maximale.
 */
void applyServoSmoothness(float targets[4]) {
  unsigned long now = millis();
  float dt = (now - lastServoUpdateTime) / 1000.0f; // Temps écoulé en secondes
  lastServoUpdateTime = now;

  if (dt <= 0.0f) return;
  if (dt > 0.1f) dt = 0.1f; // Limite le pas de temps maximum pour éviter les emballements après une pause

  // Calcul du coefficient de lissage exponentiel dépendant de dt
  float alpha = dt / (dt + SMOOTHING_TIME_CONSTANT);
  float maxAngleChange = SERVO_SPEED_LIMIT * dt; // Angle max franchissable sur cette frame

  for (int i = 0; i < 4; i++) {
    // 1. Lissage exponentiel (filtre passe-bas) pour adoucir le début et la fin des mouvements
    float smoothedTarget = currentAngles[i] + alpha * (targets[i] - currentAngles[i]);

    // 2. Limitation de la vitesse maximale (rate limiter)
    float angleDifference = smoothedTarget - currentAngles[i];

    if (abs(angleDifference) > maxAngleChange) {
      if (angleDifference > 0) {
        currentAngles[i] += maxAngleChange;
      } else {
        currentAngles[i] -= maxAngleChange;
      }
    } else {
      currentAngles[i] = smoothedTarget;
    }

    // Ajout des trims de calibrage et envoi de la consigne physique
    int physicalAngle = 90;
    if (i == 0) {
      physicalAngle = round(currentAngles[0]) + TRIM_YL;
      servoYL.write(constrain(physicalAngle, 0, 180));
    } 
    else if (i == 1) {
      physicalAngle = round(currentAngles[1]) + TRIM_YR;
      servoYR.write(constrain(physicalAngle, 0, 180));
    } 
    else if (i == 2) {
      physicalAngle = round(currentAngles[2]) + TRIM_RL;
      servoRL.write(constrain(physicalAngle, 0, 180));
    } 
    else if (i == 3) {
      physicalAngle = round(currentAngles[3]) + TRIM_RR;
      servoRR.write(constrain(physicalAngle, 0, 180));
    }
  }
}
