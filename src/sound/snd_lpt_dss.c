#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "cpu.h"
#include <86box/86box.h>
#include <86box/filters.h>
#include <86box/timer.h>
#include <86box/device.h>
#include <86box/lpt.h>
#include <86box/machine.h>
#include <86box/sound.h>
#include <86box/plat_unused.h>

typedef struct dss_t {
    void *lpt;

    uint8_t fifo[16];
    int     read_idx;
    int     write_idx;

    uint8_t dac_val;
    uint8_t status;

    pc_timer_t timer;

    int16_t buffer[SOUNDBUFLEN];
    int     pos;
} dss_t;

static void
dss_update(dss_t *dss)
{
    for (; dss->pos < sound_pos_global; dss->pos++)
        dss->buffer[dss->pos] = (int8_t) (dss->dac_val ^ 0x80) * 0x40;
}

static void
dss_update_status(dss_t *dss)
{
    uint8_t old = dss->status;

    dss->status &= ~0x40;

    if ((dss->write_idx - dss->read_idx) >= 16)
        dss->status |= 0x40;

    if ((old & 0x40) && !(dss->status & 0x40))
        lpt_irq(dss->lpt, 1);
}

static void
dss_write_data(uint8_t val, void *priv)
{
    dss_t *dss = (dss_t *) priv;

    if ((dss->write_idx - dss->read_idx) < 16) {
        dss->fifo[dss->write_idx & 15] = val;
        dss->write_idx++;
        dss_update_status(dss);
    }
}

static void
dss_write_ctrl(UNUSED(uint8_t val), UNUSED(void *priv))
{
    //
}

static uint8_t
dss_read_status(void *priv)
{
    const dss_t *dss = (dss_t *) priv;

    return dss->status | 0x0f;
}

static void
dss_get_buffer(int32_t *buffer, int len, void *priv)
{
    dss_t  *dss = (dss_t *) priv;
    int16_t val;
    float   fval;

    dss_update(dss);

    for (int c = 0; c < len * 2; c += 2) {
        fval = dss_iir((float) dss->buffer[c >> 1]);
        val  = fval;

        buffer[c] += val;
        buffer[c + 1] += val;
    }

    dss->pos = 0;
}

static void
dss_callback(void *priv)
{
    dss_t *dss = (dss_t *) priv;

    dss_update(dss);

    if ((dss->write_idx - dss->read_idx) > 0) {
        dss->dac_val = dss->fifo[dss->read_idx & 15];
        dss->read_idx++;
        dss_update_status(dss);
    }

    timer_advance_u64(&dss->timer, (TIMER_USEC * (1000000.0 / 7000.0)));
}

static void *
dss_init(void *lpt)
{
    dss_t *dss = calloc(1, sizeof(dss_t));

    dss->lpt = lpt;

    sound_add_handler(dss_get_buffer, dss);
    timer_add(&dss->timer, dss_callback, dss, 1);

    return dss;
}
static void
dss_close(void *priv)
{
    dss_t *dss = (dss_t *) priv;

    free(dss);
}

const lpt_device_t dss_device = {
    .name             = "Disney Sound Source",
    .internal_name    = "dss",
    .init             = dss_init,
    .close            = dss_close,
    .write_data       = dss_write_data,
    .autofeed         = NULL,
    .strobe           = NULL,
    .write_ctrl       = dss_write_ctrl,
    .read_status      = dss_read_status,
    .read_ctrl        = NULL,
    .epp_write_data   = NULL,
    .epp_request_read = NULL,
    .priv             = NULL,
    .lpt              = NULL
};
