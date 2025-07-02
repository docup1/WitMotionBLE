#ble_mannager.py

import asyncio

from typing import Callable, Optional, List, Dict
from bleak import BleakClient, BleakScanner

class BLEManager:
    """
    Класс для работы с BLE-устройствами (сканирование, подключение, приём/отправка данных).
    """

    def __init__(self):
        self.client: Optional[BleakClient] = None
        self.notify_char_uuid: Optional[str] = None

    async def scan(self, timeout: float = 5.0) -> List[Dict[str, str]]:
        """
        Сканирует BLE-устройства и возвращает список найденных.
        """
        devices = await BleakScanner.discover(timeout=timeout)
        return [{"name": d.name or "", "address": d.address} for d in devices]

    async def connect(self, address: str) -> None:
        """
        Подключается к BLE-устройству по MAC-адресу.
        """
        self.client = BleakClient(address)
        await self.client.connect()

        services = self.client.services
        if services is None:
            await self.client.get_services()
            services = self.client.services

        for service in services:
            for char in service.characteristics:
                if "notify" in char.properties:
                    self.notify_char_uuid = char.uuid
                    return

        raise RuntimeError("Notify характеристика не найдена")

    async def receive(self, on_data: Callable[[bytearray], None]) -> None:
        """
        Подписывается на приём данных. Вызывает on_data при каждом пакете.
        """
        if not self.client or not self.notify_char_uuid:
            raise RuntimeError("Нет подключения или notify UUID")

        def notification_handler(_, data: bytearray):
            on_data(data)

        await self.client.start_notify(self.notify_char_uuid, notification_handler)

    async def send(self, command: bytes):
        if not self.client or not self.client.is_connected:
            self.console.print("[red]Ошибка: не подключено")
            return

        if self.client.services is None:
            await self.client.get_services()
        services = self.client.services

        for service in services:
            for char in service.characteristics:
                if "write" in char.properties:
                    try:
                        await self.client.write_gatt_char(char.uuid, command)
                        return
                    except Exception as e:
                        self.console.print(f"[red]Ошибка отправки команды: {e}")
                        return
        self.console.print("[red]Write характеристика не найдена!")

    async def disconnect(self) -> None:
        """
        Отключается от BLE-устройства.
        """
        if self.client and self.client.is_connected:
            await self.client.disconnect()
