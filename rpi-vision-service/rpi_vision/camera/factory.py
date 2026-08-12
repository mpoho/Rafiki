"""
Factory for instantiating the appropriate camera implementation.
"""
import logging
from typing import Union
from .base import BaseCamera
from .opencv_cam import OpenCVCamera
from .picam2_cam import PiCamera2Adapter, PICAMERA2_AVAILABLE
from .mock_cam import MockCamera
from ..config import settings

logger = logging.getLogger("rpi_vision.factory")


def create_camera(
    camera_type: str = "auto",
    camera_index: Union[int, str] = 0,
    device_name: str = "RaspberryPi-Cam",
    width: int = 1280,
    height: int = 720
) -> BaseCamera:
    """
    Creates and returns an active BaseCamera instance according to requested configuration and available hardware.
    """
    requested_type = camera_type.lower()

    if requested_type == "mock":
        logger.info("Initializing Mock synthetic camera...")
        cam = MockCamera(device_name=device_name, default_width=width, default_height=height)
        cam.start()
        return cam

    if requested_type == "picamera2":
        if not PICAMERA2_AVAILABLE:
            raise RuntimeError("Requested 'picamera2' but python3-picamera2 is not installed.")
        logger.info("Initializing Raspberry Pi CSI Camera via Picamera2...")
        cam = PiCamera2Adapter(device_name=device_name, default_width=width, default_height=height)
        cam.start()
        return cam

    if requested_type == "opencv":
        logger.info(f"Initializing OpenCV camera (index/URL: {camera_index})...")
        cam = OpenCVCamera(
            camera_index=camera_index,
            device_name=device_name,
            default_width=width,
            default_height=height
        )
        cam.start()
        return cam

    logger.info("Auto-detecting available camera hardware...")

    if PICAMERA2_AVAILABLE:
        try:
            logger.info("Attempting to load Picamera2...")
            cam = PiCamera2Adapter(device_name=device_name, default_width=width, default_height=height)
            cam.start()
            logger.info("Successfully started Picamera2 camera!")
            return cam
        except Exception as e:
            logger.warning(f"Picamera2 initialization failed ({e}). Trying OpenCV...")

    try:
        logger.info(f"Attempting to load OpenCV camera at {camera_index}...")
        cam = OpenCVCamera(
            camera_index=camera_index,
            device_name=device_name,
            default_width=width,
            default_height=height
        )
        cam.start()
        logger.info("Successfully started OpenCV camera!")
        return cam
    except Exception as e:
        logger.warning(f"OpenCV camera initialization failed ({e}). Falling back to Mock Camera.")

    logger.warning("No physical camera could be opened. Starting Mock synthetic camera for testing.")
    cam = MockCamera(device_name=device_name, default_width=width, default_height=height)
    cam.start()
    return cam
