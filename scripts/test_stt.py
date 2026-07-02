from __future__ import annotations

import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PROJECT_ROOT))

from voice.config import get_vosk_model_path
from voice.stt import listen


def _validate_french_model(model_path: Path) -> bool:
    if not model_path.exists():
        print(
            "Erreur : modele Vosk introuvable. "
            f"Chemin attendu : {model_path}"
        )
        return False

    model_name = model_path.name.lower()
    if "fr" not in model_name:
        print(
            "Erreur : le modele Vosk configure ne semble pas etre francais. "
            f"Modele detecte : {model_path}"
        )
        return False

    print(f"Modele Vosk francais detecte : {model_path}")
    return True


def main() -> None:
    print("Test STT en cours...")
    model_path = get_vosk_model_path("fr")
    if not _validate_french_model(model_path):
        return

    print("Rafiki ecoute...")
    print("Parle maintenant.")
    text = listen(language="fr", timeout_seconds=8)

    if text:
        print(f"Texte reconnu : {text}")
    else:
        print("Je n'ai pas bien compris. Reessaie en parlant plus clairement.")


if __name__ == "__main__":
    main()
