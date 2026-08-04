# Rafiki
Rafiki : Robot compagnon pour enfants (Projet Makers 2026). Propulsé par un Raspberry Pi, il intègre la vision par ordinateur (suivi du regard), un LLM avec gestion de contexte, une synthèse vocale naturelle et un planificateur de routines.

## Developpement local

Installer les dependances dans l'environnement virtuel :

```bash
.venv/bin/python -m pip install -r requirements.txt
```

Lancer les tests unitaires :

```bash
.venv/bin/python -m pytest
```

Verifier l'etat du serveur LLM local sur la Raspberry :

```bash
.venv/bin/python scripts/check_llm.py
```

Faire un essai de generation quand `llama-server` est demarre :

```bash
.venv/bin/python scripts/generate_llm_sample.py
```

Discuter avec Rafiki depuis le terminal, avec reponse vocale :

```bash
.venv/bin/python scripts/talk_to_rafiki.py
```

Discuter avec entree micro reseau Whisper et sortie vocale Piper :

```bash
.venv/bin/python scripts/talk_to_rafiki.py \
  --input pulse \
  --pulse-device tunnel-source.tcp:10.20.20.149 \
  --tts-engine piper \
  --stt-engine whisper \
  --whisper-model models/whisper/ggml-base-q5_1.bin \
  --piper-executable tools/piper/piper \
  --piper-model models/piper/siwis/fr_FR-siwis-medium.onnx \
  --audio-player paplay \
  --audio-device tunnel-sink.tcp:10.20.20.149
```

Whisper est utilise par defaut pour l'entree PulseAudio. Vosk reste disponible
avec `--stt-engine vosk`, notamment pour les entrees ALSA et IP Webcam. Pour
garder l'ancienne voix `espeak-ng`, utiliser `--tts-engine espeak`.

Voir aussi le guide complet : `docs/voice_test_guide.md`.

Avec un telephone Android qui lance IP Webcam :

```bash
.venv/bin/python scripts/talk_to_rafiki.py \
  --input ip-webcam \
  --ip-webcam-url http://192.168.1.42:8080/audio.wav \
  --tts-engine piper \
  --stt-model models/vosk/fr \
  --piper-executable tools/piper/piper \
  --piper-model models/piper/siwis/fr_FR-siwis-medium.onnx \
  --audio-device plughw:1,0
```

Si aucun modele local n'est encore installe, lancer Gemma 3 4B dans un
deuxieme terminal :

```bash
bash scripts/start_llama_server_gemma.sh
```
