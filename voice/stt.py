from __future__ import annotations

import importlib.util
import json
import queue
import shutil
import subprocess
import tempfile
import time
import wave
from array import array
from pathlib import Path
from typing import Any


class SpeechToTextError(RuntimeError):
    """Raised when Rafiki cannot listen through the configured STT engine."""


def pcm16_rms(data: bytes) -> float:
    """Return the RMS level of little-endian signed 16-bit PCM audio."""
    if len(data) < 2:
        return 0.0

    samples = array("h")
    samples.frombytes(data[: len(data) - (len(data) % 2)])
    if not samples:
        return 0.0

    square_sum = sum(sample * sample for sample in samples)
    return (square_sum / len(samples)) ** 0.5


class VoskSpeechToText:
    def __init__(
        self,
        model_path: str | Path,
        sample_rate: int = 16000,
        device: int | str | None = None,
        block_duration: float = 0.2,
    ) -> None:
        self.model_path = Path(model_path)
        self.sample_rate = sample_rate
        self.device = device
        self.block_duration = block_duration
        self._model: Any | None = None

    def is_available(self) -> bool:
        return (
            importlib.util.find_spec("vosk") is not None
            and importlib.util.find_spec("sounddevice") is not None
            and self.model_path.exists()
        )

    def listen_once(
        self,
        timeout: float | None = None,
        phrase_time_limit: float = 8.0,
    ) -> str:
        if not self.model_path.exists():
            raise SpeechToTextError(f"Vosk model not found: {self.model_path}")

        try:
            import sounddevice as sd
            from vosk import KaldiRecognizer, Model
        except ImportError as exc:
            raise SpeechToTextError(
                "Vosk input needs the 'vosk' and 'sounddevice' packages"
            ) from exc

        audio_queue: queue.Queue[bytes] = queue.Queue()

        def callback(
            indata: bytes,
            frames: int,
            callback_time: Any,
            status: Any,
        ) -> None:
            if status:
                raise SpeechToTextError(str(status))
            audio_queue.put(bytes(indata))

        if self._model is None:
            self._model = Model(str(self.model_path))

        recognizer = KaldiRecognizer(self._model, self.sample_rate)
        block_size = max(1, int(self.sample_rate * self.block_duration))
        started_at = time.monotonic()
        recognized_parts: list[str] = []

        try:
            with sd.RawInputStream(
                samplerate=self.sample_rate,
                blocksize=block_size,
                device=self.device,
                dtype="int16",
                channels=1,
                callback=callback,
            ):
                while True:
                    elapsed = time.monotonic() - started_at
                    if timeout is not None and elapsed > timeout:
                        return self._joined_text_or_error(
                            recognized_parts,
                            recognizer.FinalResult(),
                            "No speech recognized before timeout",
                        )
                    if elapsed > phrase_time_limit:
                        return self._joined_text_or_error(
                            recognized_parts,
                            recognizer.FinalResult(),
                            "No speech recognized",
                        )

                    try:
                        data = audio_queue.get(timeout=0.2)
                    except queue.Empty:
                        continue

                    if recognizer.AcceptWaveform(data):
                        text = self._extract_text_or_empty(recognizer.Result())
                        if text:
                            recognized_parts.append(text)
        except SpeechToTextError:
            raise
        except Exception as exc:
            raise SpeechToTextError(f"Voice input failed: {exc}") from exc

    @staticmethod
    def _extract_text(raw_result: str) -> str:
        try:
            result = json.loads(raw_result)
        except json.JSONDecodeError as exc:
            raise SpeechToTextError(f"Invalid Vosk result: {raw_result!r}") from exc

        text = str(result.get("text", "")).strip()
        if not text:
            raise SpeechToTextError("No speech recognized")

        return text

    @classmethod
    def _joined_text_or_error(
        cls,
        recognized_parts: list[str],
        raw_final_result: str,
        error_message: str,
    ) -> str:
        final_text = cls._extract_text_or_empty(raw_final_result)
        if final_text:
            recognized_parts.append(final_text)

        text = " ".join(part for part in recognized_parts if part).strip()
        if not text:
            raise SpeechToTextError(error_message)

        return text

    @staticmethod
    def _extract_text_or_empty(raw_result: str) -> str:
        try:
            result = json.loads(raw_result)
        except json.JSONDecodeError:
            return ""

        return str(result.get("text", "")).strip()


