/**
 * @file rafiki_tft_test.ino
 * @brief Script d'expressions faciales de Haute Fidélité avec Animations Fluides pour Écran TFT 3.5"
 * Compatible Arduino Uno.
 * 
 * Version optimisée en taille de stockage (Flash < 32KB) pour Arduino Uno.
 */

#include <Adafruit_GFX.h>      // Bibliothèque graphique
#include <MCUFRIEND_kbv.h>     // Bibliothèque pour les shields TFT
#include <Fonts/FreeSans9pt7b.h> // Unique police vectorielle (gérée en tailles x1 et x2)

MCUFRIEND_kbv tft;

// Définition de couleurs (16-bit RGB565)
#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define YELLOW  0xFFE0
#define DARK_GRAY 0x18C3 // Sourcils (#18181b)

// Couleurs d'émotions
#define COLOR_NEUTRAL   0x0D7E // Bleu Cyan (#0ea5e9)
#define COLOR_HAPPY     0x260C // Vert (#22c55e)
#define COLOR_SAD       0x3C1F // Bleu (#3b82f6)
#define COLOR_ANGRY     0xEC88 // Rouge (#ef4444)
#define COLOR_SURPRISED 0xED61 // Jaune (#eab308)
#define COLOR_SLEEPY    0xA2B9 // Violet (#a855f7)
#define COLOR_CURIOUS   0x059D // Cyan Foncé (#06b6d4)
#define COLOR_LOVE      0xEC88 // Rose-Rouge (#ec4899)
#define COLOR_THINKING  0x15CA // Émeraude (#10b981)

// Modes d'affichage
enum ViewMode {
  VIEW_EYES,
  VIEW_TEXT
};
ViewMode currentView = VIEW_EYES;

// Expressions possibles
enum EyeExpression {
  EXPR_NEUTRAL,
  EXPR_HAPPY,
  EXPR_SAD,
  EXPR_ANGRY,
  EXPR_SURPRISED,
  EXPR_SLEEP,
  EXPR_CURIOUS,
  EXPR_LOVE,
  EXPR_THINKING,
  EXPR_BLINK
};

EyeExpression currentExpression = EXPR_NEUTRAL;
EyeExpression lastActiveExpression = EXPR_NEUTRAL;
String displayTextStr = "";
String displayTypeStr = ""; // "TEXT" ou "IMAGE"

// --- Variables d'animation fluide ---
float currentX = 0.0f;       // Décalage X actuel de la pupille
float targetX = 0.0f;        // Décalage X cible
float currentY = 0.0f;       // Décalage Y actuel de la pupille
float targetY = 0.0f;        // Décalage Y cible
float currentOpenness = 1.0f; // Ouverture actuelle des yeux (0.0 à 1.0)
float targetOpenness = 1.0f;  // Ouverture cible des yeux
float lerpFactor = 0.25f;     // Vitesse de transition (effet amorti)

// Variables pour le clignement d'yeux automatique
unsigned long lastBlinkTime = 0;
unsigned long nextBlinkInterval = 4000;
bool isBlinking = false;

// Variables pour le regard autonome
unsigned long lastLookTime = 0;
unsigned long nextLookInterval = 3000;

void setup() {
  Serial.begin(115200);
  delay(500);
  
  uint16_t identifier = tft.readID();
  if (identifier == 0xD3D3 || identifier == 0x0000) {
    identifier = 0x9486; // Forçage driver ILI9486 par défaut
  }
  tft.begin(identifier);
  tft.setRotation(1); // Mode paysage 480x320
  
  // Premier nettoyage complet de l'écran
  tft.fillScreen(WHITE);
  
  printMenu();
  updateDisplay();
  
  lastBlinkTime = millis();
  lastLookTime = millis();
}

void loop() {
  // 1. Lecture série
  readSerialCommand();

  // 2. Gestion du clignement
  manageAutoBlink();

  // 3. Regard autonome
  manageAutoLook();

  // 4. Calcul de l'animation en LERP (mouvements fluides)
  bool animActive = false;
  
  if (abs(targetX - currentX) > 0.1f) {
    currentX += lerpFactor * (targetX - currentX);
    animActive = true;
  } else {
    currentX = targetX;
  }
  
  if (abs(targetY - currentY) > 0.1f) {
    currentY += lerpFactor * (targetY - currentY);
    animActive = true;
  } else {
    currentY = targetY;
  }
  
  if (abs(targetOpenness - currentOpenness) > 0.02f) {
    currentOpenness += lerpFactor * (targetOpenness - currentOpenness);
    animActive = true;
  } else {
    currentOpenness = targetOpenness;
  }

  // 5. Redessine uniquement la zone des yeux si une animation est en cours
  if (animActive && currentView == VIEW_EYES) {
    updateDisplay();
  }
}

