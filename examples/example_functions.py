import asyncio
from lib.ble_manager import BLEManager
from lib.sensor_commands import WT9011Commands
from rich.console import Console

console = Console()

async def main():
    ble = BLEManager()
    devices = await ble.scan()
    if not devices:
        console.print("[red]Устройства не найдены")
        return

    for i, dev in enumerate(devices):
        console.print(f"[{i}] {dev['name']} - {dev['address']}")
    idx = int(console.input("Выберите устройство: "))
    await ble.connect(devices[idx]['address'])
    console.print("[green]Подключено!")

    def on_data(data: bytearray):
        # Здесь можно парсить и обрабатывать данные
        pass

    await ble.receive(on_data)

    console.print("""
    Команды:
    z - Обнуление
    c - Калибровка
    s - Сон
    w - Проснуться
    save - Сохранить настройки
    reset - Заводской сброс
    rate - Установить частоту возврата (пример: rate 50)
    accel on/off - Вкл/выкл акселерометр
    gyro on/off - Вкл/выкл гироскоп
    mag on/off - Вкл/выкл магнитометр
    q - Выход
    """)

    while True:
        cmd = (await asyncio.to_thread(input, "> ")).strip().lower()
        if cmd == "z":
            await ble.send(WT9011Commands.build_command_zeroing())
            console.print("Обнуление отправлено")
        elif cmd == "c":
            await ble.send(WT9011Commands.build_command_calibration())
            console.print("Калибровка начата")
        elif cmd == "s":
            await ble.send(WT9011Commands.build_command_sleep())
            console.print("Устройство усыплено")
        elif cmd == "w":
            await ble.send(WT9011Commands.build_command_wakeup())
            console.print("Устройство проснулось")
        elif cmd == "save":
            await ble.send(WT9011Commands.build_command_save_settings())
            console.print("Настройки сохранены")
        elif cmd == "reset":
            await ble.send(WT9011Commands.build_command_factory_reset())
            console.print("Сброс к заводским")
        elif cmd.startswith("rate"):
            try:
                rate = int(cmd.split()[1])
                await ble.send(WT9011Commands.build_command_set_return_rate(rate))
                console.print(f"Частота возврата установлена: {rate} Гц")
            except Exception as e:
                console.print(f"Ошибка: {e}")
        elif cmd in ("accel on", "accel off"):
            enable = cmd.endswith("on")
            await ble.send(WT9011Commands.build_command_accel_enable(enable))
            console.print(f"Акселерометр {'включён' if enable else 'выключен'}")
        elif cmd in ("gyro on", "gyro off"):
            enable = cmd.endswith("on")
            await ble.send(WT9011Commands.build_command_gyro_enable(enable))
            console.print(f"Гироскоп {'включён' if enable else 'выключен'}")
        elif cmd in ("mag on", "mag off"):
            enable = cmd.endswith("on")
            await ble.send(WT9011Commands.build_command_mag_enable(enable))
            console.print(f"Магнитометр {'включён' if enable else 'выключен'}")
        elif cmd == "q":
            break
        else:
            console.print("Неизвестная команда")

    await ble.disconnect()
    console.print("Отключено")

if __name__ == "__main__":
    asyncio.run(main())
