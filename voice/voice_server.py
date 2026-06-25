from fastapi import FastAPI, HTTPException

from voice.schemas import SpeakRequest, SpeakResponse
from voice.tts import speak

app = FastAPI(
    title="Arthy Voice Module",
    description="Serveur local pour faire parler et écouter Arthy",
    version="0.1.0",
)


@app.get("/health")
def health_check():
    return {
        "ok": True,
        "service": "Arthy Voice Module",
        "status": "running",
    }


@app.post("/speak", response_model=SpeakResponse)
def speak_endpoint(payload: SpeakRequest):
    try:
        speak(payload.text)

        return SpeakResponse(
            ok=True,
            message="Arthy a parlé avec succès.",
        )

    except Exception as error:
        raise HTTPException(
            status_code=500,
            detail=f"Erreur pendant la synthèse vocale : {error}",
        )
