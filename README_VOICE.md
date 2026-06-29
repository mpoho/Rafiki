# Module voix local Rafiki

Rafiki est un robot compagnon intelligent pour enfant. Ce module lui donne quatre outils simples pour l'agent principal du robot :

- `listen(language="fr")` : écouter l'enfant et retourner du texte.
- `speak(text, language="fr")` : faire parler Rafiki.
- `chat(text, language="fr")` : envoyer un texte au modèle local Gemma via Ollama.
- `voice_chat(language="fr")` : écouter, interroger le modèle, puis faire parler Rafiki.

Le périmètre est volontairement limité à la voix français/anglais et à la connexion local-first avec Ollama. Il ne contient pas la mémoire complète, la vision, les routines ou le hardware du robot.

## Technologies

- Python 3.10+
- FastAPI + Uvicorn pour l'API locale
- Vosk pour la reconnaissance vocale hors-ligne
- Piper TTS pour la synthèse vocale locale
- pyttsx3 comme fallback temporaire si Piper ou une voix Piper manque
- Ollama pour appeler Gemma localement
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
OLLAMA_BASE_URL=http://localhost:11434
OLLAMA_MODEL=gemma4
AUDIO_OUTPUT_DIR=outputs/audio
```

Les chemins relatifs sont résolus depuis le dossier qui contient le package `voice/`.

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

Une aide courte existe aussi dans `scripts/download_vosk_models.md`.

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

Si Piper n'est pas disponible ou si le modèle manque, `speak()` utilise automatiquement `pyttsx3`. Le terminal affiche clairement que le fallback est actif.

Pour une installation manuelle de Piper, utiliser les releases officielles ou la méthode recommandée pour ton système, puis vérifier :

```powershell
piper --help
```

## Ollama et Gemma

Installer Ollama, puis lancer le serveur local :

```powershell
ollama serve
```

Dans un autre terminal, installer le modèle configuré :

```powershell
ollama pull gemma4
```

Si le nom exact du modèle change dans l'environnement de l'équipe, modifier `OLLAMA_MODEL` dans `.env`.

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

Retourne l'état du module voix local.

### POST /speak

```json
{
  "text": "Bonjour, je suis Rafiki.",
  "language": "fr"
}
```

Réponse :

```json
{
  "ok": true,
  "message": "Rafiki a parle avec succes.",
  "audio_path": "outputs/audio/rafiki_fr_...wav"
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
- `test_chat.py` nécessite `ollama serve` et le modèle `gemma4`.
- `test_listen.py` et `test_voice_chat.py` nécessitent les modèles Vosk et un micro accessible.

## Structure

```text
voice/
  __init__.py
  config.py
  schemas.py
  stt.py
  tts.py
  llm_client.py
  voice_pipeline.py
  voice_server.py
scripts/
  download_vosk_models.md
  test_speak.py
  test_listen.py
  test_chat.py
  test_voice_chat.py
README_VOICE.md
.env.example
requirements.txt
```

## Utilisation par le reste de l'équipe

Depuis du code Python lancé avec le dossier du module dans le `PYTHONPATH` :

```python
from voice.stt import listen
from voice.tts import speak
from voice.llm_client import chat_with_gemma
from voice.voice_pipeline import voice_chat

text = listen(language="fr")
response = chat_with_gemma(text, language="fr")
speak(response, language="fr")
```

Ou via HTTP local :

```powershell
Invoke-RestMethod -Method Post http://127.0.0.1:8000/chat `
  -ContentType "application/json" `
  -Body '{"text":"Bonjour Rafiki","language":"fr"}'
```

## Erreurs fréquentes

### Modèle Vosk introuvable

Le chemin configuré ne contient pas le modèle. Télécharger les modèles et vérifier `VOSK_MODEL_FR_PATH` ou `VOSK_MODEL_EN_PATH`.

### Erreur microphone

Vérifier que le micro est branché, autorisé par Windows et disponible pour Python. Le module respecte `LISTEN_TIMEOUT_SECONDS` et ne doit pas bloquer indéfiniment.

### Piper absent

Ce n'est pas bloquant pour les premiers tests : `pyttsx3` prend le relais. Pour la voix finale du robot, installer Piper et placer les modèles dans `models/piper/`.

### Ollama n'est pas lancé

Lancer :

```powershell
ollama serve
```

### Modèle Gemma absent

Installer :

```powershell
ollama pull gemma4
```

## Sécurité Git

Ne pas commiter :

- `.env`
- `.venv/`
- `models/`
- `outputs/`
- fichiers WAV générés
- caches Python
- clés API

Ces éléments sont ignorés par `.gitignore`.