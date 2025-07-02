import sys
import asyncio
import json
from datetime import datetime
from typing import Dict
from PyQt5.QtWidgets import (
    QApplication, QMainWindow, QVBoxLayout, QHBoxLayout, QWidget,
    QPushButton, QLabel, QTextEdit, QComboBox, QSpinBox, QGroupBox,
    QGridLayout, QTabWidget, QCheckBox, QMessageBox,
    QFileDialog
)
from PyQt5.QtCore import QThread, pyqtSignal, Qt
import pyqtgraph as pg

# Импортируем наши модули
from lib.ble_manager import BLEManager
from lib.sensor_commands import WT9011Commands
from lib.sensor_parser import WT9011Parser


class BLEWorker(QThread):
    """Worker thread для BLE операций"""

    device_found = pyqtSignal(list)
    connected = pyqtSignal(bool)
    data_received = pyqtSignal(dict)
    error_occurred = pyqtSignal(str)

    def __init__(self):
        super().__init__()
        self.ble_manager = BLEManager()
        self.parser = WT9011Parser()
        self.is_scanning = False
        self.is_receiving = False
        self.current_address = None

    async def scan_devices(self):
        """Сканирование BLE устройств"""
        try:
            devices = await self.ble_manager.scan(timeout=5.0)
            self.device_found.emit(devices)
        except Exception as e:
            self.error_occurred.emit(f"Ошибка сканирования: {str(e)}")

    async def connect_to_device(self, address: str):
        """Подключение к устройству"""
        try:
            await self.ble_manager.connect(address)
            self.current_address = address
            self.connected.emit(True)

            # Запуск приема данных
            await self.ble_manager.receive(self.on_data_received)
            self.is_receiving = True

        except Exception as e:
            self.error_occurred.emit(f"Ошибка подключения: {str(e)}")
            self.connected.emit(False)

    def on_data_received(self, data: bytearray):
        """Обработка полученных данных"""
        parsed_data = self.parser.parse(data)
        if parsed_data:
            self.data_received.emit(parsed_data)

    async def send_command(self, command: bytes):
        """Отправка команды устройству"""
        try:
            await self.ble_manager.send(command)
        except Exception as e:
            self.error_occurred.emit(f"Ошибка отправки команды: {str(e)}")

    async def disconnect_device(self):
        """Отключение от устройства"""
        try:
            self.is_receiving = False
            await self.ble_manager.disconnect()
            self.connected.emit(False)
        except Exception as e:
            self.error_occurred.emit(f"Ошибка отключения: {str(e)}")


