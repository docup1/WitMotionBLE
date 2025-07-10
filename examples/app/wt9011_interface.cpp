#include "wt9011_interface.h"
#include <pybind11/embed.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <thread>
#include <chrono>
#include <Python.h>

namespace py = pybind11;

static std::unique_ptr<py::scoped_interpreter> guard;
static py::object ble_manager_instance;
static py::object parser_class;
static py::object commands_class;
static DataCallback global_callback = nullptr;

void run_coroutine_void(py::object coro) {
    try {
        py::gil_scoped_acquire acquire;
        py::module_ asyncio = py::module_::import("asyncio");

        py::object loop;
        try {
            loop = asyncio.attr("get_event_loop")();
        } catch (const py::error_already_set&) {
            loop = asyncio.attr("new_event_loop")();
            asyncio.attr("set_event_loop")(loop);
        }

        loop.attr("run_until_complete")(coro);
    } catch (const py::error_already_set& e) {
        std::cerr << "[PYTHON ERROR] " << e.what() << std::endl;
        throw;
    }
}

template<typename T>
T run_coroutine(py::object coro) {
    try {
        py::gil_scoped_acquire acquire;
        py::module_ asyncio = py::module_::import("asyncio");

        py::object loop;
        try {
            loop = asyncio.attr("get_event_loop")();
        } catch (const py::error_already_set&) {
            loop = asyncio.attr("new_event_loop")();
            asyncio.attr("set_event_loop")(loop);
        }

        return loop.attr("run_until_complete")(coro).cast<T>();
    } catch (const py::error_already_set& e) {
        std::cerr << "[PYTHON ERROR] " << e.what() << std::endl;
        throw;
    }
}

extern "C" SensorData convert_to_sensor_data(py::dict data) {
    SensorData result;

    try {
        result.accel.x = data["accel"]["x"].cast<float>();
        result.accel.y = data["accel"]["y"].cast<float>();
        result.accel.z = data["accel"]["z"].cast<float>();

        result.gyro.x = data["gyro"]["x"].cast<float>();
        result.gyro.y = data["gyro"]["y"].cast<float>();
        result.gyro.z = data["gyro"]["z"].cast<float>();

        result.angle.roll  = data["angle"]["roll"].cast<float>();
        result.angle.pitch = data["angle"]["pitch"].cast<float>();
        result.angle.yaw   = data["angle"]["yaw"].cast<float>();
    } catch (const py::cast_error& e) {
        std::cerr << "[ERROR] Failed to convert sensor data: " << e.what() << std::endl;
    }

    return result;
}


extern "C" bool wt9011_init() {
    try {
        std::cout << "[INFO] Initializing Python..." << std::endl;

        if (!guard) {
            guard = std::make_unique<py::scoped_interpreter>();
        }

        std::cout << "[INFO] Python initialized." << std::endl;

        py::module_::import("sys").attr("path").attr("insert")(0, ".");

        py::module_ ble_manager_module = py::module_::import("ble_manager");
        ble_manager_instance = ble_manager_module.attr("ble_manager_instance");

        parser_class = py::module_::import("sensor_parser").attr("WT9011Parser");
        commands_class = py::module_::import("sensor_commands").attr("WT9011Commands");

        std::cout << "[INFO] All modules imported successfully." << std::endl;
        return true;

    } catch (const py::error_already_set& e) {
        std::cerr << "[PYTHON ERROR] " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "[EXCEPTION] " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "[UNKNOWN ERROR] Failed to initialize Python" << std::endl;
        return false;
    }
}

extern "C" bool wt9011_scan(DeviceInfo* devices, int* count, float timeout) {
    try {
        std::cout << "[INFO] Starting BLE scan with timeout " << timeout << "s" << std::endl;

        if (!ble_manager_instance) {
            std::cerr << "[ERROR] BLE manager instance not initialized" << std::endl;
            *count = 0;
            return false;
        }

        py::gil_scoped_acquire acquire;

        auto result = run_coroutine<std::vector<py::dict>>(
            ble_manager_instance.attr("scan")(timeout)
        );

        std::cout << "[INFO] Scan completed, found " << result.size() << " devices" << std::endl;

        int max_count = *count;
        *count = std::min(static_cast<int>(result.size()), max_count);

        for (int i = 0; i < *count; ++i) {
            devices[i].name = result[i]["name"].cast<std::string>();
            devices[i].address = result[i]["address"].cast<std::string>();
            std::cout << "[INFO] Device " << i << ": " << devices[i].name
                      << " (" << devices[i].address << ")" << std::endl;
        }

        return *count > 0;

    } catch (const py::error_already_set& e) {
        std::cerr << "[PYTHON ERROR] Scan failed: " << e.what() << std::endl;
        *count = 0;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Scan failed: " << e.what() << std::endl;
        *count = 0;
        return false;
    }
}

