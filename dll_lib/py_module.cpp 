#include <pybind11/embed.h>
#include <iostream>

namespace py = pybind11;
using namespace py::literals;

PYBIND11_EMBEDDED_MODULE(sensor_api, m) {
    py::module_ embedded = py::module_::import("embedded.sensor_parser");

    m.def("parse", [=](py::bytes data) {
        auto result = embedded.attr("WT9011Parser").attr("parse")(data);
        return result;
    });

    m.def("zeroing_cmd", []() {
        return py::module_::import("embedded.sensor_parser")
            .attr("WT9011Parser")
            .attr("build_command_zeroing")();
    });
}
