from __future__ import annotations

import json
from typing import Literal

import requests
from pydantic import BaseModel, ConfigDict, ValidationError


Emotion = Literal[
    "neutral",
    "happy",
    "sad",
    "surprised",
    "thinking",
]

Movement = Literal[
    "none",
    "swing",
    "dance",
    "walk_forward",
    "walk_backward",
    "turn_left",
    "turn_right",
    "stop",
]

ScreenMode = Literal[
    "face",
    "text",
    "learning",
    "quiz",
]


class RafikiDecision(BaseModel):
    model_config = ConfigDict(extra="forbid")

    speech: str
    emotion: Emotion
    movement: Movement
    screen_mode: ScreenMode
    screen_content: str


RAFIKI_SCHEMA = {
    "type": "object",
    "properties": {
        "speech": {
            "type": "string",
            "maxLength": 180,
        },
        "emotion": {
            "type": "string",
            "enum": [
                "neutral",
                "happy",
                "sad",
                "surprised",
                "thinking",
            ],
        },
        "movement": {
            "type": "string",
            "enum": [
                "none",
                "swing",
                "dance",
                "walk_forward",
                "walk_backward",
                "turn_left",
                "turn_right",
                "stop",
            ],
        },
        "screen_mode": {
            "type": "string",
            "enum": [
                "face",
                "text",
                "learning",
                "quiz",
            ],
        },
        "screen_content": {
            "type": "string",
            "maxLength": 120,
        },
    },
    "required": [
        "speech",
        "emotion",
        "movement",
        "screen_mode",
        "screen_content",
    ],
    "additionalProperties": False,
}


class LLMClientError(RuntimeError):
    """Erreur lors de la communication avec llama-server."""

def parse_rafiki_decision(raw_content: object) -> RafikiDecision:
    if isinstance(raw_content, dict):
        return RafikiDecision.model_validate(raw_content)

    if not isinstance(raw_content, str):
        raise LLMClientError(
            f"Type de réponse inattendu : {type(raw_content).__name__}"
        )

    content = raw_content.strip()

    # Retirer d'éventuelles balises Markdown.
    if content.startswith("```json"):
        content = content[7:]
    elif content.startswith("```"):
        content = content[3:]

    if content.endswith("```"):
        content = content[:-3]

    content = content.strip()

    # Premier essai : la réponse entière est du JSON.
    try:
        return RafikiDecision.model_validate_json(content)
    except ValidationError:
        pass

    # Deuxième essai : extraire l'objet JSON si le modèle
    # a ajouté du texte avant ou après.
    start = content.find("{")
    end = content.rfind("}")

    if start != -1 and end > start:
        json_part = content[start : end + 1]

        try:
            return RafikiDecision.model_validate_json(json_part)
        except ValidationError as exc:
            raise LLMClientError(
                "Objet JSON présent mais invalide. "
                f"Réponse brute : {content[:400]!r}"
            ) from exc

    raise LLMClientError(
        "Le modèle n'a retourné aucun JSON. "
        f"Réponse brute : {content[:400]!r}"
    )

class RafikiLLMClient:
    def __init__(
        self,
        base_url: str = "http://127.0.0.1:8080",
        model: str = "rafiki-local",
        timeout: float = 120.0,
    ) -> None:
        self.base_url = base_url.rstrip("/")
        self.model = model
        self.timeout = timeout

    def is_ready(self) -> bool:
        try:
            response = requests.get(
                f"{self.base_url}/health",
                timeout=3,
            )
            return response.status_code == 200
        except requests.RequestException:
            return False

    def generate(
        self,
        user_message: str,
        language: str = "fr",
        history: list[dict[str, str]] | None = None,
    ) -> RafikiDecision:
        system_prompt = (
            "Tu es Rafiki, un robot compagnon éducatif destiné aux enfants "
            "de 5 à 10 ans. Tes réponses doivent être courtes, simples, "
            "bienveillantes et adaptées à l'âge de l'enfant. "
            f"Réponds principalement dans la langue suivante : {language}. "
            "Ne demande jamais au robot d'exécuter une action dangereuse. "
            "Choisis uniquement les émotions, mouvements et modes d'écran "
            "autorisés par le schéma JSON. "
            "Tu dois répondre uniquement avec un objet JSON valide, compact, "
            "sans Markdown, sans explication et sans texte autour. "
            "Le champ speech doit contenir au maximum deux phrases courtes. "
            "Ne répète jamais la même idée, le même groupe de mots ou la "
            "même phrase. Si tu racontes une blague, elle doit tenir en une "
            "seule phrase simple."
        )

        messages: list[dict[str, str]] = [
            {
                "role": "system",
                "content": system_prompt,
            }
        ]

        if history:
            messages.extend(history[-6:])

        messages.append(
            {
                "role": "user",
                "content": user_message,
            }
        )

        payload = {
            "model": self.model,
            "messages": messages,
            "temperature": 0.0,
            "max_tokens": 220,
            "frequency_penalty": 0.4,
            "repeat_penalty": 1.18,
            "stream": False,
            "chat_template_kwargs": {
                "enable_thinking": False,
            },
            "response_format": {
                "type": "json_object",
                "schema": RAFIKI_SCHEMA,
            },
        }

        try:
            response = requests.post(
                f"{self.base_url}/v1/chat/completions",
                json=payload,
                timeout=self.timeout,
            )
            response.raise_for_status()

            api_result = response.json()
            raw_content = api_result["choices"][0]["message"]["content"]

            return parse_rafiki_decision(raw_content)

        except (
            requests.RequestException,
            KeyError,
            IndexError,
            json.JSONDecodeError,
            ValidationError,
        ) as exc:
            raise LLMClientError(
                f"Réponse invalide de llama-server : {exc}"
            ) from exc
