from __future__ import annotations

import shutil
import subprocess


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

    def speak(self, text: str) -> None:
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

        try:
            subprocess.run(command, check=True)
        except subprocess.CalledProcessError as exc:
            raise TextToSpeechError(f"TTS command failed: {exc}") from exc
