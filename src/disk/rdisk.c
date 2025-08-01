/*
 * 86Box    A hypervisor and IBM PC system emulator that specializes in
 *          running old operating systems and software designed for IBM
 *          PC systems and compatibles from 1981 through fairly recent
 *          system designs based on the PCI bus.
 *
 *          This file is part of the 86Box distribution.
 *
 *          Implementation of the Iomega ZIP drive with SCSI(-like)
 *          commands, for both ATAPI and SCSI usage.
 *
 * Authors: Miran Grca, <mgrca8@gmail.com>
 *
 *          Copyright 2018-2025 Miran Grca.
 */
#ifdef ENABLE_RDISK_LOG
#include <stdarg.h>
#endif
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <86box/86box.h>
#include <86box/timer.h>
#include <86box/device.h>
#include <86box/log.h>
#include <86box/scsi.h>
#include <86box/scsi_device.h>
#include <86box/nvr.h>
#include <86box/path.h>
#include <86box/plat.h>
#include <86box/ui.h>
#include <86box/hdc_ide.h>
#include <86box/rdisk.h>
#include <86box/version.h>

#define IDE_ATAPI_IS_EARLY             id->sc->pad0

rdisk_drive_t rdisk_drives[RDISK_NUM];

// clang-format off
/*
   Table of all SCSI commands and their flags, needed for the new disc change /
   not ready handler.
 */
const uint8_t rdisk_command_flags[0x100] = {
    [0x00]          = IMPLEMENTED | CHECK_READY,
    [0x01]          = IMPLEMENTED | ALLOW_UA | SCSI_ONLY,
    [0x03]          = IMPLEMENTED | ALLOW_UA,
    [0x04]          = IMPLEMENTED | CHECK_READY | ALLOW_UA | SCSI_ONLY,
    [0x06]          = IMPLEMENTED,
    [0x08]          = IMPLEMENTED | CHECK_READY,
    [0x0a ... 0x0b] = IMPLEMENTED | CHECK_READY,
    [0x0c]          = IMPLEMENTED,
    [0x0d]          = IMPLEMENTED | ATAPI_ONLY,
    [0x12]          = IMPLEMENTED | ALLOW_UA,
    [0x13]          = IMPLEMENTED | CHECK_READY,
    [0x15]          = IMPLEMENTED,
    [0x16 ... 0x17] = IMPLEMENTED | SCSI_ONLY,
    [0x1a]          = IMPLEMENTED,
    [0x1b]          = IMPLEMENTED | CHECK_READY,
    [0x1d]          = IMPLEMENTED,
    [0x1e]          = IMPLEMENTED | CHECK_READY,
    [0x23]          = IMPLEMENTED | ATAPI_ONLY,
    [0x25]          = IMPLEMENTED | CHECK_READY,
    [0x28]          = IMPLEMENTED | CHECK_READY,
    [0x2a ... 0x2b] = IMPLEMENTED | CHECK_READY,
    [0x2e ... 0x2f] = IMPLEMENTED | CHECK_READY,
    [0x41]          = IMPLEMENTED | CHECK_READY,
    [0x55]          = IMPLEMENTED,
    [0x5a]          = IMPLEMENTED,
    [0xa8]          = IMPLEMENTED | CHECK_READY,
    [0xaa]          = IMPLEMENTED | CHECK_READY,
    [0xae]          = IMPLEMENTED | CHECK_READY,
    [0xaf]          = IMPLEMENTED | CHECK_READY | SCSI_ONLY,
    [0xbd]          = IMPLEMENTED
};

static uint64_t zip_mode_sense_page_flags     = (GPMODEP_R_W_ERROR_PAGE | GPMODEP_DISCONNECT_PAGE |
                                                 GPMODEP_IOMEGA_PAGE | GPMODEP_ALL_PAGES);
static uint64_t zip_250_mode_sense_page_flags = (GPMODEP_R_W_ERROR_PAGE | GPMODEP_FLEXIBLE_DISK_PAGE |
                                                 GPMODEP_CACHING_PAGE | GPMODEP_IOMEGA_PAGE |
                                                 GPMODEP_ALL_PAGES);

