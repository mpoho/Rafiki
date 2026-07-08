# Rafiki - Robot Compagnon Interactif

Rafiki (anciennement Arthy) est un robot compagnon bipède interactif conçu pour les enfants (Projet Makers 2026). Ce dépôt contient le micrologiciel pour le contrôle de ses mouvements, de ses danses, de ses séquences interactives et de sa calibration sur plateforme Arduino Uno.

---

## 📂 Structure du Projet

* **[`rafiki_uno/rafiki_uno.ino`](file:///C:/Users/Salem/Documents/projet/rafiki/rafiki_uno/rafiki_uno.ino)** : Code principal pour l'Arduino Uno gérant les mouvements cinématiques lissés, la respiration, les danses et l'écoute interactive des commandes série (numériques et textuelles).
* **[`rafiki_servo_test/rafiki_servo_test.ino`](file:///C:/Users/Salem/Documents/projet/rafiki/rafiki_servo_test/rafiki_servo_test.ino)** : Script de test unitaire des servomoteurs pour valider le câblage, le sens de rotation et l'alignement mécanique.
* **[`calibrage.ino`](file:///C:/Users/Salem/Documents/projet/rafiki/calibrage.ino)** : Script utilitaire pour forcer l'alignement mécanique de tous les servomoteurs à 0° pour le calibrage physique initial.
* **`OttoDIYLib/`** : Bibliothèque Arduino pour Otto DIY, contenant les fonctions de base.

---

## 🔌 Brochage matériel (Arduino Uno)

| Broche Arduino | Composant | Rôle |
| :---: | :---: | :---: |
| **Pin 4** | Servo Jambe Gauche (YL) | Contrôle l'inclinaison avant/arrière de la jambe gauche |
| **Pin 5** | Servo Jambe Droite (YR) | Contrôle l'inclinaison avant/arrière de la jambe droite |
| **Pin 6** | Servo Pied Gauche (RL) | Contrôle l'inclinaison latérale du pied gauche (cheville) |
| **Pin 7** | Servo Pied Droite (RR) | Contrôle l'inclinaison latérale du pied droite (cheville) |
| **Pin 8** | Buzzer (Optionnel) | Émet des tonalités interactives robotiques (bips/parole) |

---

## 🛠️ Calibration Initiale (0°)

Avant de faire marcher Rafiki, assurez-vous d'avoir correctement aligné ses jambes et ses pieds.
1. Téléversez le fichier [`calibrage.ino`](file:///C:/Users/Salem/Documents/projet/rafiki/calibrage.ino) sur l'Arduino Uno.
2. Fixez les palonniers de servomoteurs de sorte que le robot se tienne droit (jambes verticales et pieds bien à plat).
3. Si un décalage subsiste, ajustez-le en modifiant les variables `TRIM_YL`, `TRIM_YR`, `TRIM_RL`, `TRIM_RR` au début du fichier [`rafiki_uno.ino`](file:///C:/Users/Salem/Documents/projet/rafiki/rafiki_uno/rafiki_uno.ino).

---

## 💬 Moniteur Série & Commandes (115200 Bauds)

Toutes les commandes peuvent être envoyées dans le **Moniteur Série** de l'IDE Arduino.
Vous pouvez saisir **soit le numéro** de la commande, **soit son nom/mot-clé en français ou anglais** (insensible à la casse).

### 📋 Liste complète des commandes de mouvement

| Numéro | Mot-clé Principal | Mots-clés alternatifs | Action / Effet en Français |
| :---: | :--- | :--- | :--- |
| **`0`** | `STOP` | `OFF` | **Arrêt complet** immédiat de tous les mouvements. Les servos retournent à 0° + trim. |
| **`15`** | `REPOS` | `IDLE` / `BREATH` | **Mouvement au repos (Respiration)** : Micro-oscillations lentes des jambes et des chevilles pour soulager la tension mécanique des servos et les réaligner avant la marche. |
| **`16`** | `SEQ1` | `SEQUENCE1` | **Séquence longue automatique** : Avancer de quelques pas ➔ Tourner ➔ Reculer de deux petits pas ➔ Hausser la tête ➔ Faire de la négation ➔ Danser ➔ Parler en bougeant la tête (bips et phrases). *S'arrête automatiquement à la fin.* |
| **`17`** | `SEQ2` | `SEQUENCE2` | **Séquence courte automatique** : Avancer d'un tout petit pas ➔ Reculer d'un tout petit pas ➔ Tourner doucement. *Idéal pour les zones étroites.* |
| **`1`** | `AVANCER` | `WALK_FORWARD` | Marche avant continue en ligne droite. |
| **`2`** | `RECULER` | `WALK_BACKWARD` | Marche arrière continue. |
| **`3`** | `GAUCHE` | `TURN_LEFT` | Tourner en continu vers la gauche. |
| **`4`** | `DROITE` | `TURN_RIGHT` | Tourner en continu vers la droite. |
| **`18`** | `HAUSSER` | `HEAD_UP` | Hausser la tête (inclinaison lente du corps vers l'arrière pour regarder vers le haut). |
| **`19`** | `NON` | `NEGATION` | Dire "Non" (secouer la tête et le corps de gauche à droite). |
| **`20`** | `DANSE` | `DANCE` / `DANSER` | Danse standard (balancement latéral rythmé sur les chevilles). |
| **`21`** | `PARLER` | `TALK` | Parler en bougeant la tête (mouvements légers de pitch/yaw + bips sonores sur le buzzer + affichage de phrases aléatoires sur la console). |
| **`22`** | `TINY_FORWARD` | `PETIT_AVANT` | Avancer d'un tout petit pas (1 pas complet) puis s'arrêter. |
| **`23`** | `TINY_BACKWARD`| `PETIT_ARRIERE`| Reculer d'un tout petit pas (1 pas complet) puis s'arrêter. |
| **`5`** | `BEND_LEFT` | - | Se pencher doucement vers la gauche. |
| **`6`** | `BEND_RIGHT` | - | Se pencher doucement vers la droite. |
| **`7`** | `JUMP` | `SAUTER` | Sauter sur place (les deux chevilles oscillent en phase). |
| **`8`** | `MOONWALK` | - | Faire le pas de danse "Moonwalk" (glissement arrière fluide). |
| **`9`** | `DANCE_SHAKE` | - | Danse : Secousses latérales rapides des jambes. |
| **`10`** | `DANCE_SWING` | - | Danse : Balancement latéral d'amplitude moyenne. |
| **`11`** | `DANCE_TIPTOE`| - | Danse : Balancement sur la pointe des pieds. |
| **`12`** | `DANCE_CRUSAITO`| - | Danse : Pas croisés glissés (Crusaito). |
| **`13`** | `DANCE_JITTER` | - | Danse : Tremblement rapide des jambes. |
| **`14`** | `DANCE_FLAP` | - | Danse : Battements Up-Down des jambes. |

---

## 🧠 Séquences interactives non-bloquantes

Les mouvements et les transitions de séquences sont codés sans utiliser de fonctions bloquantes comme `delay()`. Ils utilisent l'horloge interne de l'Arduino (`millis()`).
Cela garantit :
* Une **fluidité optimale** de l'interpolation grâce au filtre de lissage exponentiel (les transitions de mouvements ne provoquent pas d'à-coups).
* Une **réactivité totale** : vous pouvez interrompre une séquence automatique en cours de route simplement en envoyant une autre commande par le port série (ex: `0` pour forcer l'arrêt immédiat).

---

## 🧪 Tests Unitaires des Servos

Si vous rencontrez des difficultés à faire bouger Rafiki :
1. Téléversez le script [`rafiki_servo_test.ino`](file:///C:/Users/Salem/Documents/projet/rafiki/rafiki_servo_test/rafiki_servo_test.ino) sur votre carte.
2. Ouvrez le moniteur série à 115200 bauds.
3. Utilisez les commandes suivantes pour tester chaque moteur individuellement :
   * **`0`** : Retour en position neutre (Repos).
   * **`1`** : Teste la Jambe Gauche (YL - Pin 4).
   * **`2`** : Teste la Jambe Droite (YR - Pin 5).
   * **`3`** : Teste le Pied Gauche (RL - Pin 6).
   * **`4`** : Teste le Pied Droite (RR - Pin 7).
   * **`5`** : Test séquentiel automatique de tous les servos (chacun bouge 3 secondes à tour de rôle).
