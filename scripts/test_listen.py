from __future__ import annotations

import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PROJECT_ROOT))

from voice.stt import listen


def main() -> None:
    for language in ("fr", "en"):
        print(f"Test listen [{language}] : parle maintenant...")
        try:
            text = listen(language=language, timeout_seconds=7)
        except Exception as exc:
            print(f"Erreur listen [{language}] : {exc}")
            continue

        print(f"Texte reconnu [{language}] : {text or '(rien reconnu)'}")


if __name__ == "__main__":
    main()