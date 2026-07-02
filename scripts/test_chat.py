from __future__ import annotations

import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PROJECT_ROOT))

from voice.llm_client import chat_with_local_model, get_llm_model_label, get_llm_provider


def main() -> None:
    prompt = "Explique-moi le fleuve Congo simplement."
    print(f"Provider : {get_llm_provider()}")
    print(f"Modele : {get_llm_model_label()}")
    print(f"Question : {prompt}")
    response = chat_with_local_model(prompt, language="fr")
    print(f"Reponse : {response}")


if __name__ == "__main__":
    main()