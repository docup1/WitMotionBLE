# Документация библиотеки WT9011 C++ DLL

Библиотека для работы с датчиком WT9011 через Bluetooth Low Energy (BLE) с использованием C++ DLL, которая вызывает Python-функции через `pybind11`.

## Описание

Библиотека `wt9011_dll.dll` предоставляет C-стилевой интерфейс для взаимодействия с датчиком WT9011. Она использует существующие Python-модули (`ble_manager.py`, `sensor_parser.py`, `sensor_commands.py`) для управления BLE-соединением, отправки команд и обработки данных. Библиотека встраивает Python-интерпретатор с помощью `pybind11` и обеспечивает синхронное выполнение асинхронных Python-функций.

## Зависимости

- **C++ компилятор**: MSVC (Windows), GCC (Linux) или другой, совместимый с C++11.
- **Python**: Версия 3.8+ с установленными библиотеками:
  - `pybind11` (`pip install pybind11`)
  - `bleak` (`pip install bleak`)
- **CMake**: Для сборки DLL (версия 3.10+).
- **Python-модули**: `ble_manager.py`, `sensor_parser.py`, `sensor_commands.py` должны находиться в той же директории, что и DLL, или в пути Python.

## Сборка

1. **Установите зависимости**:
   ```bash
   pip install pybind11 bleak
   ```

2. **Создайте CMakeLists.txt**:
   ```cmake
   cmake_minimum_required(VERSION 3.10)
   project(WT9011DLL)

   find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
   find_package(pybind11 REQUIRED)

   add_library(wt9011_dll SHARED wt9011_dll.cpp)
   target_link_libraries(wt9011_dll PRIVATE pybind11::embed)
   set_target_properties(wt9011_dll PROPERTIES SUFFIX ".dll")
   ```

3. **Соберите DLL**:
   ```bash
   mkdir build
   cd build
   cmake ..
   cmake --build . --config Release
   ```

   После сборки файл `wt9011_dll.dll` появится в папке `build/Release` (на Windows).

4. **Убедитесь, что Python-файлы доступны**:
   Поместите `ble_manager.py`, `sensor_parser.py` и `sensor_commands.py` в ту же директорию, что и DLL, или добавьте их директорию в `sys.path`.

## Функции библиотеки

### Инициализация и очистка

- **wt9011_init() -> bool**
  Инициализирует Python-интерпретатор и загружает модули.  
  **Возвращает**: `true` при успехе, `false` при ошибке.  
  **Пример**:
  ```cpp
  if (!wt9011_init()) {
      std::cerr << "Ошибка инициализации\n";
  }
  ```

- **wt9011_cleanup() -> void**
  Освобождает ресурсы и завершает работу Python-интерпретатора.  
  **Пример**:
  ```cpp
  wt9011_cleanup();
  ```

### Сканирование и подключение

- **wt9011_scan(DeviceInfo* devices, int* count, float timeout) -> bool**
  Сканирует BLE-устройства.  
  **Параметры**:
  - `devices`: массив структур `DeviceInfo` для хранения имени и адреса устройств.
  - `count`: входное значение — максимальное количество устройств, выходное — число найденных устройств.
  - `timeout`: время сканирования в секундах (например, 5.0).  
  **Возвращает**: `true` при успехе, `false` при ошибке.  
  **Пример**:
  ```cpp
  DeviceInfo devices[10];
  int count = 10;
  if (wt9011_scan(devices, &count, 5.0)) {
      for (int i = 0; i < count; ++i) {
          std::cout << devices[i].name << " (" << devices[i].address << ")\n";
      }
  }
  ```

- **wt9011_connect(const char* address) -> bool**
  Подключается к устройству по MAC-адресу.  
  **Параметры**:
  - `address`: MAC-адрес в формате "XX:XX:XX:XX:XX:XX".  
  **Возвращает**: `true` при успехе, `false` при ошибке.  
  **Пример**:
  ```cpp
  wt9011_connect("12:34:56:78:9A:BC");
  ```

- **wt9011_disconnect() -> bool**
  Отключается от устройства.  
  **Возвращает**: `true` при успехе, `false` при ошибке.  
  **Пример**:
  ```cpp
  wt9011_disconnect();
  ```

### Получение данных

- **wt9011_receive(DataCallback callback) -> bool**
  Запускает получение данных от датчика.  
  **Параметры**:
  - `callback`: указатель на функцию обратного вызова с сигнатурой `void(const SensorData*)`, где `SensorData` содержит:
    ```cpp
    struct SensorData {
        struct Accel { float x, y, z; }; // м/с²
        struct Gyro { float x, y, z; };  // °/с
        struct Angle { float roll, pitch, yaw; }; // градусы
        Accel accel;
        Gyro gyro;
        Angle angle;
    };
    ```
  **Возвращает**: `true` при успехе, `false` при ошибке.  
  **Пример**:
  ```cpp
  void data_callback(const SensorData* data) {
      std::cout << "Accel: " << data->accel.x << ", " << data->accel.y << ", " << data->accel.z << "\n";
  }
  wt9011_receive(data_callback);
  ```

### Отправка команд

- **wt9011_send(const unsigned char* command, int length) -> bool**
  Отправляет сырую команду устройству.  
  **Параметры**:
  - `command`: массив байтов команды.
  - `length`: длина команды (обычно 4 байта).  
  **Возвращает**: `true` при успехе, `false` при ошибке.  
  **Пример**:
  ```cpp
  unsigned char cmd[] = {0xFF, 0xAA, 0x52, 0x00};
  wt9011_send(cmd, 4);
  ```

- **wt9011_zeroing() -> bool**
  Устанавливает текущие значения акселерометра и гироскопа как ноль.  
  **Возвращает**: `true` при успехе, `false` при ошибке.

