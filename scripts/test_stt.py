from __future__ import annotations

import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PROJECT_ROOT))

from voice.config import get_vosk_model_path
from voice.stt import listen


def main() -> None:
    print("Test STT en cours...")
    model_path = get_vosk_model_path("fr")
    if not model_path.exists():
        print(
            "Erreur : modele Vosk introuvable. "
            f"Chemin attendu : {model_path}"
        )
        return

    print("Parle maintenant...")
    text = listen(language="fr", timeout_seconds=6)
    print(f"Texte reconnu : {text or '(rien reconnu)'}")


if __name__ == "__main__":
    main()