void printMenu() {
  Serial.println(F("\n=== RAFIKI TFT TEST ==="));
  Serial.println(F("Commandes: NEUTRAL (0), HAPPY (1), SAD (2), ANGRY (3), SURPRISED (4)"));
  Serial.println(F("SLEEP (5), CURIOUS (6), LOVE (7), THINKING (8)"));
  Serial.println(F("TEXT:<message>, IMAGE:<nom>, SHOW_EYES"));
}

void readSerialCommand() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    
    if (cmd.length() == 0) return;

    Serial.print(F("ACK: "));
    Serial.println(cmd);

    if (cmd.startsWith("TEXT:")) {
      currentView = VIEW_TEXT;
      displayTypeStr = "TEXT";
      displayTextStr = cmd.substring(5);
      tft.fillScreen(WHITE); // Effacement complet pour le texte
      updateDisplay();
      return;
    } 
    else if (cmd.startsWith("IMAGE:")) {
      currentView = VIEW_TEXT;
      displayTypeStr = "IMAGE";
      displayTextStr = cmd.substring(6);
      tft.fillScreen(WHITE);
      updateDisplay();
      return;
    }
    else if (cmd == "SHOW_EYES" || cmd == "EYES") {
      currentView = VIEW_EYES;
      tft.fillScreen(WHITE);
      currentExpression = lastActiveExpression;
      targetOpenness = 1.0f;
      updateDisplay();
      return;
    }

    cmd.toUpperCase();
    bool valid = true;
    EyeExpression newExpr = currentExpression;
    ViewMode newView = currentView;

    if (cmd == "0" || cmd == "NEUTRAL" || cmd == "NEUTRE" || cmd == "STOP") {
      newExpr = EXPR_NEUTRAL;
      newView = VIEW_EYES;
      targetOpenness = 1.0f;
    } else if (cmd == "1" || cmd == "HAPPY" || cmd == "JOYEUX" || cmd == "DANCE_HAPPY" || cmd == "WALK_FORWARD") {
      newExpr = EXPR_HAPPY;
      newView = VIEW_EYES;
      targetOpenness = 1.0f;
    } else if (cmd == "2" || cmd == "SAD" || cmd == "TRISTE") {
      newExpr = EXPR_SAD;
      newView = VIEW_EYES;
      targetOpenness = 1.0f;
    } else if (cmd == "3" || cmd == "ANGRY" || cmd == "FACHE" || cmd == "COLERE") {
      newExpr = EXPR_ANGRY;
      newView = VIEW_EYES;
      targetOpenness = 1.0f;
    } else if (cmd == "4" || cmd == "SURPRISED" || cmd == "SURPRIS") {
      newExpr = EXPR_SURPRISED;
      newView = VIEW_EYES;
      targetOpenness = 1.0f;
    } else if (cmd == "5" || cmd == "SLEEP" || cmd == "SOMMEIL") {
      newExpr = EXPR_SLEEP;
      newView = VIEW_EYES;
      targetOpenness = 0.15f; // Paupières semi-fermées pour dormir
    } else if (cmd == "6" || cmd == "CURIOUS" || cmd == "CURIEUX") {
      newExpr = EXPR_CURIOUS;
      newView = VIEW_EYES;
      targetOpenness = 1.0f;
    } else if (cmd == "7" || cmd == "LOVE" || cmd == "AMOUR") {
      newExpr = EXPR_LOVE;
      newView = VIEW_EYES;
      targetOpenness = 1.0f;
    } else if (cmd == "8" || cmd == "THINKING" || cmd == "PENSE") {
      newExpr = EXPR_THINKING;
      newView = VIEW_EYES;
      targetOpenness = 1.0f;
    } else {
      valid = false;
    }

    if (valid) {
      if (newView != currentView) {
        currentView = newView;
        tft.fillScreen(WHITE);
      }
      
      if (newExpr != currentExpression) {
        currentExpression = newExpr;
        if (newExpr != EXPR_BLINK) {
          lastActiveExpression = newExpr;
        }
        isBlinking = false;
        targetX = 0.0f;
        targetY = 0.0f;
        updateDisplay();
      }
    }
  }
}

