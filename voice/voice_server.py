from __future__ import annotations

from fastapi import FastAPI, HTTPException

from .config import settings
from .llm_client import chat_with_local_model, get_llm_model_label, get_llm_provider
from .schemas import (
    ChatRequest,
    ChatResponse,
    ListenRequest,
    ListenResponse,
    SpeakRequest,
    SpeakResponse,
    VoiceChatRequest,
    VoiceChatResponse,
)
from .stt import listen
from .tts import speak
from .voice_pipeline import is_llm_error, voice_chat


app = FastAPI(
    title="Rafiki Voice Module",
    description="Serveur local pour faire parler, ecouter et connecter Rafiki a LM Studio ou Ollama",
    version="0.1.0",
)


@app.get("/health")
def health_check():
    return {
        "ok": True,
        "service": "Rafiki Voice Module",
        "status": "running",
        "default_language": settings.default_language,
        "llm_provider": get_llm_provider(),
        "llm_model": get_llm_model_label(),
    }


@app.post("/speak", response_model=SpeakResponse)
def speak_endpoint(payload: SpeakRequest):
    try:
        audio_path = speak(payload.text, payload.language)
        return SpeakResponse(
            ok=True,
            message="Rafiki a parle avec succes.",
            audio_path=audio_path or None,
        )
    except Exception as error:
        raise HTTPException(
            status_code=500,
            detail=f"Erreur pendant la synthese vocale : {error}",
        )


@app.post("/listen", response_model=ListenResponse)
def listen_endpoint(payload: ListenRequest):
    try:
        text = listen(payload.language, payload.timeout_seconds)
        return ListenResponse(
            ok=bool(text),
            text=text,
            language=payload.language,
            message="Texte reconnu." if text else "Aucun texte reconnu.",
        )
    except Exception as error:
        raise HTTPException(
            status_code=500,
            detail=f"Erreur pendant la reconnaissance vocale : {error}",
        )


@app.post("/chat", response_model=ChatResponse)
def chat_endpoint(payload: ChatRequest):
    try:
        response_text = chat_with_local_model(
            payload.text,
            payload.language,
            payload.system_prompt,
        )
        return ChatResponse(
            ok=bool(response_text) and not is_llm_error(response_text),
            response=response_text,
            model=get_llm_model_label(),
        )
    except Exception as error:
        raise HTTPException(
            status_code=500,
            detail=f"Erreur pendant l'appel au modele local : {error}",
        )


@app.post("/voice-chat", response_model=VoiceChatResponse)
def voice_chat_endpoint(payload: VoiceChatRequest):
    try:
        result = voice_chat(payload.language, payload.timeout_seconds)
        response_text = result["response_text"]
        heard_text = result["heard_text"]

        if not heard_text:
            message = "Aucun texte reconnu."
        elif is_llm_error(response_text):
            message = response_text
        else:
            message = "Interaction vocale terminee."

        return VoiceChatResponse(
            ok=bool(heard_text) and not is_llm_error(response_text),
            heard_text=heard_text,
            response_text=response_text,
            language=result["language"],
            message=message,
        )
    except Exception as error:
        raise HTTPException(
            status_code=500,
            detail=f"Erreur pendant le voice chat : {error}",
        )