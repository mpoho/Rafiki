from __future__ import annotations

from pydantic import BaseModel, Field


class SpeakRequest(BaseModel):
    text: str = Field(..., min_length=1)
    language: str = "fr"


class SpeakResponse(BaseModel):
    ok: bool
    message: str
    audio_path: str | None = None


class ListenRequest(BaseModel):
    language: str = "fr"
    timeout_seconds: int | None = None


class ListenResponse(BaseModel):
    ok: bool
    text: str
    language: str
    message: str


class ChatRequest(BaseModel):
    text: str = Field(..., min_length=1)
    language: str = "fr"
    system_prompt: str | None = None


class ChatResponse(BaseModel):
    ok: bool
    response: str
    model: str


class VoiceChatRequest(BaseModel):
    language: str = "fr"
    timeout_seconds: int | None = None


class VoiceChatResponse(BaseModel):
    ok: bool
    heard_text: str
    response_text: str
    language: str
    message: str