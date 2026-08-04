import subprocess
from pathlib import Path

import pytest

from voice.tts import EspeakTTS, PiperTTS, TextToSpeechError, prepare_spoken_text


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


def test_piper_tts_runs_piper_then_player(
    monkeypatch: pytest.MonkeyPatch,
    tmp_path: Path,
) -> None:
    model_path = tmp_path / "voice.onnx"
    model_path.write_text("model")
    calls = []

    monkeypatch.setattr("shutil.which", lambda executable: f"/usr/bin/{executable}")

    def fake_run(command, **kwargs):
        calls.append((command, kwargs))

    monkeypatch.setattr(subprocess, "run", fake_run)

    PiperTTS(
        model_path=model_path,
        player="aplay",
        audio_device="plughw:1,0",
    ).speak("Bonjour")

    assert calls[0][0][:5] == [
        "piper",
        "--model",
        str(model_path),
        "--output_file",
        calls[0][0][4],
    ]
    assert "--length_scale" in calls[0][0]
    assert calls[0][1]["input"] == "Bonjour."
    assert calls[1][0] == ["aplay", "-D", "plughw:1,0", calls[0][0][4]]


def test_piper_tts_uses_paplay_device_arg(
    monkeypatch: pytest.MonkeyPatch,
    tmp_path: Path,
) -> None:
    model_path = tmp_path / "voice.onnx"
    model_path.write_text("model")
    calls = []

    monkeypatch.setattr("shutil.which", lambda executable: f"/usr/bin/{executable}")

    def fake_run(command, **kwargs):
        calls.append((command, kwargs))

    monkeypatch.setattr(subprocess, "run", fake_run)

    PiperTTS(
        model_path=model_path,
        player="paplay",
        audio_device="tunnel-sink.tcp:10.20.20.149",
    ).speak("Bonjour")

    assert calls[1][0] == [
        "paplay",
        "--device",
        "tunnel-sink.tcp:10.20.20.149",
        calls[0][0][4],
    ]


def test_piper_wraps_only_audio_playback_with_callbacks(
    monkeypatch: pytest.MonkeyPatch,
    tmp_path: Path,
) -> None:
    model_path = tmp_path / "voice.onnx"
    model_path.write_text("model")
    events = []

    monkeypatch.setattr("shutil.which", lambda executable: f"/usr/bin/{executable}")

    def fake_run(command, **kwargs):
        events.append(Path(command[0]).name)

    monkeypatch.setattr(subprocess, "run", fake_run)

    PiperTTS(model_path=model_path).speak(
        "Bonjour",
        on_playback_start=lambda: events.append("body-start"),
        on_playback_end=lambda: events.append("body-stop"),
    )

    assert events == ["piper", "body-start", "aplay", "body-stop"]


def test_piper_tts_reports_missing_model(tmp_path: Path) -> None:
    with pytest.raises(TextToSpeechError):
        PiperTTS(model_path=tmp_path / "missing.onnx").speak("Bonjour")


def test_prepare_spoken_text_shortens_and_punctuates() -> None:
    text = prepare_spoken_text(
        "Voici une phrase beaucoup trop longue pour une voix de robot qui doit "
        "rester claire pendant une demonstration devant les enfants",
        max_words=8,
    )

    assert text == "Voici une phrase beaucoup trop longue pour une."


def test_prepare_spoken_text_does_not_truncate_by_default() -> None:
    text = "Cette phrase doit etre prononcee entierement meme si elle est assez longue"

    assert prepare_spoken_text(text) == f"{text}."
