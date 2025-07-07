#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <memory>

namespace py = pybind11;

// Structure to hold device information
struct DeviceInfo {
    std::string name;
    std::string address;
};

// Structure to hold sensor data
struct SensorData {
    struct Accel { float x, y, z; };
    struct Gyro { float x, y, z; };
    struct Angle { float roll, pitch, yaw; };
    Accel accel;
    Gyro gyro;
    Angle angle;
};

// Global Python interpreter guard
static std::unique_ptr<py::scoped_interpreter> guard;

// Global Python objects
static py::object ble_manager_class;
static py::object ble_manager_instance;
static py::object parser_class;
static py::object commands_class;

// Callback function pointer type for sensor data
using DataCallback = void(*)(const SensorData*);

// Global callback for receiving data
static DataCallback global_callback = nullptr;

// Convert Python dict to SensorData
SensorData convert_to_sensor_data(py::dict data) {
    SensorData result;
    if (data.is_none()) {
        throw std::runtime_error("Invalid sensor data");
    }
    auto accel = data["accel"].cast<py::dict>();
    auto gyro = data["gyro"].cast<py::dict>();
    auto angle = data["angle"].cast<py::dict>();
    result.accel = { accel["x"].cast<float>(), accel["y"].cast<float>(), accel["z"].cast<float>() };
    result.gyro = { gyro["x"].cast<float>(), gyro["y"].cast<float>(), gyro["z"].cast<float>() };
    result.angle = { angle["roll"].cast<float>(), angle["pitch"].cast<float>(), angle["yaw"].cast<float>() };
    return result;
}

// Run Python coroutine synchronously
template<typename T>
T run_coroutine(py::object coro) {
    py::module_ asyncio = py::module_::import("asyncio");
    py::object loop = asyncio.attr("get_event_loop")();
    return loop.attr("run_until_complete")(coro).cast<T>();
}

// Initialize Python environment and load modules
extern "C" __declspec(dllexport) bool wt9011_init() {
    try {
        if (!guard) {
            guard = std::make_unique<py::scoped_interpreter>();
        }
        py::module_ sys = py::module_::import("sys");
        sys.attr("path").attr("append")("./lib"); // Add lib directory to Python path
        ble_manager_class = py::module_::import("ble_manager").attr("BLEManager");
        parser_class = py::module_::import("sensor_parser").attr("WT9011Parser");
        commands_class = py::module_::import("sensor_commands").attr("WT9011Commands");
        ble_manager_instance = ble_manager_class();
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

// Scan for BLE devices
extern "C" __declspec(dllexport) bool wt9011_scan(DeviceInfo* devices, int* count, float timeout) {
    try {
        if (!ble_manager_instance) {
            return false;
        }
        auto result = run_coroutine<std::vector<py::dict>>(ble_manager_instance.attr("scan")(timeout));
        int max_count = *count;
        *count = std::min(static_cast<int>(result.size()), max_count);
        for (int i = 0; i < *count; ++i) {
            devices[i].name = result[i]["name"].cast<std::string>();
            devices[i].address = result[i]["address"].cast<std::string>();
        }
        return true;
    } catch (const std::exception& e) {
        *count = 0;
        return false;
    }
}

// Connect to a BLE device
extern "C" __declspec(dllexport) bool wt9011_connect(const char* address) {
    try {
        if (!ble_manager_instance) {
            return false;
        }
        run_coroutine<void>(ble_manager_instance.attr("connect")(address));
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

// Callback wrapper for Python to call C++ callback
void data_callback_wrapper(py::bytes data) {
    if (global_callback) {
        py::gil_scoped_acquire acquire;
        auto parsed = parser_class.attr("parse")(data);
        if (!parsed.is_none()) {
            SensorData sensor_data = convert_to_sensor_data(parsed);
            global_callback(&sensor_data);
        }
    }
}

// Start receiving data
extern "C" __declspec(dllexport) bool wt9011_receive(DataCallback callback) {
    try {
        if (!ble_manager_instance) {
            return false;
        }
        global_callback = callback;
        run_coroutine<void>(ble_manager_instance.attr("receive")(py::cpp_function(data_callback_wrapper)));
        return true;
    } catch (const std::exception& e) {
        global_callback = nullptr;
        return false;
    }
}

// Send a command
extern "C" __declspec(dllexport) bool wt9011_send(const unsigned char* command, int length) {
    try {
        if (!ble_manager_instance) {
            return false;
        }
        std::string command_str(reinterpret_cast<const char*>(command), length);
        run_coroutine<void>(ble_manager_instance.attr("send")(py::bytes(command_str)));
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

// Disconnect from the device
extern "C" __declspec(dllexport) bool wt9011_disconnect() {
    try {
        if (!ble_manager_instance) {
            return false;
        }
        run_coroutine<void>(ble_manager_instance.attr("disconnect")());
        global_callback = nullptr;
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

// Convert Python bytes to const unsigned char* and length
bool send_pybytes(py::object py_bytes) {
    auto str = py_bytes.cast<std::string>();
    return wt9011_send(reinterpret_cast<const unsigned char*>(str.data()), static_cast<int>(str.size()));
}

// Command: Zeroing
extern "C" __declspec(dllexport) bool wt9011_zeroing() {
    try {
        return send_pybytes(commands_class.attr("build_command_zeroing")());
    } catch (const std::exception& e) {
        return false;
    }
}

// Command: Calibration
extern "C" __declspec(dllexport) bool wt9011_calibration() {
    try {
        return send_pybytes(commands_class.attr("build_command_calibration")());
    } catch (const std::exception& e) {
        return false;
    }
}

// Command: Save settings
extern "C" __declspec(dllexport) bool wt9011_save_settings() {
    try {
        return send_pybytes(commands_class.attr("build_command_save_settings")());
    } catch (const std::exception& e) {
        return false;
    }
}

// Command: Factory reset
extern "C" __declspec(dllexport) bool wt9011_factory_reset() {
    try {
        return send_pybytes(commands_class.attr("build_command_factory_reset")());
    } catch (const std::exception& e) {
        return false;
    }
}

// Command: Sleep
extern "C" __declspec(dllexport) bool wt9011_sleep() {
    try {
        return send_pybytes(commands_class.attr("build_command_sleep")());
    } catch (const std::exception& e) {
        return false;
    }
}

// Command: Wakeup
extern "C" __declspec(dllexport) bool wt9011_wakeup() {
    try {
        return send_pybytes(commands_class.attr("build_command_wakeup")());
    } catch (const std::exception& e) {
        return false;
    }
}

// Command: Set return rate
extern "C" __declspec(dllexport) bool wt9011_set_return_rate(int rate_hz) {
    try {
        return send_pybytes(commands_class.attr("build_command_set_return_rate")(rate_hz));
    } catch (const std::exception& e) {
        return false;
    }
}

// Command: Enable/disable accelerometer
extern "C" __declspec(dllexport) bool wt9011_accel_enable(bool enable) {
    try {
        return send_pybytes(commands_class.attr("build_command_accel_enable")(enable));
    } catch (const std::exception& e) {
        return false;
    }
}

// Command: Enable/disable gyroscope
extern "C" __declspec(dllexport) bool wt9011_gyro_enable(bool enable) {
    try {
        return send_pybytes(commands_class.attr("build_command_gyro_enable")(enable));
    } catch (const std::exception& e) {
        return false;
    }
}

// Cleanup Python environment
extern "C" __declspec(dllexport) void wt9011_cleanup() {
    ble_manager_instance = py::object();
    ble_manager_class = py::object();
    parser_class = py::object();
    commands_class = py::object();
    global_callback = nullptr;
    guard.reset();
}
