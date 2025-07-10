# ble_manager.py

import asyncio
import sys
from typing import Callable, Optional, List, Dict
from bleak import BleakClient, BleakScanner, BleakError
import logging

# Настройка логирования для отладки
logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger(__name__)


class BLEManager:
    """
    Класс для работы с BLE-устройствами (сканирование, подключение, приём/отправка данных).
    """

    def __init__(self):
        self.client: Optional[BleakClient] = None
        self.notify_char_uuid: Optional[str] = None
        self.write_char_uuid: Optional[str] = None
        self.loop = None
        logger.info("BLEManager initialized")

    def _get_loop(self):
        """Получение или создание event loop"""
        try:
            loop = asyncio.get_event_loop()
            if loop.is_running():
                return loop
        except RuntimeError:
            pass

        # Создаем новый loop если текущий не существует или не работает
        if sys.platform == "win32":
            asyncio.set_event_loop_policy(asyncio.WindowsProactorEventLoopPolicy())

        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        return loop

    async def scan(self, timeout: float = 10.0) -> List[Dict[str, str]]:
        """
        Сканирует BLE-устройства и возвращает список найденных.
        """
        try:
            logger.info(f"Starting BLE scan with timeout {timeout}s")

            # Увеличиваем таймаут для более надежного сканирования
            devices = await BleakScanner.discover(timeout=timeout)

            result = []
            for device in devices:
                device_info = {
                    "name": device.name if device.name else "Unknown Device",
                    "address": device.address
                }
                result.append(device_info)
                logger.info(f"Found device: {device_info['name']} ({device_info['address']})")

            logger.info(f"Scan completed. Found {len(result)} devices")
            return result

        except Exception as e:
            logger.error(f"Scan failed: {str(e)}")
            return []

    async def connect(self, address: str, max_attempts: int = 3, timeout: float = 15.0) -> None:
        """
        Connects to a BLE device by its MAC address with retries.

        Args:
            address: The MAC address of the device.
            max_attempts: Number of connection attempts.
            timeout: Connection timeout in seconds.
        """
        try:
            logger.info(f"Attempting to connect to {address} (max_attempts={max_attempts}, timeout={timeout}s)")

            # Disconnect if already connected
            if self.client and self.client.is_connected:
                logger.debug("Disconnecting existing connection")
                await self.client.disconnect()
                await asyncio.sleep(0.5)  # Brief delay to ensure disconnection

            # Retry connection
            for attempt in range(1, max_attempts + 1):
                try:
                    logger.debug(f"Connection attempt {attempt}/{max_attempts}")
                    self.client = BleakClient(address, timeout=timeout)
                    await self.client.connect()
                    logger.info(f"Connected to {address} on attempt {attempt}")
                    break
                except (BleakError, asyncio.TimeoutError) as e:
                    logger.warning(f"Connection attempt {attempt} failed: {str(e)}")
                    if attempt == max_attempts:
                        raise RuntimeError(f"Failed to connect after {max_attempts} attempts: {str(e)}")
                    await asyncio.sleep(2.0)  # Increased delay before retrying

            # Verify connection
            if not self.client.is_connected:
                raise RuntimeError("Connection established but client reports not connected")

            # Access services
            services = self.client.services
            if not services or not services.services:
                raise RuntimeError("No services found on the device")
            logger.info(f"Found {len(services.services)} services")

            # Iterate through services to find notify and write characteristics
            for service in services:
                logger.debug(f"Service: {service.uuid}")
                for char in service.characteristics:
                    logger.debug(f"  Characteristic: {char.uuid}, Properties: {char.properties}")
                    if "notify" in char.properties and not self.notify_char_uuid:
                        self.notify_char_uuid = char.uuid
                        logger.info(f"Found notify characteristic: {char.uuid}")
                    if (
                            "write" in char.properties or "write-without-response" in char.properties) and not self.write_char_uuid:
                        self.write_char_uuid = char.uuid
                        logger.info(f"Found write characteristic: {char.uuid}")

            if not self.notify_char_uuid:
                logger.error("No notify characteristic found")
                raise RuntimeError("No notify characteristic found")
            if not self.write_char_uuid:
                logger.error("No write characteristic found")
                raise RuntimeError("No write characteristic found")

        except Exception as e:
            logger.error(f"Connection failed: {str(e)}")
            if self.client and self.client.is_connected:
                await self.client.disconnect()
            raise RuntimeError(f"Connection failed: {str(e)}")

    async def receive(self, on_data: Callable[[bytearray], None]) -> None:
        if not self.client or not self.client.is_connected:
            raise RuntimeError("No connection established")

        if not self.notify_char_uuid:
            raise RuntimeError("No notify characteristic found")

        def notification_handler(sender, data: bytearray):
            logger.debug(f"Raw data received (len={len(data)}): {data.hex()}")
            try:
                # Проверяем минимальную длину и заголовок пакета
                if len(data) >= 2 and data[0] == 0x55:
                    logger.debug(f"Valid packet header found: 0x{data[0]:02X} 0x{data[1]:02X}")
                    on_data(data)
                else:
                    logger.warning(f"Invalid packet: {data.hex()}")
            except Exception as e:
                logger.error(f"Error in notification handler: {str(e)}")

        try:
            await self.client.start_notify(self.notify_char_uuid, notification_handler)
            logger.info("Notifications started successfully")
        except Exception as e:
            logger.error(f"Failed to start notifications: {str(e)}")
            raise

    async def send(self, command: bytes) -> None:
        """
        Отправляет команду на устройство.
        """
        if not self.client or not self.client.is_connected:
            logger.error("No connection established")
            raise RuntimeError("No connection established")

        if not self.write_char_uuid:
            logger.error("No write characteristic found")
            raise RuntimeError("No write characteristic found")

        try:
            await self.client.write_gatt_char(self.write_char_uuid, command)
            logger.info(f"Sent command: {command.hex()}")
        except Exception as e:
            logger.error(f"Failed to send command: {str(e)}")
            raise

    async def disconnect(self) -> None:
        """
        Отключается от BLE-устройства.
        """
        if self.client and self.client.is_connected:
            try:
                await self.client.disconnect()
                logger.info("Disconnected from device")
            except Exception as e:
                logger.error(f"Disconnect failed: {str(e)}")
                raise

        self.client = None
        self.notify_char_uuid = None
        self.write_char_uuid = None


# Глобальный экземпляр для использования в C++
ble_manager_instance = BLEManager()