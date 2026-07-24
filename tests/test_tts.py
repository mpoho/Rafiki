import subprocess

import pytest

from voice.tts import EspeakTTS, TextToSpeechError


def test_tts_skips_empty_text(monkeypatch: pytest.MonkeyPatch) -> None:
    calls = []
    monkeypatch.setattr(subprocess, "run", lambda command, check: calls.append(command))

    EspeakTTS().speak("   ")

    assert calls == []


def test_tts_runs_espeak_command(monkeypatch: pytest.MonkeyPatch) -> None:
    calls = []
    monkeypatch.setattr("shutil.which", lambda executable: f"/usr/bin/{executable}")
    monkeypatch.setattr(subprocess, "run", lambda command, check: calls.append(command))

    EspeakTTS(voice="fr-fr", speed=140, pitch=50).speak("Bonjour")

    assert calls == [
        ["espeak-ng", "-v", "fr-fr", "-s", "140", "-p", "50", "Bonjour"]
    ]


def test_tts_reports_missing_engine(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr("shutil.which", lambda executable: None)

    with pytest.raises(TextToSpeechError):
        EspeakTTS().speak("Bonjour")
