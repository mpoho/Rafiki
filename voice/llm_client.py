from __future__ import annotations

import requests

from .config import normalize_language, settings


LM_STUDIO_NOT_RUNNING_MESSAGE = (
    "LM Studio n'est pas lance. Ouvre LM Studio, charge un modele, "
    "puis demarre le serveur local dans Developer > Start server."
)
LM_STUDIO_MODEL_MISSING_MESSAGE = (
    "Aucun modele LM Studio n'est charge. Charge un modele dans LM Studio "
    "ou renseigne LM_STUDIO_MODEL dans .env."
)
OLLAMA_NOT_RUNNING_MESSAGE = "Ollama n'est pas lance. Lance : ollama serve"
OLLAMA_MODEL_MISSING_MESSAGE = (
    f"Installe le modele avec : ollama pull {settings.ollama_model}"
)
KNOWN_LLM_ERROR_MESSAGES = {
    LM_STUDIO_NOT_RUNNING_MESSAGE,
    LM_STUDIO_MODEL_MISSING_MESSAGE,
    OLLAMA_NOT_RUNNING_MESSAGE,
    OLLAMA_MODEL_MISSING_MESSAGE,
}
LLM_ERROR_PREFIXES = (
    "Erreur LM Studio",
    "LM Studio ne repond",
    "Reponse LM Studio invalide",
    "Modele LM Studio introuvable",
    "Impossible de lister les modeles LM Studio",
    "Erreur Ollama",
    "Ollama ne repond",
    "Reponse Ollama invalide",
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


def get_llm_provider() -> str:
    provider = settings.llm_provider.lower().strip()
    if provider in {"lmstudio", "lm_studio", "lm-studio"}:
        return "lmstudio"

    if provider == "ollama":
        return "ollama"

    return provider


def get_llm_model_label() -> str:
    provider = get_llm_provider()
    if provider == "lmstudio":
        model = settings.lmstudio_model.strip() or "auto"
        return f"lmstudio:{model}"

    if provider == "ollama":
        return f"ollama:{settings.ollama_model}"

    return provider


def chat_with_local_model(
    text: str,
    language: str = "fr",
    system_prompt: str | None = None,
) -> str:
    language = normalize_language(language)
    prompt = text.strip()
    if not prompt:
        return ""

    provider = get_llm_provider()
    if provider == "lmstudio":
        return _chat_with_lmstudio(prompt, language, system_prompt)

    if provider == "ollama":
        return _chat_with_ollama(prompt, language, system_prompt)

    return "Erreur LLM : LLM_PROVIDER doit etre 'lmstudio' ou 'ollama'."


# Backward-compatible name used by existing scripts and API code.
def chat_with_gemma(
    text: str,
    language: str = "fr",
    system_prompt: str | None = None,
) -> str:
    return chat_with_local_model(text, language, system_prompt)


def is_llm_error_response(response_text: str) -> bool:
    return response_text in KNOWN_LLM_ERROR_MESSAGES or response_text.startswith(
        LLM_ERROR_PREFIXES
    )


def _chat_with_lmstudio(
    prompt: str,
    language: str,
    system_prompt: str | None,
) -> str:
    model, model_error = _resolve_lmstudio_model()
    if model_error:
        return model_error

    payload = {
        "model": model,
        "messages": [
            {"role": "system", "content": system_prompt or _default_system_prompt(language)},
            {"role": "user", "content": prompt},
        ],
        "temperature": 0.7,
        "max_tokens": settings.llm_max_tokens,
        "stream": False,
    }
    url = f"{settings.lmstudio_base_url.rstrip('/')}/chat/completions"

    try:
        response = requests.post(url, json=payload, timeout=settings.llm_timeout_seconds)
    except requests.exceptions.ConnectionError:
        return LM_STUDIO_NOT_RUNNING_MESSAGE
    except requests.exceptions.Timeout:
        return "LM Studio ne repond pas assez vite. Verifie le modele local."
    except requests.RequestException as exc:
        return f"Erreur LM Studio : {exc}"

    try:
        data = response.json()
    except ValueError:
        return f"Reponse LM Studio invalide : {response.text[:200]}"

    error = data.get("error")
    if error:
        message = error.get("message") if isinstance(error, dict) else str(error)
        normalized_error = message.lower()
        if "model" in normalized_error or "not found" in normalized_error:
            return (
                "Modele LM Studio introuvable ou non charge. "
                "Charge le modele dans LM Studio ou corrige LM_STUDIO_MODEL."
            )

        return f"Erreur LM Studio : {message}"

    if not response.ok:
        return f"Erreur LM Studio HTTP {response.status_code} : {response.text[:200]}"

    choices = data.get("choices") or []
    if not choices:
        return "Reponse LM Studio invalide : aucune reponse du modele."

    message = choices[0].get("message", {})
    content = message.get("content", "")
    return content.strip()


def _resolve_lmstudio_model() -> tuple[str, str | None]:
    configured_model = settings.lmstudio_model.strip()
    if configured_model:
        return configured_model, None

    url = f"{settings.lmstudio_base_url.rstrip('/')}/models"
    try:
        response = requests.get(url, timeout=5)
    except requests.exceptions.ConnectionError:
        return "", LM_STUDIO_NOT_RUNNING_MESSAGE
    except requests.exceptions.Timeout:
        return "", "LM Studio ne repond pas assez vite. Verifie le serveur local."
    except requests.RequestException as exc:
        return "", f"Erreur LM Studio : {exc}"

    if not response.ok:
        return "", f"Impossible de lister les modeles LM Studio HTTP {response.status_code}."

    try:
        data = response.json()
    except ValueError:
        return "", f"Reponse LM Studio invalide : {response.text[:200]}"

    models = data.get("data") or []
    if not models:
        return "", LM_STUDIO_MODEL_MISSING_MESSAGE

    model_id = models[0].get("id")
    if not model_id:
        return "", LM_STUDIO_MODEL_MISSING_MESSAGE

    return model_id, None


def _chat_with_ollama(
    prompt: str,
    language: str,
    system_prompt: str | None,
) -> str:
    payload = {
        "model": settings.ollama_model,
        "prompt": prompt,
        "system": system_prompt or _default_system_prompt(language),
        "stream": False,
    }
    url = f"{settings.ollama_base_url.rstrip('/')}/api/generate"

    try:
        response = requests.post(url, json=payload, timeout=settings.llm_timeout_seconds)
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