extern "C" bool wt9011_connect(const char* address) {
    try {
        if (!ble_manager_instance) {
            std::cerr << "[ERROR] BLE manager instance not initialized" << std::endl;
            return false;
        }

        std::cout << "[INFO] Connecting to " << address << std::endl;
        py::gil_scoped_acquire acquire;

        int max_attempts = 3;
        for (int attempt = 1; attempt <= max_attempts; ++attempt) {
            try {
                run_coroutine_void(ble_manager_instance.attr("connect")(address));
                std::cout << "[INFO] Connected to " << address << " on attempt " << attempt << std::endl;
                return true;
            } catch (const py::error_already_set& e) {
                std::cerr << "[PYTHON ERROR] Connection attempt " << attempt << " failed: " << e.what() << std::endl;
                if (attempt == max_attempts) throw;
                std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            }
        }
    } catch (const py::error_already_set& e) {
        std::cerr << "[PYTHON ERROR] Connection failed: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Connection failed: " << e.what() << std::endl;
        return false;
    }
    return false;
}

extern "C" void data_callback_wrapper(py::object data) {
    try {
        py::gil_scoped_acquire acquire;

        py::bytearray byte_array = data.cast<py::bytearray>();
        char* buffer = PyByteArray_AsString(byte_array.ptr());
        Py_ssize_t length = PyByteArray_Size(byte_array.ptr());

        // Логирование сырых данных
        std::string hex_dump;
        for (Py_ssize_t i = 0; i < length; ++i) {
            char buf[4];
            snprintf(buf, sizeof(buf), "%02X ", (unsigned char)buffer[i]);
            hex_dump += buf;
        }
        std::cerr << "[BLE] Received " << length << " bytes: " << hex_dump << std::endl;

        // Парсинг
        auto parsed = parser_class.attr("parse")(data);
        if (parsed.is_none()) {
            std::cerr << "[ERROR] Parser returned None" << std::endl;
            return;
        }

        // Конвертация и вызов callback
        SensorData sensor_data = convert_to_sensor_data(parsed);
        if (global_callback) {
            std::cerr << "[DEBUG] Sending data to callback" << std::endl;
            global_callback(&sensor_data);
        }
    } catch (const std::exception& e) {
        std::cerr << "[EXCEPTION] In callback: " << e.what() << std::endl;
    }
}

extern "C" bool wt9011_receive(DataCallback callback) {
    try {
        if (!ble_manager_instance) {
            std::cerr << "[ERROR] BLE manager instance not initialized" << std::endl;
            return false;
        }

        global_callback = callback;
        py::gil_scoped_acquire acquire;

        run_coroutine_void(ble_manager_instance.attr("receive")(
            py::cpp_function(data_callback_wrapper)
        ));

        std::cout << "[INFO] Started receiving data" << std::endl;
        return true;

    } catch (const py::error_already_set& e) {
        std::cerr << "[PYTHON ERROR] Receive failed: " << e.what() << std::endl;
        global_callback = nullptr;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Receive failed: " << e.what() << std::endl;
        global_callback = nullptr;
        return false;
    }
}

extern "C" bool wt9011_send(const unsigned char* command, int length) {
    try {
        if (!ble_manager_instance) {
            std::cerr << "[ERROR] BLE manager instance not initialized" << std::endl;
            return false;
        }

        py::gil_scoped_acquire acquire;
        std::string command_str(reinterpret_cast<const char*>(command), length);

        run_coroutine_void(ble_manager_instance.attr("send")(py::bytes(command_str)));

        std::cout << "[INFO] Sent command of length " << length << std::endl;
        return true;

    } catch (const py::error_already_set& e) {
        std::cerr << "[PYTHON ERROR] Send failed: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Send failed: " << e.what() << std::endl;
        return false;
    }
}

