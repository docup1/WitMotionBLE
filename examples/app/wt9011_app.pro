QT += core gui widgets
CONFIG += c++17

SOURCES += \
    main.cpp \
    wt9011_interface.cpp \
    qcustomplot.cpp

HEADERS += \
    wt9011_interface.h \
    qcustomplot.h

# Detect platform
win32 {
    message(Building on Windows/MSYS2)

    PYTHON_VER = 3.12
    PYTHON_PREFIX = /mingw64

    INCLUDEPATH += $$PYTHON_PREFIX/include/python$$PYTHON_VER
    INCLUDEPATH += $$PYTHON_PREFIX/include/pybind11
    LIBS += -L$$PYTHON_PREFIX/lib -lpython$$PYTHON_VER
}
