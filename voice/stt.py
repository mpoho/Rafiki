from __future__ import annotations

import json
import queue
import time
from functools import lru_cache

from .config import get_vosk_model_path, normalize_language, settings


@lru_cache(maxsize=2)
def load_vosk_model(language: str):
    language = normalize_language(language)
    model_path = get_vosk_model_path(language)

    if not model_path.exists():
        raise FileNotFoundError(
            f"Modele Vosk introuvable pour '{language}'. "
            f"Chemin attendu : {model_path}"
        )

    try:
        from vosk import Model
    except ImportError as exc:
        raise RuntimeError(
            "La dependance 'vosk' est manquante. Installe : pip install vosk"
        ) from exc

    return Model(str(model_path))


def listen(language: str = "fr", timeout_seconds: int | None = None) -> str:
    language = normalize_language(language)
    timeout = timeout_seconds or settings.listen_timeout_seconds
    timeout = max(1, int(timeout))
    model = load_vosk_model(language)

    try:
        import sounddevice as sd
        from vosk import KaldiRecognizer
    except ImportError as exc:
        raise RuntimeError(
            "Les dependances audio sont manquantes. "
            "Installe : pip install sounddevice vosk"
        ) from exc

    audio_queue: queue.Queue[bytes] = queue.Queue()
    recognizer = KaldiRecognizer(model, settings.vosk_sample_rate)

    def audio_callback(indata, frames, callback_time, status):
        if status:
            print(f"Alerte microphone : {status}")
        audio_queue.put(bytes(indata))

    recognized_parts: list[str] = []

    try:
        with sd.RawInputStream(
            samplerate=settings.vosk_sample_rate,
            blocksize=8000,
            dtype="int16",
            channels=1,
            callback=audio_callback,
        ):
            deadline = time.monotonic() + timeout

            while time.monotonic() < deadline:
                remaining = max(0.05, min(0.25, deadline - time.monotonic()))

                try:
                    data = audio_queue.get(timeout=remaining)
                except queue.Empty:
                    continue

                if recognizer.AcceptWaveform(data):
                    result = json.loads(recognizer.Result())
                    text = result.get("text", "").strip()
                    if text:
                        recognized_parts.append(text)

            final_result = json.loads(recognizer.FinalResult())
            final_text = final_result.get("text", "").strip()
            if final_text:
                recognized_parts.append(final_text)

    except Exception as exc:
        raise RuntimeError(
            f"Erreur microphone : impossible d'ecouter pendant {timeout} s. {exc}"
        ) from exc

    return " ".join(recognized_parts).strip()