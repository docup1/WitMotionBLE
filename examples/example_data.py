import asyncio
from datetime import datetime
from rich.console import Console
from rich.table import Table
from lib.ble_manager import BLEManager
from lib.sensor_parser import WT9011Parser

console = Console()

async def main():
    parser = WT9011Parser()
    ble = BLEManager()

    # 1. Сканирование
    devices = await ble.scan()
    if not devices:
        console.print("[red]Устройства не найдены")
        return

    # 2. Вывод списка
    table = Table(title="BLE устройства")
    table.add_column("№")
    table.add_column("Имя")
    table.add_column("MAC")

    for i, dev in enumerate(devices):
        table.add_row(str(i), dev["name"], dev["address"])
    console.print(table)

    # 3. Выбор
    idx = int(console.input("Введите номер устройства: "))
    address = devices[idx]["address"]
    await ble.connect(address)

    # 4. Подписка на данные
    async def start_receiving():
        await ble.receive(on_data)

    def on_data(data: bytearray):
        parsed = parser.parse(data)
        if parsed:
            timestamp = datetime.now().strftime("%H:%M:%S")
            console.print(f"[{timestamp}] {parsed}")

    task = asyncio.create_task(start_receiving())

    # 5. Команды
    console.print("[yellow]Команды: [q] выйти | [z] нули | [c] калибровка")
    while True:
        cmd = await asyncio.to_thread(input)
        if cmd == "q":
            break
        elif cmd == "z":
            await ble.send(parser.build_command_zeroing())
            console.print("[cyan]Отправлена команда нуля")
        elif cmd == "c":
            await ble.send(parser.build_command_calibration())
            console.print("[cyan]Отправлена команда калибровки")

    await ble.disconnect()
    task.cancel()

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        console.print("[red]Остановлено пользователем")
