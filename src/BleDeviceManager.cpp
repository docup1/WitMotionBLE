#include "BleDeviceManager.h"
#include <QDebug>

BleDeviceManager::BleDeviceManager(QObject* parent) : QObject(parent) {
    m_discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &BleDeviceManager::onDeviceDiscovered);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this, &BleDeviceManager::onScanFinished);
}

void BleDeviceManager::startScan() {
    m_devices.clear();
    m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}

void BleDeviceManager::onDeviceDiscovered(const QBluetoothDeviceInfo& info) {
    if (info.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration) {
        m_devices.append(info);
    }
}

void BleDeviceManager::onScanFinished() {
    emit devicesFound(m_devices);
}

void BleDeviceManager::connectToDevice(const QString& address) {
    auto device = std::find_if(m_devices.begin(), m_devices.end(), [&](const QBluetoothDeviceInfo& d){
        return d.address().toString() == address;
    });
    if (device == m_devices.end()) {
        emit errorOccurred("Device not found");
        return;
    }

    m_controller = std::make_unique<QLowEnergyController>(*device, this);
    connect(m_controller.get(), &QLowEnergyController::connected, this, &BleDeviceManager::onDeviceConnected);
    connect(m_controller.get(), &QLowEnergyController::disconnected, this, &BleDeviceManager::onDeviceDisconnected);
    connect(m_controller.get(), &QLowEnergyController::serviceDiscovered, this, &BleDeviceManager::onServiceDiscovered);
    connect(m_controller.get(), &QLowEnergyController::discoveryFinished, this, &BleDeviceManager::onServiceScanDone);

    m_controller->connectToDevice();
}

void BleDeviceManager::onDeviceConnected() {
    m_controller->discoverServices();
}

void BleDeviceManager::onDeviceDisconnected() {
    emit disconnected();
}

void BleDeviceManager::onServiceDiscovered(const QBluetoothUuid& uuid) {
    Q_UNUSED(uuid);
}

void BleDeviceManager::onServiceScanDone() {
    auto services = m_controller->services();
    for (const auto& uuid : services) {
        QLowEnergyService* service = m_controller->createServiceObject(uuid, this);
        if (!service) continue;

        connect(service, &QLowEnergyService::stateChanged, this, [this, service](QLowEnergyService::ServiceState s){
            if (s == QLowEnergyService::ServiceDiscovered) {
                auto chars = service->characteristics();
                for (auto& ch : chars) {
                    if (ch.properties() & QLowEnergyCharacteristic::Notify) {
                        m_notifyChar = ch;
                        connect(service, &QLowEnergyService::characteristicChanged, this, &BleDeviceManager::onCharacteristicChanged
