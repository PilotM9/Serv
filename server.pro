QT += core network

CONFIG += c++11

SOURCES += \
    main.cpp \
    server.cpp \
    time_thread.cpp

HEADERS += \
    server.h \
    time_thread.h

TARGET = server
TEMPLATE = app
