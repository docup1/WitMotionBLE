# WT9011 Library Documentation

Основная библиотека для работы с датчиком WT9011 через Bluetooth Low Energy.

## Модули

### 🔗 BLEManager (`ble_manager.py`)

Класс для управления BLE подключением к датчику.

#### Методы

##### `async scan(timeout: float = 5.0) -> List[Dict[str, str]]`
Сканирует доступные BLE устройства.

**Параметры:**
- `timeout` - время сканирования в секундах (по умолчанию 5.0)

**Возвращает:**
```python
[
    {"name": "Device Name", "address": "XX:XX:XX:XX:XX:XX"},
    {"name": "", "address": "YY:YY:YY:YY:YY:YY"}  # имя может быть пустым
]
```

**Пример:**
```python
ble = BLEManager()
devices = await ble.scan(timeout=10.0)
for device in devices:
    print(f"{device['name']} - {device['address']}")
```

##### `async connect(address: str) -> None`
Подключается к BLE устройству по MAC адресу.

**Параметры:**
- `address` - MAC адрес устройства в формате "XX:XX:XX:XX:XX:XX"

**Исключения:**
- `RuntimeError` - если не найдена notify характеристика

**Пример:**
```python
await ble.connect("12:34:56:78:9A:BC")
```

##### `async receive(on_data: Callable[[bytearray], None]) -> None`
Подписывается на получение данных от датчика.

**Параметры:**
- `on_data` - callback функция, вызываемая при получении данных

**Пример:**
```python
def handle_data(data: bytearray):
    print(f"Получено {len(data)} байт: {data.hex()}")

await ble.receive(handle_data)
```

##### `async send(command: bytes)`
Отправляет команду устройству.

**Параметры:**
- `command` - команда в виде байт-массива

**Пример:**
```python
command = bytes([0xFF, 0xAA, 0x52, 0x00])  # команда обнуления
await ble.send(command)
```

##### `async disconnect() -> None`
Отключается от устройства.

**Пример:**
```python
await ble.disconnect()
```

---

### 📊 WT9011Parser (`sensor_parser.py`)

Класс для парсинга данных датчика и генерации базовых команд.

#### Методы

##### `static parse(data: bytearray) -> Optional[Dict[str, Dict[str, float]]]`
Парсит сырые данные от датчика.

**Параметры:**
- `data` - байт-массив от датчика (минимум 20 байт)

**Возвращает:**
```python
{
    "accel": {"x": 0.0, "y": 0.0, "z": 9.8},      # м/с²
    "gyro": {"x": 0.0, "y": 0.0, "z": 0.0},       # °/с
    "angle": {"roll": 0.0, "pitch": 0.0, "yaw": 0.0}  # градусы
}
```

**Возвращает `None` если:**
- Данных меньше 20 байт
- Неверный заголовок (должен быть 0x55, 0x61)
- Ошибка при распаковке данных

**Пример:**
```python
parser = WT9011Parser()

def on_data(data: bytearray):
    parsed = parser.parse(data)
    if parsed:
        accel = parsed['accel']
        print(f"Ускорение: X={accel['x']:.2f} Y={accel['y']:.2f} Z={accel['z']:.2f}")
```

**Формула преобразования:**
```python
# Акселерометр: ±16g диапазон
accel_g = raw_value / 32768 * 16

# Гироскоп: ±2000°/с диапазон  
gyro_dps = raw_value / 32768 * 2000

# Углы: ±180° диапазон
angle_deg = raw_value / 32768 * 180
# Специальная обработка для yaw (приведение к диапазону -180..+180)
```

##### `static build_command_zeroing() -> bytes`
Создает команду обнуления текущего положения.

**Возвращает:** `bytes([0xFF, 0xAA, 0x52, 0x00])`

**Пример:**
```python
zero_cmd = WT9011Parser.build_command_zeroing()
await ble.send(zero_cmd)
```

##### `static build_command_calibration() -> bytes`
Создает команду калибровки акселерометра и гироскопа.

**Возвращает:** `bytes([0xFF, 0xAA, 0x01, 0x00])`

**Пример:**
```python
cal_cmd = WT9011Parser.build_command_calibration()
await ble.send(cal_cmd)
```

---

### ⚙️ WT9011Commands (`sensor_commands.py`)

Расширенный набор команд для управления датчиком.

#### Методы

##### Основные команды

```python
@staticmethod
def build_command_zeroing() -> bytes
```
**Назначение:** Установить текущие значения акселерометра и гироскопа как ноль  
**Код команды:** `0x52`  
**Возвращает:** `bytes([0xFF, 0xAA, 0x52, 0x00])`

```python
@staticmethod  
def build_command_calibration() -> bytes
```
**Назначение:** Начать процедуру калибровки  
**Код команды:** `0x01`  
**Возвращает:** `bytes([0xFF, 0xAA, 0x01, 0x00])`

```python
@staticmethod
def build_command_save_settings() -> bytes
```
**Назначение:** Сохранить текущие настройки в энергонезависимую память  
**Код команды:** `0x00`  
**Возвращает:** `bytes([0xFF, 0xAA, 0x00, 0x00])`

```python
@staticmethod
def build_command_factory_reset() -> bytes
```
**Назначение:** Сброс всех настроек к заводским значениям  
**Код команды:** `0x03`  
**Возвращает:** `bytes([0xFF, 0xAA, 0x03, 0x00])`

