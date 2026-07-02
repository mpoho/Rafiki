# Module voix local Rafiki

Rafiki est un robot compagnon intelligent pour enfant. Ce module lui donne quatre outils simples pour l'agent principal du robot :

- `listen(language="fr")` : écouter l'enfant et retourner du texte.
- `speak(text, language="fr")` : faire parler Rafiki.
- `chat(text, language="fr")` : envoyer un texte au modèle local via LM Studio par défaut.
- `voice_chat(language="fr")` : écouter, interroger le modèle local, puis faire parler Rafiki.

Le périmètre est limité à la voix français/anglais et à la connexion local-first avec un LLM local. Il ne contient pas la mémoire complète, la vision, les routines ou le hardware du robot.

## Technologies

- Python 3.10+
- FastAPI + Uvicorn pour l'API locale
- Vosk pour la reconnaissance vocale hors-ligne
- Piper TTS pour la synthèse vocale locale
- pyttsx3 comme fallback temporaire si Piper ou une voix Piper manque
- LM Studio par défaut pour appeler un modèle local via API OpenAI-compatible
- Ollama reste disponible en option avec `LLM_PROVIDER=ollama`
- Variables d'environnement via `python-dotenv`

## Installation locale

Depuis le dossier qui contient `voice/`, `scripts/` et `requirements.txt` :

```powershell
cd <dossier_du_module_python>
```

Créer et activer un environnement Python :

```powershell
py -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install --upgrade pip
pip install -r requirements.txt
```

Créer un fichier `.env` à partir de `.env.example` si tu veux modifier les chemins :

```powershell
Copy-Item .env.example .env
```

Le module fonctionne avec les valeurs par défaut même si `.env` n'existe pas.

## Configuration

Variables disponibles :

```env
VOICE_DEFAULT_LANGUAGE=fr
VOSK_MODEL_FR_PATH=models/vosk/vosk-model-small-fr-0.22
VOSK_MODEL_EN_PATH=models/vosk/vosk-model-small-en-us-0.15
VOSK_SAMPLE_RATE=16000
LISTEN_TIMEOUT_SECONDS=7
PIPER_VOICE_FR_PATH=models/piper/fr/fr_FR-upmc-medium.onnx
PIPER_VOICE_EN_PATH=models/piper/en/en_US-lessac-medium.onnx
LLM_PROVIDER=lmstudio
LLM_TIMEOUT_SECONDS=60
LLM_MAX_TOKENS=120
LM_STUDIO_BASE_URL=http://localhost:1234/v1
LM_STUDIO_MODEL=
OLLAMA_BASE_URL=http://localhost:11434
OLLAMA_MODEL=gemma4
AUDIO_OUTPUT_DIR=outputs/audio
```

Si `LM_STUDIO_MODEL` est vide, le module demande à LM Studio la liste des modèles chargés via `/v1/models` et utilise le premier modèle disponible. `LLM_TIMEOUT_SECONDS` controle le temps maximum d attente du modele, et `LLM_MAX_TOKENS` limite la longueur des reponses.

## LM Studio

1. Ouvrir LM Studio.
2. Télécharger ou sélectionner un modèle local.
3. Charger le modèle en mémoire.
4. Aller dans l'onglet Developer.
5. Démarrer le serveur local.
6. Vérifier que l'API répond sur :

```text
http://localhost:1234/v1
```

Le module utilise l'endpoint OpenAI-compatible :

```text
POST /v1/chat/completions
GET /v1/models
```

Test rapide :

```powershell
python scripts/test_chat.py
```

## Option Ollama

LM Studio est le provider par défaut. Pour repasser sur Ollama, mettre dans `.env` :

```env
LLM_PROVIDER=ollama
OLLAMA_BASE_URL=http://localhost:11434
OLLAMA_MODEL=gemma4
```

Puis lancer :

```powershell
ollama serve
ollama pull gemma4
```

## Modèles Vosk