class SensorDataWidget(QWidget):
    """Виджет для отображения данных датчика"""

    def __init__(self):
        super().__init__()
        self.init_ui()
        self.data_history = []

    def init_ui(self):
        layout = QVBoxLayout()

        # Группа акселерометра
        accel_group = QGroupBox("Акселерометр (g)")
        accel_layout = QGridLayout()

        self.accel_x_label = QLabel("X: 0.00")
        self.accel_y_label = QLabel("Y: 0.00")
        self.accel_z_label = QLabel("Z: 0.00")

        accel_layout.addWidget(QLabel("X:"), 0, 0)
        accel_layout.addWidget(self.accel_x_label, 0, 1)
        accel_layout.addWidget(QLabel("Y:"), 1, 0)
        accel_layout.addWidget(self.accel_y_label, 1, 1)
        accel_layout.addWidget(QLabel("Z:"), 2, 0)
        accel_layout.addWidget(self.accel_z_label, 2, 1)

        accel_group.setLayout(accel_layout)

        # Группа гироскопа
        gyro_group = QGroupBox("Гироскоп (°/с)")
        gyro_layout = QGridLayout()

        self.gyro_x_label = QLabel("X: 0.00")
        self.gyro_y_label = QLabel("Y: 0.00")
        self.gyro_z_label = QLabel("Z: 0.00")

        gyro_layout.addWidget(QLabel("X:"), 0, 0)
        gyro_layout.addWidget(self.gyro_x_label, 0, 1)
        gyro_layout.addWidget(QLabel("Y:"), 1, 0)
        gyro_layout.addWidget(self.gyro_y_label, 1, 1)
        gyro_layout.addWidget(QLabel("Z:"), 2, 0)
        gyro_layout.addWidget(self.gyro_z_label, 2, 1)

        gyro_group.setLayout(gyro_layout)

        # Группа углов
        angle_group = QGroupBox("Углы поворота (°)")
        angle_layout = QGridLayout()

        self.roll_label = QLabel("Roll: 0.00")
        self.pitch_label = QLabel("Pitch: 0.00")
        self.yaw_label = QLabel("Yaw: 0.00")

        angle_layout.addWidget(QLabel("Roll:"), 0, 0)
        angle_layout.addWidget(self.roll_label, 0, 1)
        angle_layout.addWidget(QLabel("Pitch:"), 1, 0)
        angle_layout.addWidget(self.pitch_label, 1, 1)
        angle_layout.addWidget(QLabel("Yaw:"), 2, 0)
        angle_layout.addWidget(self.yaw_label, 2, 1)

        angle_group.setLayout(angle_layout)

        # Добавляем группы в основной layout
        data_layout = QHBoxLayout()
        data_layout.addWidget(accel_group)
        data_layout.addWidget(gyro_group)
        data_layout.addWidget(angle_group)

        layout.addLayout(data_layout)

        # График в реальном времени
        self.graph_widget = pg.PlotWidget(title="Данные в реальном времени")
        self.graph_widget.setLabel('left', 'Значение')
        self.graph_widget.setLabel('bottom', 'Время (с)')
        self.graph_widget.addLegend()

        # Линии для графика
        self.accel_x_curve = self.graph_widget.plot(pen='r', name='Accel X')
        self.accel_y_curve = self.graph_widget.plot(pen='g', name='Accel Y')
        self.accel_z_curve = self.graph_widget.plot(pen='b', name='Accel Z')

        layout.addWidget(self.graph_widget)

        self.setLayout(layout)

        # Данные для графика
        self.time_data = []
        self.accel_x_data = []
        self.accel_y_data = []
        self.accel_z_data = []
        self.start_time = datetime.now()

    def update_data(self, data: Dict):
        """Обновление отображаемых данных"""
        # Обновляем текстовые поля
        accel = data.get('accel', {})
        gyro = data.get('gyro', {})
        angle = data.get('angle', {})

        self.accel_x_label.setText(f"X: {accel.get('x', 0):.2f}")
        self.accel_y_label.setText(f"Y: {accel.get('y', 0):.2f}")
        self.accel_z_label.setText(f"Z: {accel.get('z', 0):.2f}")

        self.gyro_x_label.setText(f"X: {gyro.get('x', 0):.2f}")
        self.gyro_y_label.setText(f"Y: {gyro.get('y', 0):.2f}")
        self.gyro_z_label.setText(f"Z: {gyro.get('z', 0):.2f}")

        self.roll_label.setText(f"Roll: {angle.get('roll', 0):.2f}")
        self.pitch_label.setText(f"Pitch: {angle.get('pitch', 0):.2f}")
        self.yaw_label.setText(f"Yaw: {angle.get('yaw', 0):.2f}")

        # Обновляем график
        current_time = (datetime.now() - self.start_time).total_seconds()
        self.time_data.append(current_time)
        self.accel_x_data.append(accel.get('x', 0))
        self.accel_y_data.append(accel.get('y', 0))
        self.accel_z_data.append(accel.get('z', 0))

        # Ограничиваем количество точек на графике
        if len(self.time_data) > 100:
            self.time_data = self.time_data[-100:]
            self.accel_x_data = self.accel_x_data[-100:]
            self.accel_y_data = self.accel_y_data[-100:]
            self.accel_z_data = self.accel_z_data[-100:]

        # Обновляем кривые
        self.accel_x_curve.setData(self.time_data, self.accel_x_data)
        self.accel_y_curve.setData(self.time_data, self.accel_y_data)
        self.accel_z_curve.setData(self.time_data, self.accel_z_data)

        # Сохраняем данные для истории
        data_point = {
            'timestamp': datetime.now().isoformat(),
            'data': data
        }
        self.data_history.append(data_point)