##### Управление питанием

```python
@staticmethod
def build_command_sleep() -> bytes
```
**Назначение:** Перевести устройство в режим сна (низкое энергопотребление)  
**Код команды:** `0x06`  
**Возвращает:** `bytes([0xFF, 0xAA, 0x06, 0x00])`

```python
@staticmethod
def build_command_wakeup() -> bytes
```
**Назначение:** Вывести устройство из режима сна  
**Код команды:** `0x06`  
**Возвращает:** `bytes([0xFF, 0xAA, 0x06, 0x01])`

##### Настройка частоты

```python
@staticmethod
def build_command_set_return_rate(rate_hz: int) -> bytes
```
**Назначение:** Установить частоту передачи данных  
**Параметры:** `rate_hz` - частота от 1 до 100 Гц  
**Код команды:** `0x15`  
**Исключения:** `ValueError` если частота вне диапазона  
**Возвращает:** `bytes([0xFF, 0xAA, 0x15, rate_hz])`

**Пример:**
```python
# Установить частоту 50 Гц
cmd = WT9011Commands.build_command_set_return_rate(50)
await ble.send(cmd)
```

##### Управление датчиками

```python
@staticmethod
def build_command_accel_enable(enable: bool) -> bytes
```
**Назначение:** Включить/выключить акселерометр  
**Параметры:** `enable` - True для включения, False для выключения  
**Код команды:** `0x10`  
**Возвращает:** `bytes([0xFF, 0xAA, 0x10, 0x01 if enable else 0x00])`

```python
@staticmethod
def build_command_gyro_enable(enable: bool) -> bytes
```
**Назначение:** Включить/выключить гироскоп  
**Параметры:** `enable` - True для включения, False для выключения  
**Код команды:** `0x11`  
**Возвращает:** `bytes([0xFF, 0xAA, 0x11, 0x01 if enable else 0x00])`

## Протокол связи

### Формат пакета данных (20 байт)

| Байты | Описание | Тип данных | Примечание |
|-------|----------|------------|------------|
| 0-1 | Заголовок | `0x55, 0x61` | Идентификатор пакета |
| 2-3 | Акселерометр X | `int16_le` | Little-endian |
| 4-5 | Акселерометр Y | `int16_le` | Little-endian |
| 6-7 | Акселерометр Z | `int16_le` | Little-endian |
| 8-9 | Гироскоп X | `int16_le` | Little-endian |
| 10-11 | Гироскоп Y | `int16_le` | Little-endian |
| 12-13 | Гироскоп Z | `int16_le` | Little-endian |
| 14-15 | Roll | `int16_le` | Little-endian |
| 16-17 | Pitch | `int16_le` | Little-endian |
| 18-19 | Yaw | `int16_le` | Little-endian |

### Формат команды (4 байта)

| Байт | Описание | Значение |
|------|----------|----------|
| 0 | Стартовый байт | `0xFF` |
| 1 | Адрес устройства | `0xAA` |
| 2 | Код команды | Зависит от команды |
| 3 | Параметр | Зависит от команды |

### Коды команд

| Код | Команда | Параметр | Описание |
|-----|---------|----------|----------|
| `0x00` | Сохранить настройки | `0x00` | Записать в EEPROM |
| `0x01` | Калибровка | `0x00` | Начать калибровку |
| `0x03` | Заводской сброс | `0x00` | Сброс настроек |
| `0x06` | Сон/Пробуждение | `0x00/0x01` | 0=сон, 1=пробуждение |
| `0x10` | Акселерометр | `0x00/0x01` | 0=выкл, 1=вкл |
| `0x11` | Гироскоп | `0x00/0x01` | 0=выкл, 1=вкл |
| `0x15` | Частота возврата | `1-100` | Частота в Гц |
| `0x52` | Обнуление | `0x00` | Сброс углов |

## Обработка ошибок

### BLEManager исключения
- `RuntimeError` - нет подключения или не найдены характеристики
- `BleakError` - ошибки BLE подсистемы
- `TimeoutError` - таймаут операции

### Примеры обработки
```python
try:
    await ble.connect("12:34:56:78:9A:BC")
except RuntimeError as e:
    print(f"Ошибка подключения: {e}")
except Exception as e:
    print(f"Неожиданная ошибка: {e}")
```

## Производительность

### Рекомендуемые настройки
- **Частота данных**: 20-50 Гц для большинства применений
- **Buffer size**: Обрабатывайте данные немедленно в callback
- **Timeout сканирования**: 5-10 секунд

### Ограничения
- Максимальная частота: 100 Гц
- Дальность BLE: обычно 10-50 метров
- Одновременные подключения: 1 устройство на BLEManager

## Интеграция

### Использование с asyncio
```python
import asyncio
from lib.ble_manager import BLEManager

async def main():
    ble = BLEManager()
    try:
        devices = await ble.scan()
        if devices:
            await ble.connect(devices[0]['address'])
            # Ваш код здесь
    finally:
        await ble.disconnect()

asyncio.run(main())
```

### Использование с GUI (Qt)
```python
import qasync
from PyQt5.QtWidgets import QApplication

app = QApplication([])
loop = qasync.QEventLoop(app)
asyncio.set_event_loop(loop)

# Ваш GUI код

with loop:
    loop.run_forever()
```