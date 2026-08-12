# 📘 Architecture & Fonctionnement : Raspberry Pi Vision Service x Orchestrateur Kin Opere

Ce document explique le fonctionnement complet du module **Raspberry Pi Vision Service**, son rôle exact dans l'écosystème de la plateforme **Kin Opere**, et comment les captures visuelles du Raspberry Pi sont transmises à l'**Agent Orchestrateur (n8n + Gemini)**.

---

## 🎯 1. Contexte & Découpage des Rôles

Le système se compose de deux parties distinctes :

1. **Le Raspberry Pi (Microservice de Capture Visuelle) :**
   - Il n'exécute **aucun LLM localement** (pour préserver sa mémoire et son processeur).
   - Il héberge un serveur **FastAPI** léger et réactif.
   - Il gère la caméra (Webcam USB, Caméra officielle CSI `Picamera2`, flux IP RTSP, ou Simulateur Mock).
   - Il expose des endpoints REST permettant de prendre des photos à la demande et de les renvoyer en format **Base64 JSON** ou **Binaire**.

2. **L'Agent Orchestrateur (Kin Opere : Next.js + n8n + Gemini) :**
   - **Frontend & Backend Next.js** (sur Vercel ou Local) : Reçoit la demande de l'utilisateur, vérifie et décrémente les crédits en base de données PostgreSQL (Prisma), puis déclenche l'exécution sur n8n.
   - **Moteur d'Orchestration n8n** : Contient les workflows d'agents IA. Lorsqu'un agent a besoin d'une vue du monde réel, il effectue un appel HTTP vers le Raspberry Pi, récupère la photo et la passe au modèle **Google Gemini Vision**.
   - **Callback Webhook** : n8n renvoie l'analyse finale de Gemini à Next.js qui l'affiche à l'utilisateur dans l'interface web.

---

## 🏗️ 2. Diagramme d'Architecture & Flux de Données

```text
 ┌────────────────────────────────────────────────────────┐
 │           1. CLIENT / INTERFACE NEXT.JS                │
 │  - L'utilisateur sélectionne un Agent Vision/Inspection│
 │  - Next.js valide les crédits (Prisma ORM)             │
 └──────────────────────────┬─────────────────────────────┘
                            │ Appel Webhook (POST JSON)
                            ▼
 ┌────────────────────────────────────────────────────────┐
 │             2. PASSERELLE DE ROUTAGE n8n               │
 │  - Workflow `kin_opere_router.json`                    │
 │  - Aiguillage vers l'Agent IA Spécialisé               │
 └──────────────────────────┬─────────────────────────────┘
                            │ Appel Tool HTTP (GET /capture/json)
                            ▼
 ┌────────────────────────────────────────────────────────┐
 │       3. RASPBERRY PI VISION CAPTURE SERVICE           │
 │  - Tourne sur le Pi (FastAPI - Port 8000)              │
 │  - Détecte et s'interface avec la Caméra (V4L2/Picam2) │
 │  - Encode le frame en Base64 Data URI                   │
 └──────────────────────────┬─────────────────────────────┘
                            │ Réponse JSON ({ "data_uri": "..." })
                            ▼
 ┌────────────────────────────────────────────────────────┐
 │          4. ANALYSE IA (GEMINI PRO/FLASH VISION)       │
 │  - Nœud Gemini dans n8n consomme l'image Base64        │
 │  - Génération du rapport d'inspection / réponse        │
 └──────────────────────────┬─────────────────────────────┘
                            │ POST Webhook Callback (SAVE_MESSAGE)
                            ▼
 ┌────────────────────────────────────────────────────────┐
 │           5. RESTITUTION DANS NEXT.JS (A2UI)           │
 │  - Affichage du résultat dans l'UI du client           │
 └────────────────────────────────────────────────────────┘
```

---

## 📷 3. Détails Techniques du Microservice Raspberry Pi

