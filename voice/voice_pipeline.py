from __future__ import annotations

from .config import normalize_language
from .llm_client import chat_with_local_model, is_llm_error_response
from .stt import listen
from .tts import speak


REPEAT_MESSAGES = {
    "fr": "Je n'ai pas bien entendu. Peux-tu repeter ?",
    "en": "I did not hear you clearly. Can you repeat?",
}

SCHOOL_KEYWORDS = (
    "ecole",
    "école",
    "cours",
    "devoir",
    "devoirs",
    "lecon",
    "leçon",
    "exercice",
    "matiere",
    "matière",
)


def generate_simple_reply(text: str, language: str = "fr") -> str:
    language = normalize_language(language)
    normalized = text.lower().strip()

    if not normalized:
        return REPEAT_MESSAGES.get(language, REPEAT_MESSAGES["fr"])

    if language == "fr":
        if "bonjour" in normalized or "salut" in normalized:
            return "Bonjour, je suis Rafiki. Je suis content de te parler."

        if any(keyword in normalized for keyword in SCHOOL_KEYWORDS):
            return "D'accord, je peux t'aider avec tes devoirs."

        return (
            "J'ai entendu ce que tu as dit. "
            "Je vais bientot apprendre a mieux te repondre."
        )

    if "hello" in normalized or "hi" in normalized:
        return "Hello, I am Rafiki. I am happy to talk with you."

    if any(keyword in normalized for keyword in ("school", "homework", "class")):
        return "Okay, I can help you with your homework."

    return "I heard what you said. I will soon learn to answer you better."


def resolve_reply(text: str, language: str = "fr") -> str:
    language = normalize_language(language)
    llm_response = chat_with_local_model(text, language=language)

    if llm_response and not is_llm_error_response(llm_response):
        return llm_response

    return generate_simple_reply(text, language=language)


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

    response_text = resolve_reply(heard_text, language=language)
    speak(response_text, language=language)

    return {
        "heard_text": heard_text,
        "response_text": response_text,
        "language": language,
    }


def is_llm_error(response_text: str) -> bool:
    return is_llm_error_response(response_text)