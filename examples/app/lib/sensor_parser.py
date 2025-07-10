import logging
import struct
from typing import Dict, Optional

logger = logging.getLogger(__name__)

class WT9011Parser:
    @staticmethod
    def parse(data: bytearray) -> Optional[Dict[str, Dict[str, float]]]:
        try:
            logger.debug(f"Raw data (len={len(data)}): {data.hex()}")

            # Минимальная длина пакета - 20 байт (без checksum)
            if len(data) < 20:
                logger.error(f"Packet too short: {len(data)} bytes")
                return None

            if data[0] != 0x55 or data[1] != 0x61:
                logger.error(f"Invalid header: 0x{data[0]:02X} 0x{data[1]:02X}")
                return None

            # Проверяем, есть ли checksum (21-й байт)
            if len(data) >= 21:
                checksum = sum(data[:20]) & 0xFF
                if checksum != data[20]:
                    logger.error(f"Checksum error: calc 0x{checksum:02X} != rcvd 0x{data[20]:02X}")
                    return None

            # Парсим данные (первые 20 байт)
            ax, ay, az = struct.unpack('<hhh', data[2:8])
            gx, gy, gz = struct.unpack('<hhh', data[8:14])
            roll, pitch, yaw = struct.unpack('<hhh', data[14:20])

            result = {
                "accel": {
                    "x": ax / 32768 * 16,
                    "y": ay / 32768 * 16,
                    "z": az / 32768 * 16
                },
                "gyro": {
                    "x": gx / 32768 * 2000,
                    "y": gy / 32768 * 2000,
                    "z": gz / 32768 * 2000
                },
                "angle": {
                    "roll": roll / 32768 * 180,
                    "pitch": pitch / 32768 * 180,
                    "yaw": (yaw / 32768 * 180) - 360 if yaw / 32768 * 180 > 180 else yaw / 32768 * 180
                }
            }
            logger.debug(f"Parsed data: {result}")
            return result

        except Exception as e:
            logger.error(f"Parse error: {str(e)}", exc_info=True)
            return None