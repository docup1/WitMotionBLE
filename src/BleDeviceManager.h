#pragma once

#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QLowEnergyController>
#include <QLowEnergyCharacteristic>
#include <QLowEnergyService>

#include <functional>
#include <memory>

class BleDeviceManager : public QObject {
    Q_OBJECT
public:
    explicit BleDeviceManager(QObject* parent = nullptr);

    void startScan();
    void connectToDevice(const QString& address);

    using DataCallback = std::function<void(const QByteArray&)>;

    void setDataCallback(DataCallback cb);

signals:
    void devicesFound(const QList<QBluetoothDeviceInfo>& devices);
    void connected();
    void disconnected();
    void errorOccurred(const QString& error);

private slots:
    void onDeviceDiscovered(const QBluetoothDeviceInfo& info);
    void onScanFinished();

    void onDeviceConnected();
    void onDeviceDisconnected();
    void onServiceDiscovered(const QBluetoothUuid& uuid);
    void onServiceScanDone();
    void onCharacteristicChanged(const QLowEnergyCharacteristic& c, const QByteArray& value);

private:
    QBluetoothDeviceDiscoveryAgent* m_discoveryAgent;
    QList<QBluetoothDeviceInfo> m_devices;

    std::unique_ptr<QLowEnergyController> m_controller;
    QLowEnergyService* m_service = nullptr;
    QLowEnergyCharacteristic m_notifyChar;

    DataCallback m_dataCallback;
};
