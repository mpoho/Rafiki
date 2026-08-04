import pytest

from voice.stt import (
    IPWebcamSpeechToText,
    PulseSpeechToText,
    PulseWhisperSpeechToText,
    SpeechToTextError,
    VoskSpeechToText,
    pcm16_rms,
)

from array import array


def test_vosk_extracts_text() -> None:
    assert VoskSpeechToText._extract_text('{"text": "bonjour rafiki"}') == (
        "bonjour rafiki"
    )


def test_vosk_rejects_empty_text() -> None:
    with pytest.raises(SpeechToTextError):
        VoskSpeechToText._extract_text('{"text": ""}')


def test_vosk_rejects_invalid_json() -> None:
    with pytest.raises(SpeechToTextError):
        VoskSpeechToText._extract_text("pas du json")


def test_ip_webcam_requires_audio_url(monkeypatch, tmp_path) -> None:
    monkeypatch.setattr("shutil.which", lambda executable: f"/usr/bin/{executable}")
    monkeypatch.setattr("importlib.util.find_spec", lambda package: object())
    model_path = tmp_path / "vosk"
    model_path.mkdir()

    assert not IPWebcamSpeechToText("", model_path).is_available()


def test_ip_webcam_is_available(monkeypatch, tmp_path) -> None:
    monkeypatch.setattr("shutil.which", lambda executable: f"/usr/bin/{executable}")
    monkeypatch.setattr("importlib.util.find_spec", lambda package: object())
    model_path = tmp_path / "vosk"
    model_path.mkdir()

    assert IPWebcamSpeechToText("http://phone:8080/audio.wav", model_path).is_available()


def test_pulse_is_available(monkeypatch, tmp_path) -> None:
    monkeypatch.setattr("shutil.which", lambda executable: f"/usr/bin/{executable}")
    monkeypatch.setattr("importlib.util.find_spec", lambda package: object())
    model_path = tmp_path / "vosk"
    model_path.mkdir()

    assert PulseSpeechToText(
        model_path,
        device="tunnel-source.tcp:10.20.20.149",
    ).is_available()


def test_pulse_requires_device(monkeypatch, tmp_path) -> None:
    monkeypatch.setattr("shutil.which", lambda executable: f"/usr/bin/{executable}")
    monkeypatch.setattr("importlib.util.find_spec", lambda package: object())
    model_path = tmp_path / "vosk"
    model_path.mkdir()

    assert not PulseSpeechToText(model_path, device=" ").is_available()


def test_pcm16_rms_detects_silence_and_signal() -> None:
    assert pcm16_rms(b"\x00\x00" * 20) == 0.0
    signal = array("h", [1000, -1000] * 20).tobytes()
    assert pcm16_rms(signal) == 1000.0


def test_pulse_whisper_requires_model_and_binary(monkeypatch, tmp_path) -> None:
    monkeypatch.setattr("shutil.which", lambda executable: f"/usr/bin/{executable}")

    assert not PulseWhisperSpeechToText(
        model_path=tmp_path / "missing-model.bin",
        executable=str(tmp_path / "missing-whisper"),
    ).is_available()
