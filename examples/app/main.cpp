#ifdef slots
#undef slots
#endif
#include <Python.h>
#include <pybind11/embed.h>

#include <iostream>
#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QGroupBox>
#include <QTabWidget>
#include <QTextEdit>
#include <QCheckBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QDateTime>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <vector>
#include <memory>
#include "qcustomplot.h"
#include "wt9011_interface.h"

struct DataPoint {
    QDateTime timestamp;
    SensorData data;
};

class SensorDataWidget : public QWidget {
    Q_OBJECT
public:
    SensorDataWidget(QWidget* parent = nullptr) : QWidget(parent) {
        setupUi();
        dataHistory.reserve(1000);

        // Инициализация графиков
        accelXCurve = graphWidget->addGraph();
        accelYCurve = graphWidget->addGraph();
        accelZCurve = graphWidget->addGraph();

        accelXCurve->setPen(QPen(Qt::red));
        accelYCurve->setPen(QPen(Qt::green));
        accelZCurve->setPen(QPen(Qt::blue));

        accelXCurve->setName("Accel X");
        accelYCurve->setName("Accel Y");
        accelZCurve->setName("Accel Z");

        graphWidget->xAxis->setLabel("Время (с)");
        graphWidget->yAxis->setLabel("Значение");
        graphWidget->legend->setVisible(true);
        graphWidget->yAxis->setRange(-2, 2);

        updateTimer = new QTimer(this);
        connect(updateTimer, &QTimer::timeout, this, [this]() {
            if (!timeData.isEmpty()) {
                graphWidget->replot();
            }
        });
        updateTimer->start(100);

        startTime = QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000.0;
    }

    void updateData(const SensorData* data) {
    if (!data) return;  // Проверка на нулевой указатель

    double time = QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000.0 - startTime;

    // Всегда добавляем новые точки
    timeData.append(time);
    accelXData.append(data->accel.x);
    accelYData.append(data->accel.y);
    accelZData.append(data->accel.z);

    // Ограничиваем размер буфера данных
    if (timeData.size() > 100) {
        timeData.removeFirst();
        accelXData.removeFirst();
        accelYData.removeFirst();
        accelZData.removeFirst();
    }

    // Обновляем графики
    accelXCurve->setData(timeData, accelXData);
    accelYCurve->setData(timeData, accelYData);
    accelZCurve->setData(timeData, accelZData);

    // Автомасштабирование по оси X
    if (timeData.size() == 1) {
        double time = timeData.first();
        graphWidget->xAxis->setRange(time - 0.1, time + 0.1);
    } else {
        graphWidget->xAxis->setRange(timeData.first(), timeData.last());
    }

    // Автомасштабирование по оси Y
    double minY = qMin(qMin(accelXData.last(), accelYData.last()), accelZData.last());
    double maxY = qMax(qMax(accelXData.last(), accelYData.last()), accelZData.last());
    double padding = 0.5;
    graphWidget->yAxis->setRange(minY - padding, maxY + padding);

    graphWidget->replot();
}

    void stopUpdates() {
        updateTimer->stop();
    }

    void clearData() {
        timeData.clear();
        accelXData.clear();
        accelYData.clear();
        accelZData.clear();

        accelXCurve->setData(timeData, accelXData);
        accelYCurve->setData(timeData, accelYData);
        accelZCurve->setData(timeData, accelZData);

        graphWidget->xAxis->setRange(0, 10);
        graphWidget->yAxis->setRange(-2, 2);

        graphWidget->replot();
    }

