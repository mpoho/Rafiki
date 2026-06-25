from pydantic import BaseModel, Field


class SpeakRequest(BaseModel):
    text: str = Field(..., min_length=1, description="Texte que Arthy doit prononcer")


class SpeakResponse(BaseModel):
    ok: bool
    message: str
