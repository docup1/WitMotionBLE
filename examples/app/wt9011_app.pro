QT += core gui widgets
CONFIG += c++17

SOURCES += \
    main.cpp \
    wt9011_interface.cpp \
    qcustomplot.cpp

HEADERS += \
    wt9011_interface.h \
    qcustomplot.h

# Имя итогового исполняемого файла
TARGET = wt9011_app

# Универсальный путь к Python и pybind11
contains(QMAKE_HOST.os, Darwin) {
    message("Building on macOS")

    PYTHON_VER = 3.12
    PYTHON_PREFIX = /opt/homebrew/Cellar/python@3.12/3.12.6

    INCLUDEPATH += $$PYTHON_PREFIX/Frameworks/Python.framework/Versions/$$PYTHON_VER/include/python$$PYTHON_VER
    LIBS += -L$$PYTHON_PREFIX/Frameworks/Python.framework/Versions/$$PYTHON_VER/lib -lpython$$PYTHON_VER
    QMAKE_RPATHDIR += $$PYTHON_PREFIX/Frameworks/Python.framework/Versions/$$PYTHON_VER/lib

    # Pybind11 может быть установлен brew install pybind11
    INCLUDEPATH += /opt/homebrew/include
} else {
    message("Building on Windows/MSYS2")

    PYTHON_VER = 3.12
    INCLUDEPATH += /mingw64/include/python$$PYTHON_VER
    LIBS += -L/mingw64/lib -lpython$$PYTHON_VER
    QMAKE_RPATHDIR += /mingw64/lib

    # Pybind11 из пакета mingw-w64-x86_64-pybind11
    INCLUDEPATH += /mingw64/include
}
