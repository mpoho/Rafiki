import pyttsx3
from voice.config import VOICE_RATE, VOICE_VOLUME

_engine = None


def get_engine():
    global _engine

    if _engine is None:
        _engine = pyttsx3.init()
        _engine.setProperty("rate", VOICE_RATE)
        _engine.setProperty("volume", VOICE_VOLUME)

    return _engine


def speak(text: str) -> None:
    """
    Fait parler Arthy localement.

    Version 1 : pyttsx3.
    Version future : Piper TTS.
    """
    if not text or not text.strip():
        raise ValueError("Le texte à prononcer est vide.")

    engine = get_engine()
    engine.say(text)
    engine.runAndWait()