void manageAutoBlink() {
  unsigned long now = millis();
  if (currentExpression == EXPR_SLEEP || currentView == VIEW_TEXT) return;

  if (!isBlinking) {
    if (now - lastBlinkTime > nextBlinkInterval) {
      isBlinking = true;
      lastBlinkTime = now;
      targetOpenness = 0.05f; // Ferme progressivement les yeux
    }
  } else {
    if (currentOpenness <= 0.1f && targetOpenness == 0.05f) {
      targetOpenness = (currentExpression == EXPR_SLEEP) ? 0.15f : 1.0f;
    }
    if (currentOpenness >= 0.9f && targetOpenness == 1.0f) {
      isBlinking = false;
      lastBlinkTime = now;
      nextBlinkInterval = random(3000, 7000);
    }
  }
}

void manageAutoLook() {
  unsigned long now = millis();
  if (currentExpression == EXPR_SLEEP || isBlinking || currentView == VIEW_TEXT) return;

  if (now - lastLookTime > nextLookInterval) {
    lastLookTime = now;
    nextLookInterval = random(2000, 6000);

    int r = random(3);
    if (r == 0) {
      targetX = random(-18, -6);
      targetY = random(-6, 6);
    } else if (r == 1) {
      targetX = random(6, 18);
      targetY = random(-6, 6);
    } else {
      targetX = 0.0f;
      targetY = 0.0f;
    }
  }
}

// Trace une ligne épaisse
void drawThickLine(int x0, int y0, int x1, int y1, int thickness, uint16_t color) {
  for (int i = 0; i < thickness; i++) {
    tft.drawLine(x0, y0 + i, x1, y1 + i, color);
  }
}

// Trace un cœur rouge
void drawHeart(int x, int y) {
  tft.fillCircle(x - 18, y - 15, 20, RED);
  tft.fillCircle(x + 18, y - 15, 20, RED);
  tft.fillTriangle(x - 38, y - 7, x + 38, y - 7, x, y + 35, RED);
}