    const QVector<DataPoint>& getDataHistory() const { return dataHistory; }

private:
    void setupUi() {
        QVBoxLayout* layout = new QVBoxLayout(this);

        QGroupBox* accelGroup = new QGroupBox("Акселерометр (g)");
        QGridLayout* accelLayout = new QGridLayout;
        accelXLabel = new QLabel("X: 0.00");
        accelYLabel = new QLabel("Y: 0.00");
        accelZLabel = new QLabel("Z: 0.00");
        accelLayout->addWidget(new QLabel("X:"), 0, 0);
        accelLayout->addWidget(accelXLabel, 0, 1);
        accelLayout->addWidget(new QLabel("Y:"), 1, 0);
        accelLayout->addWidget(accelYLabel, 1, 1);
        accelLayout->addWidget(new QLabel("Z:"), 2, 0);
        accelLayout->addWidget(accelZLabel, 2, 1);
        accelGroup->setLayout(accelLayout);

        QGroupBox* gyroGroup = new QGroupBox("Гироскоп (°/с)");
        QGridLayout* gyroLayout = new QGridLayout;
        gyroXLabel = new QLabel("X: 0.00");
        gyroYLabel = new QLabel("Y: 0.00");
        gyroZLabel = new QLabel("Z: 0.00");
        gyroLayout->addWidget(new QLabel("X:"), 0, 0);
        gyroLayout->addWidget(gyroXLabel, 0, 1);
        gyroLayout->addWidget(new QLabel("Y:"), 1, 0);
        gyroLayout->addWidget(gyroYLabel, 1, 1);
        gyroLayout->addWidget(new QLabel("Z:"), 2, 0);
        gyroLayout->addWidget(gyroZLabel, 2, 1);
        gyroGroup->setLayout(gyroLayout);

        QGroupBox* angleGroup = new QGroupBox("Углы поворота (°)");
        QGridLayout* angleLayout = new QGridLayout;
        rollLabel = new QLabel("Roll: 0.00");
        pitchLabel = new QLabel("Pitch: 0.00");
        yawLabel = new QLabel("Yaw: 0.00");
        angleLayout->addWidget(new QLabel("Roll:"), 0, 0);
        angleLayout->addWidget(rollLabel, 0, 1);
        angleLayout->addWidget(new QLabel("Pitch:"), 1, 0);
        angleLayout->addWidget(pitchLabel, 1, 1);
        angleLayout->addWidget(new QLabel("Yaw:"), 2, 0);
        angleLayout->addWidget(yawLabel, 2, 1);
        angleGroup->setLayout(angleLayout);

        QHBoxLayout* dataLayout = new QHBoxLayout;
        dataLayout->addWidget(accelGroup);
        dataLayout->addWidget(gyroGroup);
        dataLayout->addWidget(angleGroup);
        layout->addLayout(dataLayout);

        graphWidget = new QCustomPlot;
        accelXCurve = graphWidget->addGraph();
        accelYCurve = graphWidget->addGraph();
        accelZCurve = graphWidget->addGraph();
        accelXCurve->setPen(QPen(Qt::red));
        accelYCurve->setPen(QPen(Qt::green));
        accelZCurve->setPen(QPen(Qt::blue));
        graphWidget->xAxis->setLabel("Время (с)");
        graphWidget->yAxis->setLabel("Значение");
        graphWidget->legend->setVisible(true);
        accelXCurve->setName("Accel X");
        accelYCurve->setName("Accel Y");
        accelZCurve->setName("Accel Z");
        graphWidget->yAxis->setRange(-2, 2);
        layout->addWidget(graphWidget);

        startTime = QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000.0;
    }