- **wt9011_calibration() -> bool**
  Запускает калибровку акселерометра и гироскопа.  
  **Возвращает**: `true` при успехе, `false` при ошибке.

- **wt9011_save_settings() -> bool**
  Сохраняет текущие настройки в энергонезависимую память.  
  **Возвращает**: `true` при успехе, `false` при ошибке.

- **wt9011_factory_reset() -> bool**
  Сбрасывает настройки к заводским значениям.  
  **Возвращает**: `true` при успехе, `false` при ошибке.

- **wt9011_sleep() -> bool**
  Переводит устройство в спящий режим.  
  **Возвращает**: `true` при успехе, `false` при ошибке.

- **wt9011_wakeup() -> bool**
  Выводит устройство из спящего режима.  
  **Возвращает**: `true` при успехе, `false` при ошибке.

- **wt9011_set_return_rate(int rate_hz) -> bool**
  Устанавливает частоту передачи данных (1–100 Гц).  
  **Параметры**:
  - `rate_hz`: частота в Герцах.  
  **Возвращает**: `true` при успехе, `false` при ошибке.  
  **Пример**:
  ```cpp
  wt9011_set_return_rate(50);
  ```

- **wt9011_accel_enable(bool enable) -> bool**
  Включает (`true`) или выключает (`false`) акселерометр.  
  **Возвращает**: `true` при успехе, `false` при ошибке.

- **wt9011_gyro_enable(bool enable) -> bool**
  Включает (`true`) или выключает (`false`) гироскоп.  
  **Возвращает**: `true` при успехе, `false` при ошибке.

## Формат данных

### Структура `SensorData`

```cpp
struct SensorData {
    struct Accel { float x, y, z; }; // Ускорение в м/с² (±16g)
    struct Gyro { float x, y, z; };  // Угловая скорость в °/с (±2000°/с)
    struct Angle { 
        float roll;  // Крен (±180°)
        float pitch; // Тангаж (±180°)
        float yaw;   // Рысканье (-180°..+180°)
    };
    Accel accel;
    Gyro gyro;
    Angle angle;
};
```

### Формат команды

Команды состоят из 4 байтов, как описано в исходной документации Python:
- Байт 0: `0xFF` (стартовый байт)
- Байт 1: `0xAA` (адрес устройства)
- Байт 2: Код команды (например, `0x52` для обнуления)
- Байт 3: Параметр (например, `0x00` или частота в Гц)

## Пример использования

```cpp
#include <iostream>
#include "wt9011_dll.h"

void data_callback(const SensorData* data) {
    std::cout << "Ускорение: X=" << data->accel.x << " Y=" << data->accel.y << " Z=" << data->accel.z << "\n";
    std::cout << "Углы: Roll=" << data->angle.roll << " Pitch=" << data->angle.pitch << " Yaw=" << data->angle.yaw << "\n";
}

int main() {
    if (!wt9011_init()) {
        std::cerr << "Ошибка инициализации\n";
        return 1;
    }

    DeviceInfo devices[10];
    int count = 10;
    if (wt9011_scan(devices, &count, 5.0)) {
        for (int i = 0; i < count; ++i) {
            std::cout << "Устройство: " << devices[i].name << " (" << devices[i].address << ")\n";
        }
        if (count > 0) {
            if (wt9011_connect(devices[0].address.c_str())) {
                wt9011_zeroing();
                wt9011_set_return_rate(50);
                wt9011_receive(data_callback);
                Sleep(5000); // Ждать данные 5 секунд
                wt9011_disconnect();
            }
        }
    }

    wt9011_cleanup();
    return 0;
}
```

## Обработка ошибок

- Все функции возвращают `bool`, где `false` указывает на ошибку.
- Возможные причины ошибок:
  - Отсутствие Python или необходимых библиотек.
  - Ошибки BLE (например, устройство не найдено).
  - Неверные параметры (например, частота вне диапазона 1–100 Гц).

**Пример обработки ошибок**:
```cpp
if (!wt9011_connect("12:34:56:78:9A:BC")) {
    std::cerr << "Ошибка подключения\n";
}
```

## Ограничения

- **Асинхронность**: Функция `wt9011_receive` блокирует поток, так как выполняет асинхронный цикл `asyncio`. Для неблокирующей работы используйте отдельный поток.
- **Одно подключение**: Поддерживается подключение только к одному устройству за раз.
- **BLE**: Требуется поддержка Bluetooth на платформе (например, адаптер Bluetooth).
- **Дальность**: Обычно 10–50 метров, зависит от оборудования.

## Интеграция

### С другими C++ приложениями

1. Слинкуйте DLL или загрузите динамически (`LoadLibrary` на Windows).
2. Включите заголовочный файл с декларациями функций (например, `wt9011_dll.h`).
3. Вызовите `wt9011_init` перед использованием и `wt9011_cleanup` после.

### С GUI-приложениями

Для интеграции с GUI (например, Qt) рекомендуется запускать `wt9011_receive` в отдельном потоке, чтобы не блокировать интерфейс.

**Пример с Qt**:
```cpp
#include <QThread>
#include <QApplication>
#include "wt9011_dll.h"

class SensorThread : public QThread {
    void run() override {
        wt9011_receive(data_callback);
    }
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    wt9011_init();
    SensorThread thread;
    // Подключение и настройка
    thread.start();
    return app.exec();
}
```

## Производительность

- **Частота данных**: Рекомендуется 20–50 Гц.
- **Задержка**: Зависит от BLE-соединения и производительности Python.
- **Ресурсы**: Встроенный Python-интерпретатор увеличивает потребление памяти.