void updateDisplay() {
  if (currentView == VIEW_TEXT) {
    drawTextView();
    return;
  }

  // OPTIMISATION : Efface uniquement la zone utile (dirty rectangle) pour éviter le scintillement
  tft.fillRect(50, 30, 380, 230, WHITE);

  // Coordonnées de base
  int leftX = 140;
  int rightX = 340;
  int Y = 160;

  // Positions animées
  int lX = leftX + (int)currentX;
  int rX = rightX + (int)currentX;

  // Clignement progressif
  int eyeHeight = 120;
  int currentH = (int)(eyeHeight * currentOpenness);
  if (currentH < 8) currentH = 8;

  switch (currentExpression) {
    case EXPR_NEUTRAL:
    case EXPR_BLINK: {
      uint16_t color = COLOR_NEUTRAL;
      
      tft.fillRoundRect(lX - 40, Y - currentH/2, 80, currentH, 25, color);
      if (currentOpenness > 0.3f) {
        tft.fillCircle(lX - 15, Y - 30 + (int)currentY, 12, WHITE);
        tft.fillCircle(lX - 25, Y - 12 + (int)currentY, 6, WHITE);
      }
      
      tft.fillRoundRect(rX - 40, Y - currentH/2, 80, currentH, 25, color);
      if (currentOpenness > 0.3f) {
        tft.fillCircle(rX - 15, Y - 30 + (int)currentY, 12, WHITE);
        tft.fillCircle(rX - 25, Y - 12 + (int)currentY, 6, WHITE);
      }
      
      if (currentExpression != EXPR_BLINK) {
        int browY = Y - 85 - (1 - currentOpenness) * 15;
        drawThickLine(leftX - 45, browY, leftX + 45, browY, 8, DARK_GRAY);
        drawThickLine(rightX - 45, browY, rightX + 45, browY, 8, DARK_GRAY);
      }
      break;
    }

    case EXPR_HAPPY: {
      uint16_t color = COLOR_HAPPY;
      tft.fillCircle(lX, Y - 10, 52, color);
      tft.fillCircle(lX, Y + 25 + (int)(52 * (1 - currentOpenness)), 52, WHITE);
      
      tft.fillCircle(rX, Y - 10, 52, color);
      tft.fillCircle(rX, Y + 25 + (int)(52 * (1 - currentOpenness)), 52, WHITE);
      
      drawThickLine(leftX - 45, Y - 80, leftX + 45, Y - 90, 8, DARK_GRAY);
      drawThickLine(rightX - 45, Y - 90, rightX + 45, Y - 80, 8, DARK_GRAY);
      break;
    }

    case EXPR_SAD: {
      uint16_t color = COLOR_SAD;
      tft.fillRoundRect(lX - 40, Y - currentH/2, 80, currentH, 25, color);
      tft.fillTriangle(lX - 40, Y - currentH/2, lX + 40, Y - currentH/2, lX - 40, Y - currentH/2 + 45, WHITE);
      if (currentOpenness > 0.3f) {
        tft.fillCircle(lX - 10, Y - 10 + (int)currentY, 12, WHITE);
      }
      
      tft.fillRoundRect(rX - 40, Y - currentH/2, 80, currentH, 25, color);
      tft.fillTriangle(rX - 40, Y - currentH/2, rX + 40, Y - currentH/2, rX + 40, Y - currentH/2 + 45, WHITE);
      if (currentOpenness > 0.3f) {
        tft.fillCircle(rX - 10, Y - 10 + (int)currentY, 12, WHITE);
      }
      
      drawThickLine(leftX - 45, Y - 75, leftX + 45, Y - 95, 8, DARK_GRAY);
      drawThickLine(rightX - 45, Y - 95, rightX + 45, Y - 75, 8, DARK_GRAY);
      break;
    }

    case EXPR_ANGRY: {
      uint16_t color = COLOR_ANGRY;
      tft.fillRoundRect(lX - 40, Y - currentH/2, 80, currentH, 25, color);
      tft.fillTriangle(lX - 40, Y - currentH/2, lX + 40, Y - currentH/2, lX + 40, Y - currentH/2 + 45, WHITE);
      if (currentOpenness > 0.3f) {
        tft.fillCircle(lX - 25, Y - 10 + (int)currentY, 12, WHITE);
      }
      
      tft.fillRoundRect(rX - 40, Y - currentH/2, 80, currentH, 25, color);
      tft.fillTriangle(rX - 40, Y - currentH/2, rX + 40, Y - currentH/2, rX - 40, Y - currentH/2 + 45, WHITE);
      if (currentOpenness > 0.3f) {
        tft.fillCircle(rX - 25, Y - 10 + (int)currentY, 12, WHITE);
      }
      
      drawThickLine(leftX - 45, Y - 95, leftX + 45, Y - 75, 8, DARK_GRAY);
      drawThickLine(rightX - 45, Y - 75, rightX + 45, Y - 95, 8, DARK_GRAY);
      break;
    }

    case EXPR_SURPRISED: {
      uint16_t color = COLOR_SURPRISED;
      int rad = (int)(55 * currentOpenness);
      int innerRad = (int)(35 * currentOpenness);
      int pupilRad = (int)(18 * currentOpenness);
      
      if (rad > 10) {
        tft.fillCircle(lX, Y, rad, color);
        tft.fillCircle(lX, Y, innerRad, WHITE);
        tft.fillCircle(lX, Y, pupilRad, color);
        
        tft.fillCircle(rX, Y, rad, color);
        tft.fillCircle(rX, Y, innerRad, WHITE);
        tft.fillCircle(rX, Y, pupilRad, color);
      }
      drawThickLine(leftX - 45, Y - 105, leftX + 45, Y - 105, 8, DARK_GRAY);
      drawThickLine(rightX - 45, Y - 105, rightX + 45, Y - 105, 8, DARK_GRAY);
      break;
    }

    case EXPR_SLEEP: {
      uint16_t color = COLOR_SLEEPY;
      tft.fillRoundRect(lX - 40, Y - currentH/2, 80, currentH, 10, color);
      tft.fillRoundRect(rX - 40, Y - currentH/2, 80, currentH, 10, color);
      drawThickLine(leftX - 45, Y - 40, leftX + 45, Y - 35, 6, DARK_GRAY);
      drawThickLine(rightX - 45, Y - 35, rightX + 45, Y - 40, 6, DARK_GRAY);
      break;
    }

    case EXPR_CURIOUS: {
      uint16_t color = COLOR_CURIOUS;
      tft.fillRoundRect(lX - 40, Y - currentH/2, 80, currentH, 25, color);
      if (currentOpenness > 0.3f) {
        tft.fillCircle(lX - 15, Y - 30 + (int)currentY, 12, WHITE);
        tft.fillCircle(lX - 25, Y - 12 + (int)currentY, 6, WHITE);
      }
      
      int curH = (int)(100 * currentOpenness);
      if (curH < 8) curH = 8;
      tft.fillRoundRect(rX - 40, Y - 10 - curH/2, 80, curH, 25, color);
      tft.fillRect(rX - 45, Y - 10 - curH/2, 90, 20, WHITE);
      if (currentOpenness > 0.3f) {
        tft.fillCircle(rX - 15, Y - 40 + (int)currentY, 12, WHITE);
      }
      
      drawThickLine(leftX - 45, Y - 85, leftX + 45, Y - 85, 8, DARK_GRAY);
      drawThickLine(rightX - 45, Y - 110, rightX + 45, Y - 100, 8, DARK_GRAY);
      break;
    }

    case EXPR_LOVE: {
      drawHeart(lX, Y);
      drawHeart(rX, Y);
      drawThickLine(leftX - 45, Y - 80, leftX + 45, Y - 90, 6, DARK_GRAY);
      drawThickLine(rightX - 45, Y - 90, rightX + 45, Y - 80, 6, DARK_GRAY);
      break;
    }

    case EXPR_THINKING: {
      uint16_t color = COLOR_THINKING;
      tft.fillRoundRect(lX - 40, Y - currentH/2, 80, currentH, 25, color);
      tft.fillRect(lX - 45, Y - currentH/2, 90, 35, WHITE);
      if (currentOpenness > 0.3f) {
        tft.fillCircle(lX + 15 + (int)currentX, Y - 10 + (int)currentY, 12, WHITE);
      }
      
      tft.fillRoundRect(rX - 40, Y - currentH/2, 80, currentH, 25, color);
      tft.fillRect(rX - 45, Y - currentH/2, 90, 35, WHITE);
      if (currentOpenness > 0.3f) {
        tft.fillCircle(rX + 15 + (int)currentX, Y - 10 + (int)currentY, 12, WHITE);
      }
      
      drawThickLine(leftX - 45, Y - 80, leftX + 45, Y - 85, 8, DARK_GRAY);
      drawThickLine(rightX - 45, Y - 85, rightX + 45, Y - 70, 8, DARK_GRAY);
      break;
    }
  }
}

