/*
 * ty, a collection of GUI and command-line tools to manage Teensy devices
 *
 * Distributed under the MIT license (see LICENSE.txt or http://opensource.org/licenses/MIT)
 * Copyright (c) 2015 Niels Martignène <niels.martignene@gmail.com>
 */

#ifndef TY_BOARD_H
#define TY_BOARD_H

#include "common.h"

TY_C_BEGIN

struct ty_descriptor_set;
struct hs_device;
struct ty_monitor;
struct ty_firmware;
struct hs_handle;
struct ty_task;

typedef struct ty_board ty_board;
typedef struct ty_board_interface ty_board_interface;

typedef struct ty_board_family ty_board_family;
typedef struct ty_board_model ty_board_model;

// Keep in sync with capability_names in board.c
typedef enum ty_board_capability {
    TY_BOARD_CAPABILITY_UNIQUE,
    TY_BOARD_CAPABILITY_RUN,
    TY_BOARD_CAPABILITY_UPLOAD,
    TY_BOARD_CAPABILITY_RESET,
    TY_BOARD_CAPABILITY_REBOOT,
    TY_BOARD_CAPABILITY_SERIAL,

    TY_BOARD_CAPABILITY_COUNT
} ty_board_capability;

typedef enum ty_board_state {
    TY_BOARD_STATE_DROPPED,
    TY_BOARD_STATE_MISSING,
    TY_BOARD_STATE_ONLINE
} ty_board_state;

enum {
    TY_UPLOAD_WAIT = 1,
    TY_UPLOAD_NORESET = 2,
    TY_UPLOAD_NOCHECK = 4
};

#define TY_UPLOAD_MAX_FIRMWARES 256

typedef int ty_board_model_list_func(const ty_board_model *model, void *udata);
typedef int ty_board_list_interfaces_func(ty_board_interface *iface, void *udata);
typedef int ty_board_upload_progress_func(const ty_board *board, const struct ty_firmware *fw, size_t uploaded, void *udata);

TY_PUBLIC extern const ty_board_family *ty_board_families[];

TY_PUBLIC const char *ty_board_family_get_name(const ty_board_family *family);

TY_PUBLIC int ty_board_model_list(ty_board_model_list_func *f, void *udata);
TY_PUBLIC const ty_board_model *ty_board_model_find(const char *name);

TY_PUBLIC bool ty_board_model_is_real(const ty_board_model *model);
TY_PUBLIC bool ty_board_model_test_firmware(const ty_board_model *model, const struct ty_firmware *fw,
                                             const ty_board_model **rguesses, unsigned int *rcount);

TY_PUBLIC const char *ty_board_model_get_name(const ty_board_model *model);
TY_PUBLIC const char *ty_board_model_get_mcu(const ty_board_model *model);
TY_PUBLIC size_t ty_board_model_get_code_size(const ty_board_model *model);

TY_PUBLIC const char *ty_board_capability_get_name(ty_board_capability cap);

TY_PUBLIC ty_board *ty_board_ref(ty_board *board);
TY_PUBLIC void ty_board_unref(ty_board *board);

TY_PUBLIC bool ty_board_matches_tag(ty_board *board, const char *id);

TY_PUBLIC void ty_board_set_udata(ty_board *board, void *udata);
TY_PUBLIC void *ty_board_get_udata(const ty_board *board);

TY_PUBLIC struct ty_monitor *ty_board_get_monitor(const ty_board *board);

TY_PUBLIC ty_board_state ty_board_get_state(const ty_board *board);

TY_PUBLIC const char *ty_board_get_id(const ty_board *board);
TY_PUBLIC int ty_board_set_tag(ty_board *board, const char *tag);
TY_PUBLIC const char *ty_board_get_tag(const ty_board *board);

TY_PUBLIC const char *ty_board_get_location(const ty_board *board);
TY_PUBLIC uint64_t ty_board_get_serial_number(const ty_board *board);

TY_PUBLIC const ty_board_model *ty_board_get_model(const ty_board *board);
TY_PUBLIC const char *ty_board_get_model_name(const ty_board *board);

TY_PUBLIC int ty_board_list_interfaces(ty_board *board, ty_board_list_interfaces_func *f, void *udata);
TY_PUBLIC int ty_board_open_interface(ty_board *board, ty_board_capability cap, ty_board_interface **riface);

TY_PUBLIC int ty_board_get_capabilities(const ty_board *board);
static inline bool ty_board_has_capability(const ty_board *board, ty_board_capability cap)
{
    return ty_board_get_capabilities(board) & (1 << cap);
}

TY_PUBLIC int ty_board_wait_for(ty_board *board, ty_board_capability capability, int timeout);

TY_PUBLIC int ty_board_serial_set_attributes(ty_board *board, uint32_t rate, int flags);
TY_PUBLIC ssize_t ty_board_serial_read(ty_board *board, char *buf, size_t size, int timeout);
TY_PUBLIC ssize_t ty_board_serial_write(ty_board *board, const char *buf, size_t size);

TY_PUBLIC int ty_board_upload(ty_board *board, struct ty_firmware *fw, ty_board_upload_progress_func *pf, void *udata);
TY_PUBLIC int ty_board_reset(ty_board *board);
TY_PUBLIC int ty_board_reboot(ty_board *board);

TY_PUBLIC ty_board_interface *ty_board_interface_ref(ty_board_interface *iface);
TY_PUBLIC void ty_board_interface_unref(ty_board_interface *iface);
TY_PUBLIC int ty_board_interface_open(ty_board_interface *iface);
TY_PUBLIC void ty_board_interface_close(ty_board_interface *iface);

TY_PUBLIC const char *ty_board_interface_get_name(const ty_board_interface *iface);
TY_PUBLIC int ty_board_interface_get_capabilities(const ty_board_interface *iface);

TY_PUBLIC uint8_t ty_board_interface_get_interface_number(const ty_board_interface *iface);
TY_PUBLIC const char *ty_board_interface_get_path(const ty_board_interface *iface);

TY_PUBLIC struct hs_device *ty_board_interface_get_device(const ty_board_interface *iface);
TY_PUBLIC struct hs_handle *ty_board_interface_get_handle(const ty_board_interface *iface);
TY_PUBLIC void ty_board_interface_get_descriptors(const ty_board_interface *iface, struct ty_descriptor_set *set, int id);

TY_PUBLIC int ty_upload(ty_board *board, struct ty_firmware **fws, unsigned int fws_count,
                         int flags, struct ty_task **rtask);
TY_PUBLIC int ty_reset(ty_board *board, struct ty_task **rtask);
TY_PUBLIC int ty_reboot(ty_board *board, struct ty_task **rtask);

TY_C_END

#endif