class ControlPanel(QWidget):
    """Панель управления датчиком"""

    command_sent = pyqtSignal(bytes)

    def __init__(self):
        super().__init__()
        self.commands = WT9011Commands()
        self.init_ui()

    def init_ui(self):
        layout = QVBoxLayout()

        # Основные команды
        basic_group = QGroupBox("Основные команды")
        basic_layout = QGridLayout()

        self.zero_btn = QPushButton("Обнулить положение")
        self.calibrate_btn = QPushButton("Калибровка")
        self.save_btn = QPushButton("Сохранить настройки")
        self.reset_btn = QPushButton("Сброс к заводским")

        self.zero_btn.clicked.connect(self.send_zeroing)
        self.calibrate_btn.clicked.connect(self.send_calibration)
        self.save_btn.clicked.connect(self.send_save_settings)
        self.reset_btn.clicked.connect(self.send_factory_reset)

        basic_layout.addWidget(self.zero_btn, 0, 0)
        basic_layout.addWidget(self.calibrate_btn, 0, 1)
        basic_layout.addWidget(self.save_btn, 1, 0)
        basic_layout.addWidget(self.reset_btn, 1, 1)

        basic_group.setLayout(basic_layout)

        # Управление питанием
        power_group = QGroupBox("Управление питанием")
        power_layout = QHBoxLayout()

        self.sleep_btn = QPushButton("Спящий режим")
        self.wakeup_btn = QPushButton("Пробуждение")

        self.sleep_btn.clicked.connect(self.send_sleep)
        self.wakeup_btn.clicked.connect(self.send_wakeup)

        power_layout.addWidget(self.sleep_btn)
        power_layout.addWidget(self.wakeup_btn)
        power_group.setLayout(power_layout)

        # Настройки частоты
        rate_group = QGroupBox("Частота обновления")
        rate_layout = QHBoxLayout()

        rate_layout.addWidget(QLabel("Частота (Гц):"))
        self.rate_spinbox = QSpinBox()
        self.rate_spinbox.setRange(1, 100)
        self.rate_spinbox.setValue(50)

        self.set_rate_btn = QPushButton("Установить")
        self.set_rate_btn.clicked.connect(self.send_set_rate)

        rate_layout.addWidget(self.rate_spinbox)
        rate_layout.addWidget(self.set_rate_btn)
        rate_group.setLayout(rate_layout)

        # Управление датчиками
        sensors_group = QGroupBox("Управление датчиками")
        sensors_layout = QGridLayout()

        self.accel_checkbox = QCheckBox("Акселерометр")
        self.gyro_checkbox = QCheckBox("Гироскоп")

        self.accel_checkbox.setChecked(True)
        self.gyro_checkbox.setChecked(True)

        self.accel_checkbox.stateChanged.connect(self.toggle_accel)
        self.gyro_checkbox.stateChanged.connect(self.toggle_gyro)

        sensors_layout.addWidget(self.accel_checkbox, 0, 0)
        sensors_layout.addWidget(self.gyro_checkbox, 0, 1)

        sensors_group.setLayout(sensors_layout)

        # Добавляем все группы
        layout.addWidget(basic_group)
        layout.addWidget(power_group)
        layout.addWidget(rate_group)
        layout.addWidget(sensors_group)
        layout.addStretch()

        self.setLayout(layout)

    def send_zeroing(self):
        command = self.commands.build_command_zeroing()
        self.command_sent.emit(command)

    def send_calibration(self):
        command = self.commands.build_command_calibration()
        self.command_sent.emit(command)

    def send_save_settings(self):
        command = self.commands.build_command_save_settings()
        self.command_sent.emit(command)

    def send_factory_reset(self):
        command = self.commands.build_command_factory_reset()
        self.command_sent.emit(command)

    def send_sleep(self):
        command = self.commands.build_command_sleep()
        self.command_sent.emit(command)

    def send_wakeup(self):
        command = self.commands.build_command_wakeup()
        self.command_sent.emit(command)

    def send_set_rate(self):
        rate = self.rate_spinbox.value()
        command = self.commands.build_command_set_return_rate(rate)
        self.command_sent.emit(command)

    def toggle_accel(self, state):
        command = self.commands.build_command_accel_enable(state == Qt.Checked)
        self.command_sent.emit(command)

    def toggle_gyro(self, state):
        command = self.commands.build_command_gyro_enable(state == Qt.Checked)
        self.command_sent.emit(command)

    def toggle_mag(self, state):
        command = self.commands.build_command_mag_enable(state == Qt.Checked)
        self.command_sent.emit(command)


