from __future__ import annotations

import shutil
import subprocess
import tempfile
from pathlib import Path
from typing import Callable


class TextToSpeechError(RuntimeError):
    """Raised when Rafiki cannot speak through the configured TTS engine."""


class EspeakTTS:
    def __init__(
        self,
        executable: str = "espeak-ng",
        voice: str = "fr-fr",
        speed: int = 155,
        pitch: int = 45,
        enabled: bool = True,
    ) -> None:
        self.executable = executable
        self.voice = voice
        self.speed = speed
        self.pitch = pitch
        self.enabled = enabled

    def is_available(self) -> bool:
        return shutil.which(self.executable) is not None

    def speak(
        self,
        text: str,
        on_playback_start: Callable[[], None] | None = None,
        on_playback_end: Callable[[], None] | None = None,
    ) -> None:
        content = text.strip()
        if not self.enabled or not content:
            return

        if not self.is_available():
            raise TextToSpeechError(f"{self.executable} is not installed")

        command = [
            self.executable,
            "-v",
            self.voice,
            "-s",
            str(self.speed),
            "-p",
            str(self.pitch),
            content,
        ]

        playback_started = False
        try:
            if on_playback_start:
                on_playback_start()
            playback_started = True
            subprocess.run(command, check=True)
        except subprocess.CalledProcessError as exc:
            raise TextToSpeechError(f"TTS command failed: {exc}") from exc
        finally:
            if playback_started and on_playback_end:
                on_playback_end()


class PiperTTS:
    def __init__(
        self,
        model_path: str | Path,
        executable: str = "piper",
        player: str = "aplay",
        audio_device: str | None = None,
        config_path: str | Path | None = None,
        length_scale: float = 1.05,
        noise_scale: float = 0.6,
        noise_width: float = 0.7,
        sentence_silence: float = 0.18,
        ffmpeg_executable: str = "ffmpeg",
        post_filter: str | None = None,
        enabled: bool = True,
    ) -> None:
        self.executable = executable
        self.model_path = Path(model_path)
        self.config_path = Path(config_path) if config_path else None
        self.player = player
        self.audio_device = audio_device
        self.length_scale = length_scale
        self.noise_scale = noise_scale
        self.noise_width = noise_width
        self.sentence_silence = sentence_silence
        self.ffmpeg_executable = ffmpeg_executable
        self.post_filter = post_filter
        self.enabled = enabled

    def is_available(self) -> bool:
        return (
            shutil.which(self.executable) is not None
            and shutil.which(self.player) is not None
            and (not self.post_filter or shutil.which(self.ffmpeg_executable) is not None)
            and self.model_path.exists()
        )

    def speak(
        self,
        text: str,
        on_playback_start: Callable[[], None] | None = None,
        on_playback_end: Callable[[], None] | None = None,
    ) -> None:
        content = prepare_spoken_text(text)
        if not self.enabled or not content:
            return

        if shutil.which(self.executable) is None:
            raise TextToSpeechError(f"{self.executable} is not installed")

        if shutil.which(self.player) is None:
            raise TextToSpeechError(f"{self.player} is not installed")

        if not self.model_path.exists():
            raise TextToSpeechError(f"Piper model not found: {self.model_path}")

        with tempfile.TemporaryDirectory(prefix="rafiki-tts-") as temp_dir:
            wav_path = Path(temp_dir) / "speech.wav"
            command = [
                self.executable,
                "--model",
                str(self.model_path),
                "--output_file",
                str(wav_path),
                "--length_scale",
                str(self.length_scale),
                "--noise_scale",
                str(self.noise_scale),
                "--noise_w",
                str(self.noise_width),
                "--sentence_silence",
                str(self.sentence_silence),
            ]

            if self.config_path:
                command.extend(["--config", str(self.config_path)])

            try:
                subprocess.run(
                    command,
                    input=content,
                    text=True,
                    check=True,
                    stdout=subprocess.DEVNULL,
                )
                playable_file = self._postprocess(wav_path, Path(temp_dir))
                play_command = [self.player]
                if self.audio_device:
                    play_command.extend(self._device_args())
                play_command.append(str(playable_file))

                playback_started = False
                try:
                    if on_playback_start:
                        on_playback_start()
                    playback_started = True
                    subprocess.run(play_command, check=True)
                finally:
                    if playback_started and on_playback_end:
                        on_playback_end()
            except subprocess.CalledProcessError as exc:
                raise TextToSpeechError(f"TTS command failed: {exc}") from exc

    def _device_args(self) -> list[str]:
        player_name = Path(self.player).name
        if player_name in {"paplay", "parec", "parecord"}:
            return ["--device", self.audio_device or ""]

        return ["-D", self.audio_device or ""]

    def _postprocess(self, wav_path: Path, temp_dir: Path) -> Path:
        if not self.post_filter:
            return wav_path

        if shutil.which(self.ffmpeg_executable) is None:
            raise TextToSpeechError(f"{self.ffmpeg_executable} is not installed")

        filtered = temp_dir / "speech-filtered.wav"
        command = [
            self.ffmpeg_executable,
            "-hide_banner",
            "-loglevel",
            "error",
            "-y",
            "-i",
            str(wav_path),
            "-af",
            self.post_filter,
            str(filtered),
        ]
        subprocess.run(command, check=True)
        return filtered


def prepare_spoken_text(text: str, max_words: int | None = None) -> str:
    normalized = " ".join(text.strip().split())
    replacements = {
        "LLM": "cerveau",
        "JSON": "données",
        "STT": "écoute",
        "TTS": "voix",
        "Rafiki !": "Rafiki.",
    }
    for source, target in replacements.items():
        normalized = normalized.replace(source, target)

    words = normalized.split()
    if max_words is not None and len(words) > max_words:
        normalized = " ".join(words[:max_words]).rstrip(" ,;:")
        if not normalized.endswith((".", "!", "?")):
            normalized += "."

    if normalized and normalized[-1] not in ".!?":
        normalized += "."

    return normalized
