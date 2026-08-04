# Guide de test voix Whisper + Piper

Ce guide permet de verifier l'entree vocale Whisper et la sortie vocale Piper sur
la Raspberry Pi.

## Fichiers installes

- Reconnaissance vocale Whisper : `models/whisper/ggml-base-q5_1.bin`
- Binaire Whisper ARM64 : `tools/whisper.cpp/build/bin/whisper-cli`
- Reconnaissance Vosk de secours : `models/vosk/fr`
- Voix Piper claire : `models/piper/siwis/fr_FR-siwis-medium.onnx`
- Ancienne voix Piper : `models/piper/fr_FR-upmc-medium.onnx`
- Binaire Piper ARM64 : `tools/piper/piper`

## 1. Verifier les dependances

```bash
.venv/bin/python -m pip install -r requirements.txt
tools/piper/piper --help
arecord -l
aplay -l
```

Si `arecord -l` ne liste aucun micro, verifier le micro USB ou la carte son.
Un telephone connecte en Bluetooth n'apparait pas toujours comme micro ALSA :
sur Android, le profil Bluetooth expose souvent le telephone comme source media
ou passerelle mains libres, sans creer de source micro utilisable par Vosk.

## 2. Tester seulement Piper

Generer un fichier WAV :

```bash
echo "Bonjour, je suis Rafiki." | \
  tools/piper/piper \
    --model models/piper/fr_FR-upmc-medium.onnx \
    --output_file /tmp/rafiki-test.wav
```

Jouer le fichier :

```bash
aplay /tmp/rafiki-test.wav
```

## 3. Tester seulement le micro

Enregistrer 3 secondes :

```bash
arecord -d 3 -r 16000 -f S16_LE -c 1 /tmp/rafiki-micro.wav
aplay /tmp/rafiki-micro.wav
```

Si le son est trop faible, regler le gain avec `alsamixer`.

## 4. Tester Rafiki en vocal complet

Lancer le serveur LLM dans un terminal si necessaire :

```bash
bash scripts/start_llama_server_gemma.sh
```

Pour une demo plus reactive, utiliser le petit modele Qwen deja cache :

```bash
bash scripts/start_llama_server_fast.sh
```

Puis lancer la conversation vocale :

```bash
.venv/bin/python scripts/talk_to_rafiki.py \
  --input voice \
  --tts-engine piper \
  --stt-model models/vosk/fr \
  --piper-executable tools/piper/piper \
  --piper-model models/piper/siwis/fr_FR-siwis-medium.onnx \
  --audio-device plughw:1,0
```

Avec un micro et un baffle exposes par PulseAudio/PipeWire via le reseau :

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

Avec le corps Arduino Mega branche en USB, ajouter le pilote corps :

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
  --audio-device tunnel-sink.tcp:10.20.20.149 \
  --body-driver arduino \
  --body-port /dev/ttyACM0
```

Le sketch Arduino accepte `B0` a `B9` pour les comportements synchronises,
`TEXT:<message>` pour l'ecran texte, `SHOW_EYES`, `BLINK`, `BSTOP` et `S0`.
Rafiki lance le comportement au debut reel de la lecture audio et envoie `S0`
des que la voix finit, ainsi qu'a la fermeture. Une commande `B` pilote ensemble
l'expression et les servos, sans qu'une commande d'ecran annule le mouvement.
Le Mega est initialise huit secondes au lancement, avant la premiere parole,
car son ecran TFT ralentit le demarrage serie.

## Option : telephone avec IP Webcam

Dans l'application IP Webcam sur Android :

1. Ouvrir IP Webcam.
2. Autoriser la camera et le microphone.
3. Activer l'audio dans les preferences si necessaire.
4. Descendre tout en bas et appuyer sur `Start server`.
5. Noter l'adresse affichee, par exemple `http://192.168.1.42:8080`.

Verifier depuis la Raspberry :

```bash
ffmpeg -hide_banner -t 3 \
  -i http://192.168.1.42:8080/audio.wav \
  -f wav /tmp/ip-webcam-test.wav

aplay -D plughw:1,0 /tmp/ip-webcam-test.wav
```

Lancer Rafiki directement sur le flux audio HTTP :

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

Au signal `Toi > j'ecoute...`, parler clairement pendant une phrase courte.
Pour quitter, faire `Ctrl+C` ou dire `quit`.

## Depannage rapide

- `moteur vocal indisponible` : verifier `tools/piper/piper`, `aplay` et le
  fichier `models/piper/siwis/fr_FR-siwis-medium.onnx`.
- Sortie HDMI muette ou erreur ALSA : essayer `--audio-device plughw:1,0`.
- `entree vocale indisponible` : verifier le modele Whisper, son binaire et le micro.
- Telephone Bluetooth visible mais pas de micro : verifier `wpctl status`.
  Si aucune entree n'apparait dans `Audio > Sources`, utiliser plutot un micro
  USB ou une application qui expose le telephone comme micro reseau/USB audio.
- `No speech recognized` : rapprocher le micro ou ajuster
  `--stt-speech-threshold` (260 par defaut).
- Mauvais micro : passer un device, par exemple `--stt-device 1`, apres avoir
  regarde la liste avec `arecord -l`.
