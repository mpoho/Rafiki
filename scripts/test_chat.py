from __future__ import annotations

import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PROJECT_ROOT))

from voice.llm_client import chat_with_gemma


def main() -> None:
    prompt = "Explique-moi le fleuve Congo simplement."
    print(f"Question : {prompt}")
    response = chat_with_gemma(prompt, language="fr")
    print(f"Reponse : {response}")


if __name__ == "__main__":
    main()