    QLabel* accelXLabel;
    QLabel* accelYLabel;
    QLabel* accelZLabel;
    QLabel* gyroXLabel;
    QLabel* gyroYLabel;
    QLabel* gyroZLabel;
    QLabel* rollLabel;
    QLabel* pitchLabel;
    QLabel* yawLabel;
    QCustomPlot* graphWidget;
    QCPGraph* accelXCurve;
    QCPGraph* accelYCurve;
    QCPGraph* accelZCurve;
    QVector<double> timeData;
    QVector<double> accelXData;
    QVector<double> accelYData;
    QVector<double> accelZData;
    QVector<DataPoint> dataHistory;
    double startTime;
    QTimer* updateTimer;
};
class ControlPanel : public QWidget {
    Q_OBJECT
public:
    ControlPanel(QWidget* parent = nullptr) : QWidget(parent) {
        setupUi();
    }

signals:
    void commandSent(const QByteArray& command);

private slots:
    void sendZeroing() { emit commandSent(QByteArray()); wt9011_zeroing(); }
    void sendCalibration() { emit commandSent(QByteArray()); wt9011_calibration(); }
    void sendSaveSettings() { emit commandSent(QByteArray()); wt9011_save_settings(); }
    void sendFactoryReset() { emit commandSent(QByteArray()); wt9011_factory_reset(); }
    void sendSleep() { emit commandSent(QByteArray()); wt9011_sleep(); }
    void sendWakeup() { emit commandSent(QByteArray()); wt9011_wakeup(); }
    void sendSetRate() { emit commandSent(QByteArray()); wt9011_set_return_rate(rateSpinBox->value()); }
    void toggleAccel(int state) { emit commandSent(QByteArray()); wt9011_accel_enable(state == Qt::Checked); }
    void toggleGyro(int state) { emit commandSent(QByteArray()); wt9011_gyro_enable(state == Qt::Checked); }

private:
    void setupUi() {
        QVBoxLayout* layout = new QVBoxLayout(this);

        QGroupBox* basicGroup = new QGroupBox("Основные команды");
        QGridLayout* basicLayout = new QGridLayout;
        QPushButton* zeroBtn = new QPushButton("Обнулить положение");
        QPushButton* calibrateBtn = new QPushButton("Калибровка");
        QPushButton* saveBtn = new QPushButton("Сохранить настройки");
        QPushButton* resetBtn = new QPushButton("Сброс к заводским");
        connect(zeroBtn, &QPushButton::clicked, this, &ControlPanel::sendZeroing);
        connect(calibrateBtn, &QPushButton::clicked, this, &ControlPanel::sendCalibration);
        connect(saveBtn, &QPushButton::clicked, this, &ControlPanel::sendSaveSettings);
        connect(resetBtn, &QPushButton::clicked, this, &ControlPanel::sendFactoryReset);
        basicLayout->addWidget(zeroBtn, 0, 0);
        basicLayout->addWidget(calibrateBtn, 0, 1);
        basicLayout->addWidget(saveBtn, 1, 0);
        basicLayout->addWidget(resetBtn, 1, 1);
        basicGroup->setLayout(basicLayout);

        QGroupBox* powerGroup = new QGroupBox("Управление питанием");
        QHBoxLayout* powerLayout = new QHBoxLayout;
        QPushButton* sleepBtn = new QPushButton("Спящий режим");
        QPushButton* wakeupBtn = new QPushButton("Пробуждение");
        connect(sleepBtn, &QPushButton::clicked, this, &ControlPanel::sendSleep);
        connect(wakeupBtn, &QPushButton::clicked, this, &ControlPanel::sendWakeup);
        powerLayout->addWidget(sleepBtn);
        powerLayout->addWidget(wakeupBtn);
        powerGroup->setLayout(powerLayout);

        QGroupBox* rateGroup = new QGroupBox("Частота обновления");
        QHBoxLayout* rateLayout = new QHBoxLayout;
        rateLayout->addWidget(new QLabel("Частота (Гц):"));
        rateSpinBox = new QSpinBox;
        rateSpinBox->setRange(1, 100);
        rateSpinBox->setValue(50);
        QPushButton* setRateBtn = new QPushButton("Установить");
        connect(setRateBtn, &QPushButton::clicked, this, &ControlPanel::sendSetRate);
        rateLayout->addWidget(rateSpinBox);
        rateLayout->addWidget(setRateBtn);
        rateGroup->setLayout(rateLayout);

        QGroupBox* sensorsGroup = new QGroupBox("Управление датчиками");
        QGridLayout* sensorsLayout = new QGridLayout;
        accelCheckBox = new QCheckBox("Акселерометр");
        gyroCheckBox = new QCheckBox("Гироскоп");
        accelCheckBox->setChecked(true);
        gyroCheckBox->setChecked(true);
        connect(accelCheckBox, &QCheckBox::stateChanged, this, &ControlPanel::toggleAccel);
        connect(gyroCheckBox, &QCheckBox::stateChanged, this, &ControlPanel::toggleGyro);
        sensorsLayout->addWidget(accelCheckBox, 0, 0);
        sensorsLayout->addWidget(gyroCheckBox, 0, 1);
        sensorsGroup->setLayout(sensorsLayout);

        layout->addWidget(basicGroup);
        layout->addWidget(powerGroup);
        layout->addWidget(rateGroup);
        layout->addWidget(sensorsGroup);
        layout->addStretch();
    }

