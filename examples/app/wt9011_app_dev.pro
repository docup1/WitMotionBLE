QT += core gui widgets
CONFIG += c++17

SOURCES += \
    main.cpp \
    wt9011_interface.cpp \
    qcustomplot.cpp

HEADERS += \
    wt9011_interface.h \
    qcustomplot.h

# Python config
PYTHON_VER = 3.12
PYTHON_PREFIX = /opt/homebrew/Cellar/python@3.12/3.12.6

# Пути к Python
INCLUDEPATH += $$PYTHON_PREFIX/Frameworks/Python.framework/Versions/3.12/include/python$$PYTHON_VER
LIBS += -L$$PYTHON_PREFIX/Frameworks/Python.framework/Versions/3.12/lib -lpython$$PYTHON_VER
QMAKE_RPATHDIR += $$PYTHON_PREFIX/Frameworks/Python.framework/Versions/3.12/lib

# Пути к pybind11
INCLUDEPATH += /opt/homebrew/include

# Имя итогового файла
TARGET = wt9011_app
