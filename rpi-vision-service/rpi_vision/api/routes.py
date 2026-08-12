"""
API Routes for Raspberry Pi Vision Service.
"""
from fastapi import APIRouter, Depends, HTTPException, Header, Query, Response
from fastapi.responses import StreamingResponse
from pydantic import BaseModel, Field
from typing import Optional, Literal
import logging

from ..config import settings
from ..camera.base import BaseCamera, FrameData

logger = logging.getLogger("rpi_vision.api")
router = APIRouter()

_camera_instance: Optional[BaseCamera] = None


def set_global_camera(cam: BaseCamera):
    global _camera_instance
    _camera_instance = cam


def get_camera() -> BaseCamera:
    if _camera_instance is None or not _camera_instance.is_active:
        raise HTTPException(status_code=503, detail="Camera service is not active or available.")
    return _camera_instance


def verify_api_key(x_api_key: Optional[str] = Header(None, alias="X-API-Key")):
    """Security check if API key is enabled in settings."""
    if settings.API_KEY and x_api_key != settings.API_KEY:
        raise HTTPException(status_code=401, detail="Invalid or missing X-API-Key header.")


class ConfigUpdateModel(BaseModel):
    width: Optional[int] = Field(None, ge=160, le=4096)
    height: Optional[int] = Field(None, ge=120, le=2160)
    quality: Optional[int] = Field(None, ge=1, le=100)
    flip_horizontal: Optional[bool] = None
    flip_vertical: Optional[bool] = None
    rotation: Optional[int] = Field(None, description="0, 90, 180, or 270 degrees")


@router.get("/health", dependencies=[Depends(verify_api_key)])
def get_health(cam: BaseCamera = Depends(get_camera)):
    """Health check endpoint returning hardware status and camera configuration."""
    return {
        "status": "online",
        "device_name": settings.DEVICE_NAME,
        "camera_type": cam.get_camera_type(),
        "is_active": cam.is_active,
        "settings": {
            "default_width": settings.DEFAULT_WIDTH,
            "default_height": settings.DEFAULT_HEIGHT,
            "default_quality": settings.DEFAULT_QUALITY,
            "format": settings.IMAGE_FORMAT,
            "flip_h": settings.FLIP_HORIZONTAL,
            "flip_v": settings.FLIP_VERTICAL,
            "rotation": settings.ROTATION,
        }
    }


@router.get("/capture", dependencies=[Depends(verify_api_key)])
def capture_image_binary(
    width: Optional[int] = Query(None, ge=160, le=4096),
    height: Optional[int] = Query(None, ge=120, le=2160),
    quality: Optional[int] = Query(None, ge=1, le=100),
    img_format: Optional[Literal["jpeg", "png"]] = Query(None, alias="format"),
    flip_h: Optional[bool] = Query(None),
    flip_v: Optional[bool] = Query(None),
    rotate: Optional[int] = Query(None),
    cam: BaseCamera = Depends(get_camera)
):
    """
    Captures a frame and returns raw binary image (image/jpeg or image/png).
    """
    w = width or settings.DEFAULT_WIDTH
    h = height or settings.DEFAULT_HEIGHT
    q = quality or settings.DEFAULT_QUALITY
    fmt = img_format or settings.IMAGE_FORMAT
    fh = flip_h if flip_h is not None else settings.FLIP_HORIZONTAL
    fv = flip_v if flip_v is not None else settings.FLIP_VERTICAL
    rot = rotate if rotate is not None else settings.ROTATION

    frame_data = cam.capture_frame_data(
        target_width=w,
        target_height=h,
        quality=q,
        img_format=fmt,
        flip_h=fh,
        flip_v=fv,
        rotation=rot
    )

    return Response(
        content=frame_data.raw_bytes,
        media_type=frame_data.mime_type,
        headers={
            "X-Timestamp": frame_data.timestamp,
            "X-Width": str(frame_data.width),
            "X-Height": str(frame_data.height),
            "X-Camera-Type": frame_data.camera_type,
            "X-Device-Name": frame_data.device_name,
        }
    )


@router.get("/capture/json", dependencies=[Depends(verify_api_key)])
def capture_image_json(
    width: Optional[int] = Query(None, ge=160, le=4096),
    height: Optional[int] = Query(None, ge=120, le=2160),
    quality: Optional[int] = Query(None, ge=1, le=100),
    img_format: Optional[Literal["jpeg", "png"]] = Query(None, alias="format"),
    flip_h: Optional[bool] = Query(None),
    flip_v: Optional[bool] = Query(None),
    rotate: Optional[int] = Query(None),
    cam: BaseCamera = Depends(get_camera)
):
    """
    Captures a frame and returns JSON formatted with Base64 payload and Data URI.
    Tailor-made for direct injection into Vision LLM prompts (OpenAI, Claude, Ollama, Gemini).
    """
    w = width or settings.DEFAULT_WIDTH
    h = height or settings.DEFAULT_HEIGHT
    q = quality or settings.DEFAULT_QUALITY
    fmt = img_format or settings.IMAGE_FORMAT
    fh = flip_h if flip_h is not None else settings.FLIP_HORIZONTAL
    fv = flip_v if flip_v is not None else settings.FLIP_VERTICAL
    rot = rotate if rotate is not None else settings.ROTATION

    frame_data = cam.capture_frame_data(
        target_width=w,
        target_height=h,
        quality=q,
        img_format=fmt,
        flip_h=fh,
        flip_v=fv,
        rotation=rot
    )

    return frame_data.to_llm_payload()


@router.get("/stream", dependencies=[Depends(verify_api_key)])
def mjpeg_video_stream(
    fps: int = Query(15, ge=1, le=60),
    quality: int = Query(70, ge=1, le=100),
    flip_h: Optional[bool] = Query(None),
    flip_v: Optional[bool] = Query(None),
    rotate: Optional[int] = Query(None),
    cam: BaseCamera = Depends(get_camera)
):
    """
    Returns an MJPEG multipart video stream for real-time preview in web browsers.
    """
    fh = flip_h if flip_h is not None else settings.FLIP_HORIZONTAL
    fv = flip_v if flip_v is not None else settings.FLIP_VERTICAL
    rot = rotate if rotate is not None else settings.ROTATION

    generator = cam.generate_mjpeg_stream(
        fps=fps,
        quality=quality,
        flip_h=fh,
        flip_v=fv,
        rotation=rot
    )

    return StreamingResponse(
        generator,
        media_type="multipart/x-mixed-replace; boundary=frame"
    )


@router.post("/config", dependencies=[Depends(verify_api_key)])
def update_configuration(config: ConfigUpdateModel):
    """Dynamically updates default settings without restarting the service."""
    if config.width is not None:
        settings.DEFAULT_WIDTH = config.width
    if config.height is not None:
        settings.DEFAULT_HEIGHT = config.height
    if config.quality is not None:
        settings.DEFAULT_QUALITY = config.quality
    if config.flip_horizontal is not None:
        settings.FLIP_HORIZONTAL = config.flip_horizontal
    if config.flip_vertical is not None:
        settings.FLIP_VERTICAL = config.flip_vertical
    if config.rotation is not None:
        settings.ROTATION = config.rotation

    return {
        "status": "updated",
        "current_settings": {
            "width": settings.DEFAULT_WIDTH,
            "height": settings.DEFAULT_HEIGHT,
            "quality": settings.DEFAULT_QUALITY,
            "flip_h": settings.FLIP_HORIZONTAL,
            "flip_v": settings.FLIP_VERTICAL,
            "rotation": settings.ROTATION,
        }
    }
