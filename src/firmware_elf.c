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

#include "ty/common.h"
#include "compat.h"
#include "ty/firmware.h"

#define EI_NIDENT 16

typedef struct Elf32_Ehdr {
    unsigned char e_ident[EI_NIDENT];

    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf32_Ehdr;

#define ELFMAG "\177ELF"
#define SELFMAG 4

#define EI_CLASS 4
#define ELFCLASS32 1

#define EI_DATA 5
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2

typedef struct Elf32_Phdr {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} Elf32_Phdr;

#define PT_NULL 0
#define PT_LOAD 1

struct loader_context {
    ty_firmware *f;

    FILE *fp;
    const char *filename;

    Elf32_Ehdr ehdr;
};

// The compiler will reduce this to a simple conditional branch
static inline bool is_endianness_reversed(struct loader_context *ctx)
{
    union { uint16_t u; uint8_t raw[2]; } u;
    u.raw[1] = 1;

    return (u.u == 1) == (ctx->ehdr.e_ident[EI_CLASS] == ELFDATA2LSB);
}

static inline void reverse_uint16(uint16_t *u)
{
    *u = (uint16_t)(((*u & 0xFF) << 8) | ((*u & 0xFF00) >> 8));
}

static inline void reverse_uint32(uint32_t *u)
{
    *u = ((*u & 0xFF) << 24) | ((*u & 0xFF00) << 8)
            | ((*u & 0xFF0000) >> 8) | ((*u & 0xFF000000) >> 24);
}

static int read_chunk(struct loader_context *ctx, off_t offset, size_t size, void *buf)
{
    ssize_t r;

    r = fseeko(ctx->fp, offset, SEEK_SET);
    if (r < 0)
        return ty_error(TY_ERROR_SYSTEM, "fseeko() failed: %s", strerror(errno));

    r = (ssize_t)fread(buf, 1, size, ctx->fp);
    if (r < (ssize_t)size) {
        if (ferror(ctx->fp)) {
            if (errno == EIO)
                return ty_error(TY_ERROR_IO, "I/O error while reading from '%s'", ctx->filename);
            return ty_error(TY_ERROR_SYSTEM, "fread('%s') failed: %s", ctx->filename, strerror(errno));
        }

        return ty_error(TY_ERROR_PARSE, "ELF file '%s' is truncated", ctx->filename);
    }

    return 0;
}

static int load_program_header(struct loader_context *ctx, size_t i, Elf32_Phdr *rphdr)
{
    int r;

    r = read_chunk(ctx, (off_t)(ctx->ehdr.e_phoff + i * ctx->ehdr.e_phentsize), sizeof(*rphdr), rphdr);
    if (r < 0)
        return r;

    if (is_endianness_reversed(ctx)) {
        reverse_uint32(&rphdr->p_type);
        reverse_uint32(&rphdr->p_offset);
        reverse_uint32(&rphdr->p_vaddr);
        reverse_uint32(&rphdr->p_paddr);
        reverse_uint32(&rphdr->p_filesz);
        reverse_uint32(&rphdr->p_memsz);
        reverse_uint32(&rphdr->p_flags);
        reverse_uint32(&rphdr->p_align);
    }

    return 0;
}

static int load_segment(struct loader_context *ctx, size_t i)
{
    Elf32_Phdr phdr;
    int r;

    r = load_program_header(ctx, i ,&phdr);
    if (r < 0)
        return (int)r;

    if (phdr.p_type != PT_LOAD) {
        if (phdr.p_type == PT_NULL)
            return 0;

        return ty_error(TY_ERROR_UNSUPPORTED, "ELF object '%s' contains non-loadable segments",
                        ctx->filename);
    }

    if (!phdr.p_filesz)
        return 0;

    if (phdr.p_paddr + phdr.p_filesz > ctx->f->size) {
        ctx->f->size = phdr.p_paddr + phdr.p_filesz;

        if (ctx->f->size > ty_firmware_max_size)
            return ty_error(TY_ERROR_RANGE, "Firmware too big (max %zu bytes) in '%s'",
                            ty_firmware_max_size, ctx->filename);
    }

    r = read_chunk(ctx, phdr.p_offset, phdr.p_filesz, ctx->f->image + phdr.p_paddr);
    if (r < 0)
        return r;

    return 1;
}

int _ty_firmware_load_elf(const char *filename, ty_firmware **rfirmware)
{
    assert(rfirmware);
    assert(filename);

    struct loader_context ctx = {0};
    int r;

    ctx.f = malloc(sizeof(ty_firmware) + ty_firmware_max_size);
    if (!ctx.f)
        return ty_error(TY_ERROR_MEMORY, NULL);
    memset(ctx.f, 0, sizeof(*ctx.f));
    memset(ctx.f->image, 0xFF, ty_firmware_max_size);

#ifdef _WIN32
    ctx.fp = fopen(filename, "rb");
#else
    ctx.fp = fopen(filename, "rbe");
#endif
    if (!ctx.fp) {
        switch (errno) {
        case EACCES:
            r = ty_error(TY_ERROR_ACCESS, "Permission denied for '%s'", filename);
            break;
        case EIO:
            r = ty_error(TY_ERROR_IO, "I/O error while opening '%s' for reading", filename);
            break;
        case ENOENT:
        case ENOTDIR:
            r = ty_error(TY_ERROR_NOT_FOUND, "File '%s' does not exist", filename);
            break;

        default:
            r = ty_error(TY_ERROR_SYSTEM, "fopen('%s') failed: %s", filename, strerror(errno));
            break;
        }
        goto cleanup;
    }
    ctx.filename = filename;

    r = read_chunk(&ctx, 0, sizeof(ctx.ehdr), &ctx.ehdr);
    if (r < 0)
        goto cleanup;

    if (memcmp(ctx.ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
        r = ty_error(TY_ERROR_PARSE, "Missing ELF signature in '%s'", ctx.filename);
        goto cleanup;
    }

    if (ctx.ehdr.e_ident[EI_CLASS] != ELFCLASS32) {
        r = ty_error(TY_ERROR_UNSUPPORTED, "ELF object '%s' is not supported (not 32-bit)",
                     ctx.filename);
        goto cleanup;
    }

    if (is_endianness_reversed(&ctx)) {
        reverse_uint16(&ctx.ehdr.e_type);
        reverse_uint16(&ctx.ehdr.e_machine);
        reverse_uint32(&ctx.ehdr.e_entry);
        reverse_uint32(&ctx.ehdr.e_phoff);
        reverse_uint32(&ctx.ehdr.e_shoff);
        reverse_uint32(&ctx.ehdr.e_flags);
        reverse_uint16(&ctx.ehdr.e_ehsize);
        reverse_uint16(&ctx.ehdr.e_phentsize);
        reverse_uint16(&ctx.ehdr.e_phnum);
        reverse_uint16(&ctx.ehdr.e_shentsize);
        reverse_uint16(&ctx.ehdr.e_shnum);
        reverse_uint16(&ctx.ehdr.e_shstrndx);
    }

    if (!ctx.ehdr.e_phoff) {
        r = ty_error(TY_ERROR_PARSE, "ELF file '%s' has no program headers", filename);
        goto cleanup;
    }

    for (size_t i = 0; i < ctx.ehdr.e_phnum; i++) {
        r = load_segment(&ctx, i);
        if (r < 0)
            goto cleanup;
    }

    *rfirmware = ctx.f;
    ctx.f = NULL;

    r = 0;
cleanup:
    if (ctx.fp)
        fclose(ctx.fp);
    free(ctx.f);
    return r;
}