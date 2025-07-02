#include <pybind11/embed.h>
#include <iostream>

namespace py = pybind11;

int main() {
    py::scoped_interpreter guard{};  // Встроенный интерпретатор

    py::module sensor = py::module::import("sensor_api");

    py::bytes fake_data("\x55\x61\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12", 20);
    auto result = sensor.attr("parse")(fake_data);
    std::cout << py::str(result).cast<std::string>() << std::endl;

    return 0;
}
