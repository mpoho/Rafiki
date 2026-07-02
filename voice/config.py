from __future__ import annotations

import os
from dataclasses import dataclass
from pathlib import Path

from dotenv import load_dotenv


PROJECT_ROOT = Path(__file__).resolve().parents[1]
load_dotenv(PROJECT_ROOT / ".env")


def _env_int(name: str, default: int) -> int:
    value = os.getenv(name)
    if value is None or value == "":
        return default

    try:
        return int(value)
    except ValueError as exc:
        raise ValueError(f"La variable {name} doit etre un entier.") from exc


def _env_str(name: str, default: str) -> str:
    return os.getenv(name, default)


@dataclass(frozen=True)
class VoiceSettings:
    default_language: str = _env_str("VOICE_DEFAULT_LANGUAGE", "fr")
    vosk_model_fr_path: str = _env_str(
        "VOSK_MODEL_FR_PATH",
        "models/vosk/vosk-model-small-fr-0.22",
    )
    vosk_model_en_path: str = _env_str(
        "VOSK_MODEL_EN_PATH",
        "models/vosk/vosk-model-small-en-us-0.15",
    )
    vosk_sample_rate: int = _env_int("VOSK_SAMPLE_RATE", 16000)
    listen_timeout_seconds: int = _env_int("LISTEN_TIMEOUT_SECONDS", 8)
    piper_voice_fr_path: str = _env_str(
        "PIPER_VOICE_FR_PATH",
        "models/piper/fr/fr_FR-upmc-medium.onnx",
    )
    piper_voice_en_path: str = _env_str(
        "PIPER_VOICE_EN_PATH",
        "models/piper/en/en_US-lessac-medium.onnx",
    )
    llm_provider: str = _env_str("LLM_PROVIDER", "lmstudio")
    llm_timeout_seconds: int = _env_int("LLM_TIMEOUT_SECONDS", 60)
    llm_max_tokens: int = _env_int("LLM_MAX_TOKENS", 120)
    lmstudio_base_url: str = _env_str("LM_STUDIO_BASE_URL", "http://localhost:1234/v1")
    lmstudio_model: str = _env_str("LM_STUDIO_MODEL", "")
    ollama_base_url: str = _env_str("OLLAMA_BASE_URL", "http://localhost:11434")
    ollama_model: str = _env_str("OLLAMA_MODEL", "gemma4")
    audio_output_dir: str = _env_str("AUDIO_OUTPUT_DIR", "outputs/audio")


settings = VoiceSettings()


def resolve_project_path(path_value: str | Path) -> Path:
    path = Path(path_value)
    if path.is_absolute():
        return path

    return PROJECT_ROOT / path


def normalize_language(language: str | None = None) -> str:
    language = (language or settings.default_language).lower().strip()

    if language.startswith("fr"):
        return "fr"

    if language.startswith("en"):
        return "en"

    raise ValueError("Langue non supportee. Utilise 'fr' ou 'en'.")


def get_vosk_model_path(language: str | None = None) -> Path:
    language = normalize_language(language)
    if language == "fr":
        return resolve_project_path(settings.vosk_model_fr_path)

    return resolve_project_path(settings.vosk_model_en_path)


def get_piper_voice_path(language: str | None = None) -> Path:
    language = normalize_language(language)
    if language == "fr":
        return resolve_project_path(settings.piper_voice_fr_path)

    return resolve_project_path(settings.piper_voice_en_path)


def get_audio_output_dir() -> Path:
    return resolve_project_path(settings.audio_output_dir)