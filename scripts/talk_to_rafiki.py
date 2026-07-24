from __future__ import annotations

import argparse
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[1]
if str(PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(PROJECT_ROOT))

from orchestrator.services.fallback_client import FallbackRafikiClient
from orchestrator.services.llm_client import LLMClientError, RafikiLLMClient
from voice.tts import EspeakTTS, TextToSpeechError


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Talk with Rafiki from the Raspberry Pi terminal."
    )
    parser.add_argument("--base-url", default="http://127.0.0.1:8080")
    parser.add_argument("--model", default="rafiki-local")
    parser.add_argument("--language", default="fr")
    parser.add_argument("--no-voice", action="store_true")
    parser.add_argument("--no-fallback", action="store_true")
    parser.add_argument("--tts-voice", default="fr-fr")
    parser.add_argument("--tts-speed", type=int, default=155)
    parser.add_argument("--tts-pitch", type=int, default=45)
    return parser


def main() -> int:
    args = build_parser().parse_args()
    llm_client = RafikiLLMClient(base_url=args.base_url, model=args.model)
    fallback_client = FallbackRafikiClient()
    tts = EspeakTTS(
        voice=args.tts_voice,
        speed=args.tts_speed,
        pitch=args.tts_pitch,
        enabled=not args.no_voice,
    )

    use_fallback = False
    if llm_client.is_ready():
        print(f"Rafiki: LLM pret sur {args.base_url}.")
    elif args.no_fallback:
        print(f"Rafiki: llama-server n'est pas pret sur {args.base_url}.")
        return 2
    else:
        use_fallback = True
        print("Rafiki: llama-server n'est pas pret, mode secours active.")

    if not args.no_voice and not tts.is_available():
        print("Rafiki: espeak-ng n'est pas installe, sortie vocale desactivee.")
        tts.enabled = False

    print("Rafiki: ecris ton message. Tape /quit pour sortir.")
    history: list[dict[str, str]] = []

    while True:
        try:
            user_message = input("Toi > ").strip()
        except (EOFError, KeyboardInterrupt):
            print()
            break

        if user_message.lower() in {"/quit", "/exit", "quit", "exit"}:
            break

        if not user_message:
            continue

        client = fallback_client if use_fallback else llm_client

        try:
            decision = client.generate(
                user_message,
                language=args.language,
                history=history,
            )
        except LLMClientError as exc:
            if args.no_fallback:
                print(f"Rafiki: erreur LLM: {exc}")
                continue

            use_fallback = True
            decision = fallback_client.generate(user_message, language=args.language)

        print(f"Rafiki > {decision.speech}")
        print(
            "Decision:",
            f"emotion={decision.emotion},",
            f"movement={decision.movement},",
            f"screen={decision.screen_mode}",
        )

        try:
            tts.speak(decision.speech)
        except TextToSpeechError as exc:
            print(f"Rafiki: sortie vocale impossible: {exc}")

        history.extend(
            [
                {"role": "user", "content": user_message},
                {"role": "assistant", "content": decision.model_dump_json()},
            ]
        )

    print("Rafiki: a bientot !")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
