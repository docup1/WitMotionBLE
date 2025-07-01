#include <QCoreApplication>
#include <QTimer>
#include <QTextStream>
#include <iostream>
#include "BleDeviceManager.h"
#include "WT9011Parser.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    BleDeviceManager manager;

    QTextStream cin(stdin);
    QTextStream cout(stdout);

    QObject::connect(&manager, &BleDeviceManager::devicesFound, [&](const QList<QBluetoothDeviceInfo>& devices){
        cout << "Найденные устройства BLE:\n";
        int i = 0;
        for (const auto& dev : devices) {
            cout << i << ": " << dev.name() << " [" << dev.address().toString() << "]\n";
            ++i;
        }
        cout << "Введите номер устройства для подключения: ";
        cout.flush();

        // Запускаем чтение номера устройства в отдельном таймере чтобы не блокировать основной цикл
        QTimer::singleShot(0, [&](){
            QString line = cin.readLine();
            bool ok = false;
            int idx = line.toInt(&ok);
            if (ok && idx >= 0 && idx < devices.size()) {
                manager.connectToDevice(devices[idx].address().toString());
            } else {
                cout << "Неверный номер устройства\n";
                app.quit();
            }
        });
    });

    QObject::connect(&manager, &BleDeviceManager::connected, [&](){
        cout << "Устройство подключено, ожидаем данные...\n";
    });

    QObject::connect(&manager, &BleDeviceManager::disconnected, [&](){
        cout << "Устройство отключено\n";
        app.quit();
    });

    QObject::connect(&manager, &BleDeviceManager::errorOccurred, [&](const QString& error){
        cout << "Ошибка: " << error << "\n";
        app.quit();
    });

    manager.setDataCallback([&](const QByteArray& data){
        if (data.size() == 20) {
            std::array<uint8_t, 20> arr;
            std::memcpy(arr.data(), data.constData(), 20);
            auto parsed = WT9011Parser::parse(arr);
            if (parsed) {
                cout << WT9011Parser::format(*parsed).c_str() << "\n";
            } else {
                cout << "Получены некорректные данные\n";
            }
        } else {
            cout << "Получены данные неправильного размера: " << data.size() << "\n";
        }
    });

    manager.startScan();

    return app.exec();
}