// Affiche la vue texte/image (optimisée en taille avec mise à l'échelle d'une unique police)
void drawTextView() {
  tft.drawRoundRect(10, 10, 460, 300, 15, DARK_GRAY);
  tft.drawRoundRect(12, 12, 456, 296, 13, COLOR_NEUTRAL);
  
  tft.setFont(&FreeSans9pt7b);
  
  if (displayTypeStr == "IMAGE") {
    tft.setTextColor(COLOR_THINKING);
    tft.setTextSize(1); // Échelle x1 (9pt)
    tft.setCursor(40, 50);
    tft.println(F("[CHARGEMENT IMAGE]"));
    
    tft.setTextColor(DARK_GRAY);
    tft.setCursor(40, 110);
    tft.print(F("Fichier: "));
    tft.setTextSize(2); // Échelle x2 (18pt)
    tft.println(displayTextStr);
    
    tft.setFont(&FreeSans9pt7b);
    tft.setTextSize(1);
    tft.setCursor(40, 200);
    tft.setTextColor(COLOR_ANGRY);
    tft.println(F("(L'affichage direct d'image requiert"));
    tft.setCursor(40, 230);
    tft.println(F("une carte SD sur Arduino Uno)"));
  } else {
    tft.setTextColor(COLOR_NEUTRAL);
    tft.setTextSize(1); // Échelle x1 (9pt)
    tft.setCursor(40, 50);
    tft.println(F("[MESSAGE RAFIKI]"));
    
    tft.setTextColor(DARK_GRAY);
    tft.setTextSize(2); // Échelle x2 (18pt)
    tft.setCursor(40, 130);
    tft.println(displayTextStr);
  }
}
