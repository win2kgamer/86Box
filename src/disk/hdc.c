/*
 * 86Box    A hypervisor and IBM PC system emulator that specializes in
 *          running old operating systems and software designed for IBM
 *          PC systems and compatibles from 1981 through fairly recent
 *          system designs based on the PCI bus.
 *
 *          This file is part of the 86Box distribution.
 *
 *          Common code to handle all sorts of disk controllers.
 *
 *
 *
 * Authors: Miran Grca, <mgrca8@gmail.com>
 *          Fred N. van Kempen, <decwiz@yahoo.com>
 *
 *          Copyright 2016-2018 Miran Grca.
 *          Copyright 2017-2018 Fred N. van Kempen.
 */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#define HAVE_STDARG_H
#include <86box/86box.h>
#include <86box/machine.h>
#include <86box/timer.h>
#include <86box/device.h>
#include <86box/hdc.h>
#include <86box/hdc_ide.h>
#include <86box/hdd.h>

int hdc_current[HDC_MAX] = { 0, 0 };

#ifdef ENABLE_HDC_LOG
int hdc_do_log = ENABLE_HDC_LOG;

static void
hdc_log(const char *fmt, ...)
{
    va_list ap;

    if (hdc_do_log) {
        va_start(ap, fmt);
        pclog_ex(fmt, ap);
        va_end(ap);
    }
}
#else
#    define hdc_log(fmt, ...)
#endif

static const struct {
    const device_t *device;
} controllers[] = {
    // clang-format off
    { &device_none                 },
    { &device_internal             },
    /* ISA */
    { &xtide_acculogic_device      },
    { &st506_xt_dtc5150x_device    },
    { &st506_xt_xebec_device       },
    { &xtide_device                },
    { &st506_xt_st11_m_device      },
    { &st506_xt_st11_r_device      },
    { &xta_st50x_device            },
    { &st506_xt_victor_v86p_device },
    { &st506_xt_wd1002a_27x_device },
    { &st506_xt_wd1002a_wx1_device },
    { &st506_xt_wd1004_27x_device  },
    { &st506_xt_wd1004a_27x_device },
    { &st506_xt_wd1004a_wx1_device },
    { &xta_wdxt150_device          },
    { &st506_xt_wdxt_gen_device    },
    /* ISA16 */
    { &ide_isa_device              },
    { &ide_isa_2ch_device          },
    { &xtide_at_device             },
    { &xtide_at_2ch_device         },
    { &xtide_at_ps2_device         },
    { &xtide_at_ps2_2ch_device     },
    { &ide_ter_device              },
    { &ide_qua_device              },
    { &st506_at_wd1003_device      },
    { &esdi_at_wd1007vse1_device   },
    /* MCA */
    { &esdi_ps2_device             },
    { &esdi_integrated_device      },
    { &mcide_device                },
    /* VLB */
    { &ide_vlb_device              },
    { &ide_vlb_2ch_device          },
    /* PCI */
    { &ide_cmd646_ter_qua_device   },
    { &ide_cmd648_ter_qua_device   },
    { &ide_cmd649_ter_qua_device   },
    { &ide_pci_device              },
    { &ide_pci_2ch_device          },
    { NULL                         }
    // clang-format on
};

/* Initialize the 'hdc_current' value based on configured HDC name. */
void
hdc_init(void)
{
    hdc_log("HDC: initializing..\n");

    /* Zero all the hard disk image arrays. */
    hdd_image_init();
}

/* Reset the HDC, whichever one that is. */
void
hdc_reset(void)
{
    for (int i = 0; i < HDC_MAX; i++) {
        hdc_log("HDC %i: reset(current=%d, internal=%d)\n", i,
                hdc_current[i], hdc_current[i] == HDC_INTERNAL);

        /* If we have a valid controller, add its device. */
        if (hdc_current[i] > HDC_INTERNAL)
            device_add_inst(controllers[hdc_current[i]].device, i + 1);
    }
}

const char *
hdc_get_internal_name(int hdc)
{
    return device_get_internal_name(controllers[hdc].device);
}

int
hdc_get_from_internal_name(const char *s)
{
    int c = 0;

    while (controllers[c].device != NULL) {
        if (!strcmp(controllers[c].device->internal_name, s))
            return c;
        c++;
    }

    return 0;
}

const device_t *
hdc_get_device(int hdc)
{
    return (controllers[hdc].device);
}

int
hdc_has_config(int hdc)
{
    const device_t *dev = hdc_get_device(hdc);

    if (dev == NULL)
        return 0;

    if (!device_has_config(dev))
        return 0;

    return 1;
}

int
hdc_get_flags(int hdc)
{
    return (controllers[hdc].device->flags);
}

int
hdc_available(int hdc)
{
    return (device_available(controllers[hdc].device));
}
