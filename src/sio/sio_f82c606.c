/*
 * 86Box    A hypervisor and IBM PC system emulator that specializes in
 *          running old operating systems and software designed for IBM
 *          PC systems and compatibles from 1981 through fairly recent
 *          system designs based on the PCI bus.
 *
 *          This file is part of the 86Box distribution.
 *
 *          Implementation of the Chips & Technologies 82C606 CHIPSpak
 *          Multifunction Controller.
 *
 * Relevant literature:
 *
 *          [1] Chips and Technologies, Inc.,
 *              82C605/82C606 CHIPSpak/CHIPSport MULTIFUNCTION CONTROLLERS,
 *              PRELIMINARY Data Sheet, Revision 1, May 1987.
 *              <https://archive.org/download/82C606/82C606.pdf>
 *
 * Authors: Eluan Costa Miranda, <eluancm@gmail.com>
 *          Lubomir Rintel, <lkundrak@v3.sk>
 *          Miran Grca, <mgrca8@gmail.com>
 *
 *          Copyright 2020-2025 Eluan Costa Miranda.
 *          Copyright 2021-2025 Lubomir Rintel.
 *          Copyright 2025 Miran Grca.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <86box/86box.h>
#include <86box/io.h>
#include <86box/timer.h>
#include <86box/device.h>
#include <86box/lpt.h>
#include <86box/serial.h>
#include <86box/gameport.h>
#include <86box/hdc.h>
#include <86box/hdc_ide.h>
#include <86box/fdd.h>
#include <86box/fdc.h>
#include <86box/nvr.h>
#include <86box/sio.h>
#include <86box/machine.h>

typedef struct upc_t {
    int      configuration_state; /* state of algorithm to enter configuration mode */
    int      configuration_mode;
    uint16_t cri_addr; /* cri = configuration index register, addr is even */
    uint16_t cap_addr; /* cap = configuration access port, addr is odd and is cri_addr + 1 */
    uint8_t  cri;      /* currently indexed register */
    uint8_t  last_write;

    /* these regs are not affected by reset */
    uint8_t   regs[15]; /* there are 16 indexes, but there is no need to store the last one which is: R = cri_addr / 4, W = exit config mode */
    nvr_t    *nvr;
    void     *gameport;
    serial_t *uart[2];
    lpt_t    *lpt;
} upc_t;

#ifdef ENABLE_F82C606_LOG
int f82c606_do_log = ENABLE_F82C606_LOG;

static void
f82c606_log(const char *fmt, ...)
{
    va_list ap;

    if (f82c606_do_log) {
        va_start(ap, fmt);
        pclog_ex(fmt, ap);
        va_end(ap);
    }
}
#else
#    define f82c606_log(fmt, ...)
#endif

static void
f82c606_update_ports(upc_t *dev, int set)
{
    uint8_t uart1_int = 0xff;
    uint8_t uart2_int = 0xff;
    uint8_t lpt1_int  = 0xff;
    int     nvr_int   = -1;

    serial_remove(dev->uart[0]);
    serial_remove(dev->uart[1]);
    lpt_port_remove(dev->lpt);

    nvr_at_handler(0, ((uint16_t) dev->regs[3]) << 2, dev->nvr);
    nvr_at_handler(0, 0x70, dev->nvr);

    gameport_remap(dev->gameport, 0);

    if (!set)
        return;

    switch (dev->regs[8] & 0xc0) {
        case 0x40:
            nvr_int = 3;
            break;
        case 0x80:
            uart1_int = COM2_IRQ;
            break;
        case 0xc0:
            uart2_int = COM2_IRQ;
            break;

        default:
            break;
    }

    switch (dev->regs[8] & 0x30) {
        case 0x10:
            nvr_int = 4;
            break;
        case 0x20:
            uart1_int = COM1_IRQ;
            break;
        case 0x30:
            uart2_int = COM1_IRQ;
            break;

        default:
            break;
    }

    switch (dev->regs[8] & 0x0c) {
        case 0x04:
            nvr_int = 5;
            break;
        case 0x08:
            uart1_int = 5;
            break;
        case 0x0c:
            lpt1_int = LPT2_IRQ;
            break;

        default:
            break;
    }

    switch (dev->regs[8] & 0x03) {
        case 0x01:
            nvr_int = 7;
            break;
        case 0x02:
            uart2_int = 7;
            break;
        case 0x03:
            lpt1_int = LPT1_IRQ;
            break;

        default:
            break;
    }

    if (dev->regs[0] & 1) {
        gameport_remap(dev->gameport, ((uint16_t) dev->regs[7]) << 2);
        f82c606_log("Game port at %04X\n", ((uint16_t) dev->regs[7]) << 2);
    }

    if (dev->regs[0] & 2) {
        serial_setup(dev->uart[0], ((uint16_t) dev->regs[4]) << 2, uart1_int);
        f82c606_log("UART 1 at %04X, IRQ %i\n", ((uint16_t) dev->regs[4]) << 2, uart1_int);
    }

    if (dev->regs[0] & 4) {
        serial_setup(dev->uart[1], ((uint16_t) dev->regs[5]) << 2, uart2_int);
        f82c606_log("UART 2 at %04X, IRQ %i\n", ((uint16_t) dev->regs[5]) << 2, uart2_int);
    }

    if (dev->regs[0] & 8) {
        lpt_port_setup(dev->lpt, ((uint16_t) dev->regs[6]) << 2);
        lpt_port_irq(dev->lpt, lpt1_int);
        f82c606_log("LPT1 at %04X, IRQ %i\n", ((uint16_t) dev->regs[6]) << 2, lpt1_int);
    }

    nvr_at_handler(1, ((uint16_t) dev->regs[3]) << 2, dev->nvr);
    nvr_irq_set(nvr_int, dev->nvr);
    f82c606_log("RTC at %04X, IRQ %i\n", ((uint16_t) dev->regs[3]) << 2, nvr_int);
}