    QSpinBox* rateSpinBox;
    QCheckBox* accelCheckBox;
    QCheckBox* gyroCheckBox;
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr) : QMainWindow(parent), isConnected(false) {
        if (!wt9011_init()) {
            QMessageBox::critical(this, "Ошибка", "Не удалось инициализировать Python среду");
            QTimer::singleShot(0, this, &MainWindow::close);
            return;
        }
        setupUi();
        setupConnections();
        addLog("Приложение запущено");
    }

    ~MainWindow() {
        wt9011_disconnect();
        wt9011_cleanup();
    }

private slots:
    void scanDevices() {
        scanBtn->setEnabled(false);
        scanBtn->setText("Сканирование...");
        deviceCombo->clear();
        addLog("Начало сканирования BLE устройств...");

        DeviceInfo devices[100];
        int count = 100;
        if (wt9011_scan(devices, &count, 5.0)) {
            for (int i = 0; i < count; ++i) {
                QString name = devices[i].name.empty() ? "Неизвестное устройство" : QString::fromStdString(devices[i].name);
                deviceCombo->addItem(QString("%1 (%2)").arg(name).arg(QString::fromStdString(devices[i].address)), QString::fromStdString(devices[i].address));
            }
            connectBtn->setEnabled(true);
            addLog(QString("Найдено устройств: %1").arg(count));
        } else {
            addLog("Устройства не найдены");
        }
        scanBtn->setEnabled(true);
        scanBtn->setText("Сканировать");
    }

    void connectDevice() {
        if (!deviceCombo->currentData().toString().isEmpty()) {
            connectBtn->setEnabled(false);
            connectBtn->setText("Подключение...");
            QString address = deviceCombo->currentData().toString();
            addLog(QString("Подключение к устройству: %1").arg(address));

            if (wt9011_connect(address.toStdString().c_str())) {
                isConnected = true;
                statusLabel->setText("Статус: Подключено");
                statusLabel->setStyleSheet("color: green; font-weight: bold;");
                disconnectBtn->setEnabled(true);
                connectBtn->setText("Подключить");
                addLog("Успешно подключено к устройству");

                if (wt9011_receive([](const SensorData* data) {
                    QCoreApplication::postEvent(qApp->activeWindow(), new SensorDataEvent(*data));
                })) {
                    addLog("Начало приема данных");
                } else {
                    handleError("Ошибка начала приема данных");
                }
            } else {
                handleError("Ошибка подключения к устройству");
            }
        }
    }

    void disconnectDevice() {
    if (isConnected) {
        disconnectBtn->setEnabled(false);
        addLog("Отключение от устройства...");

        // Используем публичный метод для остановки таймера
        sensorDataWidget->stopUpdates();

        if (wt9011_disconnect()) {
            isConnected = false;
            statusLabel->setText("Статус: Не подключено");
            statusLabel->setStyleSheet("color: red; font-weight: bold;");
            connectBtn->setEnabled(true);
            connectBtn->setText("Подключить");
            sensorDataWidget->clearData();
            addLog("Отключено от устройства");
        } else {
            handleError("Ошибка отключения");
        }
    }
}
    void sendCommand(const QByteArray& command) {
        if (isConnected) {
            QString commandHex;
            for (unsigned char b : command) {
                commandHex += QString("%1 ").arg(b, 2, 16, QChar('0')).toUpper();
            }
            addLog(QString("Отправлена команда: %1").arg(commandHex.trimmed()));
        } else {
            QMessageBox::warning(this, "Предупреждение", "Устройство не подключено");
        }
    }

    void saveJson() {
        const auto& history = sensorDataWidget->getDataHistory();
        if (history.isEmpty()) {
            QMessageBox::information(this, "Информация", "Нет данных для сохранения");
            return;
        }

        QString filename = QFileDialog::getSaveFileName(
            this, "Сохранить данные",
            QString("sensor_data_%1.json").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
            "JSON files (*.json)"
        );

        if (!filename.isEmpty()) {
            QJsonArray jsonArray;
            for (const auto& point : history) {
                QJsonObject obj;
                obj["timestamp"] = point.timestamp.toString(Qt::ISODate);
                QJsonObject dataObj;
                QJsonObject accelObj;
                accelObj["x"] = point.data.accel.x;
                accelObj["y"] = point.data.accel.y;
                accelObj["z"] = point.data.accel.z;
                QJsonObject gyroObj;
                gyroObj["x"] = point.data.gyro.x;
                gyroObj["y"] = point.data.gyro.y;
                gyroObj["z"] = point.data.gyro.z;
                QJsonObject angleObj;
                angleObj["roll"] = point.data.angle.roll;
                angleObj["pitch"] = point.data.angle.pitch;
                angleObj["yaw"] = point.data.angle.yaw;
                dataObj["accel"] = accelObj;
                dataObj["gyro"] = gyroObj;
                dataObj["angle"] = angleObj;
                obj["data"] = dataObj;
                jsonArray.append(obj);
            }

            QFile file(filename);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(QJsonDocument(jsonArray).toJson());
                addLog(QString("Данные сохранены в файл: %1").arg(filename));
                QMessageBox::information(this, "Успех", "Данные успешно сохранены");
            } else {
                QString error = QString("Ошибка сохранения: %1").arg(file.errorString());
                addLog(error);
                QMessageBox::critical(this, "Ошибка", error);
            }
        }
    }

    void saveCsv() {
        const auto& history = sensorDataWidget->getDataHistory();
        if (history.isEmpty()) {
            QMessageBox::information(this, "Информация", "Нет данных для сохранения");
            return;
        }

        QString filename = QFileDialog::getSaveFileName(
            this, "Сохранить данные",
            QString("sensor_data_%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
            "CSV files (*.csv)"
        );

        if (!filename.isEmpty()) {
            QFile file(filename);
            if (file.open(QIODevice::WriteOnly)) {
                QTextStream stream(&file);
                stream << "timestamp,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,roll,pitch,yaw\n";
                for (const auto& point : history) {
                    stream << point.timestamp.toString(Qt::ISODate) << ","
                           << point.data.accel.x << "," << point.data.accel.y << "," << point.data.accel.z << ","
                           << point.data.gyro.x << "," << point.data.gyro.y << "," << point.data.gyro.z << ","
                           << point.data.angle.roll << "," << point.data.angle.pitch << "," << point.data.angle.yaw << "\n";
                }
                file.close();
                addLog(QString("Данные сохранены в файл: %1").arg(filename));
                QMessageBox::information(this, "Успех", "Данные успешно сохранены");
            } else {
                QString error = QString("Ошибка сохранения: %1").arg(file.errorString());
                addLog(error);
                QMessageBox::critical(this, "Ошибка", error);
            }
        }
    }

private:
    void setupUi() {
        setWindowTitle("WT9011 BLE Sensor Control");
        resize(1200, 800);
        QWidget* centralWidget = new QWidget;
        setCentralWidget(centralWidget);
        QVBoxLayout* mainLayout = new QVBoxLayout;

        QGroupBox* connectionGroup = new QGroupBox("Подключение к устройству");
        QHBoxLayout* connectionLayout = new QHBoxLayout;
        deviceCombo = new QComboBox;
        scanBtn = new QPushButton("Сканировать");
        connectBtn = new QPushButton("Подключить");
        disconnectBtn = new QPushButton("Отключить");
        connectBtn->setEnabled(false);
        disconnectBtn->setEnabled(false);
        connectionLayout->addWidget(new QLabel("Устройство:"));
        connectionLayout->addWidget(deviceCombo);
        connectionLayout->addWidget(scanBtn);
        connectionLayout->addWidget(connectBtn);
        connectionLayout->addWidget(disconnectBtn);
        connectionGroup->setLayout(connectionLayout);

        statusLabel = new QLabel("Статус: Не подключено");
        statusLabel->setStyleSheet("color: red; font-weight: bold;");

        tabWidget = new QTabWidget;
        sensorDataWidget = new SensorDataWidget;
        controlPanel = new ControlPanel;
        logWidget = new QTextEdit;
        logWidget->setReadOnly(true);
        tabWidget->addTab(sensorDataWidget, "Данные датчика");
        tabWidget->addTab(controlPanel, "Управление");
        tabWidget->addTab(logWidget, "Логи");

        QGroupBox* dataGroup = new QGroupBox("Экспорт данных");
        QHBoxLayout* dataLayout = new QHBoxLayout;
        QPushButton* saveJsonBtn = new QPushButton("Сохранить JSON");
        QPushButton* saveCsvBtn = new QPushButton("Сохранить CSV");
        connect(saveJsonBtn, &QPushButton::clicked, this, &MainWindow::saveJson);
        connect(saveCsvBtn, &QPushButton::clicked, this, &MainWindow::saveCsv);
        dataLayout->addWidget(saveJsonBtn);
        dataLayout->addWidget(saveCsvBtn);
        dataGroup->setLayout(dataLayout);

        mainLayout->addWidget(connectionGroup);
        mainLayout->addWidget(statusLabel);
        mainLayout->addWidget(tabWidget);
        mainLayout->addWidget(dataGroup);
        centralWidget->setLayout(mainLayout);
    }

    void setupConnections() {
        connect(scanBtn, &QPushButton::clicked, this, &MainWindow::scanDevices);
        connect(connectBtn, &QPushButton::clicked, this, &MainWindow::connectDevice);
        connect(disconnectBtn, &QPushButton::clicked, this, &MainWindow::disconnectDevice);
        connect(controlPanel, &ControlPanel::commandSent, this, &MainWindow::sendCommand);
    }

    void addLog(const QString& message) {
        QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
        logWidget->append(QString("[%1] %2").arg(timestamp).arg(message));
    }

    void handleError(const QString& errorMessage) {
        addLog(QString("ОШИБКА: %1").arg(errorMessage));
        QMessageBox::warning(this, "Ошибка", errorMessage);
        scanBtn->setEnabled(true);
        scanBtn->setText("Сканировать");
        connectBtn->setEnabled(true);
        connectBtn->setText("Подключить");
        disconnectBtn->setEnabled(false);
        isConnected = false;
    }

    class SensorDataEvent : public QEvent {
    public:
        SensorDataEvent(const SensorData& data) : QEvent(Type::User), sensorData(data) {}
        SensorData sensorData;
    };

    bool event(QEvent* event) override {
        if (event->type() == QEvent::User) {
            auto* sensorEvent = static_cast<SensorDataEvent*>(event);
            qDebug() << "[DEBUG] SensorDataEvent received: accel=("
                     << sensorEvent->sensorData.accel.x << ", "
                     << sensorEvent->sensorData.accel.y << ", "
                     << sensorEvent->sensorData.accel.z << ")";
            sensorDataWidget->updateData(&sensorEvent->sensorData);
            return true;
        }
        return QMainWindow::event(event);
    }

    QComboBox* deviceCombo;
    QPushButton* scanBtn;
    QPushButton* connectBtn;
    QPushButton* disconnectBtn;
    QLabel* statusLabel;
    QTabWidget* tabWidget;
    SensorDataWidget* sensorDataWidget;
    ControlPanel* controlPanel;
    QTextEdit* logWidget;
    bool isConnected;
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    MainWindow window;
    window.show();
    return app.exec();
}

#include "main.moc"