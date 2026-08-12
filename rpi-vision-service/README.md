# 📷 Raspberry Pi Vision Service (Module Capture & Stream pour LLM Vision)

Ce module est un microservice autonome et performant développé en Python (FastAPI + OpenCV / Picamera2) destiné à s'exécuter directement sur votre **Raspberry Pi**.

Il permet de transformer n'importe quel Raspberry Pi (avec caméra USB, caméra officielle CSI, ou flux RTSP) en un **service de capture visuelle à la demande**, parfaitement structuré pour être consommé par les outils (Tools/Function Calling) d'un **Agent Orchestrator LLM** (ex: OpenAI GPT-4o Vision, Claude 3.5 Sonnet, Ollama, Gemini, etc.).

---

## 🌟 Fonctionnalités

- **Détection Automatique du Matériel (Auto-Detection)** :
  - **Picamera2** : Caméras officielles Raspberry Pi CSI (`libcamera`).
  - **OpenCV / V4L2** : Webcams USB (`/dev/video0`) et caméras IP (`rtsp://...`).
  - **Mock Simulator** : Générateur de mires dynamiques avec horodatage pour tester le service sur votre PC avant transfert sur le Pi.
- **Endpoints Optimisés pour les LLM** :
  - `GET /capture/json` : Capture l'image et la renvoie directement formatée en **Base64** (`Data URI`), prête à être injectée dans les requêtes de vision LLM.
  - `GET /capture` : Renvoie l'image brute au format binaire (JPEG ou PNG).
  - `GET /stream` : Flux vidéo **MJPEG** en direct pour visualiser la caméra dans un navigateur web.
  - `POST /config` : Ajustement dynamique de la résolution, qualité JPEG, rotation (0°, 90°, 180°, 270°) et inversion miroir (H/V).
  - `GET /health` : État de santé du matériel et configuration active.
- **Client Python Prêt à l'Emploi (`RpiVisionClient`)** :
  - Un SDK Python minimaliste utilisable en 2 lignes par l'équipe qui développe l'agent orchestrateur.

---

## 📁 Structure du Projet

```text
rpi-vision-service/
├── rpi_vision/
│   ├── config.py              # Paramètres de configuration (Pydantic / Env)
│   ├── camera/
│   │   ├── base.py            # Interface abstraite & Modèle de données FrameData
│   │   ├── opencv_cam.py      # Support Webcams USB & flux RTSP
│   │   ├── picam2_cam.py      # Support Caméras officielles Raspberry Pi (Picamera2)
│   │   ├── mock_cam.py        # Simulateur de caméra pour dev/test
│   │   └── factory.py         # Factory de détection auto du matériel
│   ├── api/
│   │   ├── app.py             # Application FastAPI & gestion de l'état
│   │   └── routes.py          # Endpoints HTTP (Capture JSON/Binaire, Stream, Health)
│   └── client/
│       ├── __init__.py
│       └── tool_wrapper.py    # Client Python & Schéma Tool pour l'Orchestrateur
├── systemd/
│   └── rpi-vision.service    # Fichier de service Daemon pour Raspberry Pi OS
├── run_server.py              # Script principal de lancement du serveur
├── example_orchestrator_usage.py # Exemple d'appel côté Orchestrateur
├── Dockerfile                 # Conteneurisation Docker
├── docker-compose.yml
├── requirements.txt
└── pyproject.toml
```

---

## 🚀 Démarrage Rapide

### 1. Installation des dépendances

Sur votre Raspberry Pi (ou votre machine de dev) :

```bash
git clone <votre-repo>
cd rpi-vision-service
pip install -r requirements.txt
```

> **Remarque (Raspberry Pi OS) :** Si vous utilisez le module caméra officiel du Pi, assurez-vous que `picamera2` est installé :
> `sudo apt update && sudo apt install -y python3-picamera2`

---

### 2. Lancement du Service

#### Mode Automatique (Recommandé sur le Raspberry Pi)
```bash
python run_server.py --host 0.0.0.0 --port 8000
```

#### Mode Simulation / Mock (Pour tester sans caméra physique)
```bash
python run_server.py --camera-type mock --port 8000
```

#### Options de Ligne de Commande :
- `--camera-type` : `auto`, `opencv`, `picamera2`, ou `mock` (défaut: `auto`)
- `--camera-index` : Index USB (`0`, `1`) ou URL RTSP
- `--width` & `--height` : Résolution par défaut (ex: `1280` `720`)
- `--api-key` : (Optionnel) Clé secrète d'API (requiert le header `X-API-Key`)

---

## 🔌 Intégration pour l'Agent Orchestrateur

La personne/équipe qui gère l'Agent Orchestrateur peut importer le client `RpiVisionClient` inclus dans le module :

```python
from rpi_vision.client.tool_wrapper import RpiVisionClient, get_vision_tool_definition

# 1. Connexion au service Raspberry Pi (remplacer par l'IP du Pi)
client = RpiVisionClient(base_url="http://192.168.1.50:8000")

# 2. Capture de la photo au format JSON Base64 pour LLM Vision
payload = client.capture_image_json(quality=85, width=1280, height=720)

# 3. Payload prêt à être envoyé à OpenAI / Claude / Ollama
message_llm = {
    "role": "user",
    "content": [
        {"type": "text", "text": "Décris ce que tu vois sur l'image capturée par le Raspberry Pi."},
        {"type": "image_url", "image_url": {"url": payload["data_uri"]}}
    ]
}
```

---

## 🛠️ Déploiement en arrière-plan sur Raspberry Pi OS (Systemd)

Pour que le service se lance automatiquement au démarrage du Raspberry Pi :

1. Copier le fichier service :
   ```bash
   sudo cp systemd/rpi-vision.service /etc/systemd/system/
   ```
2. Activer et démarrer le service :
   ```bash
   sudo systemctl daemon-reload
   sudo systemctl enable rpi-vision.service
   sudo systemctl start rpi-vision.service
   ```
3. Vérifier le statut :
   ```bash
   sudo systemctl status rpi-vision.service
   ```

---

## 🌐 Documentation Swagger / OpenAPI
Une fois le service démarré, vous pouvez tester tous les endpoints interactivement sur :  
👉 **`http://<IP_DU_RASPBERRY_PI>:8000/docs`**
