from __future__ import annotations

import os
import shutil
import subprocess
import sys
from datetime import datetime
from pathlib import Path

from .config import get_audio_output_dir, get_piper_voice_path, normalize_language


def speak(text: str, language: str = "fr") -> str:
    if not text or not text.strip():
        raise ValueError("Le texte a prononcer est vide.")

    language = normalize_language(language)
    text = text.strip()
    output_path = _build_output_path(language)
    voice_path = get_piper_voice_path(language)

    if voice_path.exists():
        piper_executable = _find_piper_executable()
        if piper_executable:
            try:
                _run_piper(piper_executable, voice_path, output_path, text)
                _play_audio_file(output_path)
                return str(output_path)
            except Exception as exc:
                print(f"Piper a echoue, fallback pyttsx3 actif : {exc}")
        else:
            print("Piper n'est pas disponible dans le PATH, fallback pyttsx3 actif.")
    else:
        print(
            "Modele vocal Piper introuvable, fallback pyttsx3 actif. "
            f"Chemin attendu : {voice_path}"
        )

    return _speak_with_pyttsx3(text, output_path)


def _build_output_path(language: str) -> Path:
    output_dir = get_audio_output_dir()
    output_dir.mkdir(parents=True, exist_ok=True)
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S_%f")

    return output_dir / f"rafiki_{language}_{timestamp}.wav"


def _find_piper_executable() -> str | None:
    configured = os.getenv("PIPER_EXECUTABLE")
    if configured:
        configured_path = Path(configured)
        if configured_path.exists():
            return str(configured_path)

    return shutil.which("piper")


def _run_piper(
    piper_executable: str,
    voice_path: Path,
    output_path: Path,
    text: str,
) -> None:
    completed = subprocess.run(
        [
            piper_executable,
            "--model",
            str(voice_path),
            "--output_file",
            str(output_path),
        ],
        input=text,
        text=True,
        capture_output=True,
        timeout=60,
        check=False,
    )

    if completed.returncode != 0:
        stderr = completed.stderr.strip() or completed.stdout.strip()
        raise RuntimeError(stderr or "commande Piper en echec")

    if not output_path.exists() or output_path.stat().st_size == 0:
        raise RuntimeError("Piper n'a pas genere de fichier WAV.")


def _speak_with_pyttsx3(text: str, output_path: Path) -> str:
    try:
        import pyttsx3
    except ImportError as exc:
        raise RuntimeError(
            "pyttsx3 est manquant. Installe : pip install pyttsx3"
        ) from exc

    print("Fallback pyttsx3 utilise pour la synthese vocale locale.")
    engine = pyttsx3.init()
    engine.save_to_file(text, str(output_path))
    engine.runAndWait()

    if output_path.exists() and output_path.stat().st_size > 0:
        _play_audio_file(output_path)
        return str(output_path)

    print("pyttsx3 n'a pas cree de WAV ; lecture directe du texte.")
    engine.say(text)
    engine.runAndWait()

    return ""


def _play_audio_file(path: Path) -> None:
    try:
        if sys.platform.startswith("win"):
            import winsound

            winsound.PlaySound(str(path), winsound.SND_FILENAME)
            return

        if sys.platform == "darwin":
            subprocess.run(["afplay", str(path)], check=False)
            return

        for player in ("aplay", "paplay", "ffplay"):
            executable = shutil.which(player)
            if not executable:
                continue

            command = [executable, str(path)]
            if player == "ffplay":
                command = [executable, "-nodisp", "-autoexit", str(path)]

            subprocess.run(command, check=False)
            return

        print(f"Aucun lecteur audio local trouve pour lire : {path}")
    except Exception as exc:
        print(f"Lecture audio impossible pour {path} : {exc}")