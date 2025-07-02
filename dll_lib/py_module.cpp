#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
#include <pybind11/eval.h>
#include <iostream>

namespace py = pybind11;

PYBIND11_EMBEDDED_MODULE(wt9011lib, m) {
    py::module_ sensor = py::module_::import("sensor_parser");
    py::module_ cmds = py::module_::import("sensor_commands");

    m.def("parse", [sensor](py::bytes data) {
        return sensor.attr("WT9011Parser").attr("parse")(data);
    });

    m.def("zeroing_cmd", [sensor]() {
        return sensor.attr("WT9011Parser").attr("build_command_zeroing")();
    });

    m.def("calibrate_cmd", [cmds]() {
        return cmds.attr("WT9011Commands").attr("build_command_calibration")();
    });
}
