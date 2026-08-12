# 🎯 Kin Opere — Intégration du Raspberry Pi Vision Service dans n8n

Ce document explique précisément comment connecter votre microservice **Raspberry Pi Vision** à la plateforme **Kin Opere** (Next.js + n8n + Agents IA Gemini).

---

## 🏗️ Flux d'Orchestration Visuelle (Kin Opere ➔ n8n ➔ Raspberry Pi)

```text
 ┌────────────────────────────────────────────────────────┐
 │            1. Frontend Next.js (Kin Opere)             │
 │  - L'utilisateur clique sur "Lancer l'Agent Vision"    │
 └──────────────────────────┬─────────────────────────────┘
                            │ POST Webhook (avec agentId)
                            ▼
 ┌────────────────────────────────────────────────────────┐
 │              2. Passerelle n8n (Router)                │
 │  - Workflows n8n (kin_opere_router.json)               │
 │  - Aiguillage vers l'Agent IA Gemini                   │
 └──────────────────────────┬─────────────────────────────┘
                            │ Appel Tool HTTP (GET /capture/json)
                            ▼
 ┌────────────────────────────────────────────────────────┐
 │        3. Raspberry Pi Vision Microservice             │
 │  - S'exécute sur le Raspberry Pi (port 8000)           │
 │  - Capture la photo via USB Webcam, Picamera2 ou Mock  │
 │  - Renvoie le JSON Base64 ({ "data_uri": "..." })      │
 └──────────────────────────┬─────────────────────────────┘
                            │
                            ▼
 ┌────────────────────────────────────────────────────────┐
 │            4. Analyse Gemini Vision (n8n)              │
 │  - Le modèle Gemini Pro/Flash analyse l'image Base64   │
 └──────────────────────────┬─────────────────────────────┘
                            │ Callback HTTPS (n8n-callback)
                            ▼
 ┌────────────────────────────────────────────────────────┐
 │         5. Restitution UI Next.js (Kin Opere)          │
 │  - Affiche l'analyse visuelle et le rapport final      │
 └────────────────────────────────────────────────────────┘
```

---

## 🛠️ Configuration Étape par Étape dans n8n

### Étape 1 : Ajouter le Nœud HTTP Request dans n8n

Dans le workflow de votre agent n8n (ex: `agent_comptable.json` ou un nouvel `agent_inspection_visuelle.json`) :

1. Ajoutez un nœud **HTTP Request**.
2. Réglages :
   - **Method** : `GET`
   - **URL** : `http://<IP_DU_RASPBERRY_PI>:8000/capture/json`
   - **Headers** (Optionnel si API Key activée) :
     - Name : `X-API-Key`
     - Value : `votre_cle_secrete_rpi`
3. Le nœud renvoie directement la structure JSON suivante :
   ```json
   {
     "mime_type": "image/jpeg",
     "data_uri": "data:image/jpeg;base64,/9j/4AAQSkZJRg...",
     "base64": "/9j/4AAQSkZJRg...",
     "metadata": {
       "timestamp": "2026-08-12T18:45:00Z",
       "width": 1280,
       "height": 720,
       "camera_type": "opencv_v4l2",
       "device_name": "RaspberryPi-Vision-01"
     }
   }
   ```

---

### Étape 2 : Connecter l'image au Modèle Gemini Vision dans n8n

1. Dans n8n, reliez la sortie du nœud **HTTP Request** au nœud **Google Gemini Chat Model** ou au nœud **AI Agent / Chain**.
2. Dans le prompt d'entrée de Gemini, passez la variable de l'image :
   - Champ Image / Binary / URL : `{{ $json.data_uri }}`
   - Instruction texte : `"Analyse l'image capturée par le Raspberry Pi ci-dessus et indique ce que tu observes."`

---

### Étape 3 : Renvoyer le résultat vers Next.js (Callback Kin Opere)

Comme configuré dans Kin Opere, le résultat de l'analyse visuelle est renvoyé à Next.js via le nœud HTTP Callback de n8n :
- **URL** : `{{ $json.callbackUrl }}` (ou `http://localhost:3000/api/webhook/n8n-callback`)
- **Headers** : `x-api-key: {{ $env.N8N_CALLBACK_SECRET }}`
- **Body JSON** :
  ```json
  {
    "action": "SAVE_MESSAGE",
    "conversationId": "...",
    "content": "📷 **Rapport d'Inspection Visuelle Raspberry Pi** :\n\nL'image montre..."
  }
  ```

---

## 🌐 Remarque Réseau (Si n8n est sur VPS Vercel/Cloud)

Si votre instance n8n ou Next.js est hébergée sur le Cloud (ex: VPS / Vercel) tandis que le Raspberry Pi est sur votre réseau local privé :
- **Option 1 (Recommandée - Sécurisée)** : Utilisez **Tailscale** ou **Cloudflare Tunnel** sur le Raspberry Pi pour lui donner une URL HTTPS privée accessible par n8n (`https://rpi-vision.votre-domaine.com`).
- **Option 2 (Tunnel temporaire)** : Lancez **ngrok** sur le Raspberry Pi :
  ```bash
  ngrok http 8000
  ```
  Utilisez l'URL ngrok générée dans n8n.
