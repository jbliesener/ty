/**
 * ty, command-line program to manage Teensy devices
 * http://github.com/Koromix/ty
 * Copyright (C) 2014 Niels Martignène
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TY_DEVICE_H
#define TY_DEVICE_H

#include "common.h"

TY_C_BEGIN

struct ty_descriptor_set;

typedef struct ty_device_monitor ty_device_monitor;
typedef struct ty_device ty_device;
typedef struct ty_handle ty_handle;

typedef enum ty_device_type {
    TY_DEVICE_HID,
    TY_DEVICE_SERIAL
} ty_device_type;

typedef enum ty_device_event {
    TY_DEVICE_EVENT_ADDED,
    TY_DEVICE_EVENT_REMOVED
} ty_device_event;

typedef struct ty_hid_descriptor {
    uint16_t usage;
    uint16_t usage_page;
} ty_hid_descriptor;

typedef int ty_device_callback_func(ty_device *dev, ty_device_event event, void *udata);

enum {
    TY_SERIAL_CSIZE_MASK   = 0x3,
    TY_SERIAL_7BITS_CSIZE  = 0x1,
    TY_SERIAL_6BITS_CSIZE  = 0x2,
    TY_SERIAL_5BITS_CSIZE  = 0x3,

    TY_SERIAL_PARITY_MASK  = 0xC,
    TY_SERIAL_ODD_PARITY   = 0x4,
    TY_SERIAL_EVEN_PARITY  = 0x8,

    TY_SERIAL_STOP_MASK    = 0x10,
    TY_SERIAL_2BITS_STOP   = 0x10,

    TY_SERIAL_FLOW_MASK    = 0x60,
    TY_SERIAL_XONXOFF_FLOW = 0x20,
    TY_SERIAL_RTSCTS_FLOW  = 0x40,

    TY_SERIAL_CLOSE_MASK   = 0x80,
    TY_SERIAL_NOHUP_CLOSE  = 0x80,
};

int ty_device_monitor_new(ty_device_monitor **rmonitor);
void ty_device_monitor_free(ty_device_monitor *monitor);

int _ty_device_monitor_init(ty_device_monitor *monitor);
void _ty_device_monitor_release(ty_device_monitor *monitor);

void ty_device_monitor_set_udata(ty_device_monitor *monitor, void *udata);
void *ty_device_monitor_get_udata(ty_device_monitor *monitor);

void ty_device_monitor_get_descriptors(ty_device_monitor *monitor, struct ty_descriptor_set *set, int id);

int ty_device_monitor_register_callback(ty_device_monitor *monitor, ty_device_callback_func *f, void *udata);
void ty_device_monitor_deregister_callback(ty_device_monitor *monitor, int id);

int _ty_device_monitor_add(ty_device_monitor *monitor, ty_device *dev);
void _ty_device_monitor_remove(ty_device_monitor *monitor, const char *key);

int ty_device_monitor_refresh(ty_device_monitor *monitor);

int ty_device_monitor_list(ty_device_monitor *monitor, ty_device_callback_func *f, void *udata);

ty_device *ty_device_ref(ty_device *dev);
void ty_device_unref(ty_device *dev);

void ty_device_set_udata(ty_device *dev, void *udata);
void *ty_device_get_udata(ty_device *dev);

int ty_device_open(ty_device *dev, bool block, ty_handle **rh);
void ty_device_close(ty_handle *h);

void ty_device_get_descriptors(ty_handle *h, struct ty_descriptor_set *set, int id);

ty_device_type ty_device_get_type(ty_device *dev);

const char *ty_device_get_location(ty_device *dev);
const char *ty_device_get_path(ty_device *dev);

uint16_t ty_device_get_vid(ty_device *dev);
uint16_t ty_device_get_pid(ty_device *dev);
const char *ty_device_get_serial_number(ty_device *dev);

uint8_t ty_device_get_interface_number(ty_device *dev);

int ty_serial_set_control(ty_handle *h, uint32_t rate, uint16_t flags);

ssize_t ty_serial_read(ty_handle *h, char *buf, size_t size);
ssize_t ty_serial_write(ty_handle *h, const char *buf, ssize_t size);

int ty_hid_parse_descriptor(ty_handle *h, ty_hid_descriptor *desc);

ssize_t ty_hid_read(ty_handle *h, uint8_t *buf, size_t size);
ssize_t ty_hid_write(ty_handle *h, const uint8_t *buf, size_t size);

int ty_hid_send_feature_report(ty_handle *h, const uint8_t *buf, size_t size);

TY_C_END

#endif
