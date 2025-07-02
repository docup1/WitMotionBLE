# sensor_commands.py
# Расширенный модуль с командами для WT9011

class WT9011Commands:
    """Команды управления датчиком WT9011"""

    @staticmethod
    def build_command_zeroing() -> bytes:
        """Обнулить текущее положение"""
        return bytes([0xFF, 0xAA, 0x52, 0x00])

    @staticmethod
    def build_command_calibration() -> bytes:
        """Начать калибровку акселерометра"""
        return bytes([0xFF, 0xAA, 0x01, 0x00])

    @staticmethod
    def build_command_sleep() -> bytes:
        """Увести устройство в спящий режим"""
        return bytes([0xFF, 0xAA, 0x06, 0x00])

    @staticmethod
    def build_command_wakeup() -> bytes:
        """Вывести устройство из спящего режима"""
        return bytes([0xFF, 0xAA, 0x06, 0x01])

    @staticmethod
    def build_command_save_settings() -> bytes:
        """Сохранить текущие настройки в память"""
        return bytes([0xFF, 0xAA, 0x00, 0x00])

    @staticmethod
    def build_command_factory_reset() -> bytes:
        """Сброс настроек к заводским"""
        return bytes([0xFF, 0xAA, 0x03, 0x00])

    @staticmethod
    def build_command_set_return_rate(rate_hz: int) -> bytes:
        """
        Установить частоту возврата данных (1-100 Гц)
        rate_hz: Частота обновления в Герцах
        """
        if not (1 <= rate_hz <= 100):
            raise ValueError("Частота должна быть от 1 до 100 Гц")
        return bytes([0xFF, 0xAA, 0x15, rate_hz])

    @staticmethod
    def build_command_accel_enable(enable: bool) -> bytes:
        """Включить (True) или выключить (False) акселерометр"""
        return bytes([0xFF, 0xAA, 0x10, 0x01 if enable else 0x00])

    @staticmethod
    def build_command_gyro_enable(enable: bool) -> bytes:
        """Включить (True) или выключить (False) гироскоп"""
        return bytes([0xFF, 0xAA, 0x11, 0x01 if enable else 0x00])

