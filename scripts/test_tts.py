from __future__ import annotations

import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PROJECT_ROOT))

from voice.tts import speak


def main() -> None:
    text = "Bonjour, je suis Rafiki. Le test de voix fonctionne correctement."
    print("Test TTS en cours...")
    print("Rafiki parle maintenant...")
    audio_path = speak(text, language="fr")

    if audio_path:
        print(f"Audio genere : {audio_path}")
    else:
        print("Audio lu via fallback sans fichier WAV utilisable.")


if __name__ == "__main__":
    main()
