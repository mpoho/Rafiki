from __future__ import annotations

import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PROJECT_ROOT))

from voice.voice_pipeline import voice_chat


def main() -> None:
    print("Voice chat : Rafiki ecoute, interroge Gemma via Ollama, puis parle.")
    try:
        result = voice_chat(language="fr", timeout_seconds=7)
    except Exception as exc:
        print(f"Erreur voice_chat : {exc}")
        return

    print(f"Texte entendu : {result['heard_text'] or '(rien reconnu)'}")
    print(f"Reponse : {result['response_text']}")


if __name__ == "__main__":
    main()