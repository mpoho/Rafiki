# 🤖 Rafiki - Robot Compagnon Bipède Interactif

Ce dépôt contient le micrologiciel pour contrôler les mouvements, les danses et les séquences interactives du robot bipède Rafiki sur plateforme **Arduino Uno**.

---

## 🔌 Brochage Matériel (Arduino Uno)

| Broche | Composant | Rôle |
| :---: | :---: | :--- |
| **Pin 4** | Servo Jambe Gauche (YL) | Contrôle l'inclinaison avant/arrière de la jambe gauche |
| **Pin 5** | Servo Jambe Droite (YR) | Contrôle l'inclinaison avant/arrière de la jambe droite |
| **Pin 6** | Servo Pied Gauche (RL) | Contrôle l'inclinaison latérale du pied gauche (cheville) |
| **Pin 7** | Servo Pied Droite (RR) | Contrôle l'inclinaison latérale du pied droite (cheville) |
| **Pin 8** | Buzzer (Optionnel) | Émet des tonalités, bips et paroles robotiques |

---

## ⚙️ Configuration & Trims de Calibrage

Les servomoteurs sont configurés avec les paramètres d'alignement physique suivants (à ne pas modifier pour respecter l'assemblage actuel) :

```cpp
const int BASE_ANGLE = 0;   // Base de référence absolue à 0°
const int TRIM_YL = 80;    // Compensation matérielle de la jambe gauche
const int TRIM_YR = 0;     // Pas de trim requis
const int TRIM_RL = 0;     // Pas de trim requis
const int TRIM_RR = 0;     // Pas de trim requis
```

---

## 💬 Commandes Numériques du Moniteur Série (115200 Bauds)

Vous pouvez contrôler Rafiki en envoyant les codes numériques ou les mots-clés correspondants depuis la console série.

### 📋 Mouvements de Base
* **`0` / `STOP` :** Arrêt complet de tous les mouvements. Les servos retournent à leur position d'origine ($0^\circ$ + trim).
* **`1` / `AVANCER` :** Marche avant continue (lissée et corrigée).
* **`2` / `RECULER` :** Marche arrière continue.
* **`3` / `GAUCHE` :** Rotation sur place vers la gauche.
* **`4` / `DROITE` :** Rotation sur place vers la droite.
* **`22` / `PETIT_AVANT` :** Avance d'un seul pas puis s'arrête.
* **`23` / `PETIT_ARRIERE` :** Recule d'un seul pas puis s'arrête.

### 💃 Liste des Danses & Gestes
* **`20` / `DANSE` / `DANCE` (Salsa Polyrythmique) :** Hanchement chaloupé lent ($1200\text{ms}$) avec tapotement rapide des pieds ($600\text{ms}$).
* **`24` / `WINE` / `DANCE_RNB` (R&B Slow Wining) :** Rotation circulaire ultra-fluide et sensuelle du bassin ($2400\text{ms}$).
* **`8` / `MOONWALK` :** Glissement fluide vers l'arrière.
* **`7` / `SAUTER` :** Saut en phase avec balancement des deux chevilles.
* **`9` / `DANCE_SHAKE` :** Secousses latérales rapides des jambes.
* **`11` / `DANCE_TIPTOE` :** Balancement sur la pointe des pieds.
* **`12` / `DANCE_CRUSAITO` :** Pas croisés latéraux glissés.
* **`13` / `DANCE_JITTER` :** Tremblements rapides de stress/excitation.
* **`14` / `DANCE_FLAP` :** Battements verticaux alternés.
* **`5` / `BEND_LEFT` :** Inclinaison maintenue sur la gauche.
* **`6` / `BEND_RIGHT` :** Inclinaison maintenue sur la droite.

### 🗣️ Gestes & Voix
* **`18` / `HAUSSER` :** Le robot se penche vers l'arrière pour lever la tête.
* **`19` / `NON` :** Secoue la tête de gauche à droite (négation).
* **`21` / `PARLER` :** Mouvements de tête couplés à des sons du buzzer.
* **`15` / `REPOS` :** Mode "Respiration" (légères oscillations pour soulager la tension mécanique).

---

## 🛠️ Résolution du bug de la marche avant (`Commande 1`)

> [!NOTE]
> **Pourquoi le robot n'avançait pas initialement ?**
> Avec `BASE_ANGLE = 0` et des trims de pieds à `0` (`TRIM_RL = 0` et `TRIM_RR = 0`), toute oscillation négative demandée aux pieds (ex: $-20^\circ$) était immédiatement rabattue à $0^\circ$ par la butée logicielle `constrain(angle, 0, 180)`. Par conséquent, les pieds restaient immobiles pendant toute la moitié négative du signal, empêchant le robot de déplacer son centre de gravité pour avancer.
> 
> **Comment ce bug a été résolu ?**
> Nous avons appliqué un **biais positif** (offset) à l'oscillation des pieds et de la jambe droite pour relever la courbe. Au lieu d'osciller entre $-20^\circ$ et $+20^\circ$, la courbe oscille désormais de façon optimale entre $0^\circ$ et $40^\circ$. Les pieds exécutent désormais leur course complète sans jamais saturer, ce qui rétablit une marche avant parfaitement fonctionnelle.