static const mode_sense_pages_t zip_mode_sense_pages_default = {
    { [0x01] = { GPMODE_R_W_ERROR_PAGE,               0x0a, 0xc8, 0x16, 0x00, 0x00, 0x00, 0x00,
                 0x5a,                                0x00, 0x50, 0x20                         },
      [0x02] = { GPMODE_DISCONNECT_PAGE,              0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                 0x00,                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
      [0x2f] = { GPMODE_IOMEGA_PAGE,                  0x04, 0x5c, 0x0f, 0xff, 0x0f             } }
};

static const mode_sense_pages_t zip_250_mode_sense_pages_default = {
    { [0x01] = { GPMODE_R_W_ERROR_PAGE,               0x06, 0xc8, 0x64, 0x00, 0x00, 0x00, 0x00 },
      [0x05] = { GPMODE_FLEXIBLE_DISK_PAGE,           0x1e, 0x80, 0x00, 0x40, 0x20, 0x02, 0x00,
                 0x00,                                0xef, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                 0x00,                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                 0x00,                                0x00, 0x00, 0x00, 0x0b, 0x7d, 0x00, 0x00 },
      [0x08] = { GPMODE_CACHING_PAGE,                 0x0a, 0x04, 0x00, 0xff, 0xff, 0x00, 0x00,
                 0xff,                                0xff, 0xff, 0xff                         },
      [0x2f] = { GPMODE_IOMEGA_PAGE,                  0x04, 0x5c, 0x0f, 0x3c, 0x0f             } }
};

static const mode_sense_pages_t zip_mode_sense_pages_default_scsi = {
    { [0x01] = { GPMODE_R_W_ERROR_PAGE,               0x0a, 0xc8, 0x16, 0x00, 0x00, 0x00, 0x00,
                 0x5a,                                0x00, 0x50, 0x20                         },
      [0x02] = { GPMODE_DISCONNECT_PAGE,              0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                 0x00,                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
      [0x2f] = { GPMODE_IOMEGA_PAGE,                  0x04, 0x5c, 0x0f, 0xff, 0x0f             } }
};

static const mode_sense_pages_t zip_250_mode_sense_pages_default_scsi = {
    { [0x01] = { GPMODE_R_W_ERROR_PAGE,               0x06, 0xc8, 0x64, 0x00, 0x00, 0x00, 0x00 },
      [0x05] = { GPMODE_FLEXIBLE_DISK_PAGE,           0x1e, 0x80, 0x00, 0x40, 0x20, 0x02, 0x00,
                 0x00,                                0xef, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                 0x00,                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                 0x00,                                0x00, 0x00, 0x00, 0x0b, 0x7d, 0x00, 0x00 },
      [0x08] = { GPMODE_CACHING_PAGE,                 0x0a, 0x04, 0x00, 0xff, 0xff, 0x00, 0x00,
                 0xff,                                0xff, 0xff, 0xff                         },
      [0x2f] = { GPMODE_IOMEGA_PAGE,                  0x04, 0x5c, 0x0f, 0x3c, 0x0f             } }
};

static const mode_sense_pages_t zip_mode_sense_pages_changeable = {
    { [0x01] = { GPMODE_R_W_ERROR_PAGE,               0x0a, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff,
                 0x5a,                                0xff, 0xff, 0xff                         },
      [0x02] = { GPMODE_DISCONNECT_PAGE,              0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                 0x00,                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
      [0x2f] = { GPMODE_IOMEGA_PAGE,                  0x04, 0xff, 0xff, 0xff, 0xff             } }
};

static const mode_sense_pages_t zip_250_mode_sense_pages_changeable = {
    { [0x01] = { GPMODE_R_W_ERROR_PAGE,               0x06, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00 },
      [0x05] = { GPMODE_FLEXIBLE_DISK_PAGE,           0x1e, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                 0xff,                                0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                 0xff,                                0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                 0xff,                                0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00 },
      [0x08] = { GPMODE_CACHING_PAGE,                 0x0a, 0x04, 0x00, 0xff, 0xff, 0x00, 0x00,
                 0xff,                                0xff, 0xff, 0xff                         },
      [0x2f] = { GPMODE_IOMEGA_PAGE,                  0x04, 0xff, 0xff, 0xff, 0xff             } }
};
// clang-format on

static void rdisk_command_complete(rdisk_t *dev);
static void rdisk_init(rdisk_t *dev);

#ifdef ENABLE_RDISK_LOG
int rdisk_do_log = ENABLE_RDISK_LOG;

static void
rdisk_log(void *priv, const char *fmt, ...)
{
    va_list ap;

    if (rdisk_do_log) {
        va_start(ap, fmt);
        log_out(priv, fmt, ap);
        va_end(ap);
    }
}
#else
#    define rdisk_log(priv, fmt, ...)
#endif

static int
rdisk_load_abort(const rdisk_t *dev)
{
    if (dev->drv->fp)
        fclose(dev->drv->fp);
    dev->drv->fp           = NULL;
    dev->drv->medium_size = 0;
    rdisk_eject(dev->id); /* Make sure the host OS knows we've rejected (and ejected) the image. */
    return 0;
}

int
image_is_zdi(const char *s)
{
    return !strcasecmp(path_get_extension((char *) s), "ZDI");
}

int
rdisk_is_empty(const uint8_t id)
{
    const rdisk_t *dev = (const rdisk_t *) rdisk_drives[id].priv;
    int          ret = 0;

    if ((dev->drv == NULL) || (dev->drv->fp == NULL))
        ret = 1;

    return ret;
}

void
rdisk_load(const rdisk_t *dev, const char *fn, const int skip_insert)
{
    const int was_empty = rdisk_is_empty(dev->id);
    int       ret       = 0;
    int       offs      = 0;

    if (strstr(fn, "wp://") == fn) {
        offs                = 5;
        dev->drv->read_only = 1;
    }

    fn += offs;

    if (dev->drv == NULL)
        rdisk_eject(dev->id);
    else {
        const int is_zdi = image_is_zdi(fn);

        dev->drv->fp     = plat_fopen(fn, dev->drv->read_only ? "rb" : "rb+");
        ret              = 1;

        if (dev->drv->fp == NULL) {
            if (!dev->drv->read_only) {
                dev->drv->fp = plat_fopen(fn, "rb");
                if (dev->drv->fp == NULL)
                    ret = rdisk_load_abort(dev);
                else
                    dev->drv->read_only = 1;
            } else
                ret = rdisk_load_abort(dev);
        }

        if (ret) {
            fseek(dev->drv->fp, 0, SEEK_END);
            int size = ftell(dev->drv->fp);

            if (is_zdi) {
                /* This is a ZDI image. */
                size -= 0x1000;
                dev->drv->base = 0x1000;
            } else
                dev->drv->base = 0;

            if (dev->drv->type != RDISK_TYPE_ZIP_100) {
                if ((size != (ZIP_250_SECTORS << 9)) && (size != (ZIP_SECTORS << 9))) {
                    rdisk_log(dev->log, "File is incorrect size for a RDISK image\n");
                    rdisk_log(dev->log, "Must be exactly %i or %i bytes\n",
                            ZIP_250_SECTORS << 9, ZIP_SECTORS << 9);
                    ret = rdisk_load_abort(dev);
                }
            } else if (size != (ZIP_SECTORS << 9)) {
                rdisk_log(dev->log, "File is incorrect size for a RDISK image\n");
                rdisk_log(dev->log, "Must be exactly %i bytes\n", ZIP_SECTORS << 9);
                ret = rdisk_load_abort(dev);
            }

            if (ret)
                dev->drv->medium_size = size >> 9;
        }

         if (ret) {
             if (fseek(dev->drv->fp, dev->drv->base, SEEK_SET) == -1)
                 log_fatal(dev->log, "rdisk_load(): Error seeking to the beginning of "
                           "the file\n");

             strncpy(dev->drv->image_path, fn - offs, sizeof(dev->drv->image_path) - 1);
             /*
                After using strncpy, dev->drv->image_path needs to be explicitly null
                terminated to make gcc happy.
                In the event strlen(dev->drv->image_path) == sizeof(dev->drv->image_path)
                (no null terminator) it is placed at the very end. Otherwise, it is placed
                right after the string.
              */
             const size_t term = strlen(dev->drv->image_path) ==
                 sizeof(dev->drv->image_path) ? sizeof(dev->drv->image_path) - 1 :
                                                strlen(dev->drv->image_path);
             dev->drv->image_path[term] = '\0';
        }
    }

    if (ret && !skip_insert) {
        /* Signal media change to the emulated machine. */
        rdisk_insert((rdisk_t *) dev);

        /* The drive was previously empty, transition directly to UNIT ATTENTION. */
        if (was_empty)
            rdisk_insert((rdisk_t *) dev);
    }

    if (ret)
        ui_sb_update_icon_wp(SB_RDISK | dev->id, dev->drv->read_only);
}

void
rdisk_disk_reload(const rdisk_t *dev)
{
    if (strlen(dev->drv->prev_image_path) != 0)
        (void) rdisk_load(dev, dev->drv->prev_image_path, 0);
}

static void
rdisk_disk_unload(const rdisk_t *dev)
{
    if ((dev->drv != NULL) && (dev->drv->fp != NULL)) {
        fclose(dev->drv->fp);
        dev->drv->fp = NULL;
    }
}

void
rdisk_disk_close(const rdisk_t *dev)
{
    if ((dev->drv != NULL) && (dev->drv->fp != NULL)) {
        rdisk_disk_unload(dev);

        memcpy(dev->drv->prev_image_path, dev->drv->image_path,
               sizeof(dev->drv->prev_image_path));
        memset(dev->drv->image_path, 0, sizeof(dev->drv->image_path));

        dev->drv->medium_size = 0;

        rdisk_insert((rdisk_t *) dev);
    }
}

static void
rdisk_set_callback(const rdisk_t *dev)
{
    if (dev->drv->bus_type != RDISK_BUS_SCSI)
        ide_set_callback(ide_drives[dev->drv->ide_channel], dev->callback);
}

static void
rdisk_init(rdisk_t *dev)
{
    if (dev->id < RDISK_NUM) {
        dev->requested_blocks = 1;
        dev->sense[0]         = 0xf0;
        dev->sense[7]         = 10;
        dev->drv->bus_mode    = 0;
        if (dev->drv->bus_type >= RDISK_BUS_ATAPI)
            dev->drv->bus_mode |= 2;
        if (dev->drv->bus_type < RDISK_BUS_SCSI)
            dev->drv->bus_mode |= 1;
        rdisk_log(dev->log, "Bus type %i, bus mode %i\n", dev->drv->bus_type, dev->drv->bus_mode);
        if (dev->drv->bus_type < RDISK_BUS_SCSI) {
            dev->tf->phase          = 1;
            dev->tf->request_length = 0xEB14;
        }
        dev->tf->status    = READY_STAT | DSC_STAT;
        dev->tf->pos       = 0;
        dev->packet_status = PHASE_NONE;
        rdisk_sense_key = rdisk_asc = rdisk_ascq = dev->unit_attention = dev->transition = 0;
        rdisk_info      = 0x00000000;
    }
}

static int
rdisk_supports_pio(const rdisk_t *dev)
{
    return (dev->drv->bus_mode & 1);
}

static int
rdisk_supports_dma(const rdisk_t *dev)
{
    return (dev->drv->bus_mode & 2);
}

/* Returns: 0 for none, 1 for PIO, 2 for DMA. */
static int
rdisk_current_mode(const rdisk_t *dev)
{
    if (!rdisk_supports_pio(dev) && !rdisk_supports_dma(dev))
        return 0;
    if (rdisk_supports_pio(dev) && !rdisk_supports_dma(dev)) {
        rdisk_log(dev->log, "Drive does not support DMA, setting to PIO\n");
        return 1;
    }
    if (!rdisk_supports_pio(dev) && rdisk_supports_dma(dev))
        return 2;
    if (rdisk_supports_pio(dev) && rdisk_supports_dma(dev)) {
        rdisk_log(dev->log, "Drive supports both, setting to %s\n",
                (dev->tf->features & 1) ? "DMA" : "PIO");
        return (dev->tf->features & 1) ? 2 : 1;
    }

    return 0;
}

static void
rdisk_mode_sense_load(rdisk_t *dev)
{
    char  fn[512] = { 0 };

    memset(&dev->ms_pages_saved, 0, sizeof(mode_sense_pages_t));

    if (dev->drv->type == RDISK_TYPE_ZIP_100) {
        if (rdisk_drives[dev->id].bus_type == RDISK_BUS_SCSI)
            memcpy(&dev->ms_pages_saved, &zip_mode_sense_pages_default_scsi, sizeof(mode_sense_pages_t));
        else
            memcpy(&dev->ms_pages_saved, &zip_mode_sense_pages_default, sizeof(mode_sense_pages_t));
    } else {
        if (rdisk_drives[dev->id].bus_type == RDISK_BUS_SCSI)
            memcpy(&dev->ms_pages_saved, &zip_250_mode_sense_pages_default_scsi, sizeof(mode_sense_pages_t));
        else
            memcpy(&dev->ms_pages_saved, &zip_250_mode_sense_pages_default, sizeof(mode_sense_pages_t));
    }

    if (dev->drv->bus_type == RDISK_BUS_SCSI)
        sprintf(fn, "scsi_rdisk_%02i_mode_sense_bin", dev->id);
    else
        sprintf(fn, "rdisk_%02i_mode_sense_bin", dev->id);
    FILE *fp = plat_fopen(nvr_path(fn), "rb");
    if (fp) {
        /* Nothing to read, not used by RDISK. */
        fclose(fp);
    }
}

static void
rdisk_mode_sense_save(const rdisk_t *dev)
{
    char  fn[512] = { 0 };

    if (dev->drv->bus_type == RDISK_BUS_SCSI)
        sprintf(fn, "scsi_rdisk_%02i_mode_sense_bin", dev->id);
    else
        sprintf(fn, "rdisk_%02i_mode_sense_bin", dev->id);
    FILE *fp = plat_fopen(nvr_path(fn), "wb");
    if (fp) {
        /* Nothing to write, not used by RDISK. */
        fclose(fp);
    }
}

/* SCSI Mode Sense 6/10. */
static uint8_t
zip_mode_sense_read(const rdisk_t *dev, const uint8_t pgctl,
                    const uint8_t page, const uint8_t pos)
{
    switch (pgctl) {
        case 0:
        case 3:
            if ((dev->drv->type != RDISK_TYPE_ZIP_100) && (page == 5) && (pos == 9) &&
                (dev->drv->medium_size == ZIP_SECTORS))
                return 0x60;
            return dev->ms_pages_saved.pages[page][pos];
        case 1:
            if (dev->drv->type == RDISK_TYPE_ZIP_100)
                return zip_mode_sense_pages_changeable.pages[page][pos];
            else
                return zip_250_mode_sense_pages_changeable.pages[page][pos];
        case 2:
            if (dev->drv->type == RDISK_TYPE_ZIP_100) {
                if (dev->drv->bus_type == RDISK_BUS_SCSI)
                    return zip_mode_sense_pages_default_scsi.pages[page][pos];
                else
                    return zip_mode_sense_pages_default.pages[page][pos];
            } else {
                if ((page == 5) && (pos == 9) && (dev->drv->medium_size == ZIP_SECTORS))
                    return 0x60;
                if (dev->drv->bus_type == RDISK_BUS_SCSI)
                    return zip_250_mode_sense_pages_default_scsi.pages[page][pos];
                else
                    return zip_250_mode_sense_pages_default.pages[page][pos];
            }

        default:
            break;
    }

    return 0;
}

static uint32_t
rdisk_mode_sense(const rdisk_t *dev, uint8_t *buf, uint32_t pos,
               uint8_t page, const uint8_t block_descriptor_len)
{
    uint64_t       pf;
    const uint8_t  pgctl = (page >> 6) & 3;

    if (dev->drv->type == RDISK_TYPE_ZIP_100)
        pf = zip_mode_sense_page_flags;
    else
        pf = zip_250_mode_sense_page_flags;

    page &= 0x3f;

    if (block_descriptor_len) {
        buf[pos++] = ((dev->drv->medium_size >> 24) & 0xff);
        buf[pos++] = ((dev->drv->medium_size >> 16) & 0xff);
        buf[pos++] = ((dev->drv->medium_size >> 8) & 0xff);
        buf[pos++] = (dev->drv->medium_size & 0xff);
        buf[pos++] = 0; /* Reserved. */
        buf[pos++] = 0; /* Block length (0x200 = 512 bytes). */
        buf[pos++] = 2;
        buf[pos++] = 0;
    }

    for (uint8_t i = 0; i < 0x40; i++) {
        if ((page == GPMODE_ALL_PAGES) || (page == i)) {
            if (pf & (1LL << ((uint64_t) page))) {
                const uint8_t msplen = zip_mode_sense_read(dev, pgctl, i, 1);
                buf[pos++]           = zip_mode_sense_read(dev, pgctl, i, 0);
                buf[pos++]           = msplen;
                rdisk_log(dev->log, "MODE SENSE: Page [%02X] length %i\n", i, msplen);
                for (uint8_t j = 0; j < msplen; j++)
                    buf[pos++] = zip_mode_sense_read(dev, pgctl, i, 2 + j);
            }
        }
    }

    return pos;
}

static void
rdisk_update_request_length(rdisk_t *dev, int len, int block_len)
{
    int bt;
    int min_len = 0;

    dev->max_transfer_len = dev->tf->request_length;

    /*
       For media access commands, make sure the requested DRQ length matches the
       block length.
     */
    switch (dev->current_cdb[0]) {
        case 0x08:
        case 0x0a:
        case 0x28:
        case 0x2a:
        case 0xa8:
        case 0xaa:
            /* Round it to the nearest 2048 bytes. */
            dev->max_transfer_len = (dev->max_transfer_len >> 9) << 9;

            /*
               Make sure total length is not bigger than sum of the lengths of
               all the requested blocks.
             */
            bt = (dev->requested_blocks * block_len);
            if (len > bt)
                len = bt;

            min_len = block_len;

            if (len <= block_len) {
                /* Total length is less or equal to block length. */
                if (dev->max_transfer_len < block_len) {
                    /* Transfer a minimum of (block size) bytes. */
                    dev->max_transfer_len = block_len;
                    dev->packet_len       = block_len;
                    break;
                }
            }
            fallthrough;

        default:
            dev->packet_len = len;
            break;
    }
    /*
       If the DRQ length is odd, and the total remaining length is bigger,
       make sure it's even.
     */
    if ((dev->max_transfer_len & 1) && (dev->max_transfer_len < len))
        dev->max_transfer_len &= 0xfffe;
    /*
       If the DRQ length is smaller or equal in size to the total remaining length,
       set it to that.
     */
    if (!dev->max_transfer_len)
        dev->max_transfer_len = 65534;

    if ((len <= dev->max_transfer_len) && (len >= min_len))
        dev->tf->request_length = dev->max_transfer_len = len;
    else if (len > dev->max_transfer_len)
        dev->tf->request_length = dev->max_transfer_len;

    return;
}

static double
rdisk_bus_speed(rdisk_t *dev)
{
    double ret = -1.0;

    if (dev && dev->drv)
        ret = ide_atapi_get_period(dev->drv->ide_channel);

    if (ret == -1.0) {
        if (dev)
            dev->callback = -1.0;
        ret = 0.0;
    }

    return ret;
}

static void
rdisk_command_common(rdisk_t *dev)
{
    dev->tf->status = BUSY_STAT;
    dev->tf->phase  = 1;
    dev->tf->pos    = 0;
    if (dev->packet_status == PHASE_COMPLETE)
        dev->callback = 0.0;
    else if (dev->drv->bus_type == RDISK_BUS_SCSI)
        dev->callback = -1.0; /* Speed depends on SCSI controller */
    else
        dev->callback = rdisk_bus_speed(dev) * (double) (dev->packet_len);

    rdisk_set_callback(dev);
}

static void
rdisk_command_complete(rdisk_t *dev)
{
    dev->packet_status = PHASE_COMPLETE;
    rdisk_command_common(dev);
}

static void
rdisk_command_read(rdisk_t *dev)
{
    dev->packet_status = PHASE_DATA_IN;
    rdisk_command_common(dev);
}

static void
rdisk_command_read_dma(rdisk_t *dev)
{
    dev->packet_status = PHASE_DATA_IN_DMA;
    rdisk_command_common(dev);
}

static void
rdisk_command_write(rdisk_t *dev)
{
    dev->packet_status = PHASE_DATA_OUT;
    rdisk_command_common(dev);
}

static void
rdisk_command_write_dma(rdisk_t *dev)
{
    dev->packet_status = PHASE_DATA_OUT_DMA;
    rdisk_command_common(dev);
}

/*
   dev = Pointer to current RDISK device;
   len = Total transfer length;
   block_len = Length of a single block (why does it matter?!);
   alloc_len = Allocated transfer length;
   direction = Transfer direction (0 = read from host, 1 = write to host).
 */
static void
rdisk_data_command_finish(rdisk_t *dev, int len, const int block_len,
                        const int alloc_len, const int direction)
{
    rdisk_log(dev->log, "Finishing command (%02X): %i, %i, %i, %i, %i\n",
            dev->current_cdb[0], len, block_len, alloc_len,
            direction, dev->tf->request_length);
    dev->tf->pos = 0;
    if (alloc_len >= 0) {
        if (alloc_len < len)
            len = alloc_len;
    }
    if ((len == 0) || (rdisk_current_mode(dev) == 0)) {
        if (dev->drv->bus_type != RDISK_BUS_SCSI)
            dev->packet_len = 0;

        rdisk_command_complete(dev);
    } else {
        if (rdisk_current_mode(dev) == 2) {
            if (dev->drv->bus_type != RDISK_BUS_SCSI)
                dev->packet_len = alloc_len;

            if (direction == 0)
                rdisk_command_read_dma(dev);
            else
                rdisk_command_write_dma(dev);
        } else {
            rdisk_update_request_length(dev, len, block_len);
            if ((dev->drv->bus_type != RDISK_BUS_SCSI) &&
                (dev->tf->request_length == 0))
                rdisk_command_complete(dev);
            else if (direction == 0)
                rdisk_command_read(dev);
            else
                rdisk_command_write(dev);
        }
    }

    rdisk_log(dev->log, "Status: %i, cylinder %i, packet length: %i, position: %i, phase: %i\n",
            dev->packet_status, dev->tf->request_length, dev->packet_len,
            dev->tf->pos, dev->tf->phase);
}

static void
rdisk_sense_clear(rdisk_t *dev, UNUSED(int command))
{
    rdisk_sense_key = rdisk_asc = rdisk_ascq = 0;
    rdisk_info      = 0x00000000;
}

static void
rdisk_set_phase(const rdisk_t *dev, const uint8_t phase)
{
    const uint8_t scsi_bus = (dev->drv->scsi_device_id >> 4) & 0x0f;
    const uint8_t scsi_id  = dev->drv->scsi_device_id & 0x0f;

    if (dev->drv->bus_type == RDISK_BUS_SCSI)
        scsi_devices[scsi_bus][scsi_id].phase = phase;
}

static void
rdisk_cmd_error(rdisk_t *dev)
{
    rdisk_set_phase(dev, SCSI_PHASE_STATUS);
    dev->tf->error = ((rdisk_sense_key & 0xf) << 4) | ABRT_ERR;
    dev->tf->status    = READY_STAT | ERR_STAT;
    dev->tf->phase     = 3;
    dev->tf->pos       = 0;
    dev->packet_status = PHASE_ERROR;
    dev->callback      = 50.0 * RDISK_TIME;
    rdisk_set_callback(dev);
    ui_sb_update_icon(SB_RDISK | dev->id, 0);
    ui_sb_update_icon_write(SB_RDISK | dev->id, 0);
    rdisk_log(dev->log, "[%02X] ERROR: %02X/%02X/%02X\n", dev->current_cdb[0], rdisk_sense_key,
            rdisk_asc, rdisk_ascq);
}

static void
rdisk_unit_attention(rdisk_t *dev)
{
    rdisk_set_phase(dev, SCSI_PHASE_STATUS);
    dev->tf->error = (SENSE_UNIT_ATTENTION << 4) | ABRT_ERR;
    dev->tf->status    = READY_STAT | ERR_STAT;
    dev->tf->phase     = 3;
    dev->tf->pos       = 0;
    dev->packet_status = PHASE_ERROR;
    dev->callback      = 50.0 * RDISK_TIME;
    rdisk_set_callback(dev);
    ui_sb_update_icon(SB_RDISK | dev->id, 0);
    ui_sb_update_icon_write(SB_RDISK | dev->id, 0);
    rdisk_log(dev->log, "UNIT ATTENTION\n", dev->id);
}

static void
rdisk_buf_alloc(rdisk_t *dev, const uint32_t len)
{
    rdisk_log(dev->log, "Allocated buffer length: %i\n", len);
    if (dev->buffer == NULL)
        dev->buffer = (uint8_t *) malloc(len);
}

static void
rdisk_buf_free(rdisk_t *dev)
{
    if (dev->buffer != NULL) {
        rdisk_log(dev->log, "Removable Disk %i: Freeing buffer...\n");
        free(dev->buffer);
        dev->buffer = NULL;
    }
}

static void
rdisk_bus_master_error(scsi_common_t *sc)
{
    rdisk_t *dev = (rdisk_t *) sc;

    rdisk_buf_free(dev);
    rdisk_sense_key = rdisk_asc = rdisk_ascq = 0;
    rdisk_info      =  (dev->sector_pos >> 24)        |
                    ((dev->sector_pos >> 16) <<  8) |
                    ((dev->sector_pos >> 8)  << 16) |
                    ( dev->sector_pos        << 24);
    rdisk_cmd_error(dev);
}

static void
rdisk_not_ready(rdisk_t *dev)
{
    rdisk_sense_key = SENSE_NOT_READY;
    rdisk_asc       = ASC_MEDIUM_NOT_PRESENT;
    rdisk_ascq      = 0;
    rdisk_info      = 0x00000000;
    rdisk_cmd_error(dev);
}

static void
rdisk_write_protected(rdisk_t *dev)
{
    rdisk_sense_key = SENSE_UNIT_ATTENTION;
    rdisk_asc       = ASC_WRITE_PROTECTED;
    rdisk_ascq      = 0;
    rdisk_info      =  (dev->sector_pos >> 24)        |
                    ((dev->sector_pos >> 16) <<  8) |
                    ((dev->sector_pos >> 8)  << 16) |
                    ( dev->sector_pos        << 24);
    rdisk_cmd_error(dev);
}

static void
rdisk_write_error(rdisk_t *dev)
{
    rdisk_sense_key = SENSE_MEDIUM_ERROR;
    rdisk_asc       = ASC_WRITE_ERROR;
    rdisk_ascq      = 0;
    rdisk_info      =  (dev->sector_pos >> 24)        |
                    ((dev->sector_pos >> 16) <<  8) |
                    ((dev->sector_pos >> 8)  << 16) |
                    ( dev->sector_pos        << 24);
    rdisk_cmd_error(dev);
}

static void
rdisk_read_error(rdisk_t *dev)
{
    rdisk_sense_key = SENSE_MEDIUM_ERROR;
    rdisk_asc       = ASC_UNRECOVERED_READ_ERROR;
    rdisk_ascq      = 0;
    rdisk_info      =  (dev->sector_pos >> 24)        |
                    ((dev->sector_pos >> 16) <<  8) |
                    ((dev->sector_pos >> 8)  << 16) |
                    ( dev->sector_pos        << 24);
    rdisk_cmd_error(dev);
}

static void
rdisk_invalid_lun(rdisk_t *dev, const uint8_t lun)
{
    rdisk_sense_key = SENSE_ILLEGAL_REQUEST;
    rdisk_asc       = ASC_INV_LUN;
    rdisk_ascq      = 0;
    rdisk_info      = lun << 24;
    rdisk_cmd_error(dev);
}

static void
rdisk_illegal_opcode(rdisk_t *dev, const uint8_t opcode)
{
    rdisk_sense_key = SENSE_ILLEGAL_REQUEST;
    rdisk_asc       = ASC_ILLEGAL_OPCODE;
    rdisk_ascq      = 0;
    rdisk_info      = opcode << 24;
    rdisk_cmd_error(dev);
}

static void
rdisk_lba_out_of_range(rdisk_t *dev)
{
    rdisk_sense_key = SENSE_ILLEGAL_REQUEST;
    rdisk_asc       = ASC_LBA_OUT_OF_RANGE;
    rdisk_ascq      = 0;
    rdisk_info      =  (dev->sector_pos >> 24)        |
                    ((dev->sector_pos >> 16) <<  8) |
                    ((dev->sector_pos >> 8)  << 16) |
                    ( dev->sector_pos        << 24);
    rdisk_cmd_error(dev);
}

static void
rdisk_invalid_field(rdisk_t *dev, const uint32_t field)
{
    rdisk_sense_key = SENSE_ILLEGAL_REQUEST;
    rdisk_asc       = ASC_INV_FIELD_IN_CMD_PACKET;
    rdisk_ascq      = 0;
    rdisk_info      =  (field >> 24)        |
                    ((field >> 16) <<  8) |
                    ((field >> 8)  << 16) |
                    ( field        << 24);
    rdisk_cmd_error(dev);
    dev->tf->status = 0x53;
}

static void
rdisk_invalid_field_pl(rdisk_t *dev, const uint32_t field)
{
    rdisk_sense_key = SENSE_ILLEGAL_REQUEST;
    rdisk_asc       = ASC_INV_FIELD_IN_PARAMETER_LIST;
    rdisk_ascq      = 0;
    rdisk_info      =  (field >> 24)        |
                    ((field >> 16) <<  8) |
                    ((field >> 8)  << 16) |
                    ( field        << 24);
    rdisk_cmd_error(dev);
    dev->tf->status = 0x53;
}

static void
rdisk_data_phase_error(rdisk_t *dev, const uint32_t info)
{
    rdisk_sense_key = SENSE_ILLEGAL_REQUEST;
    rdisk_asc       = ASC_DATA_PHASE_ERROR;
    rdisk_ascq      = 0;
    rdisk_info      =  (info >> 24)        |
                    ((info >> 16) <<  8) |
                    ((info >> 8)  << 16) |
                    ( info        << 24);
    rdisk_cmd_error(dev);
}

static int
rdisk_blocks(rdisk_t *dev, int32_t *len, const int out)
{
    int ret = 1;
    *len    = 0;

    if (dev->sector_len > 0) {
        rdisk_log(dev->log, "%sing %i blocks starting from %i...\n", out ? "Writ" : "Read",
                dev->requested_blocks, dev->sector_pos);

        if (dev->sector_pos >= dev->drv->medium_size) {
            rdisk_log(dev->log, "Trying to %s beyond the end of disk\n",
                    out ? "write" : "read");
            rdisk_lba_out_of_range(dev);
            ret = 0;
        } else {
            *len    = dev->requested_blocks << 9;

            for (int i = 0; i < dev->requested_blocks; i++) {
                if (fseek(dev->drv->fp, dev->drv->base + (dev->sector_pos << 9),
                    SEEK_SET) == -1) {
                    if (out)
                        rdisk_write_error(dev);
                    else
                        rdisk_read_error(dev);
                    ret = -1;
                } else {
                    if (feof(dev->drv->fp))
                        break;

                    if (out) {
                        if (fwrite(dev->buffer + (i << 9), 1,
                                   512, dev->drv->fp) != 512) {
                            rdisk_log(dev->log, "rdisk_blocks(): Error writing data\n");
                            rdisk_write_error(dev);
                            ret = -1;
                        } else
                            fflush(dev->drv->fp);
                    } else if (fread(dev->buffer + (i << 9), 1,
                                     512, dev->drv->fp) != 512) {
                        rdisk_log(dev->log, "rdisk_blocks(): Error reading data\n");
                        rdisk_read_error(dev);
                        ret = -1;
                    }
                }

                if (ret == -1)
                    break;

                dev->sector_pos++;
            }

            if (ret == 1) {
                rdisk_log(dev->log, "%s %i bytes of blocks...\n", out ? "Written" :
                        "Read", *len);

                dev->sector_len -= dev->requested_blocks;
            }
        }
    } else {
        rdisk_command_complete(dev);
        ret = 0;
    }

    return ret;
}

void
rdisk_insert(rdisk_t *dev)
{
    if ((dev != NULL) && (dev->drv != NULL)) {
        if (dev->drv->fp == NULL) {
            dev->unit_attention = 0;
            dev->transition     = 0;
            rdisk_log(dev->log, "Media removal\n");
        } else if (dev->transition) {
            dev->unit_attention = 1;
            /* Turn off the medium changed status. */
            dev->transition     = 0;
            rdisk_log(dev->log, "Media insert\n");
        } else {
            dev->unit_attention = 0;
            dev->transition     = 1;
            rdisk_log(dev->log, "Media transition\n");
        }
    }
}

static int
rdisk_pre_execution_check(rdisk_t *dev, const uint8_t *cdb)
{
    int ready;

    if ((cdb[0] != GPCMD_REQUEST_SENSE) && (dev->cur_lun == SCSI_LUN_USE_CDB) &&
        (cdb[1] & 0xe0)) {
        rdisk_log(dev->log, "Attempting to execute a unknown command targeted at SCSI LUN %i\n",
                ((dev->tf->request_length >> 5) & 7));
        rdisk_invalid_lun(dev, cdb[1] >> 5);
        return 0;
    }

    if (!(rdisk_command_flags[cdb[0]] & IMPLEMENTED)) {
        rdisk_log(dev->log, "Attempting to execute unknown command %02X over %s\n",
                cdb[0], (dev->drv->bus_type == RDISK_BUS_SCSI) ?
                "SCSI" : "ATAPI");

        rdisk_illegal_opcode(dev, cdb[0]);
        return 0;
    }

    if ((dev->drv->bus_type < RDISK_BUS_SCSI) &&
        (rdisk_command_flags[cdb[0]] & SCSI_ONLY)) {
        rdisk_log(dev->log, "Attempting to execute SCSI-only command %02X "
                "over ATAPI\n", cdb[0]);
        rdisk_illegal_opcode(dev, cdb[0]);
        return 0;
    }

    if ((dev->drv->bus_type == RDISK_BUS_SCSI) &&
        (rdisk_command_flags[cdb[0]] & ATAPI_ONLY)) {
        rdisk_log(dev->log, "Attempting to execute ATAPI-only command %02X "
                "over SCSI\n", cdb[0]);
        rdisk_illegal_opcode(dev, cdb[0]);
        return 0;
    }

    if (dev->transition) {
        if ((cdb[0] == GPCMD_TEST_UNIT_READY) || (cdb[0] == GPCMD_REQUEST_SENSE))
            ready = 0;
        else {
            if (!(rdisk_command_flags[cdb[0]] & ALLOW_UA)) {
                rdisk_log(dev->log, "(ext_medium_changed != 0): rdisk_insert()\n");
                rdisk_insert((void *) dev);
            }

            ready = (dev->drv->fp != NULL);
        }
    } else
        ready = (dev->drv->fp != NULL);

    /*
       If the drive is not ready, there is no reason to keep the
       UNIT ATTENTION condition present, as we only use it to mark
       disc changes.
     */
    if (!ready && (dev->unit_attention > 0))
        dev->unit_attention = 0;

    /*
       If the UNIT ATTENTION condition is set and the command does not allow
       execution under it, error out and report the condition.
     */
    if (dev->unit_attention == 1) {
        /*
           Only increment the unit attention phase if the command can
           not pass through it.
         */
        if (!(rdisk_command_flags[cdb[0]] & ALLOW_UA)) {
            rdisk_log(dev->log, "Unit attention now 2\n");
            dev->unit_attention++;
            rdisk_log(dev->log, "UNIT ATTENTION: Command %02X not allowed to pass through\n",
                    cdb[0]);
            rdisk_unit_attention(dev);
            return 0;
        }
    } else if (dev->unit_attention == 2) {
        if (cdb[0] != GPCMD_REQUEST_SENSE) {
            rdisk_log(dev->log, "Unit attention now 0\n");
            dev->unit_attention = 0;
        }
    }

    /*
       Unless the command is REQUEST SENSE, clear the sense. This will *NOT* clear
       the UNIT ATTENTION condition if it's set.
     */
    if (cdb[0] != GPCMD_REQUEST_SENSE)
        rdisk_sense_clear(dev, cdb[0]);

    if (!ready && (rdisk_command_flags[cdb[0]] & CHECK_READY)) {
        rdisk_log(dev->log, "Not ready (%02X)\n", cdb[0]);
        rdisk_not_ready(dev);
        return 0;
    }

    rdisk_log(dev->log, "Continuing with command %02X\n", cdb[0]);
    return 1;
}

static void
rdisk_seek(rdisk_t *dev, const uint32_t pos)
{
    dev->sector_pos = pos;
}

static void
rdisk_rezero(rdisk_t *dev)
{
    dev->sector_pos = dev->sector_len = 0;
    rdisk_seek(dev, 0);
}

void
rdisk_reset(scsi_common_t *sc)
{
    rdisk_t *dev = (rdisk_t *) sc;

    rdisk_rezero(dev);
    dev->tf->status         = 0;
    dev->callback           = 0.0;
    rdisk_set_callback(dev);
    dev->tf->phase          = 1;
    dev->tf->request_length = 0xEB14;
    dev->packet_status      = PHASE_NONE;
    dev->unit_attention     = 0;
    dev->cur_lun            = SCSI_LUN_USE_CDB;
    rdisk_sense_key = rdisk_asc = rdisk_ascq = dev->unit_attention = dev->transition = 0;
    rdisk_info      = 0x00000000;
}

static void
rdisk_request_sense(rdisk_t *dev, uint8_t *buffer, const uint8_t alloc_length, const int desc)
{
    /*Will return 18 bytes of 0*/
    if (alloc_length != 0) {
        memset(buffer, 0, alloc_length);
        if (!desc)
            memcpy(buffer, dev->sense, alloc_length);
        else {
            buffer[1] = rdisk_sense_key;
            buffer[2] = rdisk_asc;
            buffer[3] = rdisk_ascq;
        }
    }

    buffer[0] = desc ? 0x72 : 0xf0;
    if (!desc)
        buffer[7] = 10;

    if (dev->unit_attention && (rdisk_sense_key == 0)) {
        buffer[desc ? 1 : 2]  = SENSE_UNIT_ATTENTION;
        buffer[desc ? 2 : 12] = ASC_MEDIUM_MAY_HAVE_CHANGED;
        buffer[desc ? 3 : 13] = 0;
    }

    rdisk_log(dev->log, "Reporting sense: %02X %02X %02X\n", buffer[2],
            buffer[12], buffer[13]);

    if (buffer[desc ? 1 : 2] == SENSE_UNIT_ATTENTION) {
        /* If the last remaining sense is unit attention, clear
           that condition. */
        dev->unit_attention = 0;
    }

    /* Clear the sense stuff as per the spec. */
    rdisk_sense_clear(dev, GPCMD_REQUEST_SENSE);

    if (dev->transition) {
        rdisk_log(dev->log, "Removable Disk_TRANSITION: rdisk_insert()\n");
        rdisk_insert((void *) dev);
    }
}

static void
rdisk_request_sense_for_scsi(scsi_common_t *sc, uint8_t *buffer, const uint8_t alloc_length)
{
    rdisk_t     *dev   = (rdisk_t *) sc;
    const int  ready = (dev->drv->fp != NULL);

    if (!ready && dev->unit_attention) {
        /*
           If the drive is not ready, there is no reason to keep the UNIT ATTENTION
           condition present, as we only use it to mark disc changes.
         */
        dev->unit_attention = 0;
    }

    /* Do *NOT* advance the unit attention phase. */
    rdisk_request_sense(dev, buffer, alloc_length, 0);
}

static void
rdisk_set_buf_len(const rdisk_t *dev, int32_t *BufLen, int32_t *src_len)
{
    if (dev->drv->bus_type == RDISK_BUS_SCSI) {
        if (*BufLen == -1)
            *BufLen = *src_len;
        else {
            *BufLen  = MIN(*src_len, *BufLen);
            *src_len = *BufLen;
        }
        rdisk_log(dev->log, "Actual transfer length: %i\n", *BufLen);
    }
}

static void
rdisk_command(scsi_common_t *sc, const uint8_t *cdb)
{
    rdisk_t       *dev                = (rdisk_t *) sc;
    char           device_identify[9] = { '8', '6', 'B', '_', 'R', 'D', '0', '0', 0 };
    const uint8_t  scsi_bus           = (dev->drv->scsi_device_id >> 4) & 0x0f;
    const uint8_t  scsi_id            = dev->drv->scsi_device_id & 0x0f;
    int            pos                = 0;
    int            idx                = 0;
    int32_t        blen               = 0;
    uint32_t       i;
    unsigned       preamble_len;
    int32_t        len;
    int32_t        max_len;
    int32_t        alloc_length;
    int            block_desc;
    int            size_idx;
    int32_t *      BufLen;

    if (dev->drv->bus_type == RDISK_BUS_SCSI) {
        BufLen          = &scsi_devices[scsi_bus][scsi_id].buffer_length;
        dev->tf->status &= ~ERR_STAT;
    } else {
        BufLen         = &blen;
        dev->tf->error = 0;
    }

    dev->packet_len  = 0;
    dev->request_pos = 0;

    device_identify[7] = dev->id + 0x30;

    memcpy(dev->current_cdb, cdb, 12);

    if (cdb[0] != 0) {
        rdisk_log(dev->log, "Command 0x%02X, Sense Key %02X, Asc %02X, Ascq %02X, "
                "Unit attention: %i\n",
                cdb[0], rdisk_sense_key, rdisk_asc, rdisk_ascq, dev->unit_attention);
        rdisk_log(dev->log, "Request length: %04X\n", dev->tf->request_length);

        rdisk_log(dev->log, "CDB: %02X %02X %02X %02X %02X %02X %02X %02X "
                "%02X %02X %02X %02X\n",
                cdb[0], cdb[1], cdb[2], cdb[3], cdb[4], cdb[5], cdb[6], cdb[7],
                cdb[8], cdb[9], cdb[10], cdb[11]);
    }

    dev->sector_len = 0;

    rdisk_set_phase(dev, SCSI_PHASE_STATUS);

    /*
       This handles the Not Ready/Unit Attention check if it has to be handled at
       this point.
     */
    if (rdisk_pre_execution_check(dev, cdb) == 0)
        return;

    switch (cdb[0]) {
        case GPCMD_SEND_DIAGNOSTIC:
            if (!(cdb[1] & (1 << 2))) {
                rdisk_invalid_field(dev, cdb[1]);
                return;
            }
            fallthrough;
        case GPCMD_SCSI_RESERVE:
        case GPCMD_SCSI_RELEASE:
        case GPCMD_TEST_UNIT_READY:
            rdisk_set_phase(dev, SCSI_PHASE_STATUS);
            rdisk_command_complete(dev);
            break;

        case GPCMD_FORMAT_UNIT:
            if (dev->drv->read_only)
                rdisk_write_protected(dev);
            else {
                rdisk_set_phase(dev, SCSI_PHASE_STATUS);
                rdisk_command_complete(dev);
            }
            break;

        case GPCMD_IOMEGA_SENSE:
            rdisk_set_phase(dev, SCSI_PHASE_DATA_IN);
            max_len = cdb[4];
            rdisk_buf_alloc(dev, 256);
            rdisk_set_buf_len(dev, BufLen, &max_len);
            memset(dev->buffer, 0, 256);
            if (cdb[2] == 1) {
                /*
                   This page is related to disk health status - setting
                   this page to 0 makes disk health read as "marginal".
                 */
                dev->buffer[0] = 0x58;
                dev->buffer[1] = 0x00;
                for (i = 0x00; i < 0x58; i++)
                    dev->buffer[i + 0x02] = 0xff;
            } else if (cdb[2] == 2) {
                dev->buffer[0] = 0x3d;
                dev->buffer[1] = 0x00;
                for (i = 0x00; i < 0x13; i++)
                    dev->buffer[i + 0x02] = 0x00;
                dev->buffer[0x15] = 0x00;
                if (dev->drv->read_only)
                    dev->buffer[0x15] |= 0x02;
                for (i = 0x00; i < 0x27; i++)
                    dev->buffer[i + 0x16] = 0x00;
            } else {
                rdisk_invalid_field(dev, cdb[2]);
                rdisk_buf_free(dev);
                return;
            }
            rdisk_data_command_finish(dev, 18, 18, cdb[4], 0);
            break;

        case GPCMD_REZERO_UNIT:
            dev->sector_pos = dev->sector_len = 0;
            rdisk_seek(dev, 0);
            rdisk_set_phase(dev, SCSI_PHASE_STATUS);
            break;

        case GPCMD_REQUEST_SENSE:
            /*
               If there's a unit attention condition and there's a buffered not ready, a
               standalone REQUEST SENSE should forget about the not ready, and report unit
               attention straight away.
             */
            rdisk_set_phase(dev, SCSI_PHASE_DATA_IN);
            max_len = cdb[4];

            if (!max_len) {
                rdisk_set_phase(dev, SCSI_PHASE_STATUS);
                dev->packet_status = PHASE_COMPLETE;
                dev->callback      = 20.0 * RDISK_TIME;
                rdisk_set_callback(dev);
                break;
            }

            rdisk_buf_alloc(dev, 256);
            rdisk_set_buf_len(dev, BufLen, &max_len);
            len = (cdb[1] & 1) ? 8 : 18;
            rdisk_request_sense(dev, dev->buffer, max_len, cdb[1] & 1);
            rdisk_data_command_finish(dev, len, len, cdb[4], 0);
            break;

        case GPCMD_MECHANISM_STATUS:
            rdisk_set_phase(dev, SCSI_PHASE_DATA_IN);
            len = (cdb[8] << 8) | cdb[9];

            rdisk_buf_alloc(dev, 8);
            rdisk_set_buf_len(dev, BufLen, &len);

            memset(dev->buffer, 0, 8);
            dev->buffer[5] = 1;

            rdisk_data_command_finish(dev, 8, 8, len, 0);
            break;

        case GPCMD_READ_6:
        case GPCMD_READ_10:
        case GPCMD_READ_12:
            rdisk_set_phase(dev, SCSI_PHASE_DATA_IN);
            alloc_length = 512;

            switch (cdb[0]) {
                case GPCMD_READ_6:
                    dev->sector_len = cdb[4];
                    /*
                       For READ (6) and WRITE (6), a length of 0 indicates a
                       transfer of 256 sectors.
                     */
                    if (dev->sector_len == 0)
                        dev->sector_len = 256;
                    dev->sector_pos = ((((uint32_t) cdb[1]) & 0x1f) << 16) |
                                      (((uint32_t) cdb[2]) << 8) | ((uint32_t) cdb[3]);
                    rdisk_log(dev->log, "Length: %i, LBA: %i\n", dev->sector_len, dev->sector_pos);
                    break;
                case GPCMD_READ_10:
                    dev->sector_len = (cdb[7] << 8) | cdb[8];
                    dev->sector_pos = (cdb[2] << 24) | (cdb[3] << 16) |
                                      (cdb[4] << 8) | cdb[5];
                    rdisk_log(dev->log, "Length: %i, LBA: %i\n", dev->sector_len, dev->sector_pos);
                    break;
                case GPCMD_READ_12:
                    dev->sector_len = (((uint32_t) cdb[6]) << 24) |
                                      (((uint32_t) cdb[7]) << 16) |
                                      (((uint32_t) cdb[8]) << 8) | ((uint32_t) cdb[9]);
                    dev->sector_pos = (((uint32_t) cdb[2]) << 24) |
                                      (((uint32_t) cdb[3]) << 16) |
                                      (((uint32_t) cdb[4]) << 8) | ((uint32_t) cdb[5]);
                    break;

                default:
                    break;
            }

            if (dev->sector_pos >= dev->drv->medium_size)
                rdisk_lba_out_of_range(dev);
            else if (dev->sector_len) {
                max_len               = dev->sector_len;
                dev->requested_blocks = max_len;

                dev->packet_len = max_len * alloc_length;
                rdisk_buf_alloc(dev, dev->packet_len);

                const int ret = rdisk_blocks(dev, &alloc_length, 0);
                alloc_length  = dev->requested_blocks * 512;

                if (ret > 0) {
                    dev->requested_blocks = max_len;
                    dev->packet_len       = alloc_length;

                    rdisk_set_buf_len(dev, BufLen, (int32_t *) &dev->packet_len);

                    rdisk_data_command_finish(dev, alloc_length, 512,
                                            alloc_length, 0);

                    ui_sb_update_icon(SB_RDISK | dev->id,
                                      dev->packet_status != PHASE_COMPLETE);
                } else {
                    rdisk_set_phase(dev, SCSI_PHASE_STATUS);
                    dev->packet_status = (ret < 0) ? PHASE_ERROR : PHASE_COMPLETE;
                    dev->callback      = 20.0 * RDISK_TIME;
                    rdisk_set_callback(dev);
                    rdisk_buf_free(dev);
                }
            } else {
                rdisk_set_phase(dev, SCSI_PHASE_STATUS);
                /* rdisk_log(dev->log, "All done - callback set\n"); */
                dev->packet_status = PHASE_COMPLETE;
                dev->callback      = 20.0 * RDISK_TIME;
                rdisk_set_callback(dev);
                break;
            }
            break;

        case GPCMD_VERIFY_6:
        case GPCMD_VERIFY_10:
        case GPCMD_VERIFY_12:
            if (!(cdb[1] & 2)) {
                rdisk_set_phase(dev, SCSI_PHASE_STATUS);
                rdisk_command_complete(dev);
                break;
            }
            fallthrough;
        case GPCMD_WRITE_6:
        case GPCMD_WRITE_10:
        case GPCMD_WRITE_AND_VERIFY_10:
        case GPCMD_WRITE_12:
        case GPCMD_WRITE_AND_VERIFY_12:
            rdisk_set_phase(dev, SCSI_PHASE_DATA_OUT);
            alloc_length = 512;

            if (dev->drv->read_only) {
                rdisk_write_protected(dev);
                break;
            }

            switch (cdb[0]) {
                case GPCMD_VERIFY_6:
                case GPCMD_WRITE_6:
                    dev->sector_len = cdb[4];
                    /*
                       For READ (6) and WRITE (6), a length of 0 indicates a
                       transfer of 256 sectors.
                     */
                    if (dev->sector_len == 0)
                        dev->sector_len = 256;
                    dev->sector_pos = ((((uint32_t) cdb[1]) & 0x1f) << 16) |
                                      (((uint32_t) cdb[2]) << 8) | ((uint32_t) cdb[3]);
                    break;
                case GPCMD_VERIFY_10:
                case GPCMD_WRITE_10:
                case GPCMD_WRITE_AND_VERIFY_10:
                    dev->sector_len = (cdb[7] << 8) | cdb[8];
                    dev->sector_pos = (cdb[2] << 24) | (cdb[3] << 16) |
                                      (cdb[4] << 8) | cdb[5];
                    rdisk_log(dev->log, "Length: %i, LBA: %i\n",
                            dev->sector_len, dev->sector_pos);
                    break;
                case GPCMD_VERIFY_12:
                case GPCMD_WRITE_12:
                case GPCMD_WRITE_AND_VERIFY_12:
                    dev->sector_len = (((uint32_t) cdb[6]) << 24) |
                                      (((uint32_t) cdb[7]) << 16) |
                                      (((uint32_t) cdb[8]) << 8) | ((uint32_t) cdb[9]);
                    dev->sector_pos = (((uint32_t) cdb[2]) << 24) |
                                      (((uint32_t) cdb[3]) << 16) |
                                      (((uint32_t) cdb[4]) << 8) | ((uint32_t) cdb[5]);
                    break;

                default:
                    break;
            }

            if (dev->sector_pos >= dev->drv->medium_size)
                rdisk_lba_out_of_range(dev);
            if (dev->sector_len) {
                max_len               = dev->sector_len;
                dev->requested_blocks = max_len;

                dev->packet_len = max_len * alloc_length;
                rdisk_buf_alloc(dev, dev->packet_len);

                dev->requested_blocks = max_len;
                dev->packet_len       = max_len << 9;

                rdisk_set_buf_len(dev, BufLen, (int32_t *) &dev->packet_len);

                rdisk_data_command_finish(dev, dev->packet_len, 512,
                                        dev->packet_len, 1);

                ui_sb_update_icon_write(SB_RDISK | dev->id,
                                  dev->packet_status != PHASE_COMPLETE);
            } else {
                rdisk_set_phase(dev, SCSI_PHASE_STATUS);
                /* rdisk_log(dev->log, "All done - callback set\n"); */
                dev->packet_status = PHASE_COMPLETE;
                dev->callback      = 20.0 * RDISK_TIME;
                rdisk_set_callback(dev);
            }
            break;

        case GPCMD_WRITE_SAME_10:
            rdisk_set_phase(dev, SCSI_PHASE_DATA_OUT);
            alloc_length = 512;

            if ((cdb[1] & 6) == 6)
                rdisk_invalid_field(dev, cdb[1]);
            else {
                if (dev->drv->read_only)
                    rdisk_write_protected(dev);
                else {
                    dev->sector_len = (cdb[7] << 8) | cdb[8];
                    dev->sector_pos = (cdb[2] << 24) | (cdb[3] << 16) |
                                      (cdb[4] << 8) | cdb[5];

                    if (dev->sector_pos >= dev->drv->medium_size)
                        rdisk_lba_out_of_range(dev);
                    else if (dev->sector_len) {
                        rdisk_buf_alloc(dev, alloc_length);
                        rdisk_set_buf_len(dev, BufLen, (int32_t *) &dev->packet_len);

                        max_len               = 1;
                        dev->requested_blocks = 1;

                        dev->packet_len = alloc_length;

                        rdisk_set_phase(dev, SCSI_PHASE_DATA_OUT);

                        rdisk_data_command_finish(dev, 512, 512,
                                                alloc_length, 1);

                        ui_sb_update_icon_write(SB_RDISK | dev->id,
                                          dev->packet_status != PHASE_COMPLETE);
                    } else {
                        rdisk_set_phase(dev, SCSI_PHASE_STATUS);
                        /* rdisk_log(dev->log, "All done - callback set\n"); */
                        dev->packet_status = PHASE_COMPLETE;
                        dev->callback      = 20.0 * RDISK_TIME;
                        rdisk_set_callback(dev);
                    }
                }
            }
            break;

        case GPCMD_MODE_SENSE_6:
        case GPCMD_MODE_SENSE_10:
            rdisk_set_phase(dev, SCSI_PHASE_DATA_IN);

            if (dev->drv->bus_type == RDISK_BUS_SCSI)
                block_desc = ((cdb[1] >> 3) & 1) ? 0 : 1;
            else
                block_desc = 0;

            if (cdb[0] == GPCMD_MODE_SENSE_6) {
                len = cdb[4];
                rdisk_buf_alloc(dev, 256);
            } else {
                len = (cdb[8] | (cdb[7] << 8));
                rdisk_buf_alloc(dev, 65536);
            }

            if (zip_mode_sense_page_flags & (1LL << (uint64_t) (cdb[2] & 0x3f))) {
                memset(dev->buffer, 0, len);
                alloc_length = len;

                if (cdb[0] == GPCMD_MODE_SENSE_6) {
                    len            = rdisk_mode_sense(dev, dev->buffer, 4, cdb[2],
                                                      block_desc);
                    len            = MIN(len, alloc_length);
                    dev->buffer[0] = len - 1;
                    dev->buffer[1] = 0;
                    if (block_desc)
                        dev->buffer[3] = 8;
                } else {
                    len            = rdisk_mode_sense(dev, dev->buffer, 8, cdb[2],
                                                      block_desc);
                    len            = MIN(len, alloc_length);
                    dev->buffer[0] = (len - 2) >> 8;
                    dev->buffer[1] = (len - 2) & 255;
                    dev->buffer[2] = 0;
                    if (block_desc) {
                        dev->buffer[6] = 0;
                        dev->buffer[7] = 8;
                    }
                }

                rdisk_set_buf_len(dev, BufLen, &len);

                rdisk_log(dev->log, "Reading mode page: %02X...\n", cdb[2]);

                rdisk_data_command_finish(dev, len, len, alloc_length, 0);
            } else {
                rdisk_invalid_field(dev, cdb[2]);
                rdisk_buf_free(dev);
            }
            break;

        case GPCMD_MODE_SELECT_6:
        case GPCMD_MODE_SELECT_10:
            rdisk_set_phase(dev, SCSI_PHASE_DATA_OUT);

            if (cdb[0] == GPCMD_MODE_SELECT_6) {
                len = cdb[4];
                rdisk_buf_alloc(dev, 256);
            } else {
                len = (cdb[7] << 8) | cdb[8];
                rdisk_buf_alloc(dev, 65536);
            }

            rdisk_set_buf_len(dev, BufLen, &len);

            dev->total_length = len;
            dev->do_page_save = cdb[1] & 1;

            rdisk_data_command_finish(dev, len, len, len, 1);
            return;

        case GPCMD_START_STOP_UNIT:
            rdisk_set_phase(dev, SCSI_PHASE_STATUS);

            switch (cdb[4] & 3) {
                case 0:                 /* Stop the disc. */
                    rdisk_eject(dev->id); /* The Iomega Windows 9x drivers require this. */
                    break;
                case 1: /* Start the disc and read the TOC. */
                    break;
                case 2: /* Eject the disc if possible. */
#if 0
                    rdisk_eject(dev->id);
#endif
                    break;
                case 3: /* Load the disc (close tray). */
                    rdisk_reload(dev->id);
                    break;

                default:
                    break;
            }

            rdisk_command_complete(dev);
            break;

        case GPCMD_INQUIRY:
            rdisk_set_phase(dev, SCSI_PHASE_DATA_IN);

            max_len = cdb[3];
            max_len <<= 8;
            max_len |= cdb[4];

            rdisk_buf_alloc(dev, 65536);

            if (cdb[1] & 1) {
                preamble_len = 4;
                size_idx     = 3;

                dev->buffer[idx++] = 0;
                dev->buffer[idx++] = cdb[2];
                dev->buffer[idx++] = 0;

                idx++;

                switch (cdb[2]) {
                    case 0x00:
                        dev->buffer[idx++] = 0x00;
                        dev->buffer[idx++] = 0x83;
                        break;
                    case 0x83:
                        if (idx + 24 > max_len) {
                            rdisk_data_phase_error(dev, cdb[2]);
                            rdisk_buf_free(dev);
                            return;
                        }

                        dev->buffer[idx++] = 0x02;
                        dev->buffer[idx++] = 0x00;
                        dev->buffer[idx++] = 0x00;
                        dev->buffer[idx++] = 20;
                        ide_padstr8(dev->buffer + idx, 20, "53R141"); /* Serial */
                        idx += 20;

                        if (idx + 72 > cdb[4])
                            goto atapi_out;
                        dev->buffer[idx++] = 0x02;
                        dev->buffer[idx++] = 0x01;
                        dev->buffer[idx++] = 0x00;
                        dev->buffer[idx++] = 68;
                        /* Vendor */
                        if (dev->drv->type >= RDISK_TYPE_ZIP_100)
                            ide_padstr8(dev->buffer + idx, 8, "IOMEGA  ");
                        else
                            ide_padstr8(dev->buffer + 8, 8, EMU_NAME);          /* Vendor */
                        idx += 8;
                        /* Product */
                        if (dev->drv->type == RDISK_TYPE_ZIP_250)
                            ide_padstr8(dev->buffer + idx, 40, "ZIP 250         ");
                        else if (dev->drv->type == RDISK_TYPE_ZIP_100)
                            ide_padstr8(dev->buffer + idx, 40, "ZIP 100         ");
                        else
                            ide_padstr8(dev->buffer + 16, 40, device_identify);      /* Product */
                        idx += 40;
                        ide_padstr8(dev->buffer + idx, 20, "53R141");
                        idx += 20;
                        break;
                    default:
                        rdisk_log(dev->log, "INQUIRY: Invalid page: %02X\n", cdb[2]);
                        rdisk_invalid_field(dev, cdb[2]);
                        rdisk_buf_free(dev);
                        return;
                }
            } else {
                preamble_len = 5;
                size_idx     = 4;

                memset(dev->buffer, 0, 8);
                if ((cdb[1] & 0xe0) || ((dev->cur_lun > 0x00) && (dev->cur_lun < 0xff)))
                    dev->buffer[0] = 0x7f;    /* No physical device on this LUN */
                else
                    dev->buffer[0] = 0x00;    /* Hard disk */
                dev->buffer[1] = 0x80;        /* Removable */
                /* SCSI-2 compliant */
                dev->buffer[2] = (dev->drv->bus_type == RDISK_BUS_SCSI) ? 0x02 : 0x00;
                dev->buffer[3] = (dev->drv->bus_type == RDISK_BUS_SCSI) ? 0x02 : 0x21;
#if 0
                dev->buffer[4] = 31;
#endif
                dev->buffer[4] = 0;
                if (dev->drv->bus_type == RDISK_BUS_SCSI) {
                    dev->buffer[6] = 1;       /* 16-bit transfers supported */
                    dev->buffer[7] = 0x20;    /* Wide bus supported */
                }
                dev->buffer[7] |= 0x02;

                ide_padstr8(dev->buffer + 8, 8, "IOMEGA  ");    /* Vendor */
                if (dev->drv->type == RDISK_TYPE_ZIP_250) {
                    /* Product */
                    ide_padstr8(dev->buffer + 16, 16, "ZIP 250         ");
                    /* Revision */
                    ide_padstr8(dev->buffer + 32, 4, "42.S");
                    /* Date? */
                    if (max_len >= 44)
                        ide_padstr8(dev->buffer + 36, 8, "08/08/01");
                    if (max_len >= 122)
                        ide_padstr8(dev->buffer + 96, 26, "(c) Copyright IOMEGA 2000 "); /* Copyright string */
                } else if (dev->drv->type == RDISK_TYPE_ZIP_100) {
                    /* Product */
                    ide_padstr8(dev->buffer + 16, 16, "ZIP 100         ");
                    /* Revision */
                    ide_padstr8(dev->buffer + 32, 4, "E.08");
                } else {
                    ide_padstr8(dev->buffer + 8, 8,
                                EMU_NAME);          /* Vendor */
                    ide_padstr8(dev->buffer + 16, 16,
                                device_identify);      /* Product */
                    ide_padstr8(dev->buffer + 32, 4,
                                EMU_VERSION_EX);    /* Revision */
                }
                idx = 36;

                if (max_len == 96) {
                    dev->buffer[4] = 91;
                    idx            = 96;
                } else if (max_len == 128) {
                    dev->buffer[4] = 0x75;
                    idx            = 128;
                }
            }

atapi_out:
            dev->buffer[size_idx] = idx - preamble_len;
            len                   = idx;

            len = MIN(len, max_len);
            rdisk_set_buf_len(dev, BufLen, &len);

            rdisk_data_command_finish(dev, len, len, max_len, 0);
            break;

        case GPCMD_PREVENT_REMOVAL:
            rdisk_set_phase(dev, SCSI_PHASE_STATUS);
            rdisk_command_complete(dev);
            break;

        case GPCMD_SEEK_6:
        case GPCMD_SEEK_10:
            rdisk_set_phase(dev, SCSI_PHASE_STATUS);

            switch (cdb[0]) {
                case GPCMD_SEEK_6:
                    pos = (cdb[2] << 8) | cdb[3];
                    break;
                case GPCMD_SEEK_10:
                    pos = (cdb[2] << 24) | (cdb[3] << 16) | (cdb[4] << 8) | cdb[5];
                    break;

                default:
                    break;
            }
            rdisk_seek(dev, pos);
            rdisk_command_complete(dev);
            break;

        case GPCMD_READ_CDROM_CAPACITY:
            rdisk_set_phase(dev, SCSI_PHASE_DATA_IN);

            rdisk_buf_alloc(dev, 8);

            /* IMPORTANT: What's returned is the last LBA block. */
            max_len = dev->drv->medium_size - 1;
            memset(dev->buffer, 0, 8);
            dev->buffer[0] = (max_len >> 24) & 0xff;
            dev->buffer[1] = (max_len >> 16) & 0xff;
            dev->buffer[2] = (max_len >> 8) & 0xff;
            dev->buffer[3] = max_len & 0xff;
            dev->buffer[6] = 2; /* 512 = 0x0200 */
            len            = 8;

            rdisk_set_buf_len(dev, BufLen, &len);

            rdisk_data_command_finish(dev, len, len, len, 0);
            break;

        case GPCMD_IOMEGA_EJECT:
            rdisk_set_phase(dev, SCSI_PHASE_STATUS);
            rdisk_eject(dev->id);
            rdisk_command_complete(dev);
            break;

        case GPCMD_READ_FORMAT_CAPACITIES:
            len = (cdb[7] << 8) | cdb[8];

            rdisk_buf_alloc(dev, len);
            memset(dev->buffer, 0, len);

            pos = 0;

            /* List header */
            dev->buffer[pos++] = 0;
            dev->buffer[pos++] = 0;
            dev->buffer[pos++] = 0;
            if (dev->drv->fp != NULL)
                dev->buffer[pos++] = 16;
            else
                dev->buffer[pos++] = 8;

            /* Current/Maximum capacity header */
            if (dev->drv->type == RDISK_TYPE_ZIP_100) {
                /* ZIP 100 only supports ZIP 100 media as well, so we always return
                   the ZIP 100 size. */
                dev->buffer[pos++] = (ZIP_SECTORS >> 24) & 0xff;
                dev->buffer[pos++] = (ZIP_SECTORS >> 16) & 0xff;
                dev->buffer[pos++] = (ZIP_SECTORS >> 8) & 0xff;
                dev->buffer[pos++] = ZIP_SECTORS & 0xff;
                if (dev->drv->fp != NULL)
                    dev->buffer[pos++] = 2;
                else
                    dev->buffer[pos++] = 3;
            } else {
                /* ZIP 250 also supports ZIP 100 media, so if the medium is inserted,
                   we return the inserted medium's size, otherwise, the ZIP 250 size. */
                if (dev->drv->fp != NULL) {
                    dev->buffer[pos++] = (dev->drv->medium_size >> 24) & 0xff;
                    dev->buffer[pos++] = (dev->drv->medium_size >> 16) & 0xff;
                    dev->buffer[pos++] = (dev->drv->medium_size >> 8) & 0xff;
                    dev->buffer[pos++] = dev->drv->medium_size & 0xff;
                    dev->buffer[pos++] = 2; /* Current medium capacity */
                } else {
                    dev->buffer[pos++] = (ZIP_250_SECTORS >> 24) & 0xff;
                    dev->buffer[pos++] = (ZIP_250_SECTORS >> 16) & 0xff;
                    dev->buffer[pos++] = (ZIP_250_SECTORS >> 8) & 0xff;
                    dev->buffer[pos++] = ZIP_250_SECTORS & 0xff;
                    dev->buffer[pos++] = 3; /* Maximum medium capacity */
                }
            }

            dev->buffer[pos++] = 512 >> 16;
            dev->buffer[pos++] = 512 >> 8;
            dev->buffer[pos++] = 512 & 0xff;

            if (dev->drv->fp != NULL) {
                /* Formattable capacity descriptor */
                dev->buffer[pos++] = (dev->drv->medium_size >> 24) & 0xff;
                dev->buffer[pos++] = (dev->drv->medium_size >> 16) & 0xff;
                dev->buffer[pos++] = (dev->drv->medium_size >> 8) & 0xff;
                dev->buffer[pos++] = dev->drv->medium_size & 0xff;
                dev->buffer[pos++] = 0;
                dev->buffer[pos++] = 512 >> 16;
                dev->buffer[pos++] = 512 >> 8;
                dev->buffer[pos++] = 512 & 0xff;
            }

            rdisk_set_buf_len(dev, BufLen, &len);

            rdisk_data_command_finish(dev, len, len, len, 0);
            break;

        default:
            rdisk_illegal_opcode(dev, cdb[0]);
            break;
    }

#if 0
    rdisk_log(dev->log, "Phase: %02X, request length: %i\n",
            dev->tf->phase, dev->tf->request_length);
#endif

    if ((dev->packet_status == PHASE_COMPLETE) || (dev->packet_status == PHASE_ERROR))
        rdisk_buf_free(dev);
}

static void
rdisk_command_stop(scsi_common_t *sc)
{
    rdisk_t *dev = (rdisk_t *) sc;

    rdisk_command_complete(dev);
    rdisk_buf_free(dev);
}

/* The command second phase function, needed for Mode Select. */
static uint8_t
rdisk_phase_data_out(scsi_common_t *sc)
{
    rdisk_t *dev              = (rdisk_t *) sc;
    int      len            = 0;
    uint8_t  error          = 0;
    uint32_t last_to_write;
    uint32_t i;
    uint16_t block_desc_len;
    uint16_t pos;
    uint16_t param_list_len;
    uint8_t  hdr_len;
    uint8_t  val;

    switch (dev->current_cdb[0]) {
        case GPCMD_VERIFY_6:
        case GPCMD_VERIFY_10:
        case GPCMD_VERIFY_12:
            break;
        case GPCMD_WRITE_6:
        case GPCMD_WRITE_10:
        case GPCMD_WRITE_AND_VERIFY_10:
        case GPCMD_WRITE_12:
        case GPCMD_WRITE_AND_VERIFY_12:
            if (dev->requested_blocks > 0)
                rdisk_blocks(dev, &len, 1);
            break;
        case GPCMD_WRITE_SAME_10:
            if (!dev->current_cdb[7] && !dev->current_cdb[8]) {
                last_to_write = (dev->drv->medium_size - 1);
            } else
                last_to_write = dev->sector_pos + dev->sector_len - 1;

            for (i = dev->sector_pos; i <= last_to_write; i++) {
                if (dev->current_cdb[1] & 2) {
                    dev->buffer[0] = (i >> 24) & 0xff;
                    dev->buffer[1] = (i >> 16) & 0xff;
                    dev->buffer[2] = (i >> 8) & 0xff;
                    dev->buffer[3] = i & 0xff;
                } else if (dev->current_cdb[1] & 4) {
                    /* CHS are 96, 1, 2048 (RDISK 100) and 239, 1, 2048 (RDISK 250) */
                    const uint32_t s = (i % 2048);
                    const uint32_t h = ((i - s) / 2048) % 1;
                    const uint32_t c = ((i - s) / 2048) / 1;
                    dev->buffer[0]   = (c >> 16) & 0xff;
                    dev->buffer[1]   = (c >> 8) & 0xff;
                    dev->buffer[2]   = c & 0xff;
                    dev->buffer[3]   = h & 0xff;
                    dev->buffer[4]   = (s >> 24) & 0xff;
                    dev->buffer[5]   = (s >> 16) & 0xff;
                    dev->buffer[6]   = (s >> 8) & 0xff;
                    dev->buffer[7]   = s & 0xff;
                }
                if (fseek(dev->drv->fp, dev->drv->base + (i << 9),
                          SEEK_SET) == -1)
                    log_fatal(dev->log, "rdisk_phase_data_out(): Error seeking\n");
                if (fwrite(dev->buffer, 1, 512, dev->drv->fp) != 512)
                    log_fatal(dev->log, "rdisk_phase_data_out(): Error writing data\n");
            }

            fflush(dev->drv->fp);
            break;
        case GPCMD_MODE_SELECT_6:
        case GPCMD_MODE_SELECT_10:
            if (dev->current_cdb[0] == GPCMD_MODE_SELECT_10) {
                hdr_len        = 8;
                param_list_len = dev->current_cdb[7];
                param_list_len <<= 8;
                param_list_len |= dev->current_cdb[8];
            } else {
                hdr_len        = 4;
                param_list_len = dev->current_cdb[4];
            }

            if (dev->drv->bus_type == RDISK_BUS_SCSI) {
                if (dev->current_cdb[0] == GPCMD_MODE_SELECT_6) {
                    block_desc_len = dev->buffer[2];
                    block_desc_len <<= 8;
                    block_desc_len |= dev->buffer[3];
                } else {
                    block_desc_len = dev->buffer[6];
                    block_desc_len <<= 8;
                    block_desc_len |= dev->buffer[7];
                }
            } else
                block_desc_len = 0;

            pos = hdr_len + block_desc_len;

            while (1) {
                if (pos >= param_list_len) {
                    rdisk_log(dev->log, "Buffer has only block descriptor\n");
                    break;
                }

                const uint8_t page     = dev->buffer[pos] & 0x3f;
                const uint8_t page_len = dev->buffer[pos + 1];

                pos += 2;

                if (!(zip_mode_sense_page_flags & (1LL << ((uint64_t) page))))
                    error |= 1;
                else for (i = 0; i < page_len; i++) {
                    const uint8_t old_val = dev->ms_pages_saved.pages[page][i + 2];
                    const uint8_t ch      = zip_mode_sense_pages_changeable.pages[page][i + 2];
                    val                   = dev->buffer[pos + i];
                    if (val != old_val) {
                        if (ch)
                            dev->ms_pages_saved.pages[page][i + 2] = val;
                        else {
                            error |= 1;
                            rdisk_invalid_field_pl(dev, val);
                        }
                    }
                }

                pos += page_len;

                if (dev->drv->bus_type == RDISK_BUS_SCSI)
                    val = zip_mode_sense_pages_default_scsi.pages[page][0] & 0x80;
                else
                    val = zip_mode_sense_pages_default.pages[page][0] & 0x80;
                if (dev->do_page_save && val)
                    rdisk_mode_sense_save(dev);

                if (pos >= dev->total_length)
                    break;
            }

            if (error) {
                rdisk_buf_free(dev);
                return 0;
            }
            break;

        default:
            break;
    }

    rdisk_command_stop((scsi_common_t *) dev);
    return 1;
}

/* Peform a master init on the entire module. */
void
rdisk_global_init(void)
{
    /* Clear the global data. */
    memset(rdisk_drives, 0x00, sizeof(rdisk_drives));
}

static int
rdisk_get_max(UNUSED(const ide_t *ide), const int ide_has_dma, const int type)
{
    int ret;

    switch (type) {
        case TYPE_PIO:
            ret = ide_has_dma ? 3 : 0;
            break;
        case TYPE_SDMA:
        default:
            ret = -1;
            break;
        case TYPE_MDMA:
            ret = ide_has_dma ? 1 : -1;
            break;
        case TYPE_UDMA:
            ret = ide_has_dma ? 5 : -1;
            break;
    }

    return ret;
}

static int
rdisk_get_timings(UNUSED(const ide_t *ide), const int ide_has_dma, const int type)
{
    int ret;

    switch (type) {
        case TIMINGS_DMA:
            ret = ide_has_dma ? 0x96 : 0;
            break;
        case TIMINGS_PIO:
            ret = ide_has_dma ? 0xb4 : 0;
            break;
        case TIMINGS_PIO_FC:
            ret = ide_has_dma ? 0xb4 : 0;
            break;
        default:
            ret = 0;
            break;
    }

    return ret;
}

static void
rdisk_zip_100_identify(const ide_t *ide)
{
    ide_padstr((char *) (ide->buffer + 23), "E.08", 8);                  /* Firmware */
    ide_padstr((char *) (ide->buffer + 27), "IOMEGA ZIP 100 ATAPI", 40); /* Model */
}

static void
rdisk_zip_250_identify(const ide_t *ide, const int ide_has_dma)
{
    /* Firmware */
    ide_padstr((char *) (ide->buffer + 23), "42.S", 8);
    /* Model */
    ide_padstr((char *) (ide->buffer + 27), "IOMEGA  ZIP 250       ATAPI", 40);

    if (ide_has_dma) {
        ide->buffer[80] = 0x70;    /* Supported ATA versions : ATA/ATAPI-4 ATA/ATAPI-6 */
        /* Maximum ATA revision supported : ATA/ATAPI-6 T13 1410D revision 3a */
        ide->buffer[81] = 0x19;
    }
}

static void
rdisk_generic_identify(const ide_t *ide, const int ide_has_dma, const rdisk_t *rdisk)
{
    char model[40];

    memset(model, 0, 40);
    snprintf(model, 40, "%s %s%02i", EMU_NAME, "86B_RD", rdisk->id);
    ide_padstr((char *) (ide->buffer + 23), EMU_VERSION_EX, 8);    /* Firmware */
    ide_padstr((char *) (ide->buffer + 27), model, 40);               /* Model */

    if (ide_has_dma) {
        ide->buffer[80] = 0x70;    /* Supported ATA versions : ATA/ATAPI-4 ATA/ATAPI-6 */
        /* Maximum ATA revision supported : ATA/ATAPI-6 T13 1410D revision 3a */
        ide->buffer[81] = 0x19;
    }
}

static void
rdisk_identify(const ide_t *ide, const int ide_has_dma)
{
    const rdisk_t *rdisk = (rdisk_t *) ide->sc;

    /*
       ATAPI device, direct-access device, removable media, interrupt DRQ:

       Using (2 << 5) below makes the ASUS P/I-P54TP4XE misdentify the RDISK drive
       as a LS-120.
     */
    ide->buffer[0] = 0x8000 | (0 << 8) | 0x80 | (1 << 5);
    ide_padstr((char *) (ide->buffer + 10), "", 20);    /* Serial Number */
    ide->buffer[49]  = 0x200;                                  /* LBA supported */
    /* Interpret zero byte count limit as maximum length */
    ide->buffer[126] = 0xfffe;

    if (rdisk_drives[rdisk->id].type == RDISK_TYPE_ZIP_250)
        rdisk_zip_250_identify(ide, ide_has_dma);
    else if (rdisk_drives[rdisk->id].type == RDISK_TYPE_ZIP_100)
        rdisk_zip_100_identify(ide);
    else
        rdisk_generic_identify(ide, ide_has_dma, rdisk);
}

static void
rdisk_drive_reset(const int c)
{
    const uint8_t scsi_bus = (rdisk_drives[c].scsi_device_id >> 4) & 0x0f;
    const uint8_t scsi_id  = rdisk_drives[c].scsi_device_id & 0x0f;

    if (rdisk_drives[c].priv == NULL) {
        rdisk_drives[c].priv = (rdisk_t *) calloc(1, sizeof(rdisk_t));
        rdisk_t *dev         = (rdisk_t *) rdisk_drives[c].priv;

        char n[1024]       = { 0 };

        sprintf(n, "Removable Disk %i", c + 1);
        dev->log           = log_open(n);
    }

    rdisk_t *dev = (rdisk_t *) rdisk_drives[c].priv;

    dev->id      = c;
    dev->cur_lun = SCSI_LUN_USE_CDB;

    if (rdisk_drives[c].bus_type == RDISK_BUS_SCSI) {
        if (dev->tf == NULL)
            dev->tf        = (ide_tf_t *) calloc(1, sizeof(ide_tf_t));

        /* SCSI RDISK, attach to the SCSI bus. */
        scsi_device_t *sd = &scsi_devices[scsi_bus][scsi_id];

        sd->sc             = (scsi_common_t *) dev;
        sd->command        = rdisk_command;
        sd->request_sense  = rdisk_request_sense_for_scsi;
        sd->reset          = rdisk_reset;
        sd->phase_data_out = rdisk_phase_data_out;
        sd->command_stop   = rdisk_command_stop;
        sd->type           = SCSI_REMOVABLE_DISK;
    } else if (rdisk_drives[c].bus_type == RDISK_BUS_ATAPI) {
        /* ATAPI CD-ROM, attach to the IDE bus. */
        ide_t         *id = ide_get_drive(rdisk_drives[c].ide_channel);
        /* If the IDE channel is initialized, we attach to it,
           otherwise, we do nothing - it's going to be a drive
           that's not attached to anything. */
        if (id) {
            id->sc               = (scsi_common_t *) dev;
            dev->tf              = id->tf;
            IDE_ATAPI_IS_EARLY   = 0;
            id->get_max          = rdisk_get_max;
            id->get_timings      = rdisk_get_timings;
            id->identify         = rdisk_identify;
            id->stop             = NULL;
            id->packet_command   = rdisk_command;
            id->device_reset     = rdisk_reset;
            id->phase_data_out   = rdisk_phase_data_out;
            id->command_stop     = rdisk_command_stop;
            id->bus_master_error = rdisk_bus_master_error;
            id->interrupt_drq    = 1;

            ide_atapi_attach(id);
        }
    }
}

void
rdisk_hard_reset(void)
{
    for (uint8_t c = 0; c < RDISK_NUM; c++) {
        if ((rdisk_drives[c].bus_type == RDISK_BUS_ATAPI) || (rdisk_drives[c].bus_type == RDISK_BUS_SCSI)) {

            if (rdisk_drives[c].bus_type == RDISK_BUS_SCSI) {
                const uint8_t scsi_bus = (rdisk_drives[c].scsi_device_id >> 4) & 0x0f;
                const uint8_t scsi_id  = rdisk_drives[c].scsi_device_id & 0x0f;

                /* Make sure to ignore any SCSI RDISK drive that has an out of range SCSI bus. */
                if (scsi_bus >= SCSI_BUS_MAX)
                    continue;

                /* Make sure to ignore any SCSI RDISK drive that has an out of range ID. */
                if (scsi_id >= SCSI_ID_MAX)
                    continue;
            }

            /* Make sure to ignore any ATAPI RDISK drive that has an out of range IDE channel. */
            if ((rdisk_drives[c].bus_type == RDISK_BUS_ATAPI) && (rdisk_drives[c].ide_channel > 7))
                continue;

            rdisk_drive_reset(c);

            rdisk_t *dev = (rdisk_t *) rdisk_drives[c].priv;

            rdisk_log(dev->log, "Removable Disk hard_reset drive=%d\n", c);

            if (dev->tf == NULL)
                continue;

            dev->id  = c;
            dev->drv = &rdisk_drives[c];

            rdisk_init(dev);

            if (strlen(rdisk_drives[c].image_path))
                rdisk_load(dev, rdisk_drives[c].image_path, 0);

            rdisk_mode_sense_load(dev);

            if (rdisk_drives[c].bus_type == RDISK_BUS_SCSI)
                rdisk_log(dev->log, "SCSI RDISK drive %i attached to SCSI ID %i\n",
                        c, rdisk_drives[c].scsi_device_id);
            else if (rdisk_drives[c].bus_type == RDISK_BUS_ATAPI)
                rdisk_log(dev->log, "ATAPI RDISK drive %i attached to IDE channel %i\n",
                        c, rdisk_drives[c].ide_channel);
        }
    }
}

void
rdisk_close(void)
{
    for (uint8_t c = 0; c < RDISK_NUM; c++) {
        if (rdisk_drives[c].bus_type == RDISK_BUS_SCSI) {
            const uint8_t scsi_bus = (rdisk_drives[c].scsi_device_id >> 4) & 0x0f;
            const uint8_t scsi_id  = rdisk_drives[c].scsi_device_id & 0x0f;

            memset(&scsi_devices[scsi_bus][scsi_id], 0x00, sizeof(scsi_device_t));
        }

        rdisk_t *dev = (rdisk_t *) rdisk_drives[c].priv;

        if (dev) {
            rdisk_disk_unload(dev);

            if (dev->tf)
                free(dev->tf);

            if (dev->log != NULL) {
                rdisk_log(dev->log, "Log closed\n");

                log_close(dev->log);
                dev->log = NULL;
            }

            free(dev);
            rdisk_drives[c].priv = NULL;
        }
    }
}
