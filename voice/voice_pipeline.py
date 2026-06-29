from __future__ import annotations

from .config import normalize_language
from .llm_client import (
    OLLAMA_MODEL_MISSING_MESSAGE,
    OLLAMA_NOT_RUNNING_MESSAGE,
    chat_with_gemma,
)
from .stt import listen
from .tts import speak


REPEAT_MESSAGES = {
    "fr": "Je n'ai pas bien entendu. Peux-tu repeter ?",
    "en": "I did not hear you clearly. Can you repeat?",
}


def voice_chat(language: str = "fr", timeout_seconds: int | None = None) -> dict:
    language = normalize_language(language)
    heard_text = listen(language=language, timeout_seconds=timeout_seconds)

    if not heard_text:
        response_text = REPEAT_MESSAGES.get(language, REPEAT_MESSAGES["fr"])
        speak(response_text, language=language)
        return {
            "heard_text": "",
            "response_text": response_text,
            "language": language,
        }

    response_text = chat_with_gemma(heard_text, language=language)
    if not response_text:
        response_text = REPEAT_MESSAGES.get(language, REPEAT_MESSAGES["fr"])

    speak(response_text, language=language)

    return {
        "heard_text": heard_text,
        "response_text": response_text,
        "language": language,
    }


def is_llm_error(response_text: str) -> bool:
    return (
        response_text in {
            OLLAMA_NOT_RUNNING_MESSAGE,
            OLLAMA_MODEL_MISSING_MESSAGE,
        }
        or response_text.startswith("Erreur Ollama")
        or response_text.startswith("Ollama ne repond")
        or response_text.startswith("Reponse Ollama invalide")
    )