static uint8_t
f82c606_config_read(uint16_t port, void *priv)
{
    const upc_t  *dev  = (upc_t *) priv;
    uint8_t       temp = 0xff;

    if (dev->configuration_mode) {
        if (port == dev->cri_addr) {
            temp = dev->cri;
        } else if (port == dev->cap_addr) {
            if (dev->cri == 0xf)
                temp = dev->cri_addr / 4;
            else
                temp = dev->regs[dev->cri];
        }
    }

    return temp;
}

static void
f82c606_config_write(uint16_t port, uint8_t val, void *priv)
{
    upc_t *dev                       = (upc_t *) priv;
    int    configuration_state_event = 0;

    switch (port) {
        case 0x2fa:
            if ((dev->configuration_state == 0) && (val != 0x00) && (val != 0xff)) {
                configuration_state_event = 1;
                dev->last_write           = val;
            } else if (dev->configuration_state == 4) {
                if ((val | dev->last_write) == 0xff) {
                    dev->cri_addr           = ((uint16_t) dev->last_write) << 2;
                    dev->cap_addr           = dev->cri_addr + 1;
                    dev->configuration_mode = 1;
                    f82c606_update_ports(dev, 0);
                    /* TODO: is the value of cri reset here or when exiting configuration mode? */
                    io_sethandler(dev->cri_addr, 0x0002,
                                  f82c606_config_read, NULL, NULL,
                                  f82c606_config_write, NULL, NULL, dev);
                } else
                    dev->configuration_mode = 0;
            }
            break;
        case 0x3fa:
            if ((dev->configuration_state == 1) && ((val | dev->last_write) == 0xff))
                configuration_state_event = 1;
            else if ((dev->configuration_state == 2) && (val == 0x36))
                configuration_state_event = 1;
            else if (dev->configuration_state == 3) {
                dev->last_write           = val;
                configuration_state_event = 1;
            }
            break;
        default:
            break;
    }

    if (dev->configuration_mode) {
        if (port == dev->cri_addr) {
            dev->cri = val & 0xf;
        } else if (port == dev->cap_addr) {
            if (dev->cri == 0xf) {
                dev->configuration_mode = 0;
                io_removehandler(dev->cri_addr, 0x0002,
                                 f82c606_config_read, NULL, NULL,
                                 f82c606_config_write, NULL, NULL, dev);
                /* TODO: any benefit in updating at each register write instead of when exiting config mode? */
                f82c606_update_ports(dev, 1);
            } else
                dev->regs[dev->cri] = val;
        }
    }

    /* TODO: is the state only reset when accessing 0x2fa and 0x3fa wrongly? */
    if ((port == 0x2fa || port == 0x3fa) && configuration_state_event)
        dev->configuration_state++;
    else
        dev->configuration_state = 0;
}

static void
f82c606_reset(void *priv)
{
    upc_t *dev = (upc_t *) priv;

    /* Set power-on defaults. */
    dev->regs[0] = 0x00; /* Enable */
    dev->regs[1] = 0x00; /* Configuration Register */
    dev->regs[2] = 0x00; /* Ext Baud Rate Select */
    dev->regs[3] = 0xb0; /* RTC Base */
    dev->regs[4] = 0xfe; /* UART1 Base */
    dev->regs[5] = 0xbe; /* UART2 Base */
    dev->regs[6] = 0x9e; /* Parallel Base */
    dev->regs[7] = 0x80; /* Game Base */
    dev->regs[8] = 0xec; /* Interrupt Select */

    f82c606_update_ports(dev, 1);
}

static void
f82c606_close(void *priv)
{
    upc_t *dev = (upc_t *) priv;

    free(dev);
}

static void *
f82c606_init(const device_t *info)
{
    upc_t *dev    = (upc_t *) calloc(1, sizeof(upc_t));

    dev->nvr      = device_add(&at_nvr_old_device);
    dev->gameport = gameport_add(&gameport_sio_device);

    dev->uart[0]   = device_add_inst(&ns16450_device, 1);
    dev->uart[1]   = device_add_inst(&ns16450_device, 2);

    dev->lpt       = device_add_inst(&lpt_port_device, 1);

    io_sethandler(0x02fa, 0x0001, NULL, NULL, NULL, f82c606_config_write, NULL, NULL, dev);
    io_sethandler(0x03fa, 0x0001, NULL, NULL, NULL, f82c606_config_write, NULL, NULL, dev);

    f82c606_reset(dev);

    return dev;
}

const device_t f82c606_device = {
    .name          = "82C606 CHIPSpak Multifunction Controller",
    .internal_name = "f82c606",
    .flags         = 0,
    .local         = 0,
    .init          = f82c606_init,
    .close         = f82c606_close,
    .reset         = f82c606_reset,
    .available     = NULL,
    .speed_changed = NULL,
    .force_redraw  = NULL,
    .config        = NULL
};