class MainWindow(QMainWindow):
    """Главное окно приложения"""

    def __init__(self):
        super().__init__()
        self.worker = BLEWorker()
        self.is_connected = False
        self.init_ui()
        self.setup_connections()

    def init_ui(self):
        self.setWindowTitle("WT9011 BLE Sensor Control")
        self.setGeometry(100, 100, 1200, 800)

        central_widget = QWidget()
        self.setCentralWidget(central_widget)

        # Основной layout
        main_layout = QVBoxLayout()

        # Панель подключения
        connection_group = QGroupBox("Подключение к устройству")
        connection_layout = QHBoxLayout()

        self.device_combo = QComboBox()
        self.scan_btn = QPushButton("Сканировать")
        self.connect_btn = QPushButton("Подключить")
        self.disconnect_btn = QPushButton("Отключить")

        self.connect_btn.setEnabled(False)
        self.disconnect_btn.setEnabled(False)

        connection_layout.addWidget(QLabel("Устройство:"))
        connection_layout.addWidget(self.device_combo)
        connection_layout.addWidget(self.scan_btn)
        connection_layout.addWidget(self.connect_btn)
        connection_layout.addWidget(self.disconnect_btn)

        connection_group.setLayout(connection_layout)

        # Статус подключения
        self.status_label = QLabel("Статус: Не подключено")
        self.status_label.setStyleSheet("color: red; font-weight: bold;")

        # Табы для разных функций
        self.tab_widget = QTabWidget()

        # Таб с данными датчика
        self.sensor_data_widget = SensorDataWidget()
        self.tab_widget.addTab(self.sensor_data_widget, "Данные датчика")

        # Таб с панелью управления
        self.control_panel = ControlPanel()
        self.tab_widget.addTab(self.control_panel, "Управление")

        # Таб с логами
        self.log_widget = QTextEdit()
        self.log_widget.setReadOnly(True)
        self.tab_widget.addTab(self.log_widget, "Логи")

        # Кнопки для сохранения данных
        data_group = QGroupBox("Экспорт данных")
        data_layout = QHBoxLayout()

        self.save_json_btn = QPushButton("Сохранить JSON")
        self.save_csv_btn = QPushButton("Сохранить CSV")

        self.save_json_btn.clicked.connect(self.save_data_json)
        self.save_csv_btn.clicked.connect(self.save_data_csv)

        data_layout.addWidget(self.save_json_btn)
        data_layout.addWidget(self.save_csv_btn)
        data_group.setLayout(data_layout)

        # Сборка интерфейса
        main_layout.addWidget(connection_group)
        main_layout.addWidget(self.status_label)
        main_layout.addWidget(self.tab_widget)
        main_layout.addWidget(data_group)

        central_widget.setLayout(main_layout)

        # Добавляем в лог начальное сообщение
        self.add_log("Приложение запущено")

    def setup_connections(self):
        """Настройка сигналов и слотов"""
        self.scan_btn.clicked.connect(self.scan_devices)
        self.connect_btn.clicked.connect(self.connect_device)
        self.disconnect_btn.clicked.connect(self.disconnect_device)

        # Сигналы от worker'а
        self.worker.device_found.connect(self.on_devices_found)
        self.worker.connected.connect(self.on_connection_changed)
        self.worker.data_received.connect(self.on_data_received)
        self.worker.error_occurred.connect(self.on_error)

        # Сигналы от панели управления
        self.control_panel.command_sent.connect(self.send_command)

    def scan_devices(self):
        """Запуск сканирования устройств"""
        self.scan_btn.setEnabled(False)
        self.scan_btn.setText("Сканирование...")
        self.device_combo.clear()
        self.add_log("Начало сканирования BLE устройств...")

        # Запускаем сканирование в отдельном потоке
        asyncio.create_task(self.worker.scan_devices())

    def on_devices_found(self, devices):
        """Обработка найденных устройств"""
        self.scan_btn.setEnabled(True)
        self.scan_btn.setText("Сканировать")

        if not devices:
            self.add_log("Устройства не найдены")
            return

        for device in devices:
            name = device['name'] if device['name'] else "Неизвестное устройство"
            item_text = f"{name} ({device['address']})"
            self.device_combo.addItem(item_text, device['address'])

        self.connect_btn.setEnabled(True)
        self.add_log(f"Найдено устройств: {len(devices)}")

    def connect_device(self):
        """Подключение к выбранному устройству"""
        if self.device_combo.currentData():
            address = self.device_combo.currentData()
            self.connect_btn.setEnabled(False)
            self.connect_btn.setText("Подключение...")
            self.add_log(f"Подключение к устройству: {address}")

            asyncio.create_task(self.worker.connect_to_device(address))

    def disconnect_device(self):
        """Отключение от устройства"""
        if self.is_connected:
            self.disconnect_btn.setEnabled(False)
            self.add_log("Отключение от устройства...")
            asyncio.create_task(self.worker.disconnect_device())

    def on_connection_changed(self, connected):
        """Обработка изменения состояния подключения"""
        self.is_connected = connected

        if connected:
            self.status_label.setText("Статус: Подключено")
            self.status_label.setStyleSheet("color: green; font-weight: bold;")
            self.disconnect_btn.setEnabled(True)
            self.connect_btn.setText("Подключить")
            self.add_log("Успешно подключено к устройству")
        else:
            self.status_label.setText("Статус: Не подключено")
            self.status_label.setStyleSheet("color: red; font-weight: bold;")
            self.connect_btn.setEnabled(True)
            self.connect_btn.setText("Подключить")
            self.disconnect_btn.setEnabled(False)
            self.add_log("Отключено от устройства")

    def on_data_received(self, data):
        """Обработка полученных данных от датчика"""
        self.sensor_data_widget.update_data(data)

    def on_error(self, error_message):
        """Обработка ошибок"""
        self.add_log(f"ОШИБКА: {error_message}")
        QMessageBox.warning(self, "Ошибка", error_message)

        # Сброс состояния кнопок при ошибке
        self.scan_btn.setEnabled(True)
        self.scan_btn.setText("Сканировать")
        self.connect_btn.setEnabled(True)
        self.connect_btn.setText("Подключить")

    def send_command(self, command):
        """Отправка команды устройству"""
        if self.is_connected:
            asyncio.create_task(self.worker.send_command(command))
            command_hex = ' '.join(f'{b:02X}' for b in command)
            self.add_log(f"Отправлена команда: {command_hex}")
        else:
            QMessageBox.warning(self, "Предупреждение", "Устройство не подключено")

    def add_log(self, message):
        """Добавление сообщения в лог"""
        timestamp = datetime.now().strftime("%H:%M:%S")
        log_message = f"[{timestamp}] {message}"
        self.log_widget.append(log_message)

    def save_data_json(self):
        """Сохранение данных в JSON файл"""
        if not self.sensor_data_widget.data_history:
            QMessageBox.information(self, "Информация", "Нет данных для сохранения")
            return

        filename, _ = QFileDialog.getSaveFileName(
            self, "Сохранить данные", f"sensor_data_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json",
            "JSON files (*.json)"
        )

        if filename:
            try:
                with open(filename, 'w', encoding='utf-8') as f:
                    json.dump(self.sensor_data_widget.data_history, f, indent=2, ensure_ascii=False)
                self.add_log(f"Данные сохранены в файл: {filename}")
                QMessageBox.information(self, "Успех", "Данные успешно сохранены")
            except Exception as e:
                error_msg = f"Ошибка сохранения: {str(e)}"
                self.add_log(error_msg)
                QMessageBox.critical(self, "Ошибка", error_msg)

    def save_data_csv(self):
        """Сохранение данных в CSV файл"""
        if not self.sensor_data_widget.data_history:
            QMessageBox.information(self, "Информация", "Нет данных для сохранения")
            return

        filename, _ = QFileDialog.getSaveFileName(
            self, "Сохранить данные", f"sensor_data_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv",
            "CSV files (*.csv)"
        )

        if filename:
            try:
                import csv
                with open(filename, 'w', newline='', encoding='utf-8') as f:
                    writer = csv.writer(f)

                    # Заголовки
                    headers = [
                        'timestamp', 'accel_x', 'accel_y', 'accel_z',
                        'gyro_x', 'gyro_y', 'gyro_z', 'roll', 'pitch', 'yaw'
                    ]
                    writer.writerow(headers)

                    # Данные
                    for entry in self.sensor_data_widget.data_history:
                        data = entry['data']
                        row = [
                            entry['timestamp'],
                            data.get('accel', {}).get('x', 0),
                            data.get('accel', {}).get('y', 0),
                            data.get('accel', {}).get('z', 0),
                            data.get('gyro', {}).get('x', 0),
                            data.get('gyro', {}).get('y', 0),
                            data.get('gyro', {}).get('z', 0),
                            data.get('angle', {}).get('roll', 0),
                            data.get('angle', {}).get('pitch', 0),
                            data.get('angle', {}).get('yaw', 0)
                        ]
                        writer.writerow(row)

                self.add_log(f"Данные сохранены в файл: {filename}")
                QMessageBox.information(self, "Успех", "Данные успешно сохранены")
            except Exception as e:
                error_msg = f"Ошибка сохранения: {str(e)}"
                self.add_log(error_msg)
                QMessageBox.critical(self, "Ошибка", error_msg)


def main():
    app = QApplication(sys.argv)

    # Настраиваем event loop для asyncio
    import qasync
    loop = qasync.QEventLoop(app)
    asyncio.set_event_loop(loop)

    window = MainWindow()
    window.show()

    with loop:
        loop.run_forever()


if __name__ == "__main__":
    main()