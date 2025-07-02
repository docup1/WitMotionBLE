#sensor_parser.py
import struct
from typing import Optional, Dict

class WT9011Parser:
    """
    Класс для обработки данных WT9011 и генерации команд.
    """

    @staticmethod
    def parse(data: bytearray) -> Optional[Dict[str, Dict[str, float]]]:
        """
        Парсит входной байт-массив и возвращает измерения акселя, гироскопа и углов.
        """
        if len(data) < 20 or data[0] != 0x55 or data[1] != 0x61:
            return None

        try:
            ax, ay, az = struct.unpack('<hhh', data[2:8])
            gx, gy, gz = struct.unpack('<hhh', data[8:14])
            roll, pitch, yaw = struct.unpack('<hhh', data[14:20])
        except struct.error:
            return None

        return {
            "accel": {"x": ax / 32768 * 16, "y": ay / 32768 * 16, "z": az / 32768 * 16},
            "gyro": {"x": gx / 32768 * 2000, "y": gy / 32768 * 2000, "z": gz / 32768 * 2000},
            "angle": {
                "roll": roll / 32768 * 180,
                "pitch": pitch / 32768 * 180,
                "yaw": (yaw / 32768 * 180) - 360 if yaw / 32768 * 180 > 180 else yaw / 32768 * 180
            }
        }

    @staticmethod
    def build_command_zeroing() -> bytes:
        """
        Команда: Установить текущие значения как ноль (аксель/гиро).
        """
        return bytes([0xFF, 0xAA, 0x52, 0x00])

    @staticmethod
    def build_command_calibration() -> bytes:
        """
        Команда: Начать калибровку акселерометра и гироскопа.
        """
        return bytes([0xFF, 0xAA, 0x01, 0x00])