### A. Détection & Support Matériel (Abstraction Caméra)
Le module utilise le pattern **Factory** (`rpi_vision/camera/factory.py`) pour s'adapter automatiquement au matériel disponible :
- **Picamera2 (`picam2_cam.py`)** : Pour le module caméra officiel Raspberry Pi CSI via `libcamera`.
- **OpenCV (`opencv_cam.py`)** : Pour les webcams USB courantes (`/dev/video0`) et caméras IP (`rtsp://...`).
- **Mock Simulator (`mock_cam.py`)** : Pour développer et tester le code sur votre PC portable sans caméra physique branchée.

### B. Endpoints de l'API REST
- `GET /capture/json` : **(Préféré pour les LLM)** Capture la photo et retourne un payload JSON prêt pour les API Vision :
  ```json
  {
    "mime_type": "image/jpeg",
    "data_uri": "data:image/jpeg;base64,/9j/4AAQSkZJRg...",
    "base64": "/9j/4AAQSkZJRg...",
    "metadata": {
      "timestamp": "2026-08-12T18:59:00Z",
      "width": 1280,
      "height": 720,
      "camera_type": "opencv_v4l2",
      "device_name": "RaspberryPi-Vision-01"
    }
  }
  ```
- `GET /capture` : Renvoie l'image brute binaire (`image/jpeg` ou `image/png`).
- `GET /stream` : Flux vidéo MJPEG en direct pour prévisualisation dans un navigateur.
- `POST /config` : Ajustement dynamique de la résolution, qualité JPEG, rotation (0°, 90°, 180°, 270°) et miroir horizontal/vertical.
- `GET /health` : Rapport de santé du matériel.

---

## 🔌 4. Intégration dans n8n (Kin Opere)

### Nœud HTTP Request dans n8n
Dans les workflows n8n de Kin Opere, la capture s'effectue via un nœud **HTTP Request** :
- **Method** : `GET`
- **URL** : `http://<IP_DU_RASPBERRY_PI>:8000/capture/json`

### Connexion au Nœud Gemini Vision dans n8n
Le champ `data_uri` retourné par le Raspberry Pi est directement passé en entrée du nœud **Google Gemini Chat Model** dans n8n :
- **Image Input Expression** : `{{ $json.data_uri }}`
- **Prompt Exemple** : `"Voici la photo prise par la caméra du Raspberry Pi. Identifie les objets présents et vérifie si le poste de travail est en ordre."`

### Envoi de la réponse à Next.js
n8n envoie ensuite le résultat à l'endpoint Callback de Next.js :
- **URL Callback** : `APP_URL/api/webhook/n8n-callback`
- **Body** :
  ```json
  {
    "action": "SAVE_MESSAGE",
    "conversationId": "...",
    "content": "📷 **Résultat de l'analyse visuelle :**\n\n- Objet A détecté\n- Anomalie B identifiée..."
  }
  ```

---

## 🌐 5. Communication Réseau (Si n8n est hébergé sur VPS / Cloud)

Si n8n est hébergé sur un serveur distant (VPS / Vercel Cloud) et que le Raspberry Pi est situé sur un réseau local privé :

1. **Option Tailscale / VPN (Recommandé) :** Installez Tailscale sur le Pi et le VPS pour créer un réseau privé virtuel sécurisé.
2. **Option Tunnel HTTPS (Ngrok / Cloudflare) :** Lancez un tunnel sur le Pi :
   ```bash
   ngrok http 8000
   ```
   Renseignez l'URL HTTPS générée par ngrok dans le nœud HTTP Request de n8n.

---

## 🚀 6. Guide de Démarrage Rapide

### Sur le Raspberry Pi :
```bash
# 1. Cloner/Naviguer dans le dossier du projet
cd C:\Users\Salem\Documents\projet\rpi-vision-service

# 2. Installer les dépendances
pip install -r requirements.txt

# 3. Lancer le serveur (auto-détection de la caméra)
python run_server.py --host 0.0.0.0 --port 8000
```

### Installation comme Service Système (Automatique au boot) :
```bash
sudo cp systemd/rpi-vision.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now rpi-vision.service
```