Vosk est nécessaire pour `/listen` et `/voice-chat`.

Créer le dossier :

```powershell
mkdir models\vosk
```

Télécharger puis décompresser les modèles :

- Français : https://alphacephei.com/vosk/models/vosk-model-small-fr-0.22.zip
- Anglais : https://alphacephei.com/vosk/models/vosk-model-small-en-us-0.15.zip

Structure attendue :

```text
models/
  vosk/
    vosk-model-small-fr-0.22/
    vosk-model-small-en-us-0.15/
```

## Voix Piper

Piper est utilisé si le binaire `piper` est disponible dans le `PATH` et si le modèle vocal demandé existe.

Structure attendue :

```text
models/
  piper/
    fr/
      fr_FR-upmc-medium.onnx
      fr_FR-upmc-medium.onnx.json
    en/
      en_US-lessac-medium.onnx
      en_US-lessac-medium.onnx.json
```

Si Piper n'est pas disponible ou si le modèle manque, `speak()` utilise automatiquement `pyttsx3`.

## Lancer le serveur FastAPI

Depuis le dossier qui contient `voice/` :

```powershell
uvicorn voice.voice_server:app --reload
```

Ouvrir ensuite :

```text
http://127.0.0.1:8000/docs
```

## API

### GET /health

Retourne l'état du module voix local, le provider LLM et le modèle configuré.

### POST /speak

```json
{
  "text": "Bonjour, je suis Rafiki.",
  "language": "fr"
}
```

### POST /listen

```json
{
  "language": "fr",
  "timeout_seconds": 7
}
```

### POST /chat

```json
{
  "text": "Explique-moi le fleuve Congo simplement.",
  "language": "fr"
}
```

### POST /voice-chat

```json
{
  "language": "fr",
  "timeout_seconds": 7
}
```

## Tests locaux

Depuis le dossier qui contient `scripts/` :

```powershell
python scripts/test_speak.py
python scripts/test_chat.py
python scripts/test_listen.py
python scripts/test_voice_chat.py
```

Notes :

- `test_speak.py` peut fonctionner sans Piper grâce à `pyttsx3`.
- `test_chat.py` nécessite LM Studio lancé avec un modèle chargé, ou Ollama si `LLM_PROVIDER=ollama`.
- `test_listen.py` et `test_voice_chat.py` nécessitent les modèles Vosk et un micro accessible.

## Utilisation par le reste de l'équipe

Depuis du code Python lancé avec le dossier du module dans le `PYTHONPATH` :

```python
from voice.stt import listen
from voice.tts import speak
from voice.llm_client import chat_with_local_model
from voice.voice_pipeline import voice_chat

text = listen(language="fr")
response = chat_with_local_model(text, language="fr")
speak(response, language="fr")
```

Ou via HTTP local :

```powershell
Invoke-RestMethod -Method Post http://127.0.0.1:8000/chat `
  -ContentType "application/json" `
  -Body '{"text":"Bonjour Rafiki","language":"fr"}'
```

## Erreurs fréquentes

### LM Studio n'est pas lancé

Ouvrir LM Studio, charger un modèle, puis démarrer le serveur local dans Developer > Start server.

### Aucun modèle LM Studio chargé

Charger un modèle dans LM Studio ou renseigner `LM_STUDIO_MODEL` dans `.env` avec le nom exact du modèle exposé par `/v1/models`.

### Modèle Vosk introuvable

Télécharger les modèles et vérifier `VOSK_MODEL_FR_PATH` ou `VOSK_MODEL_EN_PATH`.

### Erreur microphone

Vérifier que le micro est branché, autorisé par Windows et disponible pour Python. Le module respecte `LISTEN_TIMEOUT_SECONDS`.

### Piper absent

Ce n'est pas bloquant pour les premiers tests : `pyttsx3` prend le relais.

## Sécurité Git

Ne pas commiter : `.env`, `.venv/`, `models/`, `outputs/`, fichiers WAV générés, caches Python, clés API.