# ty, a collection of GUI and command-line tools to manage Teensy devices
#
# Distributed under the MIT license (see LICENSE.txt or http://opensource.org/licenses/MIT)
# Copyright (c) 2015 Niels Martignène <niels.martignene@gmail.com>

include(../ty_common.pri)

TEMPLATE = app
CONFIG -= qt
CONFIG += console

TARGET = tyc

LIBS = -lty

HEADERS = main.h

SOURCES = list.c \
    main.c \
    monitor.c \
    reset.c \
    upload.c
