import pytest

from orchestrator.services.llm_client import (
    LLMClientError,
    RafikiDecision,
    parse_rafiki_decision,
)


def test_parse_rafiki_decision_from_dict() -> None:
    decision = parse_rafiki_decision(
        {
            "speech": "Bonjour, on apprend ensemble ?",
            "emotion": "happy",
            "movement": "swing",
            "screen_mode": "learning",
            "screen_content": "Quiz animaux",
        }
    )

    assert isinstance(decision, RafikiDecision)
    assert decision.emotion == "happy"
    assert decision.screen_mode == "learning"


def test_parse_rafiki_decision_from_markdown_json() -> None:
    decision = parse_rafiki_decision(
        """
        ```json
        {
          "speech": "Bonne question !",
          "emotion": "thinking",
          "movement": "none",
          "screen_mode": "text",
          "screen_content": "Je reflechis..."
        }
        ```
        """
    )

    assert decision.speech == "Bonne question !"
    assert decision.movement == "none"


def test_parse_rafiki_decision_extracts_json_from_extra_text() -> None:
    decision = parse_rafiki_decision(
        """
        Voici ma reponse:
        {
          "speech": "Bravo !",
          "emotion": "happy",
          "movement": "dance",
          "screen_mode": "face",
          "screen_content": ":)"
        }
        A bientot.
        """
    )

    assert decision.speech == "Bravo !"
    assert decision.movement == "dance"


def test_parse_rafiki_decision_rejects_invalid_json() -> None:
    with pytest.raises(LLMClientError):
        parse_rafiki_decision("Rafiki repond sans JSON.")


def test_parse_rafiki_decision_rejects_unknown_action() -> None:
    with pytest.raises(LLMClientError):
        parse_rafiki_decision(
            """
            {
              "speech": "Je decolle !",
              "emotion": "happy",
              "movement": "fly",
              "screen_mode": "face",
              "screen_content": ":)"
            }
            """
        )
