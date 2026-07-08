/**
 * @file rafiki_uno.ino
 * @brief Robot Humanoïde Bipède Interactif - Version Arduino Uno (Mouvements Otto DIY & Calibrage 0°)
 * @author Antigravity (Principal Robotics Engineer)
 * 
 * Ce script implémente les mouvements cinématiques lissés pour Arduino Uno.
 * Contrôle par nombres via le Moniteur Série (115200 bauds).
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
const int PIN_BUZZER = 8; // Buzzer optionnel pour émettre des sons/parler

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
const float SERVO_SPEED_LIMIT = 120.0f; 

// Constante de temps pour le filtre de lissage exponentiel (en secondes)
const float SMOOTHING_TIME_CONSTANT = 0.18f; 

// --- États de Mouvement du Robot ---
enum RobotState {
  STATE_STOP = 0,            // Arrêt complet
  STATE_WALK_FORWARD = 1,    // Marche avant en ligne droite
  STATE_WALK_BACKWARD = 2,   // Marche arrière
  STATE_TURN_LEFT = 3,       // Tourner à gauche
  STATE_TURN_RIGHT = 4,      // Tourner à droite
  STATE_BEND_LEFT = 5,       // Se pencher à gauche
  STATE_BEND_RIGHT = 6,      // Se pencher à droite
  STATE_JUMP = 7,            // Sauter
  STATE_MOONWALK = 8,        // Marche lunaire (Moonwalk)
  STATE_DANCE_SHAKE = 9,     // Danse : Secousses latérales
  STATE_DANCE_SWING = 10,     // Danse : Balancement latéral (Swing)
  STATE_DANCE_TIPTOE = 11,    // Danse : Sur la pointe des pieds (Tiptoe swing)
  STATE_DANCE_CRUSAITO = 12,  // Danse : Pas croisés (Crusaito)
  STATE_DANCE_JITTER = 13,    // Danse : Tremblement rapide (Jitter)
  STATE_DANCE_FLAP = 14,      // Danse : Battements Up-Down (Flapping)
  STATE_IDLE_BREATH = 15,     // Mouvements au repos (respiration/micro-oscillations pour régulariser les jambes)
  STATE_SEQUENCE_1 = 16,      // Séquence longue (avance, tourne, recule, hausse la tête, négation, danse, parle)
  STATE_SEQUENCE_2 = 17,      // Séquence courte (avance un tout petit peu, recule, tourne)
  STATE_RAISE_HEAD = 18,      // Hausser la tête (inclinaison arrière lente)
  STATE_NEGATION = 19,        // Négation (secouer la tête de gauche à droite)
  STATE_TALK = 20,            // Parler en bougeant la tête
  STATE_TINY_WALK_FORWARD = 21, // Avancer un tout petit peu (1 pas)
  STATE_TINY_WALK_BACKWARD = 22 // Reculer un tout petit peu (1 pas)
};

// --- États des Séquences de Mouvement ---
enum Sequence1Step {
  SEQ1_WALK_FORWARD,
  SEQ1_TURN,
  SEQ1_WALK_BACKWARD,
  SEQ1_RAISE_HEAD,
  SEQ1_NEGATION,
  SEQ1_DANCE,
  SEQ1_TALK,
  SEQ1_FINISHED
};

enum Sequence2Step {
  SEQ2_TINY_FORWARD,
  SEQ2_TINY_BACKWARD,
  SEQ2_TINY_TURN,
  SEQ2_FINISHED
};

RobotState currentState = STATE_STOP;
unsigned long stateStartTime = 0;

Sequence1Step currentSeq1Step = SEQ1_WALK_FORWARD;
unsigned long seqStepStartTime = 0;

Sequence2Step currentSeq2Step = SEQ2_TINY_FORWARD;
unsigned long seq2StepStartTime = 0;

// --- Variables de Contrôle Temporel ---
unsigned long lastServoUpdateTime = 0;
float currentAngles[4] = {0.0f, 0.0f, 0.0f, 0.0f}; // Positions réelles des servos (démarrent à 0)

void setup() {
  Serial.begin(115200);
  Serial.println(F("=== KIN OPERE - ARDUINO UNO INITIALIZATION (ALL OTTO MOVES) ==="));

  // Configuration du Buzzer et de la graine aléatoire
  pinMode(PIN_BUZZER, OUTPUT);
  randomSeed(analogRead(0));

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
  stateStartTime = millis();

  // Menu d'aide interactif
  printMenu();
}

void loop() {
  // 1. Lecture rapide des commandes (Texte & Nombres)
  readSerialCommand();

  // 2. Calcul des angles cibles selon le mouvement actif
  float targetAngles[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  calculateTargetAngles(targetAngles);

  // 3. Application du lissage et envoi aux servos
  applyServoSmoothness(targetAngles);
}

void printMenu() {
  Serial.println(F("\n=== RAFIKI - MENU INTERACTIF DE COMMANDE ==="));
  Serial.println(F("Entrez un nombre ou un mot-cle dans le Moniteur Serie (115200 bauds)."));
  Serial.println(F("\n--- COMMANDES GENERALES ---"));
  Serial.println(F("  0  / STOP         : Arret complet des moteurs"));
  Serial.println(F("  15 / IDLE / REPOS  : Mouvement au repos (respiration pour aligner les jambes)"));
  Serial.println(F("\n--- MOUVEMENTS DE BASE ---"));
  Serial.println(F("  1  / AVANCER       : Marche avant normale"));
  Serial.println(F("  2  / RECULER       : Marche arriere normale"));
  Serial.println(F("  3  / GAUCHE        : Tourner a gauche"));
  Serial.println(F("  4  / DROITE        : Tourner a droite"));
  Serial.println(F("  22 / TINY_FORWARD  : Avancer d'un tout petit pas"));
  Serial.println(F("  23 / TINY_BACKWARD : Reculer d'un tout petit pas"));
  Serial.println(F("\n--- GESTES & DANSES ---"));
  Serial.println(F("  18 / HAUSSER       : Hausser la tete (inclinaison arriere)"));
  Serial.println(F("  19 / NON / NEGATION: Dire non (secouer la tete)"));
  Serial.println(F("  21 / PARLER / TALK : Parler en bougeant la tete (avec beeps)"));
  Serial.println(F("  20 / DANSE / DANCE : Danser (balancement lateral)"));
  Serial.println(F("\n--- SEQUENCES AUTOMATIQUES ---"));
  Serial.println(F("  16 / SEQ1          : Sequence longue (Avancer -> Tourner -> Petit recul ->"));
  Serial.println(F("                       Hausser la tete -> Negation -> Danser -> Parler)"));
  Serial.println(F("  17 / SEQ2          : Sequence courte (Petit pas avant -> Reculer -> Tourner)"));
  Serial.println(F("\n--- AUTRES DANSES DISPONIBLES ---"));
  Serial.println(F("  5 : Pencher a gauche | 6 : Pencher a droite | 7 : Sauter"));
  Serial.println(F("  8 : Moonwalk | 9 : Shake | 11 : Pointe des pieds | 12 : Crusaito"));
  Serial.println(F("  13 : Jitter (tremblement) | 14 : Flap (battement)"));
  Serial.println(F("============================================"));
}

void printSpeechPhrase() {
  int r = random(5);
  switch (r) {
    case 0: Serial.println(F("[Rafiki] Bonjour mon ami! Comment vas-tu?")); break;
    case 1: Serial.println(F("[Rafiki] Je suis Rafiki, le robot interactif!")); break;
    case 2: Serial.println(F("[Rafiki] Regarde comme je peux marcher et danser!")); break;
    case 3: Serial.println(F("[Rafiki] Tout va bien dans mes systemes. Bip boop!")); break;
    case 4: Serial.println(F("[Rafiki] La marche est maintenant bien regularisee.")); break;
  }
}

void readSerialCommand() {
  if (Serial.available() > 0) {
    delay(10); // Court delai pour s'assurer que tout le buffer est recu
    String cmd = Serial.readString();
    cmd.trim();
    String cmdUpper = cmd;
    cmdUpper.toUpperCase();

    // Verification si numerique
    bool isNumeric = true;
    if (cmd.length() == 0) isNumeric = false;
    for (unsigned int i = 0; i < cmd.length(); i++) {
      if (!isDigit(cmd.charAt(i)) && !(i == 0 && cmd.charAt(i) == '-')) {
        isNumeric = false;
        break;
      }
    }

    int stateNum = -1;
    if (isNumeric) {
      stateNum = cmd.toInt();
    }

    RobotState newState = currentState;
    bool valid = false;

    // Correspondance Texte et Numerique
    if (cmdUpper == "STOP" || cmdUpper == "OFF" || stateNum == 0) {
      newState = STATE_STOP;
      valid = true;
    } else if (cmdUpper == "WALK_FORWARD" || cmdUpper == "AVANCER" || stateNum == 1) {
      newState = STATE_WALK_FORWARD;
      valid = true;
    } else if (cmdUpper == "WALK_BACKWARD" || cmdUpper == "RECULER" || stateNum == 2) {
      newState = STATE_WALK_BACKWARD;
      valid = true;
    } else if (cmdUpper == "TURN_LEFT" || cmdUpper == "GAUCHE" || stateNum == 3) {
      newState = STATE_TURN_LEFT;
      valid = true;
    } else if (cmdUpper == "TURN_RIGHT" || cmdUpper == "DROITE" || stateNum == 4) {
      newState = STATE_TURN_RIGHT;
      valid = true;
    } else if (cmdUpper == "BEND_LEFT" || stateNum == 5) {
      newState = STATE_BEND_LEFT;
      valid = true;
    } else if (cmdUpper == "BEND_RIGHT" || stateNum == 6) {
      newState = STATE_BEND_RIGHT;
      valid = true;
    } else if (cmdUpper == "JUMP" || cmdUpper == "SAUTER" || stateNum == 7) {
      newState = STATE_JUMP;
      valid = true;
    } else if (cmdUpper == "MOONWALK" || stateNum == 8) {
      newState = STATE_MOONWALK;
      valid = true;
    } else if (cmdUpper == "DANCE_SHAKE" || stateNum == 9) {
      newState = STATE_DANCE_SHAKE;
      valid = true;
    } else if (cmdUpper == "DANCE_SWING" || stateNum == 10) {
      newState = STATE_DANCE_SWING;
      valid = true;
    } else if (cmdUpper == "DANCE_TIPTOE" || stateNum == 11) {
      newState = STATE_DANCE_TIPTOE;
      valid = true;
    } else if (cmdUpper == "DANCE_CRUSAITO" || stateNum == 12) {
      newState = STATE_DANCE_CRUSAITO;
      valid = true;
    } else if (cmdUpper == "DANCE_JITTER" || stateNum == 13) {
      newState = STATE_DANCE_JITTER;
      valid = true;
    } else if (cmdUpper == "DANCE_FLAP" || stateNum == 14) {
      newState = STATE_DANCE_FLAP;
      valid = true;
    } else if (cmdUpper == "IDLE" || cmdUpper == "BREATH" || cmdUpper == "REPOS" || stateNum == 15) {
      newState = STATE_IDLE_BREATH;
      valid = true;
    } else if (cmdUpper == "SEQ1" || cmdUpper == "SEQUENCE1" || stateNum == 16) {
      newState = STATE_SEQUENCE_1;
      valid = true;
    } else if (cmdUpper == "SEQ2" || cmdUpper == "SEQUENCE2" || stateNum == 17) {
      newState = STATE_SEQUENCE_2;
      valid = true;
    } else if (cmdUpper == "HEAD_UP" || cmdUpper == "HAUSSER" || stateNum == 18) {
      newState = STATE_RAISE_HEAD;
      valid = true;
    } else if (cmdUpper == "NEGATION" || cmdUpper == "NON" || stateNum == 19) {
      newState = STATE_NEGATION;
      valid = true;
    } else if (cmdUpper == "DANCE" || cmdUpper == "DANSE" || cmdUpper == "DANSER" || stateNum == 20) {
      newState = STATE_DANCE_SWING;
      valid = true;
    } else if (cmdUpper == "TALK" || cmdUpper == "PARLER" || stateNum == 21) {
      newState = STATE_TALK;
      valid = true;
    } else if (cmdUpper == "TINY_FORWARD" || cmdUpper == "PETIT_AVANT" || stateNum == 22) {
      newState = STATE_TINY_WALK_FORWARD;
      valid = true;
    } else if (cmdUpper == "TINY_BACKWARD" || cmdUpper == "PETIT_ARRIERE" || stateNum == 23) {
      newState = STATE_TINY_WALK_BACKWARD;
      valid = true;
    }

    if (valid) {
      currentState = newState;
      stateStartTime = millis();
      Serial.print(F("ACK: ETAT ACTIVE -> "));
      Serial.println(cmd);
      
      // Initialisation des sequences
      if (currentState == STATE_SEQUENCE_1) {
        currentSeq1Step = SEQ1_WALK_FORWARD;
        seqStepStartTime = stateStartTime;
        Serial.println(F("[SEQ1] Demarrage de la sequence longue. Etape 1: Marche avant"));
      } else if (currentState == STATE_SEQUENCE_2) {
        currentSeq2Step = SEQ2_TINY_FORWARD;
        seq2StepStartTime = stateStartTime;
        Serial.println(F("[SEQ2] Demarrage de la sequence courte. Etape 1: Petit pas avant"));
      }
    } else {
      Serial.print(F("Commande incorrecte ou inconnue : "));
      Serial.println(cmd);
      printMenu();
    }
  }
}

void calculateTargetAngles(float targets[4]) {
  unsigned long now = millis();
  unsigned long elapsed = now - stateStartTime;

  // Valeur par défaut de repos
  for (int i = 0; i < 4; i++) {
    targets[i] = (float)BASE_ANGLE;
  }

  switch (currentState) {
    case STATE_WALK_FORWARD: {
      // Marche Avant en ligne droite (déphasage de 90° entre jambes et pieds)
      float phase = (float)(elapsed % 1200) / 1200.0f * 2.0f * M_PI;
      targets[0] = (float)BASE_ANGLE + 28.0f * sin(phase); // Jambe Gauche
      targets[1] = (float)BASE_ANGLE + 28.0f * sin(phase); // Jambe Droite
      targets[2] = (float)BASE_ANGLE + 4.0f + 20.0f * sin(phase - M_PI/2.0f); // Pied Gauche
      targets[3] = (float)BASE_ANGLE - 4.0f + 20.0f * sin(phase - M_PI/2.0f); // Pied Droite
      break;
    }

    case STATE_WALK_BACKWARD: {
      // Marche Arrière (déphasage inverse)
      float phase = (float)(elapsed % 1200) / 1200.0f * 2.0f * M_PI;
      targets[0] = (float)BASE_ANGLE + 28.0f * sin(phase);
      targets[1] = (float)BASE_ANGLE + 28.0f * sin(phase);
      targets[2] = (float)BASE_ANGLE + 4.0f + 20.0f * sin(phase + M_PI/2.0f);
      targets[3] = (float)BASE_ANGLE - 4.0f + 20.0f * sin(phase + M_PI/2.0f);
      break;
    }

    case STATE_TURN_LEFT: {
      // Tourner à gauche
      float phase = (float)(elapsed % 1000) / 1000.0f * 2.0f * M_PI;
      targets[0] = (float)BASE_ANGLE + 10.0f * sin(phase);
      targets[1] = (float)BASE_ANGLE + 35.0f * sin(phase);
      targets[2] = (float)BASE_ANGLE + 20.0f * sin(phase - M_PI/2.0f);
      targets[3] = (float)BASE_ANGLE + 20.0f * sin(phase - M_PI/2.0f);
      break;
    }

    case STATE_TURN_RIGHT: {
      // Tourner à droite
      float phase = (float)(elapsed % 1000) / 1000.0f * 2.0f * M_PI;
      targets[0] = (float)BASE_ANGLE + 35.0f * sin(phase);
      targets[1] = (float)BASE_ANGLE + 10.0f * sin(phase);
      targets[2] = (float)BASE_ANGLE + 20.0f * sin(phase - M_PI/2.0f);
      targets[3] = (float)BASE_ANGLE + 20.0f * sin(phase - M_PI/2.0f);
      break;
    }

    case STATE_BEND_LEFT: {
      // Pencher à gauche (inclinaison lente)
      float phase = (float)(elapsed % 1000) / 1000.0f * 2.0f * M_PI;
      targets[0] = (float)BASE_ANGLE;
      targets[1] = (float)BASE_ANGLE;
      targets[2] = (float)BASE_ANGLE + 30.0f * sin(phase);
      targets[3] = (float)BASE_ANGLE;
      break;
    }

    case STATE_BEND_RIGHT: {
      // Pencher à droite (inclinaison lente)
      float phase = (float)(elapsed % 1000) / 1000.0f * 2.0f * M_PI;
      targets[0] = (float)BASE_ANGLE;
      targets[1] = (float)BASE_ANGLE;
      targets[2] = (float)BASE_ANGLE;
      targets[3] = (float)BASE_ANGLE + 30.0f * sin(phase);
      break;
    }

    case STATE_JUMP: {
      // Saut en phase (les deux pieds s'inclinent en même temps)
      float phase = (float)(elapsed % 800) / 800.0f * 2.0f * M_PI;
      targets[0] = (float)BASE_ANGLE;
      targets[1] = (float)BASE_ANGLE;
      targets[2] = (float)BASE_ANGLE + 25.0f * sin(phase);
      targets[3] = (float)BASE_ANGLE + 25.0f * sin(phase);
      break;
    }

    case STATE_MOONWALK: {
      // Moonwalk (glissement arrière)
      float phase = (float)(elapsed % 1400) / 1400.0f * 2.0f * M_PI;
      targets[0] = (float)BASE_ANGLE + 25.0f * sin(phase);
      targets[1] = (float)BASE_ANGLE + 25.0f * sin(phase + M_PI);
      targets[2] = (float)BASE_ANGLE + 20.0f * sin(phase - M_PI/2.0f);
      targets[3] = (float)BASE_ANGLE + 20.0f * sin(phase + M_PI/2.0f);
      break;
    }

    case STATE_DANCE_SHAKE: {
      // Shake (secousses latérales des jambes)
      float phase = (float)(elapsed % 500) / 500.0f * 2.0f * M_PI;
      targets[0] = (float)BASE_ANGLE + 25.0f * sin(phase);
      targets[1] = (float)BASE_ANGLE - 25.0f * sin(phase); // Opposition de phase
      targets[2] = (float)BASE_ANGLE;
      targets[3] = (float)BASE_ANGLE;
      break;
    }

    case STATE_DANCE_SWING: {
      // Swing (balancement de gauche à droite, jambes droites, pieds oscillant en phase)
      float phase = (float)(elapsed % 1000) / 1000.0f * 2.0f * M_PI;
      targets[0] = (float)BASE_ANGLE;
      targets[1] = (float)BASE_ANGLE;
      targets[2] = (float)BASE_ANGLE + 20.0f * sin(phase);
      targets[3] = (float)BASE_ANGLE + 20.0f * sin(phase);
      break;
    }

    case STATE_DANCE_TIPTOE: {
      // Tiptoe (sur la pointe des pieds)
      float phase = (float)(elapsed % 800) / 800.0f * 2.0f * M_PI;
      targets[0] = (float)BASE_ANGLE;
      targets[1] = (float)BASE_ANGLE;
      targets[2] = (float)BASE_ANGLE + 20.0f + 15.0f * sin(phase);
      targets[3] = (float)BASE_ANGLE - 20.0f + 15.0f * sin(phase);
      break;
    }

    case STATE_DANCE_CRUSAITO: {
      // Crusaito (pas croisés glissés)
      float phase = (float)(elapsed % 900) / 900.0f * 2.0f * M_PI;
      targets[0] = (float)BASE_ANGLE + 25.0f * sin(phase);
      targets[1] = (float)BASE_ANGLE + 25.0f * sin(phase);
      targets[2] = (float)BASE_ANGLE + 20.0f * sin(phase - M_PI/2.0f);
      targets[3] = (float)BASE_ANGLE + 20.0f * sin(phase + M_PI/2.0f);
      break;
    }

    case STATE_DANCE_JITTER: {
      // Jitter (tremblement rapide des jambes)
      float phase = (float)(elapsed % 400) / 400.0f * 2.0f * M_PI;
      targets[0] = (float)BASE_ANGLE + 15.0f * sin(phase);
      targets[1] = (float)BASE_ANGLE - 15.0f * sin(phase);
      targets[2] = (float)BASE_ANGLE;
      targets[3] = (float)BASE_ANGLE;
      break;
    }

    case STATE_DANCE_FLAP: {
      // Flapping (battement vertical des jambes)
      float phase = (float)(elapsed % 600) / 600.0f * 2.0f * M_PI;
      targets[0] = (float)BASE_ANGLE + 20.0f * sin(phase);
      targets[1] = (float)BASE_ANGLE - 20.0f * sin(phase);
      targets[2] = (float)BASE_ANGLE;
      targets[3] = (float)BASE_ANGLE;
      break;
    }

    case STATE_IDLE_BREATH: {
      // Respiration douce : micro-mouvements pour régulariser les positions et libérer la charge des servos
      float phase = (float)(elapsed % 3000) / 3000.0f * 2.0f * M_PI;
      targets[0] = 3.0f * sin(phase);  // Micro-rotation jambe gauche
      targets[1] = -3.0f * sin(phase); // Micro-rotation jambe droite
      targets[2] = 4.0f * cos(phase);  // Micro-balancement pied gauche
      targets[3] = 4.0f * cos(phase);  // Micro-balancement pied droite
      break;
    }

    case STATE_RAISE_HEAD: {
      // Hausser la tête (inclinaison arrière lente)
      float phase = (float)(elapsed % 2500) / 2500.0f * 2.0f * M_PI;
      targets[2] = 18.0f * sin(phase);
      targets[3] = -18.0f * sin(phase);
      break;
    }

    case STATE_NEGATION: {
      // Dire non (secouer le corps/tête de gauche à droite)
      float phase = (float)(elapsed % 500) / 500.0f * 2.0f * M_PI;
      targets[0] = 20.0f * sin(phase);
      targets[1] = 20.0f * sin(phase);
      break;
    }

    case STATE_TALK: {
      // Parler en bougeant la tête
      float phase = (float)(elapsed % 800) / 800.0f * 2.0f * M_PI;
      targets[2] = 8.0f * sin(phase);
      targets[3] = -8.0f * sin(phase);
      targets[0] = 5.0f * cos(phase);
      targets[1] = 5.0f * cos(phase);

      // Vocalisation bip boop non-bloquante
      if (elapsed % 300 < 60) {
        tone(PIN_BUZZER, 500 + (elapsed % 800), 40);
      }

      // Affichage de phrases
      static unsigned long lastPhraseTime = 0;
      if (elapsed < 50) {
        lastPhraseTime = 0;
      }
      if (elapsed - lastPhraseTime > 1500) {
        lastPhraseTime = elapsed;
        printSpeechPhrase();
      }
      break;
    }

    case STATE_TINY_WALK_FORWARD: {
      // Avancer d'un tout petit pas
      float phase = (float)(elapsed % 1200) / 1200.0f * 2.0f * M_PI;
      targets[0] = (float)BASE_ANGLE + 15.0f * sin(phase);
      targets[1] = (float)BASE_ANGLE + 15.0f * sin(phase);
      targets[2] = (float)BASE_ANGLE + 4.0f + 10.0f * sin(phase - M_PI/2.0f);
      targets[3] = (float)BASE_ANGLE - 4.0f + 10.0f * sin(phase - M_PI/2.0f);
      // Auto-stop après 1 cycle (1200 ms)
      if (elapsed >= 1200) {
        currentState = STATE_STOP;
        Serial.println(F("[Info] Petit pas avant termine."));
      }
      break;
    }

    case STATE_TINY_WALK_BACKWARD: {
      // Reculer d'un tout petit pas
      float phase = (float)(elapsed % 1200) / 1200.0f * 2.0f * M_PI;
      targets[0] = (float)BASE_ANGLE + 15.0f * sin(phase);
      targets[1] = (float)BASE_ANGLE + 15.0f * sin(phase);
      targets[2] = (float)BASE_ANGLE + 4.0f + 10.0f * sin(phase + M_PI/2.0f);
      targets[3] = (float)BASE_ANGLE - 4.0f + 10.0f * sin(phase + M_PI/2.0f);
      // Auto-stop après 1 cycle (1200 ms)
      if (elapsed >= 1200) {
        currentState = STATE_STOP;
        Serial.println(F("[Info] Petit pas arriere termine."));
      }
      break;
    }

    case STATE_SEQUENCE_1: {
      unsigned long seqElapsed = now - seqStepStartTime;

      // Gestion des transitions de la séquence longue
      if (currentSeq1Step == SEQ1_WALK_FORWARD && seqElapsed >= 4800) { // ~4 pas
        currentSeq1Step = SEQ1_TURN;
        seqStepStartTime = now;
        seqElapsed = 0;
        Serial.println(F("[SEQ1] Etape suivante: Tourner"));
      } else if (currentSeq1Step == SEQ1_TURN && seqElapsed >= 3000) { // ~3 rotations
        currentSeq1Step = SEQ1_WALK_BACKWARD;
        seqStepStartTime = now;
        seqElapsed = 0;
        Serial.println(F("[SEQ1] Etape suivante: Reculer (deux petits pas)"));
      } else if (currentSeq1Step == SEQ1_WALK_BACKWARD && seqElapsed >= 2400) { // ~2 pas
        currentSeq1Step = SEQ1_RAISE_HEAD;
        seqStepStartTime = now;
        seqElapsed = 0;
        Serial.println(F("[SEQ1] Etape suivante: Hausser la tete"));
      } else if (currentSeq1Step == SEQ1_RAISE_HEAD && seqElapsed >= 2500) {
        currentSeq1Step = SEQ1_NEGATION;
        seqStepStartTime = now;
        seqElapsed = 0;
        Serial.println(F("[SEQ1] Etape suivante: Negation"));
      } else if (currentSeq1Step == SEQ1_NEGATION && seqElapsed >= 2000) {
        currentSeq1Step = SEQ1_DANCE;
        seqStepStartTime = now;
        seqElapsed = 0;
        Serial.println(F("[SEQ1] Etape suivante: Danser"));
      } else if (currentSeq1Step == SEQ1_DANCE && seqElapsed >= 4000) {
        currentSeq1Step = SEQ1_TALK;
        seqStepStartTime = now;
        seqElapsed = 0;
        Serial.println(F("[SEQ1] Etape suivante: Parler"));
      } else if (currentSeq1Step == SEQ1_TALK && seqElapsed >= 4000) {
        currentSeq1Step = SEQ1_FINISHED;
        currentState = STATE_STOP;
        Serial.println(F("[SEQ1] Sequence 1 terminee. Retour au repos."));
      }

      // Application des cibles de l'étape active
      if (currentSeq1Step == SEQ1_WALK_FORWARD) {
        float phase = (float)(seqElapsed % 1200) / 1200.0f * 2.0f * M_PI;
        targets[0] = (float)BASE_ANGLE + 28.0f * sin(phase);
        targets[1] = (float)BASE_ANGLE + 28.0f * sin(phase);
        targets[2] = (float)BASE_ANGLE + 4.0f + 20.0f * sin(phase - M_PI/2.0f);
        targets[3] = (float)BASE_ANGLE - 4.0f + 20.0f * sin(phase - M_PI/2.0f);
      } 
      else if (currentSeq1Step == SEQ1_TURN) {
        float phase = (float)(seqElapsed % 1000) / 1000.0f * 2.0f * M_PI;
        targets[0] = (float)BASE_ANGLE + 10.0f * sin(phase);
        targets[1] = (float)BASE_ANGLE + 35.0f * sin(phase);
        targets[2] = (float)BASE_ANGLE + 20.0f * sin(phase - M_PI/2.0f);
        targets[3] = (float)BASE_ANGLE + 20.0f * sin(phase - M_PI/2.0f);
      } 
      else if (currentSeq1Step == SEQ1_WALK_BACKWARD) {
        float phase = (float)(seqElapsed % 1200) / 1200.0f * 2.0f * M_PI;
        // Petits pas arrière
        targets[0] = (float)BASE_ANGLE + 15.0f * sin(phase);
        targets[1] = (float)BASE_ANGLE + 15.0f * sin(phase);
        targets[2] = (float)BASE_ANGLE + 4.0f + 12.0f * sin(phase + M_PI/2.0f);
        targets[3] = (float)BASE_ANGLE - 4.0f + 12.0f * sin(phase + M_PI/2.0f);
      } 
      else if (currentSeq1Step == SEQ1_RAISE_HEAD) {
        float phase = (float)(seqElapsed % 2500) / 2500.0f * 2.0f * M_PI;
        targets[2] = 18.0f * sin(phase);
        targets[3] = -18.0f * sin(phase);
      } 
      else if (currentSeq1Step == SEQ1_NEGATION) {
        float phase = (float)(seqElapsed % 500) / 500.0f * 2.0f * M_PI;
        targets[0] = 20.0f * sin(phase);
        targets[1] = 20.0f * sin(phase);
      } 
      else if (currentSeq1Step == SEQ1_DANCE) {
        float phase = (float)(seqElapsed % 800) / 800.0f * 2.0f * M_PI;
        targets[0] = (float)BASE_ANGLE + 15.0f * sin(phase);
        targets[1] = (float)BASE_ANGLE - 15.0f * sin(phase);
        targets[2] = (float)BASE_ANGLE + 15.0f * cos(phase);
        targets[3] = (float)BASE_ANGLE + 15.0f * cos(phase);
      } 
      else if (currentSeq1Step == SEQ1_TALK) {
        float phase = (float)(seqElapsed % 800) / 800.0f * 2.0f * M_PI;
        targets[2] = 8.0f * sin(phase);
        targets[3] = -8.0f * sin(phase);
        targets[0] = 5.0f * cos(phase);
        targets[1] = 5.0f * cos(phase);

        if (seqElapsed % 300 < 60) {
          tone(PIN_BUZZER, 500 + (seqElapsed % 800), 40);
        }

        static unsigned long lastPhraseTime = 0;
        if (seqElapsed < 50) {
          lastPhraseTime = 0;
        }
        if (seqElapsed - lastPhraseTime > 1500) {
          lastPhraseTime = seqElapsed;
          printSpeechPhrase();
        }
      }
      break;
    }

    case STATE_SEQUENCE_2: {
      unsigned long seqElapsed = now - seq2StepStartTime;

      // Gestion des transitions de la séquence courte
      if (currentSeq2Step == SEQ2_TINY_FORWARD && seqElapsed >= 1200) { // 1 pas avant
        currentSeq2Step = SEQ2_TINY_BACKWARD;
        seq2StepStartTime = now;
        seqElapsed = 0;
        Serial.println(F("[SEQ2] Etape suivante: Reculer"));
      } else if (currentSeq2Step == SEQ2_TINY_BACKWARD && seqElapsed >= 1200) { // 1 pas arriere
        currentSeq2Step = SEQ2_TINY_TURN;
        seq2StepStartTime = now;
        seqElapsed = 0;
        Serial.println(F("[SEQ2] Etape suivante: Tourner"));
      } else if (currentSeq2Step == SEQ2_TINY_TURN && seqElapsed >= 1500) { // petite rotation
        currentSeq2Step = SEQ2_FINISHED;
        currentState = STATE_STOP;
        Serial.println(F("[SEQ2] Sequence 2 terminee. Retour au repos."));
      }

      // Application des cibles de la séquence courte
      if (currentSeq2Step == SEQ2_TINY_FORWARD) {
        float phase = (float)(seqElapsed % 1200) / 1200.0f * 2.0f * M_PI;
        targets[0] = (float)BASE_ANGLE + 15.0f * sin(phase);
        targets[1] = (float)BASE_ANGLE + 15.0f * sin(phase);
        targets[2] = (float)BASE_ANGLE + 4.0f + 10.0f * sin(phase - M_PI/2.0f);
        targets[3] = (float)BASE_ANGLE - 4.0f + 10.0f * sin(phase - M_PI/2.0f);
      } 
      else if (currentSeq2Step == SEQ2_TINY_BACKWARD) {
        float phase = (float)(seqElapsed % 1200) / 1200.0f * 2.0f * M_PI;
        targets[0] = (float)BASE_ANGLE + 15.0f * sin(phase);
        targets[1] = (float)BASE_ANGLE + 15.0f * sin(phase);
        targets[2] = (float)BASE_ANGLE + 4.0f + 10.0f * sin(phase + M_PI/2.0f);
        targets[3] = (float)BASE_ANGLE - 4.0f + 10.0f * sin(phase + M_PI/2.0f);
      } 
      else if (currentSeq2Step == SEQ2_TINY_TURN) {
        float phase = (float)(seqElapsed % 1000) / 1000.0f * 2.0f * M_PI;
        targets[0] = (float)BASE_ANGLE + 8.0f * sin(phase);
        targets[1] = (float)BASE_ANGLE + 20.0f * sin(phase);
        targets[2] = (float)BASE_ANGLE + 12.0f * sin(phase - M_PI/2.0f);
        targets[3] = (float)BASE_ANGLE + 12.0f * sin(phase - M_PI/2.0f);
      }
      break;
    }
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

    // 3. Calcul de l'angle physique avec application des Trims individuels
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
