from __future__ import annotations

import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PROJECT_ROOT))

from voice.config import get_vosk_model_path
from voice.stt import listen
from voice.tts import speak
from voice.voice_pipeline import resolve_reply


def _validate_french_model(model_path: Path) -> bool:
    if not model_path.exists():
        print(
            "Erreur : modele Vosk introuvable. "
            f"Chemin attendu : {model_path}"
        )
        return False

    if "fr" not in model_path.name.lower():
        print(
            "Erreur : le modele Vosk configure ne semble pas etre francais. "
            f"Modele detecte : {model_path}"
        )
        return False

    return True


def main() -> None:
    print("Test conversation Rafiki")

    model_path = get_vosk_model_path("fr")
    if not _validate_french_model(model_path):
        return

    greeting = "Je t'ecoute."
    print(f"Rafiki parle : {greeting}")
    speak(greeting, language="fr")

    print("Rafiki ecoute...")
    heard_text = listen(language="fr", timeout_seconds=8)

    if not heard_text:
        response_text = (
            "Je n'ai pas bien compris. Reessaie en parlant plus clairement."
        )
        print("Texte reconnu : (rien reconnu)")
        print(f"Reponse Rafiki : {response_text}")
        print("Rafiki repond vocalement...")
        speak(response_text, language="fr")
        return

    print(f"Texte reconnu : {heard_text}")
    response_text = resolve_reply(heard_text, language="fr")
    print(f"Reponse Rafiki : {response_text}")
    print("Rafiki repond vocalement...")
    speak(response_text, language="fr")


if __name__ == "__main__":
    main()