class PulseSpeechToText:
    def __init__(
        self,
        model_path: str | Path,
        device: str = "@DEFAULT_SOURCE@",
        recorder: str = "parecord",
        ffmpeg_executable: str = "ffmpeg",
        sample_rate: int = 16000,
        raw_sample_rate: int = 48000,
        raw_channels: int = 2,
        chunk_duration: float = 0.25,
        audio_filter: str | None = None,
        speech_threshold: float = 260.0,
        end_silence: float = 0.8,
        min_speech_duration: float = 0.3,
    ) -> None:
        self.model_path = Path(model_path)
        self.device = device
        self.recorder = recorder
        self.ffmpeg_executable = ffmpeg_executable
        self.sample_rate = sample_rate
        self.raw_sample_rate = raw_sample_rate
        self.raw_channels = raw_channels
        self.chunk_duration = chunk_duration
        self.audio_filter = audio_filter
        self.speech_threshold = speech_threshold
        self.end_silence = end_silence
        self.min_speech_duration = min_speech_duration
        self._model: Any | None = None

    def is_available(self) -> bool:
        return (
            importlib.util.find_spec("vosk") is not None
            and shutil.which(self.recorder) is not None
            and (
                not self.audio_filter
                or shutil.which(self.ffmpeg_executable) is not None
            )
            and self.model_path.exists()
            and bool(self.device.strip())
        )

    def listen_once(
        self,
        timeout: float | None = None,
        phrase_time_limit: float = 8.0,
    ) -> str:
        if not self.model_path.exists():
            raise SpeechToTextError(f"Vosk model not found: {self.model_path}")

        if shutil.which(self.recorder) is None:
            raise SpeechToTextError(f"{self.recorder} is not installed")

        try:
            from vosk import KaldiRecognizer, Model
        except ImportError as exc:
            raise SpeechToTextError("Pulse input needs the 'vosk' package") from exc

        if self._model is None:
            self._model = Model(str(self.model_path))

        recognizer = KaldiRecognizer(self._model, self.sample_rate)
        recognizer.SetWords(True)
        chunk_size = max(3200, int(self.sample_rate * 2 * self.chunk_duration))
        recorder_command = [
            self.recorder,
            "--device",
            self.device,
            "--raw",
            "--format",
            "s16le",
            "--channels",
            str(self.raw_channels if self.audio_filter else 1),
            "--rate",
            str(self.raw_sample_rate if self.audio_filter else self.sample_rate),
        ]
        started_at = time.monotonic()

        recorder_process = subprocess.Popen(
            recorder_command,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        ffmpeg_process: subprocess.Popen[bytes] | None = None

        if self.audio_filter:
            if shutil.which(self.ffmpeg_executable) is None:
                recorder_process.terminate()
                raise SpeechToTextError(f"{self.ffmpeg_executable} is not installed")

            ffmpeg_command = [
                self.ffmpeg_executable,
                "-hide_banner",
                "-loglevel",
                "error",
                "-f",
                "s16le",
                "-ar",
                str(self.raw_sample_rate),
                "-ac",
                str(self.raw_channels),
                "-i",
                "pipe:0",
                "-af",
                self.audio_filter,
                "-f",
                "s16le",
                "-acodec",
                "pcm_s16le",
                "-ac",
                "1",
                "-ar",
                str(self.sample_rate),
                "pipe:1",
            ]
            ffmpeg_process = subprocess.Popen(
                ffmpeg_command,
                stdin=recorder_process.stdout,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
            if recorder_process.stdout is not None:
                recorder_process.stdout.close()

        audio_process = ffmpeg_process or recorder_process
        recognized_parts: list[str] = []
        speech_started_at: float | None = None
        last_voice_at: float | None = None

        try:
            if audio_process.stdout is None:
                raise SpeechToTextError("audio process did not expose a stream")

            while True:
                now = time.monotonic()
                elapsed = now - started_at
                if speech_started_at is None and timeout is not None and elapsed > timeout:
                    return VoskSpeechToText._joined_text_or_error(
                        recognized_parts,
                        recognizer.FinalResult(),
                        "No speech recognized before timeout",
                    )
                if (
                    speech_started_at is not None
                    and now - speech_started_at > phrase_time_limit
                ):
                    return VoskSpeechToText._joined_text_or_error(
                        recognized_parts,
                        recognizer.FinalResult(),
                        "No speech recognized",
                    )

                data = audio_process.stdout.read(chunk_size)
                if not data:
                    stderr = self._read_stderr(audio_process)
                    raise SpeechToTextError(
                        f"Pulse audio stream ended unexpectedly: {stderr}"
                    )

                level = pcm16_rms(data)
                if level >= self.speech_threshold:
                    if speech_started_at is None:
                        speech_started_at = now
                    last_voice_at = now

                if (
                    speech_started_at is not None
                    and last_voice_at is not None
                    and now - speech_started_at >= self.min_speech_duration
                    and now - last_voice_at >= self.end_silence
                ):
                    return VoskSpeechToText._joined_text_or_error(
                        recognized_parts,
                        recognizer.FinalResult(),
                        "No speech recognized",
                    )

                if recognizer.AcceptWaveform(data):
                    text = VoskSpeechToText._extract_text_or_empty(
                        recognizer.Result()
                    )
                    if text:
                        recognized_parts.append(text)
        finally:
            for process in [ffmpeg_process, recorder_process]:
                if process is None:
                    continue
                process.terminate()
                try:
                    process.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    process.kill()

    @staticmethod
    def _read_stderr(process: subprocess.Popen[bytes]) -> str:
        if process.stderr is None:
            return ""

        return process.stderr.read().decode(errors="replace").strip()


class PulseWhisperSpeechToText:
    """Record one PulseAudio utterance, then transcribe it with whisper.cpp."""

    def __init__(
        self,
        model_path: str | Path,
        executable: str = "tools/whisper.cpp/build/bin/whisper-cli",
        device: str = "@DEFAULT_SOURCE@",
        recorder: str = "parecord",
        ffmpeg_executable: str = "ffmpeg",
        language: str = "fr",
        threads: int = 4,
        sample_rate: int = 16000,
        raw_sample_rate: int = 48000,
        raw_channels: int = 2,
        chunk_duration: float = 0.2,
        audio_filter: str | None = None,
        speech_threshold: float = 260.0,
        end_silence: float = 0.8,
        min_speech_duration: float = 0.3,
    ) -> None:
        self.model_path = Path(model_path)
        self.executable = executable
        self.device = device
        self.recorder = recorder
        self.ffmpeg_executable = ffmpeg_executable
        self.language = language
        self.threads = threads
        self.sample_rate = sample_rate
        self.raw_sample_rate = raw_sample_rate
        self.raw_channels = raw_channels
        self.chunk_duration = chunk_duration
        self.audio_filter = audio_filter
        self.speech_threshold = speech_threshold
        self.end_silence = end_silence
        self.min_speech_duration = min_speech_duration

    def is_available(self) -> bool:
        return (
            self.model_path.exists()
            and Path(self.executable).exists()
            and shutil.which(self.recorder) is not None
            and shutil.which(self.ffmpeg_executable) is not None
            and bool(self.device.strip())
        )

    def listen_once(
        self,
        timeout: float | None = None,
        phrase_time_limit: float = 8.0,
    ) -> str:
        if not self.is_available():
            raise SpeechToTextError("Whisper input is not fully installed")

        pcm = self._record_utterance(timeout, phrase_time_limit)
        with tempfile.TemporaryDirectory(prefix="rafiki-stt-") as temp_dir:
            wav_path = Path(temp_dir) / "utterance.wav"
            output_base = Path(temp_dir) / "transcript"
            with wave.open(str(wav_path), "wb") as wav_file:
                wav_file.setnchannels(1)
                wav_file.setsampwidth(2)
                wav_file.setframerate(self.sample_rate)
                wav_file.writeframes(pcm)

            command = [
                self.executable,
                "--model",
                str(self.model_path),
                "--file",
                str(wav_path),
                "--language",
                self.language,
                "--threads",
                str(self.threads),
                "--no-timestamps",
                "--output-txt",
                "--output-file",
                str(output_base),
                "--suppress-nst",
                "--no-prints",
            ]
            try:
                subprocess.run(
                    command,
                    check=True,
                    stdout=subprocess.DEVNULL,
                    stderr=subprocess.DEVNULL,
                )
            except subprocess.CalledProcessError as exc:
                raise SpeechToTextError(f"Whisper transcription failed: {exc}") from exc

            transcript_path = output_base.with_suffix(".txt")
            if not transcript_path.exists():
                raise SpeechToTextError("Whisper produced no transcript")

            text = " ".join(transcript_path.read_text().strip().split())
            if not text:
                raise SpeechToTextError("No speech recognized")
            return text

    def _record_utterance(
        self,
        timeout: float | None,
        phrase_time_limit: float,
    ) -> bytes:
        recorder_command = [
            self.recorder,
            "--device",
            self.device,
            "--raw",
            "--format",
            "s16le",
            "--channels",
            str(self.raw_channels),
            "--rate",
            str(self.raw_sample_rate),
        ]
        filter_chain = self.audio_filter or "highpass=f=80,lowpass=f=7600"
        ffmpeg_command = [
            self.ffmpeg_executable,
            "-hide_banner",
            "-loglevel",
            "error",
            "-f",
            "s16le",
            "-ar",
            str(self.raw_sample_rate),
            "-ac",
            str(self.raw_channels),
            "-i",
            "pipe:0",
            "-af",
            filter_chain,
            "-f",
            "s16le",
            "-acodec",
            "pcm_s16le",
            "-ac",
            "1",
            "-ar",
            str(self.sample_rate),
            "pipe:1",
        ]
        recorder_process = subprocess.Popen(
            recorder_command,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        ffmpeg_process = subprocess.Popen(
            ffmpeg_command,
            stdin=recorder_process.stdout,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        if recorder_process.stdout is not None:
            recorder_process.stdout.close()

        chunk_size = max(3200, int(self.sample_rate * 2 * self.chunk_duration))
        started_at = time.monotonic()
        speech_started_at: float | None = None
        last_voice_at: float | None = None
        chunks: list[bytes] = []

        try:
            if ffmpeg_process.stdout is None:
                raise SpeechToTextError("ffmpeg did not expose an audio stream")

            while True:
                data = ffmpeg_process.stdout.read(chunk_size)
                now = time.monotonic()
                if not data:
                    raise SpeechToTextError("Pulse audio stream ended unexpectedly")

                level = pcm16_rms(data)
                if level >= self.speech_threshold:
                    if speech_started_at is None:
                        speech_started_at = now
                    last_voice_at = now

                if speech_started_at is not None:
                    chunks.append(data)

                if speech_started_at is None:
                    if timeout is not None and now - started_at >= timeout:
                        raise SpeechToTextError("No speech recognized before timeout")
                    continue

                if now - speech_started_at >= phrase_time_limit:
                    break
                if (
                    last_voice_at is not None
                    and now - speech_started_at >= self.min_speech_duration
                    and now - last_voice_at >= self.end_silence
                ):
                    break
        finally:
            for process in (ffmpeg_process, recorder_process):
                process.terminate()
                try:
                    process.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    process.kill()

        if not chunks:
            raise SpeechToTextError("No speech recorded")
        return b"".join(chunks)


class IPWebcamSpeechToText:
    def __init__(
        self,
        audio_url: str,
        model_path: str | Path,
        ffmpeg_executable: str = "ffmpeg",
        sample_rate: int = 16000,
        chunk_duration: float = 0.25,
    ) -> None:
        self.audio_url = audio_url
        self.model_path = Path(model_path)
        self.ffmpeg_executable = ffmpeg_executable
        self.sample_rate = sample_rate
        self.chunk_duration = chunk_duration
        self._model: Any | None = None

    def is_available(self) -> bool:
        return (
            importlib.util.find_spec("vosk") is not None
            and shutil.which(self.ffmpeg_executable) is not None
            and self.model_path.exists()
            and bool(self.audio_url.strip())
        )

    def listen_once(
        self,
        timeout: float | None = None,
        phrase_time_limit: float = 8.0,
    ) -> str:
        if not self.model_path.exists():
            raise SpeechToTextError(f"Vosk model not found: {self.model_path}")

        if shutil.which(self.ffmpeg_executable) is None:
            raise SpeechToTextError(f"{self.ffmpeg_executable} is not installed")

        try:
            from vosk import KaldiRecognizer, Model
        except ImportError as exc:
            raise SpeechToTextError("IP Webcam input needs the 'vosk' package") from exc

        if self._model is None:
            self._model = Model(str(self.model_path))

        recognizer = KaldiRecognizer(self._model, self.sample_rate)
        chunk_size = max(3200, int(self.sample_rate * 2 * self.chunk_duration))
        command = [
            self.ffmpeg_executable,
            "-hide_banner",
            "-loglevel",
            "error",
            "-i",
            self.audio_url,
            "-f",
            "s16le",
            "-acodec",
            "pcm_s16le",
            "-ac",
            "1",
            "-ar",
            str(self.sample_rate),
            "pipe:1",
        ]
        started_at = time.monotonic()
        recognized_parts: list[str] = []

        process = subprocess.Popen(
            command,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )

        try:
            if process.stdout is None:
                raise SpeechToTextError("ffmpeg did not expose an audio stream")

            while True:
                elapsed = time.monotonic() - started_at
                if timeout is not None and elapsed > timeout:
                    return VoskSpeechToText._joined_text_or_error(
                        recognized_parts,
                        recognizer.FinalResult(),
                        "No speech recognized before timeout",
                    )
                if elapsed > phrase_time_limit:
                    return VoskSpeechToText._joined_text_or_error(
                        recognized_parts,
                        recognizer.FinalResult(),
                        "No speech recognized",
                    )

                data = process.stdout.read(chunk_size)
                if not data:
                    stderr = self._read_stderr(process)
                    raise SpeechToTextError(
                        f"IP Webcam audio stream ended unexpectedly: {stderr}"
                    )

                if recognizer.AcceptWaveform(data):
                    text = VoskSpeechToText._extract_text_or_empty(
                        recognizer.Result()
                    )
                    if text:
                        recognized_parts.append(text)
        finally:
            process.terminate()
            try:
                process.wait(timeout=2)
            except subprocess.TimeoutExpired:
                process.kill()

    @staticmethod
    def _final_text_or_error(raw_result: str) -> str:
        return VoskSpeechToText._extract_text(raw_result)

    @staticmethod
    def _read_stderr(process: subprocess.Popen[bytes]) -> str:
        if process.stderr is None:
            return ""

        return process.stderr.read().decode(errors="replace").strip()
