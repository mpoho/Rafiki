from __future__ import annotations

import requests

from .config import normalize_language, settings


OLLAMA_NOT_RUNNING_MESSAGE = "Ollama n'est pas lance. Lance : ollama serve"
OLLAMA_MODEL_MISSING_MESSAGE = (
    f"Installe le modele avec : ollama pull {settings.ollama_model}"
)


def _default_system_prompt(language: str) -> str:
    if language == "en":
        response_language = "English"
    else:
        response_language = "French"

    return (
        "You are Rafiki, a kind companion robot for a child. "
        f"Always answer in {response_language}. "
        "Speak gently with short sentences. "
        "Use a warm and encouraging tone. "
        "Do not provide dangerous, violent, sexual, or unsafe content. "
        "If a topic is risky, redirect safely. "
        "Encourage the child and ask simple questions."
    )


def chat_with_gemma(
    text: str,
    language: str = "fr",
    system_prompt: str | None = None,
) -> str:
    language = normalize_language(language)
    prompt = text.strip()
    if not prompt:
        return ""

    payload = {
        "model": settings.ollama_model,
        "prompt": prompt,
        "system": system_prompt or _default_system_prompt(language),
        "stream": False,
    }

    url = f"{settings.ollama_base_url.rstrip('/')}/api/generate"

    try:
        response = requests.post(url, json=payload, timeout=60)
    except requests.exceptions.ConnectionError:
        return OLLAMA_NOT_RUNNING_MESSAGE
    except requests.exceptions.Timeout:
        return "Ollama ne repond pas assez vite. Verifie le modele local."
    except requests.RequestException as exc:
        return f"Erreur Ollama : {exc}"

    if response.status_code == 404:
        return OLLAMA_MODEL_MISSING_MESSAGE

    try:
        data = response.json()
    except ValueError:
        return f"Reponse Ollama invalide : {response.text[:200]}"

    error = data.get("error")
    if error:
        normalized_error = str(error).lower()
        if "not found" in normalized_error or "model" in normalized_error:
            return OLLAMA_MODEL_MISSING_MESSAGE

        return f"Erreur Ollama : {error}"

    if not response.ok:
        return f"Erreur Ollama HTTP {response.status_code} : {response.text[:200]}"

    return data.get("response", "").strip()