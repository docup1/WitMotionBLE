# WT9011Lib - C++/Python библиотека для работы с датчиком WT9011

Библиотека для работы с IMU датчиком WT9011 через BLE, объединяющая возможности C++ и Python с использованием pybind11.

## Содержание

- [Описание](#описание)
- [Требования](#требования)
- [Сборка](#сборка)
- [Архитектура](#архитектура)
- [API Функции](#api-функции)
- [Примеры использования](#примеры-использования)
- [Структура данных](#структура-данных)
- [Команды датчика](#команды-датчика)
- [BLE Manager](#ble-manager)
- [Устранение неисправностей](#устранение-неисправностей)

## Описание

WT9011Lib - это гибридная библиотека, которая позволяет работать с IMU датчиком WT9011 как из C++, так и из Python кода. Библиотека предоставляет:

- Парсинг данных акселерометра, гироскопа и углов поворота
- Генерацию команд управления датчиком
- BLE менеджер для беспроводного подключения
- Простой C++ API через встроенный Python интерпретатор

## Требования

### Системные требования
- CMake 3.16+
- C++17 совместимый компилятор
- Python 3.7+
- Qt6 (опционально, для демо приложения)

### Python зависимости
```bash
pip install bleak asyncio
```

### Внешние библиотеки
- pybind11 v2.12.0 (загружается автоматически через FetchContent)

## Сборка

### Стандартная сборка
```bash
mkdir build
cd build
cmake ..
make -j4
```

### Сборка без Qt демо
```bash
cmake -DBUILD_QT_DEMO=OFF ..
make -j4
```

### Windows (Visual Studio)
```bash
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019"
cmake --build . --config Release
```

После сборки будут созданы:
- `wt9011lib.dll` (или `.so` на Linux) - основная библиотека
- `wt9011_demo.exe` - Qt демо приложение (если включено)
- Python модули скопированы в папку сборки

## Архитектура

```
WT9011Lib/
├── CMakeLists.txt          # Конфигурация сборки
├── main.cpp               # Qt демо приложение
├── py_module.cpp          # C++ обёртка для Python модулей
├── sensor_parser.py       # Парсер данных датчика
├── sensor_commands.py     # Команды управления
└── ble_manager.py         # BLE менеджер
```

Библиотека использует встроенный Python интерпретатор (pybind11::embed) для выполнения Python кода из C++.

## API Функции

### Основные функции (экспортируемые из DLL)

#### `parse(py::bytes data) -> py::object`
Парсит данные от датчика WT9011.

**Параметры:**
- `data` - байтовый массив размером 20 байт от датчика

**Возвращает:**
- Python словарь с данными акселерометра, гироскопа и углов или `None` при ошибке

**Пример:**
```cpp
py::module sensor = py::module::import("wt9011lib");
py::bytes raw_data("\x55\x61\x01\x02...", 20);
auto result = sensor.attr("parse")(raw_data);
```

#### `zeroing_cmd() -> py::bytes`
Создаёт команду обнуления текущего положения датчика.

**Возвращает:**
- Байтовую команду для отправки датчику

**Пример:**
```cpp
py::module sensor = py::module::import("wt9011lib");
auto cmd = sensor.attr("zeroing_cmd")();
py::bytes command = cmd.cast<py::bytes>();
```

#### `calibrate_cmd() -> py::bytes`
Создаёт команду калибровки акселерометра.

**Возвращает:**
- Байтовую команду для отправки датчику

## Примеры использования

### Простое C++ приложение

```cpp
#include <pybind11/embed.h>
#include <iostream>
#include <vector>

namespace py = pybind11;

int main() {
    // Инициализация встроенного Python интерпретатора
    py::scoped_interpreter guard{};
    
    try {
        // Импорт нашей библиотеки
        py::module wt9011 = py::module::import("wt9011lib");
        
        // Пример данных от датчика (20 байт)
        std::vector<uint8_t> sensor_data = {
            0x55, 0x61,  // Заголовок
            0x01, 0x02, 0x03, 0x04, 0x05, 0x06,  // Акселерометр (X,Y,Z)
            0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C,  // Гироскоп (X,Y,Z)
            0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12   // Углы (Roll,Pitch,Yaw)
        };
        
        // Создание bytes объекта
        py::bytes data(reinterpret_cast<const char*>(sensor_data.data()), 
                       sensor_data.size());
        
        // Парсинг данных
        auto result = wt9011.attr("parse")(data);
        
        if (!result.is_none()) {
            std::cout << "Parsed data: " << py::str(result).cast<std::string>() 
                      << std::endl;
            
            // Доступ к конкретным значениям
            py::dict data_dict = result.cast<py::dict>();
            py::dict accel = data_dict["accel"].cast<py::dict>();
            
            std::cout << "Accel X: " << accel["x"].cast<double>() << " g" << std::endl;
            std::cout << "Accel Y: " << accel["y"].cast<double>() << " g" << std::endl;
            std::cout << "Accel Z: " << accel["z"].cast<double>() << " g" << std::endl;
        }
        
        // Создание команды обнуления
        auto zero_cmd = wt9011.attr("zeroing_cmd")();
        py::bytes command = zero_cmd.cast<py::bytes>();
        
        std::cout << "Zeroing command size: " << command.size() << " bytes" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
```

### Работа с расширенными командами

```cpp
#include <pybind11/embed.h>
#include <iostream>

namespace py = pybind11;

void sendCommands() {
    py::scoped_interpreter guard{};
    
    // Импорт модуля команд напрямую
    py::module cmds = py::module::import("sensor_commands");
    py::object WT9011Commands = cmds.attr("WT9011Commands");
    
    // Различные команды
    auto sleep_cmd = WT9011Commands.attr("build_command_sleep")();
    auto wakeup_cmd = WT9011Commands.attr("build_command_wakeup")();
    auto save_cmd = WT9011Commands.attr("build_command_save_settings")();
    auto reset_cmd = WT9011Commands.attr("build_command_factory_reset")();
    
    // Установка частоты обновления на 50 Гц
    auto rate_cmd = WT9011Commands.attr("build_command_set_return_rate")(50);
    
    // Включение/выключение сенсоров
    auto accel_on = WT9011Commands.attr("build_command_accel_enable")(true);
    auto gyro_off = WT9011Commands.attr("build_command_gyro_enable")(false);
    
    std::cout << "Commands created successfully" << std::endl;
}
```

### Использование BLE Manager (асинхронно)

```cpp
#include <pybind11/embed.h>
#include <future>

namespace py = pybind11;

void bleExample() {
    py::scoped_interpreter guard{};
    
    // Запуск асинхронного кода через Python
    py::exec(R"(
import asyncio
from ble_manager import BLEManager

async def main():
    ble = BLEManager()
    
    # Сканирование устройств
    devices = await ble.scan(timeout=5.0)
    print(f"Found {len(devices)} devices")
    
    for device in devices:
        print(f"Device: {device['name']} - {device['address']}")
    
    # Подключение к первому найденному устройству
    if devices:
        await ble.connect(devices[0]['address'])
        print("Connected successfully")
        
        # Обработчик входящих данных
        def handle_data(data):
            print(f"Received {len(data)} bytes")
            # Здесь можно вызвать парсер
        
        await ble.receive(handle_data)
        
        # Отправка команды обнуления
        from sensor_commands import WT9011Commands
        cmd = WT9011Commands.build_command_zeroing()
        await ble.send(cmd)
        
        # Ожидание данных
        await asyncio.sleep(5)
        
        await ble.disconnect()

asyncio.run(main())
    )");
}
```

## Структура данных

### Формат входных данных
Датчик WT9011 отправляет пакеты по 20 байт:

```
Байт 0-1:   0x55 0x61 (заголовок)
Байт 2-7:   Акселерометр X,Y,Z (int16, little-endian)
Байт 8-13:  Гироскоп X,Y,Z (int16, little-endian)  
Байт 14-19: Углы Roll,Pitch,Yaw (int16, little-endian)
```

### Формат выходных данных
Функция `parse()` возвращает словарь:

```json
{
  "accel": {
    "x": 0.123,  // g (ускорение свободного падения)
    "y": -0.456,
    "z": 0.987
  },
  "gyro": {
    "x": 12.34,  // град/сек
    "y": -56.78,
    "z": 90.12
  },
  "angle": {
    "roll": 15.6,   // градусы (-180..+180)
    "pitch": -8.9,
    "yaw": 123.4
  }
}
```

### Коэффициенты преобразования
- **Акселерометр**: `значение / 32768 * 16` (диапазон ±16g)
- **Гироскоп**: `значение / 32768 * 2000` (диапазон ±2000°/сек)  
- **Углы**: `значение / 32768 * 180` (диапазон ±180°)

## Команды датчика

### Базовые команды

| Команда | Код | Описание |
|---------|-----|----------|
| `build_command_zeroing()` | `FF AA 52 00` | Обнулить текущее положение |
| `build_command_calibration()` | `FF AA 01 00` | Калибровка акселерометра |
| `build_command_sleep()` | `FF AA 06 00` | Спящий режим |
| `build_command_wakeup()` | `FF AA 06 01` | Выход из спящего режима |
| `build_command_save_settings()` | `FF AA 00 00` | Сохранить настройки |
| `build_command_factory_reset()` | `FF AA 03 00` | Сброс к заводским настройкам |

### Команды конфигурации

| Команда | Параметр | Описание |
|---------|----------|----------|
| `build_command_set_return_rate(hz)` | 1-100 | Частота обновления данных |
| `build_command_accel_enable(bool)` | true/false | Включить акселерометр |
| `build_command_gyro_enable(bool)` | true/false | Включить гироскоп |

## BLE Manager

### Основные методы

#### `scan(timeout=5.0) -> List[Dict]`
Сканирует BLE устройства в радиусе действия.

#### `connect(address: str) -> None`
Подключается к устройству по MAC-адресу.

#### `receive(callback) -> None`
Подписывается на уведомления от устройства.

#### `send(command: bytes) -> None`
Отправляет команду устройству.

#### `disconnect() -> None`
Отключается от устройства.

## Устранение неисправностей

### Частые проблемы

#### 1. "Python module not found"
**Решение:** Убедитесь, что .py файлы скопированы в папку с исполняемым файлом.

#### 2. "pybind11 not found" 
**Решение:** Проверьте подключение к интернету - CMake загружает pybind11 автоматически.

#### 3. "Qt6 not found"
**Решение:** Установите Qt6 или отключите сборку демо: `cmake -DBUILD_QT_DEMO=OFF ..`

#### 4. Парсер возвращает None
**Проверьте:**
- Размер данных (должен быть 20 байт)
- Заголовок пакета (0x55 0x61)
- Целостность данных

#### 5. BLE подключение не работает
**Проверьте:**
- Установлен ли модуль `bleak`: `pip install bleak`
- Включен ли Bluetooth
- Права доступа к Bluetooth (Linux)

### Отладка

Для включения подробного вывода ошибок Python:

```cpp
py::scoped_interpreter guard{};
py::exec("import sys; sys.stderr = sys.stdout");
```

Проверка импорта модулей:

```cpp
try {
    py::module test = py::module::import("sensor_parser");
    std::cout << "sensor_parser imported successfully" << std::endl;
} catch (const py::error_already_set& e) {
    std::cout << "Import error: " << e.what() << std::endl;
}
```

### Поддержка

Для получения поддержки:
1. Проверьте, что все файлы из папки `lib/` скопированы в папку сборки
2. Убедитесь в совместимости версий Python и pybind11
3. Проверьте права доступа к COM-портам и Bluetooth

