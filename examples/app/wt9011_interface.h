#ifndef WT9011_INTERFACE_H
#define WT9011_INTERFACE_H

#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <memory>
#include <fmt/format.h>

namespace py = pybind11;

struct DeviceInfo {
    std::string name;
    std::string address;
};

struct SensorData {
    struct Accel { float x, y, z; };
    struct Gyro { float x, y, z; };
    struct Angle { float roll, pitch, yaw; };
    Accel accel;
    Gyro gyro;
    Angle angle;
};

using DataCallback = void(*)(const SensorData*);

extern "C" bool wt9011_init();
extern "C" bool wt9011_scan(DeviceInfo* devices, int* count, float timeout);
extern "C" bool wt9011_connect(const char* address);
extern "C" bool wt9011_receive(DataCallback callback);
extern "C" bool wt9011_send(const unsigned char* command, int length);
extern "C" bool wt9011_disconnect();
extern "C" bool wt9011_zeroing();
extern "C" bool wt9011_calibration();
extern "C" bool wt9011_save_settings();
extern "C" bool wt9011_factory_reset();
extern "C" bool wt9011_sleep();
extern "C" bool wt9011_wakeup();
extern "C" bool wt9011_set_return_rate(int rate_hz);
extern "C" bool wt9011_accel_enable(bool enable);
extern "C" bool wt9011_gyro_enable(bool enable);
extern "C" void wt9011_cleanup();
extern "C" SensorData convert_to_sensor_data(py::dict data);

#endif // WT9011_INTERFACE_H