extern "C" bool wt9011_disconnect() {
    try {
        if (!ble_manager_instance) {
            std::cerr << "[ERROR] BLE manager instance not initialized" << std::endl;
            return false;
        }

        py::gil_scoped_acquire acquire;
        run_coroutine_void(ble_manager_instance.attr("disconnect")());

        std::cout << "[INFO] Disconnected" << std::endl;
        global_callback = nullptr;
        return true;

    } catch (const py::error_already_set& e) {
        std::cerr << "[PYTHON ERROR] Disconnect failed: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Disconnect failed: " << e.what() << std::endl;
        return false;
    }
}

extern "C" bool wt9011_zeroing() {
    try {
        py::gil_scoped_acquire acquire;
        py::bytes command = commands_class.attr("build_command_zeroing")();
        run_coroutine_void(ble_manager_instance.attr("send")(command));
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Zeroing failed: " << e.what() << std::endl;
        return false;
    }
}

extern "C" bool wt9011_calibration() {
    try {
        py::gil_scoped_acquire acquire;
        py::bytes command = commands_class.attr("build_command_calibration")();
        run_coroutine_void(ble_manager_instance.attr("send")(command));
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Calibration failed: " << e.what() << std::endl;
        return false;
    }
}

extern "C" bool wt9011_save_settings() {
    try {
        py::gil_scoped_acquire acquire;
        py::bytes command = commands_class.attr("build_command_save_settings")();
        run_coroutine_void(ble_manager_instance.attr("send")(command));
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Save settings failed: " << e.what() << std::endl;
        return false;
    }
}

extern "C" bool wt9011_factory_reset() {
    try {
        py::gil_scoped_acquire acquire;
        py::bytes command = commands_class.attr("build_command_factory_reset")();
        run_coroutine_void(ble_manager_instance.attr("send")(command));
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Factory reset failed: " << e.what() << std::endl;
        return false;
    }
}

extern "C" bool wt9011_sleep() {
    try {
        py::gil_scoped_acquire acquire;
        py::bytes command = commands_class.attr("build_command_sleep")();
        run_coroutine_void(ble_manager_instance.attr("send")(command));
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Sleep failed: " << e.what() << std::endl;
        return false;
    }
}

extern "C" bool wt9011_wakeup() {
    try {
        py::gil_scoped_acquire acquire;
        py::bytes command = commands_class.attr("build_command_wakeup")();
        run_coroutine_void(ble_manager_instance.attr("send")(command));
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Wakeup failed: " << e.what() << std::endl;
        return false;
    }
}

extern "C" bool wt9011_set_return_rate(int rate_hz) {
    try {
        py::gil_scoped_acquire acquire;
        py::bytes command = commands_class.attr("build_command_set_return_rate")(rate_hz);
        run_coroutine_void(ble_manager_instance.attr("send")(command));
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Set return rate failed: " << e.what() << std::endl;
        return false;
    }
}

extern "C" bool wt9011_accel_enable(bool enable) {
    try {
        py::gil_scoped_acquire acquire;
        py::bytes command = commands_class.attr("build_command_accel_enable")(enable);
        run_coroutine_void(ble_manager_instance.attr("send")(command));
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Accel enable failed: " << e.what() << std::endl;
        return false;
    }
}

extern "C" bool wt9011_gyro_enable(bool enable) {
    try {
        py::gil_scoped_acquire acquire;
        py::bytes command = commands_class.attr("build_command_gyro_enable")(enable);
        run_coroutine_void(ble_manager_instance.attr("send")(command));
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Gyro enable failed: " << e.what() << std::endl;
        return false;
    }
}

extern "C" void wt9011_cleanup() {
    std::cout << "[INFO] Cleaning up Python environment" << std::endl;

    try {
        if (ble_manager_instance) {
            py::gil_scoped_acquire acquire;
            ble_manager_instance = py::object();
        }
    } catch (...) {
        std::cerr << "[WARNING] Error cleaning up BLE manager" << std::endl;
    }

    parser_class = py::object();
    commands_class = py::object();
    global_callback = nullptr;

    if (guard) {
        guard.reset();
    }
}