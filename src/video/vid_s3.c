/*
 * 86Box    A hypervisor and IBM PC system emulator that specializes in
 *          running old operating systems and software designed for IBM
 *          PC systems and compatibles from 1981 through fairly recent
 *          system designs based on the PCI bus.
 *
 *          This file is part of the 86Box distribution.
 *
 *          S3 emulation.
 *
 *
 *
 * Authors: Sarah Walker, <https://pcem-emulator.co.uk/>
 *          Miran Grca, <mgrca8@gmail.com>
 *
 *          Copyright 2008-2019 Sarah Walker.
 *          Copyright 2016-2019 Miran Grca.
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <wchar.h>
#include <stdatomic.h>
#define HAVE_STDARG_H
#include <86box/86box.h>
#include <86box/device.h>
#include <86box/io.h>
#include <86box/timer.h>
#include <86box/mem.h>
#include <86box/pci.h>
#include <86box/rom.h>
#include <86box/plat.h>
#include <86box/thread.h>
#include <86box/video.h>
#include <86box/i2c.h>
#include <86box/vid_ddc.h>
#include <86box/vid_xga.h>
#include <86box/vid_svga.h>
#include <86box/vid_svga_render.h>
#include "cpu.h"

#define ROM_ORCHID_86C911              "roms/video/s3/BIOS.BIN"
#define ROM_DIAMOND_STEALTH_VRAM       "roms/video/s3/Diamond Stealth VRAM BIOS v2.31 U14.BIN"
#define ROM_AMI_86C924                 "roms/video/s3/S3924AMI.BIN"
#define ROM_METHEUS_86C928             "roms/video/s3/928.VBI"
#define ROM_SPEA_MERCURY_LITE_PCI      "roms/video/s3/SPEAVGA.VBI"
#define ROM_SPEA_MIRAGE_86C801         "roms/video/s3/V7MIRAGE.VBI"
#define ROM_SPEA_MIRAGE_86C805         "roms/video/s3/86c805pspeavlbus.BIN"
#define ROM_MIROCRYSTAL8S_805          "roms/video/s3/S3_805VL_ATT20C491_miroCRYSTAL_8s_ver1.4.BIN"
#define ROM_MIROCRYSTAL10SD_805        "roms/video/s3/MIROcrystal10SD_VLB.VBI"
#define ROM_MIROCRYSTAL20SV_964_VLB    "roms/video/s3/S3_964VL_BT485_27C256_miroCRYSTAL_20sv_ver1.2.bin"
#define ROM_MIROCRYSTAL20SV_964_PCI    "roms/video/s3/mirocrystal.VBI"
#define ROM_MIROCRYSTAL20SD_864_VLB    "roms/video/s3/Miro20SD.BIN"
#define ROM_PHOENIX_86C80X             "roms/video/s3/805.VBI"
#define ROM_WINNER1000_805             "roms/video/s3/v01_05_00-C.BIN"
#define ROM_PARADISE_BAHAMAS64         "roms/video/s3/bahamas64.bin"
#define ROM_PHOENIX_VISION864          "roms/video/s3/86c864p.bin"
#define ROM_DIAMOND_STEALTH64_964      "roms/video/s3/964_107h.rom"
#define ROM_PHOENIX_TRIO32             "roms/video/s3/86c732p.bin"
#define ROM_SPEA_MIRAGE_P64            "roms/video/s3/S3_764VL_SPEAMirageP64VL_ver5_03.BIN"
#define ROM_NUMBER9_9FX                "roms/video/s3/s3_764.bin"
#define ROM_PHOENIX_TRIO64             "roms/video/s3/86c764x1.bin"
#define ROM_DIAMOND_STEALTH64_764      "roms/video/s3/stealt64.bin"
#define ROM_TRIO64V2_DX_VBE20          "roms/video/s3/86c775_2.bin"
#define ROM_STB_POWERGRAPH_64_VIDEO    "roms/video/s3/VBIOS.BIN"
#define ROM_PHOENIX_TRIO64VPLUS        "roms/video/s3/64V1506.ROM"
#define ROM_CARDEX_TRIO64VPLUS         "roms/video/s3/S3T64VP.VBI"
#define ROM_DIAMOND_STEALTH_SE         "roms/video/s3/DiamondStealthSE.VBI"
#define ROM_ELSAWIN2KPROX_964          "roms/video/s3/elsaw20004m.BIN"
#define ROM_ELSAWIN2KPROX              "roms/video/s3/elsaw20008m.BIN"
#define ROM_NUMBER9_9FX_531            "roms/video/s3/numbernine.BIN"
#define ROM_PHOENIX_VISION868          "roms/video/s3/1-DSV3868.BIN"
#define ROM_MIROVIDEO40SV_ERGO_968_PCI "roms/video/s3/S3_968PCI_TVP3026_miroVideo40SV_PCI_1.04.BIN"
#define ROM_SPEA_MERCURY_P64V          "roms/video/s3/S3_968PCI_TVP3026_SPEAMecuryP64V_ver1.01.BIN"
#define ROM_NUMBER9_9FX_771            "roms/video/s3/no9motionfx771.BIN"
#define ROM_PHOENIX_VISION968          "roms/video/s3/1-DSV3968P.BIN"
#define ROM_DIAMOND_STEALTH64_968      "roms/video/s3/vv_303.rom"

enum {
    S3_NUMBER9_9FX,
    S3_PARADISE_BAHAMAS64,
    S3_DIAMOND_STEALTH64_964,
    S3_PHOENIX_TRIO32,
    S3_PHOENIX_TRIO32_ONBOARD,
    S3_PHOENIX_TRIO64,
    S3_PHOENIX_TRIO64_ONBOARD,
    S3_PHOENIX_VISION864,
    S3_DIAMOND_STEALTH64_764,
    S3_SPEA_MIRAGE_86C801,
    S3_SPEA_MIRAGE_86C805,
    S3_PHOENIX_86C801,
    S3_PHOENIX_86C805,
    S3_ORCHID_86C911,
    S3_METHEUS_86C928,
    S3_AMI_86C924,
    S3_TRIO64V2_DX,
    S3_TRIO64V2_DX_ONBOARD,
    S3_STB_POWERGRAPH_64_VIDEO,
    S3_PHOENIX_TRIO64VPLUS,
    S3_PHOENIX_TRIO64VPLUS_ONBOARD,
    S3_CARDEX_TRIO64VPLUS,
    S3_DIAMOND_STEALTH_SE,
    S3_DIAMOND_STEALTH_VRAM,
    S3_ELSAWIN2KPROX_964,
    S3_ELSAWIN2KPROX,
    S3_PHOENIX_VISION868,
    S3_MIROVIDEO40SV_ERGO_968,
    S3_MIROCRYSTAL10SD_805,
    S3_SPEA_MIRAGE_P64,
    S3_SPEA_MERCURY_P64V,
    S3_MIROCRYSTAL20SV_964,
    S3_MIROCRYSTAL20SD_864,
    S3_PHOENIX_VISION968,
    S3_MIROCRYSTAL8S_805,
    S3_NUMBER9_9FX_531,
    S3_NUMBER9_9FX_771,
    S3_SPEA_MERCURY_LITE_PCI,
    S3_86C805_ONBOARD,
    S3_DIAMOND_STEALTH64_968,
    S3_WINNER1000_805
};

enum {
    S3_86C911    = 0x00,
    S3_86C924    = 0x02,
    S3_86C928    = 0x04,
    S3_86C928PCI = 0x06,
    S3_86C801    = 0x07,
    S3_86C805    = 0x08,
    S3_VISION964 = 0x18,
    S3_VISION968 = 0x20,
    S3_VISION864 = 0x28,
    S3_VISION868 = 0x30,
    S3_TRIO32    = 0x38,
    S3_TRIO64    = 0x40,
    S3_TRIO64V   = 0x48,
    S3_TRIO64V2  = 0x50
};

static video_timings_t timing_s3_86c911        = { .type = VIDEO_ISA, .write_b = 4, .write_w = 4, .write_l = 5, .read_b = 20, .read_w = 20, .read_l = 35 };
static video_timings_t timing_s3_86c801        = { .type = VIDEO_ISA, .write_b = 4, .write_w = 4, .write_l = 5, .read_b = 20, .read_w = 20, .read_l = 35 };
static video_timings_t timing_s3_86c805        = { .type = VIDEO_BUS, .write_b = 4, .write_w = 4, .write_l = 5, .read_b = 20, .read_w = 20, .read_l = 35 };
static video_timings_t timing_s3_86c928pci     = { .type = VIDEO_PCI, .write_b = 2, .write_w = 2, .write_l = 4, .read_b = 26, .read_w = 26, .read_l = 42 };
static video_timings_t timing_s3_stealth64_vlb = { .type = VIDEO_BUS, .write_b = 2, .write_w = 2, .write_l = 4, .read_b = 26, .read_w = 26, .read_l = 42 };
static video_timings_t timing_s3_stealth64_pci = { .type = VIDEO_PCI, .write_b = 2, .write_w = 2, .write_l = 4, .read_b = 26, .read_w = 26, .read_l = 42 };
static video_timings_t timing_s3_vision864_vlb = { .type = VIDEO_BUS, .write_b = 4, .write_w = 4, .write_l = 5, .read_b = 20, .read_w = 20, .read_l = 35 };
static video_timings_t timing_s3_vision864_pci = { .type = VIDEO_PCI, .write_b = 4, .write_w = 4, .write_l = 5, .read_b = 20, .read_w = 20, .read_l = 35 };
static video_timings_t timing_s3_vision868_pci = { .type = VIDEO_PCI, .write_b = 4, .write_w = 4, .write_l = 5, .read_b = 20, .read_w = 20, .read_l = 35 };
static video_timings_t timing_s3_vision964_vlb = { .type = VIDEO_BUS, .write_b = 2, .write_w = 2, .write_l = 4, .read_b = 20, .read_w = 20, .read_l = 35 };
static video_timings_t timing_s3_vision964_pci = { .type = VIDEO_PCI, .write_b = 2, .write_w = 2, .write_l = 4, .read_b = 20, .read_w = 20, .read_l = 35 };
static video_timings_t timing_s3_vision968_vlb = { .type = VIDEO_BUS, .write_b = 2, .write_w = 2, .write_l = 4, .read_b = 20, .read_w = 20, .read_l = 35 };
static video_timings_t timing_s3_vision968_pci = { .type = VIDEO_PCI, .write_b = 2, .write_w = 2, .write_l = 4, .read_b = 20, .read_w = 20, .read_l = 35 };
static video_timings_t timing_s3_trio32_vlb    = { .type = VIDEO_BUS, .write_b = 4, .write_w = 3, .write_l = 5, .read_b = 26, .read_w = 26, .read_l = 42 };
static video_timings_t timing_s3_trio32_pci    = { .type = VIDEO_PCI, .write_b = 4, .write_w = 3, .write_l = 5, .read_b = 26, .read_w = 26, .read_l = 42 };
static video_timings_t timing_s3_trio64_vlb    = { .type = VIDEO_BUS, .write_b = 3, .write_w = 2, .write_l = 4, .read_b = 25, .read_w = 25, .read_l = 40 };
static video_timings_t timing_s3_trio64_pci    = { .type = VIDEO_PCI, .write_b = 3, .write_w = 2, .write_l = 4, .read_b = 25, .read_w = 25, .read_l = 40 };
static video_timings_t timing_s3_trio64vp_cardex_pci    = { .type = VIDEO_PCI, .write_b = 2, .write_w = 2, .write_l = 3, .read_b = 19, .read_w = 19, .read_l = 30 };

enum {
    VRAM_4MB   = 0,
    VRAM_8MB   = 3,
    VRAM_2MB   = 4,
    VRAM_1MB   = 6,
    VRAM_512KB = 7
};

#define FIFO_SIZE       65536
#define FIFO_MASK       (FIFO_SIZE - 1)
#define FIFO_ENTRY_SIZE (1 << 31)

#define FIFO_ENTRIES    (s3->fifo_write_idx - s3->fifo_read_idx)
#define FIFO_FULL       ((s3->fifo_write_idx - s3->fifo_read_idx) >= (FIFO_SIZE - 4))
#define FIFO_EMPTY      (s3->fifo_read_idx == s3->fifo_write_idx)

#define FIFO_TYPE       0xff000000
#define FIFO_ADDR       0x00ffffff

enum {
    FIFO_INVALID     = (0x00 << 24),
    FIFO_WRITE_BYTE  = (0x01 << 24),
    FIFO_WRITE_WORD  = (0x02 << 24),
    FIFO_WRITE_DWORD = (0x03 << 24),
    FIFO_OUT_BYTE    = (0x04 << 24),
    FIFO_OUT_WORD    = (0x05 << 24),
    FIFO_OUT_DWORD   = (0x06 << 24)
};

typedef struct
{
    uint32_t addr_type;
    uint32_t val;
} fifo_entry_t;

typedef struct s3_t {
    mem_mapping_t linear_mapping;
    mem_mapping_t mmio_mapping;
    mem_mapping_t new_mmio_mapping;

    uint8_t has_bios;
    rom_t   bios_rom;

    svga_t svga;

    uint8_t bank;
    uint8_t ma_ext;
    int     width, bpp;

    int chip;
    int pci, vlb;
    int atbus;

    uint8_t id, id_ext, id_ext_pci;

    uint8_t int_line;

    int packed_mmio;

    uint32_t linear_base, linear_size;

    uint8_t pci_regs[256];

    uint8_t pci_slot;
    uint8_t irq_state;

    uint32_t vram_mask;
    uint8_t  data_available;

    int card_type;

    struct
    {
        uint16_t subsys_cntl;
        uint16_t setup_md;
        uint8_t  advfunc_cntl;
        uint16_t cur_y, cur_y2;
        uint16_t cur_x, cur_x2;
        uint16_t cur_x_overflow;
        uint16_t destx_overflow;
        uint16_t x2, ropmix;
        uint16_t pat_x, pat_y;
        int16_t  desty_axstp, desty_axstp2;
        int16_t  destx_distp;
        int16_t  maj_axis_pcnt, maj_axis_pcnt2;
        int16_t  err_term, err_term2;
        uint16_t cmd, cmd2;
        uint16_t short_stroke;
        uint32_t pat_bg_color, pat_fg_color;
        uint32_t bkgd_color;
        uint32_t frgd_color;
        uint16_t bkgd_color_back;
        uint16_t frgd_color_back;
        uint32_t wrt_mask;
        uint32_t rd_mask;
        uint32_t color_cmp;
        uint8_t  bkgd_mix;
        uint8_t  frgd_mix;
        uint16_t multifunc_cntl;
        uint16_t multifunc[16];
        uint8_t  pix_trans[4];
        uint8_t  pix_trans_val[2048][2048];
        int      pix_trans_inc;
        int      ssv_state;

        int16_t  cx, cy;
        int16_t  px, py;
        int16_t  sx, sy;
        int16_t  dx, dy;
        uint32_t src, dest, pattern;

        int poly_cx, poly_cx2;
        int poly_cy, poly_cy2;
        int poly_line_cx;
        int point_1_updated, point_2_updated;
        int poly_dx1, poly_dx2;
        int poly_x;

        uint32_t dat_buf;
        int      dat_count;
        int      b2e8_pix, temp_cnt;
        int      ssv_len;
        int      ssv_len_back;
        uint8_t  ssv_dir;
        uint8_t  ssv_draw;
        uint8_t  dat_buf_16bit;
        uint8_t  frgd_color_actual[2];
        uint8_t  bkgd_color_actual[2];
        uint8_t  wrt_mask_actual[2];
        int      color_16bit_check;
        int      color_16bit_check_pixtrans;
        int16_t  minus;
        int16_t  minus_src_24bpp;
        int      rd_mask_16bit_check;
        int      start;
        int      mix_dat_upper;
        int      overflow;

        /*For non-threaded FIFO*/
        int setup_fifo_slot;
        int draw_fifo_slot;
        int setup_fifo, setup_fifo2;
        int draw_fifo, draw_fifo2;
    } accel;

    struct {
        uint32_t   nop;
        uint32_t   cntl;
        uint32_t   stretch_filt_const;
        uint32_t   src_dst_step;
        uint32_t   crop;
        uint32_t   src_base, dest_base;
        uint32_t   src, dest;
        uint32_t   srcbase, dstbase;
        int32_t    dda_init_accumulator;
        int32_t    k1, k2;
        int        dm_index;
        int        dither_matrix_idx;
        int        src_step, dst_step;
        int        sx, sx_backup, sy;
        double     cx, dx;
        double     cy, dy;
        int        sx_scale_int, sx_scale_int_backup;
        double     sx_scale;
        double     sx_scale_dec;
        double     sx_scale_inc;
        double     sx_scale_backup;
        double     sx_scale_len;
        int        dither, host_data, scale_down;
        int        input;
        int        len, start;
        int        odf, idf, yuv;
        atomic_int busy;
    } videoengine;

    struct
    {
        uint32_t pri_ctrl;
        uint32_t chroma_ctrl;
        uint32_t sec_ctrl;
        uint32_t chroma_upper_bound;
        uint32_t sec_filter;
        uint32_t blend_ctrl;
        uint32_t pri_fb0, pri_fb1;
        uint32_t pri_stride;
        uint32_t buffer_ctrl;
        uint32_t sec_fb0, sec_fb1;
        uint32_t sec_stride;
        uint32_t overlay_ctrl;
        int32_t  k1_vert_scale;
        int32_t  k2_vert_scale;
        int32_t  dda_vert_accumulator;
        int32_t  k1_horiz_scale;
        int32_t  k2_horiz_scale;
        int32_t  dda_horiz_accumulator;
        uint32_t fifo_ctrl;
        uint32_t pri_start;
        uint32_t pri_size;
        uint32_t sec_start;
        uint32_t sec_size;

        int sdif;

        int pri_x, pri_y, pri_w, pri_h;
        int sec_x, sec_y, sec_w, sec_h;
    } streams;

    fifo_entry_t fifo[FIFO_SIZE];
    atomic_int   fifo_read_idx, fifo_write_idx;

    uint8_t fifo_thread_run;

    thread_t *fifo_thread;
    event_t  *wake_fifo_thread;
    event_t  *fifo_not_full_event;

    atomic_int blitter_busy;
    uint64_t blitter_time;
    uint64_t status_time;

    uint8_t subsys_cntl, subsys_stat;

    uint32_t hwc_fg_col, hwc_bg_col;
    int      hwc_col_stack_pos;

    int        translate;
    int        enable_8514;
    int        color_16bit;
    atomic_int busy, force_busy;

    uint8_t thread_run, serialport;
    void   *i2c, *ddc;

    int vram;

    void (*accel_start)(int count, int cpu_input, uint32_t mix_dat, uint32_t cpu_dat, void *priv);
} s3_t;

static s3_t             *reset_state = NULL;

#define INT_VSY         (1 << 0)
#define INT_GE_BSY      (1 << 1)
#define INT_FIFO_OVR    (1 << 2)
#define INT_FIFO_EMP    (1 << 3)
#define INT_MASK        0xf

#define SERIAL_PORT_SCW (1 << 0)
#define SERIAL_PORT_SDW (1 << 1)
#define SERIAL_PORT_SCR (1 << 2)
#define SERIAL_PORT_SDR (1 << 3)

static void s3_updatemapping(s3_t *s3);

static void     s3_accel_write(uint32_t addr, uint8_t val, void *priv);
static void     s3_accel_write_w(uint32_t addr, uint16_t val, void *priv);
static void     s3_accel_write_l(uint32_t addr, uint32_t val, void *priv);
static uint8_t  s3_accel_read(uint32_t addr, void *priv);
static uint16_t s3_accel_read_w(uint32_t addr, void *priv);
static uint32_t s3_accel_read_l(uint32_t addr, void *priv);

static void    s3_out(uint16_t addr, uint8_t val, void *priv);
static uint8_t s3_in(uint16_t addr, void *priv);

static void     s3_accel_out_fifo(s3_t *s3, uint16_t port, uint8_t val);
static void     s3_accel_out(uint16_t port, uint8_t val, void *priv);
static void     s3_accel_out_w(uint16_t port, uint16_t val, void *priv);
static void     s3_accel_out_l(uint16_t port, uint32_t val, void *priv);
static uint8_t  s3_accel_in(uint16_t port, void *priv);
static uint16_t s3_accel_in_w(uint16_t port, void *priv);
static uint32_t s3_accel_in_l(uint16_t port, void *priv);
static uint8_t  s3_pci_read(int func, int addr, void *priv);
static void     s3_pci_write(int func, int addr, uint8_t val, void *priv);

#ifdef ENABLE_S3_LOG
int s3_do_log = ENABLE_S3_LOG;

static void
s3_log(const char *fmt, ...)
{
    va_list ap;

    if (s3_do_log) {
        va_start(ap, fmt);
        pclog_ex(fmt, ap);
        va_end(ap);
    }
}
#else
#    define s3_log(fmt, ...)
#endif

/*Remap address for chain-4/doubleword style layout.
  These will stay for convenience.*/
static __inline uint32_t
dword_remap(svga_t *svga, uint32_t in_addr)
{
    if (svga->packed_chain4 || svga->force_old_addr)
        return in_addr;

    return ((in_addr << 2) & 0x3fff0) | ((in_addr >> 14) & 0xc) | (in_addr & ~0x3fffc);
}
static __inline uint32_t
dword_remap_w(svga_t *svga, uint32_t in_addr)
{
    if (svga->packed_chain4 || svga->force_old_addr)
        return in_addr;

    return ((in_addr << 2) & 0x1fff8) | ((in_addr >> 14) & 0x6) | (in_addr & ~0x1fffe);
}
static __inline uint32_t
dword_remap_l(svga_t *svga, uint32_t in_addr)
{
    if (svga->packed_chain4 || svga->force_old_addr)
        return in_addr;

    return ((in_addr << 2) & 0xfffc) | ((in_addr >> 14) & 0x3) | (in_addr & ~0xffff);
}

static __inline void
wake_fifo_thread(s3_t *s3)
{
    thread_set_event(s3->wake_fifo_thread); /*Wake up FIFO thread if moving from idle*/
}

static void
s3_wait_fifo_idle(s3_t *s3)
{
    while (!FIFO_EMPTY) {
        wake_fifo_thread(s3);
        thread_wait_event(s3->fifo_not_full_event, 1);
    }
}

static void
s3_queue(s3_t *s3, uint32_t addr, uint32_t val, uint32_t type)
{
    fifo_entry_t *fifo = &s3->fifo[s3->fifo_write_idx & FIFO_MASK];

    if (FIFO_FULL) {
        thread_reset_event(s3->fifo_not_full_event);
        if (FIFO_FULL) {
            thread_wait_event(s3->fifo_not_full_event, -1); /*Wait for room in ringbuffer*/
        }
    }

    fifo->val       = val;
    fifo->addr_type = (addr & FIFO_ADDR) | type;

    s3->fifo_write_idx++;

    if (FIFO_ENTRIES > 0xe000 || FIFO_ENTRIES < 8)
        wake_fifo_thread(s3);
}

static void
s3_update_irqs(s3_t *s3)
{
    if (!s3->pci)
        return;

    if (s3->subsys_cntl & s3->subsys_stat & INT_MASK)
        pci_set_irq(s3->pci_slot, PCI_INTA, &s3->irq_state);
    else
        pci_clear_irq(s3->pci_slot, PCI_INTA, &s3->irq_state);
}

void        s3_accel_start(int count, int cpu_input, uint32_t mix_dat, uint32_t cpu_dat, void *priv);
void        s3_short_stroke_start(s3_t *s3, uint8_t ssv);
static void s3_visionx68_video_engine_op(uint32_t cpu_dat, s3_t *s3);

#define WRITE8(addr, var, val)                        \
    switch ((addr) &3) {                              \
        case 0:                                       \
            var = (var & 0xffffff00) | (val);         \
            break;                                    \
        case 1:                                       \
            var = (var & 0xffff00ff) | ((val) << 8);  \
            break;                                    \
        case 2:                                       \
            var = (var & 0xff00ffff) | ((val) << 16); \
            break;                                    \
        case 3:                                       \
            var = (var & 0x00ffffff) | ((val) << 24); \
            break;                                    \
    }

#define READ_PIXTRANS_BYTE_IO(n) \
    s3->accel.pix_trans[n] = svga->vram[dword_remap(svga, (s3->accel.dest + s3->accel.cx - s3->accel.minus + n)) & s3->vram_mask];

#define READ_PIXTRANS_BYTE_MM \
    temp = svga->vram[dword_remap(svga, (s3->accel.dest + s3->accel.cx)) & s3->vram_mask];

#define READ_PIXTRANS_WORD                                                                                            \
    if ((s3->bpp == 0) && !s3->color_16bit) {                                                                         \
        temp = svga->vram[dword_remap(svga, (s3->accel.dest + s3->accel.cx)) & s3->vram_mask];                        \
        temp |= (svga->vram[dword_remap(svga, (s3->accel.dest + s3->accel.cx + 1)) & s3->vram_mask] << 8);            \
    } else                                                                                                            \
        temp = vram_w[dword_remap_w(svga, (s3->accel.dest + s3->accel.cx - s3->accel.minus)) & (s3->vram_mask >> 1)];

#define READ_PIXTRANS_LONG                                                                                       \
    if ((s3->bpp == 0) && !s3->color_16bit) {                                                                    \
        temp = svga->vram[dword_remap(svga, (s3->accel.dest + s3->accel.cx)) & s3->vram_mask];                   \
        temp |= (svga->vram[dword_remap(svga, (s3->accel.dest + s3->accel.cx + 1)) & s3->vram_mask] << 8);       \
        temp |= (svga->vram[dword_remap(svga, (s3->accel.dest + s3->accel.cx + 2)) & s3->vram_mask] << 16);      \
        temp |= (svga->vram[dword_remap(svga, (s3->accel.dest + s3->accel.cx + 3)) & s3->vram_mask] << 24);      \
    } else {                                                                                                     \
        temp = vram_w[dword_remap_w(svga, (s3->accel.dest + s3->accel.cx)) & (s3->vram_mask >> 1)];              \
        temp |= (vram_w[dword_remap_w(svga, (s3->accel.dest + s3->accel.cx + 2)) & (s3->vram_mask >> 1)] << 16); \
    }

static int
s3_cpu_src(s3_t *s3)
{
    if (!(s3->accel.cmd & 0x100))
        return 0;

    if (s3->chip >= S3_VISION964)
        return 1;

    if (s3->accel.cmd & 1)
        return 1;

    return 0;
}

static int
s3_cpu_dest(s3_t *s3)
{
    if (!(s3->accel.cmd & 0x100))
        return 0;

    if (s3->chip >= S3_VISION964)
        return 0;

    if (s3->accel.cmd & 1)
        return 0;

    return 1;
}

static int
s3_enable_fifo(s3_t *s3)
{
    const svga_t *svga = &s3->svga;

    if ((s3->chip == S3_TRIO32) || (s3->chip == S3_TRIO64) || (s3->chip == S3_TRIO64V) || (s3->chip == S3_TRIO64V2) || (s3->chip == S3_VISION864) || (s3->chip == S3_VISION964) || (s3->chip == S3_VISION968) || (s3->chip == S3_VISION868))
        return 1; /* FIFO always enabled on these chips. */

    return !!((svga->crtc[0x40] & 0x08) || (s3->accel.advfunc_cntl & 0x40));
}

static void
s3_accel_out_pixtrans_w(s3_t *s3, uint16_t val)
{
    const svga_t *svga = &s3->svga;

    if (s3->accel.cmd & 0x100) {
        s3_log("S3 PIXTRANS_W write: cmd=%03x, pixelcntl=%02x, frgdmix=%02x, bkgdmix=%02x, "
               "curx=%d, val=%04x.\n", s3->accel.cmd, s3->accel.multifunc[0x0a],
               s3->accel.frgd_mix, s3->accel.bkgd_mix, s3->accel.cur_x, val);
        switch (s3->accel.cmd & 0x600) {
            case 0x000:
                if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 0x02)) {
                    if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40)) {
                        if (s3->accel.cmd & 0x1000)
                            val = (val >> 8) | (val << 8);

                        s3->accel_start(8, 1, val | (val << 16), 0, s3);
                    } else {
                        if ((s3->bpp == 0) && s3->color_16bit) {
                            if (s3->accel.rd_mask_16bit_check) {
                                if (s3->accel.cur_x & 0x400)
                                    val = (val >> 8) | (val << 8);
                            } else {
                                if (s3->accel.cur_x & 0x400)
                                    s3->accel.color_16bit_check_pixtrans = 1;
                                else
                                    s3->accel.color_16bit_check_pixtrans = 0;
                            }
                            s3->accel_start(2, 1, 0xffffffff, val | (val << 16), s3);
                        } else
                            s3->accel_start(1, 1, 0xffffffff, val | (val << 16), s3);
                    }
                } else {
                    if ((s3->bpp == 0) && s3->color_16bit) {
                        if (s3->accel.rd_mask_16bit_check) {
                            if (s3->accel.cur_x & 0x400)
                                val = (val >> 8) | (val << 8);
                        } else {
                            if (s3->accel.cur_x & 0x400)
                                s3->accel.color_16bit_check_pixtrans = 1;
                            else
                                s3->accel.color_16bit_check_pixtrans = 0;
                        }
                        s3->accel_start(2, 1, 0xffffffff, val | (val << 16), s3);
                    } else
                        s3->accel_start(1, 1, 0xffffffff, val | (val << 16), s3);
                }
                break;
            case 0x200:
                if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 0x02)) {
                    if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40)) {
                        if (s3->accel.cmd & 0x1000)
                            val = (val >> 8) | (val << 8);

                        s3->accel_start(16, 1, val | (val << 16), 0, s3);
                    } else {
                        if (s3->accel.rd_mask_16bit_check) {
                            if ((s3->accel.cmd == 0x53f1) || (s3->accel.cmd == 0x53b1)) {
                                if (s3->accel.cur_x & 0x400)
                                    val = (val >> 8) | (val << 8);

                                s3->accel_start(2, 1, 0xffffffff, val | (val << 16), s3);

                                val = (val >> 8) | (val << 8);
                            }
                        }
                        s3->accel_start(2, 1, 0xffffffff, val | (val << 16), s3);
                    }
                } else {
                    if (s3->accel.rd_mask_16bit_check) {
                        if ((s3->accel.cmd == 0x53f1) || (s3->accel.cmd == 0x53b1)) {
                            if (s3->accel.cur_x & 0x400)
                                val = (val >> 8) | (val << 8);

                            s3->accel_start(2, 1, 0xffffffff, val | (val << 16), s3);

                            val = (val >> 8) | (val << 8);
                        }
                    }
                    s3->accel_start(2, 1, 0xffffffff, val | (val << 16), s3);
                }
                break;
            case 0x400:
                if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 0x02)) {
                    if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40)) {
                        if (s3->accel.cmd & 0x1000)
                            val = (val >> 8) | (val << 8);

                        if (svga->crtc[0x53] & 0x08)
                            s3->accel_start(32, 1, val | (val << 16), 0, s3);
                        else
                            s3->accel_start(16, 1, val | (val << 16), 0, s3);
                    } else
                        s3->accel_start(4, 1, 0xffffffff, val | (val << 16), s3);
                } else
                    s3->accel_start(4, 1, 0xffffffff, val | (val << 16), s3);
                break;
            case 0x600:
                if (s3->chip == S3_TRIO32 || s3->chip == S3_VISION968 || s3->chip == S3_VISION868 || s3->chip >= S3_TRIO64V) {
                    if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 0x02)) {
                        if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40)) {
                            if (s3->accel.cmd & 0x1000)
                                val = (val >> 8) | (val << 8);
                            s3->accel_start(8, 1, (val >> 8) & 0xff, 0, s3);
                            s3->accel_start(8, 1, val & 0xff, 0, s3);
                        }
                    }
                }
                break;

            default:
                break;
        }
    }
}

static void
s3_accel_out_pixtrans_l(s3_t *s3, uint32_t val)
{
    if (s3->accel.cmd & 0x100) {
        switch (s3->accel.cmd & 0x600) {
            case 0x000:
                if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 2)) {
                    if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40)) {
                        if (s3->accel.cmd & 0x1000)
                            val = ((val & 0xff00ff00) >> 8) | ((val & 0x00ff00ff) << 8);
                        s3->accel_start(8, 1, val, 0, s3);
                        s3->accel_start(8, 1, val >> 16, 0, s3);
                    } else {
                        s3->accel_start(1, 1, 0xffffffff, val, s3);
                        s3->accel_start(1, 1, 0xffffffff, val >> 16, s3);
                    }
                } else {
                    s3->accel_start(1, 1, 0xffffffff, val, s3);
                    s3->accel_start(1, 1, 0xffffffff, val >> 16, s3);
                }
                break;
            case 0x200:
                if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 2)) {
                    if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40)) {
                        if (s3->accel.cmd & 0x1000)
                            val = ((val & 0xff00ff00) >> 8) | ((val & 0x00ff00ff) << 8);
                        s3->accel_start(16, 1, val, 0, s3);
                        s3->accel_start(16, 1, val >> 16, 0, s3);
                    } else {
                        s3->accel_start(2, 1, 0xffffffff, val, s3);
                        s3->accel_start(2, 1, 0xffffffff, val >> 16, s3);
                    }
                } else {
                    s3->accel_start(2, 1, 0xffffffff, val, s3);
                    s3->accel_start(2, 1, 0xffffffff, val >> 16, s3);
                }
                break;
            case 0x400:
                if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 2)) {
                    if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40)) {
                        if (s3->accel.cmd & 0x1000)
                            val = ((val & 0xff000000) >> 24) | ((val & 0x00ff0000) >> 8) | ((val & 0x0000ff00) << 8) | ((val & 0x000000ff) << 24);
                        s3->accel_start(32, 1, val, 0, s3);
                    } else
                        s3->accel_start(4, 1, 0xffffffff, val, s3);
                } else
                    s3->accel_start(4, 1, 0xffffffff, val, s3);
                break;
            case 0x600:
                if (s3->chip == S3_TRIO32 || s3->chip == S3_VISION968 || s3->chip == S3_VISION868 || s3->chip >= S3_TRIO64V) {
                    if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 2)) {
                        if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40)) {
                            if (s3->accel.cmd & 0x1000)
                                val = ((val & 0xff000000) >> 24) | ((val & 0x00ff0000) >> 8) | ((val & 0x0000ff00) << 8) | ((val & 0x000000ff) << 24);
                            s3->accel_start(8, 1, (val >> 24) & 0xff, 0, s3);
                            s3->accel_start(8, 1, (val >> 16) & 0xff, 0, s3);
                            s3->accel_start(8, 1, (val >> 8) & 0xff, 0, s3);
                            s3->accel_start(8, 1, val & 0xff, 0, s3);
                        }
                    }
                }
                break;

            default:
                break;
        }
    }
}

static void
s3_accel_out_fifo(s3_t *s3, uint16_t port, uint8_t val)
{
    svga_t *svga = &s3->svga;

    switch (port) {
        case 0x8148:
        case 0x82e8:
            s3->accel.cur_y        = (s3->accel.cur_y & 0xf00) | val;
            s3->accel.poly_cy      = s3->accel.cur_y;
            break;
        case 0x8149:
        case 0x82e9:
            s3->accel.cur_y        = (s3->accel.cur_y & 0xff) | ((val & 0x0f) << 8);
            s3->accel.poly_cy      = s3->accel.cur_y;
            s3_log("[%04X:%08X] OUT PORTB=%04x, valy=%d.\n", CS, cpu_state.pc, port - 1, s3->accel.cur_y);
            break;
        case 0x814a:
        case 0x82ea:
            s3->accel.cur_y2   = (s3->accel.cur_y2 & 0xf00) | val;
            s3->accel.poly_cy2 = s3->accel.cur_y2;
            break;
        case 0x814b:
        case 0x82eb:
            s3->accel.cur_y2   = (s3->accel.cur_y2 & 0xff) | ((val & 0x0f) << 8);
            s3->accel.poly_cy2 = s3->accel.cur_y2;
            break;

        case 0x8548:
        case 0x86e8:
            s3->accel.cur_x        = (s3->accel.cur_x & 0xf00) | val;
            s3->accel.cur_x_overflow = (s3->accel.cur_x_overflow & 0xff00) | val;
            s3->accel.poly_cx      = s3->accel.cur_x << 20;
            s3->accel.poly_x       = s3->accel.poly_cx >> 20;
            break;
        case 0x8549:
        case 0x86e9:
            s3->accel.cur_x        = (s3->accel.cur_x & 0xff) | ((val & 0x0f) << 8);
            s3->accel.cur_x_overflow = (s3->accel.cur_x_overflow & 0xff) | (val << 8);
            s3->accel.poly_cx = s3->accel.poly_x = s3->accel.cur_x << 20;
            s3->accel.poly_x                     = s3->accel.poly_cx >> 20;
            s3_log("[%04X:%08X] OUT PORTB=%04x, valx=%d, valxover=%d.\n", CS, cpu_state.pc, port - 1, s3->accel.cur_x, s3->accel.cur_x_overflow);
            break;
        case 0x854a:
        case 0x86ea:
            s3->accel.cur_x2   = (s3->accel.cur_x2 & 0xf00) | val;
            s3->accel.poly_cx2 = s3->accel.cur_x2 << 20;
            break;
        case 0x854b:
        case 0x86eb:
            s3->accel.cur_x2   = (s3->accel.cur_x2 & 0xff) | ((val & 0x0f) << 8);
            s3->accel.poly_cx2 = s3->accel.cur_x2 << 20;
            break;

        case 0xcae8:
        case 0x8948:
        case 0x8ae8:
            s3->accel.desty_axstp     = (s3->accel.desty_axstp & 0x3f00) | val;
            s3->accel.point_1_updated = 1;
            break;
        case 0xcae9:
        case 0x8949:
        case 0x8ae9:
            s3->accel.desty_axstp = (s3->accel.desty_axstp & 0xff) | ((val & 0x3f) << 8);
            if (val & 0x20)
                s3->accel.desty_axstp |= ~0x3fff;
            s3->accel.point_1_updated = 1;
            break;
        case 0x894a:
        case 0x8aea:
            s3->accel.desty_axstp2    = (s3->accel.desty_axstp2 & 0x3f00) | val;
            s3->accel.point_2_updated = 1;
            break;
        case 0x849b:
        case 0x8aeb:
            s3->accel.desty_axstp2 = (s3->accel.desty_axstp2 & 0xff) | ((val & 0x3f) << 8);
            if (val & 0x20)
                s3->accel.desty_axstp2 |= ~0x3fff;
            s3->accel.point_2_updated = 1;
            break;

        case 0x8d48:
        case 0x8ee8:
            s3->accel.destx_distp     = (s3->accel.destx_distp & 0x3f00) | val;
            s3->accel.destx_overflow  = (s3->accel.destx_overflow & 0xff00) | val;
            s3->accel.point_1_updated = 1;
            break;
        case 0x8d49:
        case 0x8ee9:
            s3->accel.destx_distp = (s3->accel.destx_distp & 0xff) | ((val & 0x3f) << 8);
            s3->accel.destx_overflow  = (s3->accel.destx_overflow & 0xff) | (val << 8);
            if (val & 0x20)
                s3->accel.destx_distp |= ~0x3fff;
            s3->accel.point_1_updated = 1;
            break;
        case 0x8d4a:
        case 0x8eea:
            s3->accel.x2              = (s3->accel.x2 & 0xf00) | val;
            s3->accel.point_2_updated = 1;
            break;
        case 0x8d4b:
        case 0x8eeb:
            s3->accel.x2              = (s3->accel.x2 & 0xff) | ((val & 0x0f) << 8);
            s3->accel.point_2_updated = 1;
            break;

        case 0x9148:
        case 0x92e8:
            s3->accel.err_term = (s3->accel.err_term & 0x3f00) | val;
            break;
        case 0x9149:
        case 0x92e9:
            s3->accel.err_term = (s3->accel.err_term & 0xff) | ((val & 0x3f) << 8);
            if (val & 0x20)
                s3->accel.err_term |= ~0x1fff;
            break;
        case 0x914a:
        case 0x92ea:
            s3->accel.err_term2 = (s3->accel.err_term2 & 0x3f00) | val;
            break;
        case 0x914b:
        case 0x92eb:
            s3->accel.err_term2 = (s3->accel.err_term2 & 0xff) | ((val & 0x3f) << 8);
            if (val & 0x20)
                s3->accel.err_term2 |= ~0x1fff;
            break;

        case 0x9548:
        case 0x96e8:
            s3->accel.maj_axis_pcnt = (s3->accel.maj_axis_pcnt & 0xf00) | val;
            break;
        case 0x9459:
        case 0x96e9:
            s3->accel.maj_axis_pcnt = (s3->accel.maj_axis_pcnt & 0xff) | ((val & 0x0f) << 8);
            s3_log("[%04X:%08X] OUT PORTB=%04x, valmajx=%d.\n", CS, cpu_state.pc, port - 1, s3->accel.maj_axis_pcnt);
            break;
        case 0x954a:
        case 0x96ea:
            s3->accel.maj_axis_pcnt2 = (s3->accel.maj_axis_pcnt2 & 0xf00) | val;
            break;
        case 0x954b:
        case 0x96eb:
            s3->accel.maj_axis_pcnt2 = (s3->accel.maj_axis_pcnt2 & 0xff) | ((val & 0x0f) << 8);
            break;

        case 0x9948:
        case 0x9ae8:
            s3->accel.cmd      = (s3->accel.cmd & 0xff00) | val;
            s3->data_available = 0;
            s3->accel.b2e8_pix = 0;
            break;
        case 0x9949:
        case 0x9ae9:
            s3->accel.cmd       = (s3->accel.cmd & 0xff) | (val << 8);
            s3->accel.ssv_state = 0;
            if (s3->bpp == 3) {
                if (!(s3->accel.multifunc[0xe] & 0x200))
                    s3->accel.multifunc[0xe] &= ~0x10;
            }
            s3_log("[%04X:%08X] OUT PORTB=%04x, val=%04x.\n", CS, cpu_state.pc, port - 1, s3->accel.cmd);
            s3->accel_start(-1, 0, 0xffffffff, 0, s3);
            break;

        case 0x994a:
        case 0x9aea:
            s3->accel.cmd2 = (s3->accel.cmd2 & 0xff00) | val;
            break;
        case 0x994b:
        case 0x9aeb:
            s3->accel.cmd2 = (s3->accel.cmd2 & 0xff) | (val << 8);
            break;

        case 0x9d48:
        case 0x9ee8:
            s3->accel.short_stroke = (s3->accel.short_stroke & 0xff00) | val;
            break;
        case 0x9d49:
        case 0x9ee9:
            s3->accel.short_stroke = (s3->accel.short_stroke & 0xff) | (val << 8);
            s3->accel.ssv_state    = 1;

            s3->accel.cx = s3->accel.cur_x & 0xfff;
            s3->accel.cy = s3->accel.cur_y & 0xfff;

            if (s3->accel.cmd & 0x1000) {
                s3_short_stroke_start(s3, s3->accel.short_stroke & 0xff);
                s3_short_stroke_start(s3, s3->accel.short_stroke >> 8);
            } else {
                s3_short_stroke_start(s3, s3->accel.short_stroke >> 8);
                s3_short_stroke_start(s3, s3->accel.short_stroke & 0xff);
            }
            break;

        case 0xa148:
        case 0xa2e8:
            s3_log("[%04X:%08X] OUT PORTB=%04x (Background Color), val=%02x.\n", CS, cpu_state.pc, port, val);
            if (s3->bpp == 3) {
                if ((s3->chip >= S3_86C928) && (s3->chip < S3_VISION964)) {
                    if (s3->accel.multifunc[0xe] & 0x10)
                        s3->accel.bkgd_color = (s3->accel.bkgd_color & ~0x00ff0000) | (val << 16);
                    else
                        s3->accel.bkgd_color = (s3->accel.bkgd_color & ~0x000000ff) | val;
                } else if (s3->chip >= S3_VISION964) {
                    if ((s3->accel.multifunc[0xe] & 0x10) && !(s3->accel.multifunc[0xe] & 0x200))
                        s3->accel.bkgd_color = (s3->accel.bkgd_color & ~0x00ff0000) | (val << 16);
                    else
                        s3->accel.bkgd_color = (s3->accel.bkgd_color & ~0x000000ff) | val;
                }
            } else
                s3->accel.bkgd_color = (s3->accel.bkgd_color & ~0x000000ff) | val;
            break;
        case 0xa149:
        case 0xa2e9:
            s3_log("[%04X:%08X] OUT PORTB=%04x (Background Color), val=%02x.\n", CS, cpu_state.pc, port, val);
            if (s3->bpp == 3) {
                if ((s3->chip >= S3_86C928) && (s3->chip < S3_VISION964)) {
                    if (s3->accel.multifunc[0xe] & 0x10)
                        s3->accel.bkgd_color = (s3->accel.bkgd_color & ~0xff000000) | (val << 24);
                    else
                        s3->accel.bkgd_color = (s3->accel.bkgd_color & ~0x0000ff00) | (val << 8);

                    s3->accel.multifunc[0xe] ^= 0x10;
                } else if (s3->chip >= S3_VISION964) {
                    if ((s3->accel.multifunc[0xe] & 0x10) && !(s3->accel.multifunc[0xe] & 0x200))
                        s3->accel.bkgd_color = (s3->accel.bkgd_color & ~0xff000000) | (val << 24);
                    else
                        s3->accel.bkgd_color = (s3->accel.bkgd_color & ~0x0000ff00) | (val << 8);

                    if (!(s3->accel.multifunc[0xe] & 0x200))
                        s3->accel.multifunc[0xe] ^= 0x10;
                }
            } else {
                s3->accel.bkgd_color = (s3->accel.bkgd_color & ~0x0000ff00) | (val << 8);

                if ((s3->chip >= S3_86C928) && (s3->chip < S3_VISION964))
                    s3->accel.multifunc[0xe] ^= 0x10;
                else if (s3->chip >= S3_VISION964) {
                    if (!(s3->accel.multifunc[0xe] & 0x200))
                        s3->accel.multifunc[0xe] ^= 0x10;
                }
            }
            if (s3->accel.color_16bit_check)
                s3->accel.bkgd_color_actual[1] = s3->accel.bkgd_color & 0xff;
            else
                s3->accel.bkgd_color_actual[0] = s3->accel.bkgd_color & 0xff;
            break;
        case 0xa14a:
        case 0xa2ea:
            if (s3->chip >= S3_VISION964) {
                if (s3->accel.multifunc[0xe] & 0x200)
                    s3->accel.bkgd_color = (s3->accel.bkgd_color & ~0x00ff0000) | (val << 16);
                else if (s3->bpp == 3) {
                    if (s3->accel.multifunc[0xe] & 0x10)
                        s3->accel.bkgd_color = (s3->accel.bkgd_color & ~0x00ff0000) | (val << 16);
                    else
                        s3->accel.bkgd_color = (s3->accel.bkgd_color & ~0x000000ff) | val;
                }
            }
            break;
        case 0xa14b:
        case 0xa2eb:
            if (s3->chip >= S3_VISION964) {
                if (s3->accel.multifunc[0xe] & 0x200)
                    s3->accel.bkgd_color = (s3->accel.bkgd_color & ~0xff000000) | (val << 24);
                else if (s3->bpp == 3) {
                    if (s3->accel.multifunc[0xe] & 0x10)
                        s3->accel.bkgd_color = (s3->accel.bkgd_color & ~0xff000000) | (val << 24);
                    else
                        s3->accel.bkgd_color = (s3->accel.bkgd_color & ~0x0000ff00) | (val << 8);

                    s3->accel.multifunc[0xe] ^= 0x10;
                }
            }
            break;

        case 0xa548:
        case 0xa6e8:
            s3_log("[%04X:%08X] OUT PORTB=%04x (Foreground Color), val=%02x.\n", CS, cpu_state.pc, port, val);
            if (s3->bpp == 3) {
                if ((s3->chip >= S3_86C928) && (s3->chip < S3_VISION964)) {
                    if (s3->accel.multifunc[0xe] & 0x10)
                        s3->accel.frgd_color = (s3->accel.frgd_color & ~0x00ff0000) | (val << 16);
                    else
                        s3->accel.frgd_color = (s3->accel.frgd_color & ~0x000000ff) | val;
                } else if (s3->chip >= S3_VISION964) {
                    if ((s3->accel.multifunc[0xe] & 0x10) && !(s3->accel.multifunc[0xe] & 0x200))
                        s3->accel.frgd_color = (s3->accel.frgd_color & ~0x00ff0000) | (val << 16);
                    else
                        s3->accel.frgd_color = (s3->accel.frgd_color & ~0x000000ff) | val;
                }
            } else
                s3->accel.frgd_color = (s3->accel.frgd_color & ~0x000000ff) | val;
            break;
        case 0xa549:
        case 0xa6e9:
            if (s3->bpp == 3) {
                if ((s3->chip >= S3_86C928) && (s3->chip < S3_VISION964)) {
                    if (s3->accel.multifunc[0xe] & 0x10)
                        s3->accel.frgd_color = (s3->accel.frgd_color & ~0xff000000) | (val << 24);
                    else
                        s3->accel.frgd_color = (s3->accel.frgd_color & ~0x0000ff00) | (val << 8);

                    s3->accel.multifunc[0xe] ^= 0x10;
                } else if (s3->chip >= S3_VISION964) {
                    if ((s3->accel.multifunc[0xe] & 0x10) && !(s3->accel.multifunc[0xe] & 0x200))
                        s3->accel.frgd_color = (s3->accel.frgd_color & ~0xff000000) | (val << 24);
                    else
                        s3->accel.frgd_color = (s3->accel.frgd_color & ~0x0000ff00) | (val << 8);

                    if (!(s3->accel.multifunc[0xe] & 0x200))
                        s3->accel.multifunc[0xe] ^= 0x10;
                }
            } else {
                s3->accel.frgd_color = (s3->accel.frgd_color & ~0x0000ff00) | (val << 8);

                if ((s3->chip >= S3_86C928) && (s3->chip < S3_VISION964))
                    s3->accel.multifunc[0xe] ^= 0x10;
                else if (s3->chip >= S3_VISION964) {
                    if (!(s3->accel.multifunc[0xe] & 0x200))
                        s3->accel.multifunc[0xe] ^= 0x10;
                }
            }
            if (s3->accel.color_16bit_check)
                s3->accel.frgd_color_actual[1] = s3->accel.frgd_color & 0xff;
            else
                s3->accel.frgd_color_actual[0] = s3->accel.frgd_color & 0xff;
            break;
        case 0xa54a:
        case 0xa6ea:
            if (s3->chip >= S3_VISION964) {
                if (s3->accel.multifunc[0xe] & 0x200)
                    s3->accel.frgd_color = (s3->accel.frgd_color & ~0x00ff0000) | (val << 16);
                else if (s3->bpp == 3) {
                    if ((s3->accel.multifunc[0xe] & 0x10))
                        s3->accel.frgd_color = (s3->accel.frgd_color & ~0x00ff0000) | (val << 16);
                    else
                        s3->accel.frgd_color = (s3->accel.frgd_color & ~0x000000ff) | val;
                }
            }
            break;
        case 0xa54b:
        case 0xa6eb:
            if (s3->chip >= S3_VISION964) {
                if (s3->accel.multifunc[0xe] & 0x200)
                    s3->accel.frgd_color = (s3->accel.frgd_color & ~0xff000000) | (val << 24);
                else if (s3->bpp == 3) {
                    if (s3->accel.multifunc[0xe] & 0x10)
                        s3->accel.frgd_color = (s3->accel.frgd_color & ~0xff000000) | (val << 24);
                    else
                        s3->accel.frgd_color = (s3->accel.frgd_color & ~0x0000ff00) | (val << 8);

                    s3->accel.multifunc[0xe] ^= 0x10;
                }
            }
            break;

        case 0xa948:
        case 0xaae8:
            s3_log("[%04X:%08X] OUT PORTB=%04x (Write Mask), val=%02x.\n", CS, cpu_state.pc, port, val);
            if (s3->bpp == 3) {
                if ((s3->chip >= S3_86C928) && (s3->chip < S3_VISION964)) {
                    if (s3->accel.multifunc[0xe] & 0x10)
                        s3->accel.wrt_mask = (s3->accel.wrt_mask & ~0x00ff0000) | (val << 16);
                    else
                        s3->accel.wrt_mask = (s3->accel.wrt_mask & ~0x000000ff) | val;
                } else if (s3->chip >= S3_VISION964) {
                    if ((s3->accel.multifunc[0xe] & 0x10) && !(s3->accel.multifunc[0xe] & 0x200))
                        s3->accel.wrt_mask = (s3->accel.wrt_mask & ~0x00ff0000) | (val << 16);
                    else
                        s3->accel.wrt_mask = (s3->accel.wrt_mask & ~0x000000ff) | val;
                }
            } else
                s3->accel.wrt_mask = (s3->accel.wrt_mask & ~0x000000ff) | val;
            break;
        case 0xa949:
        case 0xaae9:
            s3_log("[%04X:%08X] OUT PORTB=%04x (Write Mask), val=%02x.\n", CS, cpu_state.pc, port, val);
            if (s3->bpp == 3) {
                if ((s3->chip >= S3_86C928) && (s3->chip < S3_VISION964)) {
                    if (s3->accel.multifunc[0xe] & 0x10)
                        s3->accel.wrt_mask = (s3->accel.wrt_mask & ~0xff000000) | (val << 24);
                    else
                        s3->accel.wrt_mask = (s3->accel.wrt_mask & ~0x0000ff00) | (val << 8);

                    s3->accel.multifunc[0xe] ^= 0x10;
                } else if (s3->chip >= S3_VISION964) {
                    if ((s3->accel.multifunc[0xe] & 0x10) && !(s3->accel.multifunc[0xe] & 0x200))
                        s3->accel.wrt_mask = (s3->accel.wrt_mask & ~0xff000000) | (val << 24);
                    else
                        s3->accel.wrt_mask = (s3->accel.wrt_mask & ~0x0000ff00) | (val << 8);

                    if (!(s3->accel.multifunc[0xe] & 0x200))
                        s3->accel.multifunc[0xe] ^= 0x10;
                }
            } else {
                s3->accel.wrt_mask = (s3->accel.wrt_mask & ~0x0000ff00) | (val << 8);

                if ((s3->chip >= S3_86C928) && (s3->chip < S3_VISION964))
                    s3->accel.multifunc[0xe] ^= 0x10;
                else if (s3->chip >= S3_VISION964) {
                    if (!(s3->accel.multifunc[0xe] & 0x200))
                        s3->accel.multifunc[0xe] ^= 0x10;
                }
            }
            if (s3->accel.color_16bit_check)
                s3->accel.wrt_mask_actual[1] = s3->accel.wrt_mask & 0xff;
            else
                s3->accel.wrt_mask_actual[0] = s3->accel.wrt_mask & 0xff;
            break;
        case 0xa94a:
        case 0xaaea:
            if (s3->chip >= S3_VISION964) {
                if (s3->accel.multifunc[0xe] & 0x200)
                    s3->accel.wrt_mask = (s3->accel.wrt_mask & ~0x00ff0000) | (val << 16);
                else if (s3->bpp == 3) {
                    if (s3->accel.multifunc[0xe] & 0x10)
                        s3->accel.wrt_mask = (s3->accel.wrt_mask & ~0x00ff0000) | (val << 16);
                    else
                        s3->accel.wrt_mask = (s3->accel.wrt_mask & ~0x000000ff) | val;
                }
            }
            break;
        case 0xa94b:
        case 0xaaeb:
            if (s3->chip >= S3_VISION964) {
                if (s3->accel.multifunc[0xe] & 0x200)
                    s3->accel.wrt_mask = (s3->accel.wrt_mask & ~0xff000000) | (val << 24);
                else if (s3->bpp == 3) {
                    if (s3->accel.multifunc[0xe] & 0x10)
                        s3->accel.wrt_mask = (s3->accel.wrt_mask & ~0xff000000) | (val << 24);
                    else
                        s3->accel.wrt_mask = (s3->accel.wrt_mask & ~0x0000ff00) | (val << 8);

                    s3->accel.multifunc[0xe] ^= 0x10;
                }
            }
            break;

        case 0xad48:
        case 0xaee8:
            if (s3->bpp == 3) {
                if ((s3->chip >= S3_86C928) && (s3->chip < S3_VISION964)) {
                    if (s3->accel.multifunc[0xe] & 0x10)
                        s3->accel.rd_mask = (s3->accel.rd_mask & ~0x00ff0000) | (val << 16);
                    else
                        s3->accel.rd_mask = (s3->accel.rd_mask & ~0x000000ff) | val;
                } else if (s3->chip >= S3_VISION964) {
                    if ((s3->accel.multifunc[0xe] & 0x10) && !(s3->accel.multifunc[0xe] & 0x200))
                        s3->accel.rd_mask = (s3->accel.rd_mask & ~0x00ff0000) | (val << 16);
                    else
                        s3->accel.rd_mask = (s3->accel.rd_mask & ~0x000000ff) | val;
                }
            } else
                s3->accel.rd_mask = (s3->accel.rd_mask & ~0x000000ff) | val;
            break;
        case 0xad49:
        case 0xaee9:
            if (s3->bpp == 3) {
                if ((s3->chip >= S3_86C928) && (s3->chip < S3_VISION964)) {
                    if (s3->accel.multifunc[0xe] & 0x10)
                        s3->accel.rd_mask = (s3->accel.rd_mask & ~0xff000000) | (val << 24);
                    else
                        s3->accel.rd_mask = (s3->accel.rd_mask & ~0x0000ff00) | (val << 8);

                    s3->accel.multifunc[0xe] ^= 0x10;
                } else if (s3->chip >= S3_VISION964) {
                    if ((s3->accel.multifunc[0xe] & 0x10) && !(s3->accel.multifunc[0xe] & 0x200))
                        s3->accel.rd_mask = (s3->accel.rd_mask & ~0xff000000) | (val << 24);
                    else
                        s3->accel.rd_mask = (s3->accel.rd_mask & ~0x0000ff00) | (val << 8);

                    if (!(s3->accel.multifunc[0xe] & 0x200))
                        s3->accel.multifunc[0xe] ^= 0x10;
                }
            } else {
                s3->accel.rd_mask = (s3->accel.rd_mask & ~0x0000ff00) | (val << 8);

                if ((s3->chip >= S3_86C928) && (s3->chip < S3_VISION964))
                    s3->accel.multifunc[0xe] ^= 0x10;
                else if (s3->chip >= S3_VISION964) {
                    if (!(s3->accel.multifunc[0xe] & 0x200))
                        s3->accel.multifunc[0xe] ^= 0x10;
                }
            }
            break;
        case 0xad4a:
        case 0xaeea:
            if (s3->chip >= S3_VISION964) {
                if (s3->accel.multifunc[0xe] & 0x200)
                    s3->accel.rd_mask = (s3->accel.rd_mask & ~0x00ff0000) | (val << 16);
                else if (s3->bpp == 3) {
                    if (s3->accel.multifunc[0xe] & 0x10)
                        s3->accel.rd_mask = (s3->accel.rd_mask & ~0x00ff0000) | (val << 16);
                    else
                        s3->accel.rd_mask = (s3->accel.rd_mask & ~0x000000ff) | val;
                }
            }
            break;
        case 0xad4b:
        case 0xaeeb:
            if (s3->chip >= S3_VISION964) {
                if (s3->accel.multifunc[0xe] & 0x200)
                    s3->accel.rd_mask = (s3->accel.rd_mask & ~0xff000000) | (val << 24);
                else if (s3->bpp == 3) {
                    if (s3->accel.multifunc[0xe] & 0x10)
                        s3->accel.rd_mask = (s3->accel.rd_mask & ~0xff000000) | (val << 24);
                    else
                        s3->accel.rd_mask = (s3->accel.rd_mask & ~0x0000ff00) | (val << 8);

                    s3->accel.multifunc[0xe] ^= 0x10;
                }
            }
            break;

        case 0xb148:
        case 0xb2e8:
            s3_log("[%04X:%08X] OUT PORTB=%04x, val=%02x, CMD=%04x, C(%d,%d).\n", CS, cpu_state.pc, port, val, s3->accel.cmd, s3->accel.cur_x, s3->accel.cur_y);
            if ((s3->accel.multifunc[0xe] & 0x100) || (s3->chip >= S3_VISION964)) {
                s3->accel.b2e8_pix = 0;
                if (s3->bpp == 3) {
                    if ((s3->chip >= S3_86C928) && (s3->chip < S3_VISION964)) {
                        if (s3->accel.multifunc[0xe] & 0x10)
                            s3->accel.color_cmp = (s3->accel.color_cmp & ~0x00ff0000) | (val << 16);
                        else
                            s3->accel.color_cmp = (s3->accel.color_cmp & ~0x000000ff) | val;
                    } else if (s3->chip >= S3_VISION964) {
                        if ((s3->accel.multifunc[0xe] & 0x10) && !(s3->accel.multifunc[0xe] & 0x200))
                            s3->accel.color_cmp = (s3->accel.color_cmp & ~0x00ff0000) | (val << 16);
                        else
                            s3->accel.color_cmp = (s3->accel.color_cmp & ~0x000000ff) | val;
                    }
                } else
                    s3->accel.color_cmp = (s3->accel.color_cmp & ~0x000000ff) | val;
            } else {
                s3->accel.b2e8_pix = 1;
                if (s3_cpu_dest(s3))
                    break;
                s3->accel.pix_trans[0] = val;
                if (s3->accel.cmd & 0x100) {
                    switch (s3->accel.cmd & 0x600) {
                        case 0x000:
                            if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 2)) {
                                if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40))
                                    s3->accel_start(8, 1, s3->accel.pix_trans[0], 0, s3);
                                else
                                    s3->accel_start(1, 1, 0xffffffff, s3->accel.pix_trans[0], s3);
                            } else
                                s3->accel_start(1, 1, 0xffffffff, s3->accel.pix_trans[0], s3);
                            break;

                        default:
                            break;
                    }
                }
            }
            break;
        case 0xb149:
        case 0xb2e9:
            s3_log("[%04X:%08X] OUT PORTB=%04x, val=%02x, CMD=%04x, C(%d,%d).\n", CS, cpu_state.pc, port, val, s3->accel.cmd, s3->accel.cur_x, s3->accel.cur_y);
            if ((s3->accel.multifunc[0xe] & 0x100) || (s3->chip >= S3_VISION964)) {
                s3->accel.b2e8_pix = 0;
                if (s3->bpp == 3) {
                    if ((s3->chip >= S3_86C928) && (s3->chip < S3_VISION964)) {
                        if (s3->accel.multifunc[0xe] & 0x10)
                            s3->accel.color_cmp = (s3->accel.color_cmp & ~0xff000000) | (val << 24);
                        else
                            s3->accel.color_cmp = (s3->accel.color_cmp & ~0x0000ff00) | (val << 8);

                        s3->accel.multifunc[0xe] ^= 0x10;
                    } else if (s3->chip >= S3_VISION964) {
                        if ((s3->accel.multifunc[0xe] & 0x10) && !(s3->accel.multifunc[0xe] & 0x200))
                            s3->accel.color_cmp = (s3->accel.color_cmp & ~0xff000000) | (val << 24);
                        else
                            s3->accel.color_cmp = (s3->accel.color_cmp & ~0x0000ff00) | (val << 8);

                        if (!(s3->accel.multifunc[0xe] & 0x200))
                            s3->accel.multifunc[0xe] ^= 0x10;
                    }
                } else {
                    s3->accel.color_cmp = (s3->accel.color_cmp & ~0x0000ff00) | (val << 8);

                    if ((s3->chip >= S3_86C928) && (s3->chip < S3_VISION964))
                        s3->accel.multifunc[0xe] ^= 0x10;
                    else if (s3->chip >= S3_VISION964) {
                        if (!(s3->accel.multifunc[0xe] & 0x200))
                            s3->accel.multifunc[0xe] ^= 0x10;
                    }
                }
            } else {
                s3->accel.b2e8_pix = 1;
                if (s3_cpu_dest(s3))
                    break;
                s3->accel.pix_trans[1] = val;
                if (s3->accel.cmd & 0x100) {
                    switch (s3->accel.cmd & 0x600) {
                        case 0x000:
                            if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 2)) {
                                if (((s3->accel.frgd_mix & 0x60) != 0x40) ||
                                    ((s3->accel.bkgd_mix & 0x60) != 0x40))
                                    s3->accel_start(8, 1, s3->accel.pix_trans[0] |
                                                    (s3->accel.pix_trans[1] << 8), 0, s3);
                                else
                                    s3->accel_start(1, 1, 0xffffffff, s3->accel.pix_trans[0] |
                                                    (s3->accel.pix_trans[1] << 8), s3);
                            } else
                                s3->accel_start(1, 1, 0xffffffff, s3->accel.pix_trans[0] |
                                                (s3->accel.pix_trans[1] << 8), s3);
                            break;
                        case 0x200:
                            if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 2)) {
                                if (((s3->accel.frgd_mix & 0x60) != 0x40) ||
                                    ((s3->accel.bkgd_mix & 0x60) != 0x40)) {
                                    if (s3->accel.cmd & 0x1000)
                                        s3->accel_start(16, 1, s3->accel.pix_trans[1] |
                                                        (s3->accel.pix_trans[0] << 8), 0, s3);
                                    else
                                        s3->accel_start(16, 1, s3->accel.pix_trans[0] |
                                                        (s3->accel.pix_trans[1] << 8), 0, s3);
                                } else {
                                    if (s3->accel.cmd & 0x1000)
                                        s3->accel_start(2, 1, 0xffffffff, s3->accel.pix_trans[1] |
                                                        (s3->accel.pix_trans[0] << 8), s3);
                                    else
                                        s3->accel_start(2, 1, 0xffffffff, s3->accel.pix_trans[0] |
                                                        (s3->accel.pix_trans[1] << 8), s3);
                                }
                            } else {
                                if (s3->accel.cmd & 0x1000)
                                    s3->accel_start(2, 1, 0xffffffff, s3->accel.pix_trans[1] |
                                                    (s3->accel.pix_trans[0] << 8), s3);
                                else
                                    s3->accel_start(2, 1, 0xffffffff, s3->accel.pix_trans[0] |
                                                    (s3->accel.pix_trans[1] << 8), s3);
                            }
                            break;
                        case 0x400:
                            if (svga->crtc[0x53] & 0x08) {
                                if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 2)) {
                                    if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40))
                                        s3->accel_start(32, 1, s3->accel.pix_trans[0] | (s3->accel.pix_trans[1] << 8), 0, s3);
                                    else
                                        s3->accel_start(4, 1, 0xffffffff, s3->accel.pix_trans[0] | (s3->accel.pix_trans[1] << 8), s3);
                                } else
                                    s3->accel_start(4, 1, 0xffffffff, s3->accel.pix_trans[0] | (s3->accel.pix_trans[1] << 8), s3);
                            }
                            break;
                        case 0x600:
                            if (s3->chip == S3_TRIO32 || s3->chip == S3_VISION968 || s3->chip == S3_VISION868 || s3->chip >= S3_TRIO64V) {
                                if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 2)) {
                                    if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40)) {
                                        s3->accel_start(8, 1, s3->accel.pix_trans[1], 0, s3);
                                        s3->accel_start(8, 1, s3->accel.pix_trans[0], 0, s3);
                                    }
                                }
                            }
                            break;

                        default:
                            break;
                    }

                }
            }
            break;
        case 0xb14a:
        case 0xb2ea:
            if (s3->accel.multifunc[0xe] & 0x200)
                s3->accel.color_cmp = (s3->accel.color_cmp & ~0x00ff0000) | (val << 16);
            else if (s3->bpp == 3) {
                if (s3->accel.multifunc[0xe] & 0x10)
                    s3->accel.color_cmp = (s3->accel.color_cmp & ~0x00ff0000) | (val << 16);
                else
                    s3->accel.color_cmp = (s3->accel.color_cmp & ~0x000000ff) | val;
            }
            break;
        case 0xb14b:
        case 0xb2eb:
            if (s3->accel.multifunc[0xe] & 0x200)
                s3->accel.color_cmp = (s3->accel.color_cmp & ~0xff000000) | (val << 24);
            else if (s3->bpp == 3) {
                if (s3->accel.multifunc[0xe] & 0x10)
                    s3->accel.color_cmp = (s3->accel.color_cmp & ~0xff000000) | (val << 24);
                else
                    s3->accel.color_cmp = (s3->accel.color_cmp & ~0x0000ff00) | (val << 8);

                s3->accel.multifunc[0xe] ^= 0x10;
            }
            break;

        case 0xb548:
        case 0xb6e8:
            s3->accel.bkgd_mix = val;
            break;

        case 0xb948:
        case 0xbae8:
            s3->accel.frgd_mix = val;
            break;

        case 0xbd48:
        case 0xbee8:
            s3->accel.multifunc_cntl = (s3->accel.multifunc_cntl & 0xff00) | val;
            break;
        case 0xbd49:
        case 0xbee9:
            s3->accel.multifunc_cntl                            = (s3->accel.multifunc_cntl & 0xff) | (val << 8);
            s3->accel.multifunc[s3->accel.multifunc_cntl >> 12] = s3->accel.multifunc_cntl & 0xfff;
            if ((s3->accel.multifunc_cntl >> 12) == 5)
                s3_log("S3 multifunc_cntl = %d, val = %03x.\n", s3->accel.multifunc_cntl >> 12, s3->accel.multifunc_cntl & 0xfff);
            break;

        case 0xd148:
        case 0xd2e8:
            s3->accel.ropmix = (s3->accel.ropmix & 0xff00) | val;
            break;
        case 0xd149:
        case 0xd2e9:
            s3->accel.ropmix = (s3->accel.ropmix & 0x00ff) | (val << 8);
            break;
        case 0xe548:
        case 0xe6e8:
            if (s3->bpp == 3) {
                if ((s3->accel.multifunc[0xe] & 0x10) && !(s3->accel.multifunc[0xe] & 0x200))
                    s3->accel.pat_bg_color = (s3->accel.pat_bg_color & ~0x00ff0000) | (val << 16);
                else
                    s3->accel.pat_bg_color = (s3->accel.pat_bg_color & ~0x000000ff) | val;
            } else
                s3->accel.pat_bg_color = (s3->accel.pat_bg_color & ~0x000000ff) | val;
            break;
        case 0xe549:
        case 0xe6e9:
            if (s3->bpp == 3) {
                if ((s3->accel.multifunc[0xe] & 0x10) && !(s3->accel.multifunc[0xe] & 0x200))
                    s3->accel.pat_bg_color = (s3->accel.pat_bg_color & ~0xff000000) | (val << 24);
                else
                    s3->accel.pat_bg_color = (s3->accel.pat_bg_color & ~0x0000ff00) | (val << 8);
            } else
                s3->accel.pat_bg_color = (s3->accel.pat_bg_color & ~0x0000ff00) | (val << 8);

            if (!(s3->accel.multifunc[0xe] & 0x200))
                s3->accel.multifunc[0xe] ^= 0x10;
            break;
        case 0xe54a:
        case 0xe6ea:
            if (s3->accel.multifunc[0xe] & 0x200)
                s3->accel.pat_bg_color = (s3->accel.pat_bg_color & ~0x00ff0000) | (val << 16);
            else if (s3->bpp == 3) {
                if (s3->accel.multifunc[0xe] & 0x10)
                    s3->accel.pat_bg_color = (s3->accel.pat_bg_color & ~0x00ff0000) | (val << 16);
                else
                    s3->accel.pat_bg_color = (s3->accel.pat_bg_color & ~0x000000ff) | val;
            }
            break;
        case 0xe54b:
        case 0xe6eb:
            if (s3->accel.multifunc[0xe] & 0x200)
                s3->accel.pat_bg_color = (s3->accel.pat_bg_color & ~0xff000000) | (val << 24);
            else if (s3->bpp == 3) {
                if (s3->accel.multifunc[0xe] & 0x10)
                    s3->accel.pat_bg_color = (s3->accel.pat_bg_color & ~0xff000000) | (val << 24);
                else
                    s3->accel.pat_bg_color = (s3->accel.pat_bg_color & ~0x0000ff00) | (val << 8);

                s3->accel.multifunc[0xe] ^= 0x10;
            }
            break;
        case 0xe948:
        case 0xeae8:
            s3->accel.pat_y = (s3->accel.pat_y & 0xf00) | val;
            break;
        case 0xe949:
        case 0xeae9:
            s3->accel.pat_y = (s3->accel.pat_y & 0xff) | ((val & 0x1f) << 8);
            break;
        case 0xe94a:
        case 0xeaea:
            s3->accel.pat_x = (s3->accel.pat_x & 0xf00) | val;
            break;
        case 0xe94b:
        case 0xeaeb:
            s3->accel.pat_x = (s3->accel.pat_x & 0xff) | ((val & 0x1f) << 8);
            break;
        case 0xed48:
        case 0xeee8:
            if (s3->bpp == 3) {
                if ((s3->accel.multifunc[0xe] & 0x10) && !(s3->accel.multifunc[0xe] & 0x200))
                    s3->accel.pat_fg_color = (s3->accel.pat_fg_color & ~0x00ff0000) | (val << 16);
                else
                    s3->accel.pat_fg_color = (s3->accel.pat_fg_color & ~0x000000ff) | val;
            } else
                s3->accel.pat_fg_color = (s3->accel.pat_fg_color & ~0x000000ff) | val;
            break;
        case 0xed49:
        case 0xeee9:
            if (s3->bpp == 3) {
                if ((s3->accel.multifunc[0xe] & 0x10) && !(s3->accel.multifunc[0xe] & 0x200))
                    s3->accel.pat_fg_color = (s3->accel.pat_fg_color & ~0xff000000) | (val << 24);
                else
                    s3->accel.pat_fg_color = (s3->accel.pat_fg_color & ~0x0000ff00) | (val << 8);
            } else
                s3->accel.pat_fg_color = (s3->accel.pat_fg_color & ~0x0000ff00) | (val << 8);

            if (!(s3->accel.multifunc[0xe] & 0x200))
                s3->accel.multifunc[0xe] ^= 0x10;
            break;
        case 0xed4a:
        case 0xeeea:
            if (s3->accel.multifunc[0xe] & 0x200)
                s3->accel.pat_fg_color = (s3->accel.pat_fg_color & ~0x00ff0000) | (val << 16);
            else if (s3->bpp == 3) {
                if (s3->accel.multifunc[0xe] & 0x10)
                    s3->accel.pat_fg_color = (s3->accel.pat_fg_color & ~0x00ff0000) | (val << 16);
                else
                    s3->accel.pat_fg_color = (s3->accel.pat_fg_color & ~0x000000ff) | val;
            }
            break;
        case 0xed4b:
        case 0xeeeb:
            if (s3->accel.multifunc[0xe] & 0x200)
                s3->accel.pat_fg_color = (s3->accel.pat_fg_color & ~0xff000000) | (val << 24);
            else if (s3->bpp == 3) {
                if (s3->accel.multifunc[0xe] & 0x10)
                    s3->accel.pat_fg_color = (s3->accel.pat_fg_color & ~0xff000000) | (val << 24);
                else
                    s3->accel.pat_fg_color = (s3->accel.pat_fg_color & ~0x0000ff00) | (val << 8);

                s3->accel.multifunc[0xe] ^= 0x10;
            }
            break;

        case 0xe148:
        case 0xe2e8:
            s3_log("[%04X:%08X] OUT PORTB=%04x, val=%02x, CMD=%04x, C(%d,%d), WRTMASK=%04x.\n", CS, cpu_state.pc, port, val, s3->accel.cmd, s3->accel.cur_x, s3->accel.cur_y, s3->accel.wrt_mask);
            s3_log(".\n");
            s3->accel.b2e8_pix = 0;
            if (s3_cpu_dest(s3))
                break;
            s3->accel.pix_trans[0] = val;
            if (s3->accel.cmd & 0x100) {
                switch (s3->accel.cmd & 0x600) {
                    case 0x000:
                        if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 2)) {
                            if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40))
                                s3->accel_start(8, 1, s3->accel.pix_trans[0], 0, s3);
                            else
                                s3->accel_start(1, 1, 0xffffffff, s3->accel.pix_trans[0], s3);
                        } else {
                            if ((s3->bpp == 0) && s3->color_16bit) {
                                if (s3->accel.rd_mask_16bit_check) {
                                    s3->accel.pix_trans[1] = svga->vram[dword_remap(svga, (s3->accel.dest + s3->accel.cx - s3->accel.minus)) & s3->vram_mask];
                                    if (s3->accel.cmd & 0x1000) {
                                        if (s3->accel.cur_x & 0x400) {
                                            s3_log("Last Pixel Written=%02x (1024) reverse.\n", s3->accel.pix_trans[1]);
                                            s3->accel_start(2, 1, 0xffffffff, s3->accel.pix_trans[0] | (s3->accel.pix_trans[1] << 8), s3);
                                        } else {
                                            s3_log("Last Pixel Written=%02x (0) reverse, cx=%d.\n", s3->accel.pix_trans[1], s3->accel.cx, s3->accel.cur_x);
                                            s3->accel_start(2, 1, 0xffffffff, s3->accel.pix_trans[1] | (s3->accel.pix_trans[0] << 8), s3);
                                        }
                                    } else {
                                        if (s3->accel.cur_x & 0x400) {
                                            s3_log("Last Pixel Written=%02x (1024) normal, cx=%d, curx=%d.\n", s3->accel.pix_trans[1], s3->accel.cx, s3->accel.cur_x);
                                            s3->accel_start(2, 1, 0xffffffff, s3->accel.pix_trans[1] | (s3->accel.pix_trans[0] << 8), s3);
                                        } else {
                                            s3_log("Last Pixel Written=%02x (0) normal, cx=%d, curx=%d.\n", s3->accel.pix_trans[1], s3->accel.cx, s3->accel.cur_x);
                                            s3->accel_start(2, 1, 0xffffffff, s3->accel.pix_trans[0] | (s3->accel.pix_trans[1] << 8), s3);
                                        }
                                    }
                                } else {
                                    s3->accel.pix_trans_val[s3->accel.cy][s3->accel.cx] = val;

                                    if (s3->accel.cur_x & 0x400) {
                                        s3->accel.color_16bit_check_pixtrans = 0;
                                        s3_log("%04X:%08X: Last Pixel Written=%04x (1024) normal, cx=%d, cy=%d.\n", CS, cpu_state.pc, s3->accel.pix_trans_val[s3->accel.cy][s3->accel.cx - s3->accel.minus] | (s3->accel.pix_trans_val[s3->accel.cy][s3->accel.cx] << 8), s3->accel.cx, s3->accel.cy);
                                        s3->accel_start(2, 1, 0xffffffff, s3->accel.pix_trans_val[s3->accel.cy][s3->accel.cx - s3->accel.minus] | (s3->accel.pix_trans_val[s3->accel.cy][s3->accel.cx] << 8), s3);
                                    } else {
                                        s3->accel.color_16bit_check_pixtrans = 1;
                                        s3->accel_start(2, 1, 0xffffffff, s3->accel.pix_trans[0], s3);
                                    }
                                }
                            } else
                                s3->accel_start(1, 1, 0xffffffff, s3->accel.pix_trans[0], s3);
                        }
                        break;

                    default:
                        break;
                }
            }
            break;
        case 0xe149:
        case 0xe2e9:
            s3_log("[%04X:%08X] OUT PORTB=%04x, val=%02x, CMD=%04x, C(%d,%d), WRTMASK=%04x.\n", CS, cpu_state.pc, port, val, s3->accel.cmd, s3->accel.cur_x, s3->accel.cur_y, s3->accel.wrt_mask);
            s3_log(".\n");
            s3->accel.b2e8_pix = 0;
            if (s3_cpu_dest(s3))
                break;
            s3->accel.pix_trans[1] = val;
            if (s3->accel.cmd & 0x100) {
                switch (s3->accel.cmd & 0x600) {
                    case 0x000:
                        if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 2)) {
                            if (((s3->accel.frgd_mix & 0x60) != 0x40) ||
                                ((s3->accel.bkgd_mix & 0x60) != 0x40))
                                s3->accel_start(8, 1, s3->accel.pix_trans[0] |
                                                (s3->accel.pix_trans[1] << 8), 0, s3);
                            else
                                s3->accel_start(1, 1, 0xffffffff, s3->accel.pix_trans[0] |
                                                (s3->accel.pix_trans[1] << 8), s3);
                        } else
                            s3->accel_start(1, 1, 0xffffffff, s3->accel.pix_trans[0] |
                                            (s3->accel.pix_trans[1] << 8), s3);
                        break;
                    case 0x200:
                        if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 2)) {
                            if (((s3->accel.frgd_mix & 0x60) != 0x40) ||
                                ((s3->accel.bkgd_mix & 0x60) != 0x40)) {
                                if (s3->accel.cmd & 0x1000)
                                    s3->accel_start(16, 1, s3->accel.pix_trans[1] |
                                                    (s3->accel.pix_trans[0] << 8), 0, s3);
                                else
                                    s3->accel_start(16, 1, s3->accel.pix_trans[0] |
                                                    (s3->accel.pix_trans[1] << 8), 0, s3);
                            } else {
                                if (s3->accel.cmd & 0x1000)
                                    s3->accel_start(2, 1, 0xffffffff, s3->accel.pix_trans[1] |
                                                    (s3->accel.pix_trans[0] << 8), s3);
                                else
                                    s3->accel_start(2, 1, 0xffffffff, s3->accel.pix_trans[0] |
                                                    (s3->accel.pix_trans[1] << 8), s3);
                            }
                        } else {
                            if (s3->accel.cmd & 0x1000)
                                s3->accel_start(2, 1, 0xffffffff, s3->accel.pix_trans[1] |
                                                (s3->accel.pix_trans[0] << 8), s3);
                            else
                                s3->accel_start(2, 1, 0xffffffff, s3->accel.pix_trans[0] |
                                                (s3->accel.pix_trans[1] << 8), s3);
                        }
                        break;
                    case 0x400:
                        if (svga->crtc[0x53] & 0x08) {
                            if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 2)) {
                                if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40))
                                    s3->accel_start(32, 1, s3->accel.pix_trans[0] | (s3->accel.pix_trans[1] << 8), 0, s3);
                                else
                                    s3->accel_start(4, 1, 0xffffffff, s3->accel.pix_trans[0] | (s3->accel.pix_trans[1] << 8), s3);
                            } else
                                s3->accel_start(4, 1, 0xffffffff, s3->accel.pix_trans[0] | (s3->accel.pix_trans[1] << 8), s3);
                        }
                        break;
                    case 0x600:
                        if (s3->chip == S3_TRIO32 || s3->chip == S3_VISION968 || s3->chip == S3_VISION868 || s3->chip >= S3_TRIO64V) {
                            if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 2)) {
                                if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40)) {
                                    s3->accel_start(8, 1, s3->accel.pix_trans[1], 0, s3);
                                    s3->accel_start(8, 1, s3->accel.pix_trans[0], 0, s3);
                                }
                            }
                        }
                        break;

                    default:
                        break;
                }

            }
            break;
        case 0xe14a:
        case 0xe2ea:
            if (s3_cpu_dest(s3))
                break;
            s3->accel.pix_trans[2] = val;
            break;
        case 0xe14b:
        case 0xe2eb:
            if (s3_cpu_dest(s3))
                break;
            s3->accel.pix_trans[3] = val;
            if (s3->accel.cmd & 0x100) {
                switch (s3->accel.cmd & 0x600) {
                    case 0x000:
                        if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 2)) {
                            if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40))
                                s3->accel_start(8, 1, s3->accel.pix_trans[0] | (s3->accel.pix_trans[1] << 8) | (s3->accel.pix_trans[2] << 16) | (s3->accel.pix_trans[3] << 24), 0, s3);
                            else
                                s3->accel_start(1, 1, 0xffffffff, s3->accel.pix_trans[0] | (s3->accel.pix_trans[1] << 8) | (s3->accel.pix_trans[2] << 16) | (s3->accel.pix_trans[3] << 24), s3);
                        } else
                            s3->accel_start(1, 1, 0xffffffff, s3->accel.pix_trans[0] | (s3->accel.pix_trans[1] << 8) | (s3->accel.pix_trans[2] << 16) | (s3->accel.pix_trans[3] << 24), s3);
                        break;
                    case 0x200:
                        if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 2)) {
                            if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40)) {
                                if (s3->accel.cmd & 0x1000)
                                    s3->accel_start(16, 1, s3->accel.pix_trans[3] | (s3->accel.pix_trans[2] << 8) | (s3->accel.pix_trans[1] << 16) | (s3->accel.pix_trans[0] << 24), 0, s3);
                                else
                                    s3->accel_start(16, 1, s3->accel.pix_trans[0] | (s3->accel.pix_trans[1] << 8) | (s3->accel.pix_trans[2] << 16) | (s3->accel.pix_trans[3] << 24), 0, s3);
                            } else {
                                if (s3->accel.cmd & 0x1000)
                                    s3->accel_start(2, 1, 0xffffffff, s3->accel.pix_trans[3] | (s3->accel.pix_trans[2] << 8) | (s3->accel.pix_trans[1] << 16) | (s3->accel.pix_trans[0] << 24), s3);
                                else
                                    s3->accel_start(2, 1, 0xffffffff, s3->accel.pix_trans[0] | (s3->accel.pix_trans[1] << 8) | (s3->accel.pix_trans[2] << 16) | (s3->accel.pix_trans[3] << 24), s3);
                            }
                        } else {
                            if (s3->accel.cmd & 0x1000)
                                s3->accel_start(2, 1, 0xffffffff, s3->accel.pix_trans[3] | (s3->accel.pix_trans[2] << 8) | (s3->accel.pix_trans[1] << 16) | (s3->accel.pix_trans[0] << 24), s3);
                            else
                                s3->accel_start(2, 1, 0xffffffff, s3->accel.pix_trans[0] | (s3->accel.pix_trans[1] << 8) | (s3->accel.pix_trans[2] << 16) | (s3->accel.pix_trans[3] << 24), s3);
                        }
                        break;
                    case 0x400:
                        if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 2)) {
                            if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40))
                                s3->accel_start(32, 1, s3->accel.pix_trans[0] | (s3->accel.pix_trans[1] << 8) | (s3->accel.pix_trans[2] << 16) | (s3->accel.pix_trans[3] << 24), 0, s3);
                            else
                                s3->accel_start(4, 1, 0xffffffff, s3->accel.pix_trans[0] | (s3->accel.pix_trans[1] << 8) | (s3->accel.pix_trans[2] << 16) | (s3->accel.pix_trans[3] << 24), s3);
                        } else
                            s3->accel_start(4, 1, 0xffffffff, s3->accel.pix_trans[0] | (s3->accel.pix_trans[1] << 8) | (s3->accel.pix_trans[2] << 16) | (s3->accel.pix_trans[3] << 24), s3);
                        break;
                    case 0x600:
                        if (s3->chip == S3_TRIO32 || s3->chip == S3_VISION968 || s3->chip == S3_VISION868 || s3->chip >= S3_TRIO64V) {
                            if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 2)) {
                                if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40)) {
                                    s3->accel_start(8, 1, s3->accel.pix_trans[3], 0, s3);
                                    s3->accel_start(8, 1, s3->accel.pix_trans[2], 0, s3);
                                    s3->accel_start(8, 1, s3->accel.pix_trans[1], 0, s3);
                                    s3->accel_start(8, 1, s3->accel.pix_trans[0], 0, s3);
                                }
                            }
                        }
                        break;

                    default:
                        break;
                }
            }
            break;

        default:
            break;
    }
}

static void
s3_accel_out_fifo_w(s3_t *s3, uint16_t port, uint16_t val)
{
    if ((port != 0x9ee8) && (port != 0x9d48)) {
        s3_log("[%04X:%08X] OUT PORTW=%04x, val=%04x, CMD=%04x, C(%d,%d), WRTMASK=%04x.\n", CS, cpu_state.pc, port, val, s3->accel.cmd, s3->accel.cur_x, s3->accel.cur_y, s3->accel.wrt_mask);
        s3_log(".\n");
        if ((port == 0xb2e8) || (port == 0xb148)) {
            if ((s3->accel.multifunc[0xe] & 0x100) || (s3->chip >= S3_86C928)) {
                s3->accel.b2e8_pix = 0;
                s3_accel_out_fifo(s3, port, val);
                s3_accel_out_fifo(s3, port + 1, val >> 8);
            } else {
                s3->accel.b2e8_pix = 1;
                s3_accel_out_pixtrans_w(s3, val);
            }
        } else {
            s3->accel.b2e8_pix = 0;
            if ((port == 0xe2e8) || (port == 0xe2ea) || (port == 0xe148) || (port == 0xe14a))
                s3_accel_out_pixtrans_w(s3, val);
            else {
                s3_accel_out_fifo(s3, port, val);
                s3_accel_out_fifo(s3, port + 1, val >> 8);
            }
        }
    } else {
        s3->accel.short_stroke = val;
        s3->accel.ssv_state    = 1;

        s3->accel.cx = s3->accel.cur_x & 0xfff;
        s3->accel.cy = s3->accel.cur_y & 0xfff;

        if (s3->accel.cmd & 0x1000) {
            s3_short_stroke_start(s3, s3->accel.short_stroke & 0xff);
            s3_short_stroke_start(s3, s3->accel.short_stroke >> 8);
        } else {
            s3_short_stroke_start(s3, s3->accel.short_stroke >> 8);
            s3_short_stroke_start(s3, s3->accel.short_stroke & 0xff);
        }
    }
}

static void
s3_accel_out_fifo_l(s3_t *s3, uint16_t port, uint32_t val)
{
    if ((port == 0xb2e8) || (port == 0xb148)) {
        s3_log("[%04X:%08X] OUT PORTL=%04x, val=%08x, CMD=%04x, C(%d,%d).\n", CS, cpu_state.pc, port, val, s3->accel.cmd, s3->accel.cur_x, s3->accel.cur_y);
        if ((s3->accel.multifunc[0xe] & 0x100) || (s3->chip >= S3_86C928)) {
            s3->accel.b2e8_pix = 0;
            s3_accel_out_fifo(s3, port, val);
            s3_accel_out_fifo(s3, port + 1, val >> 8);
            s3_accel_out_fifo(s3, port + 2, val >> 16);
            s3_accel_out_fifo(s3, port + 3, val >> 24);
        } else {
            s3->accel.b2e8_pix = 1;
            s3_accel_out_pixtrans_l(s3, val);
        }
    } else {
        s3->accel.b2e8_pix = 0;
        if ((port == 0xe2e8) || (port == 0xe148))
            s3_accel_out_pixtrans_l(s3, val);
        else {
            s3_accel_out_fifo(s3, port, val);
            s3_accel_out_fifo(s3, port + 1, val >> 8);
            s3_accel_out_fifo(s3, port + 2, val >> 16);
            s3_accel_out_fifo(s3, port + 3, val >> 24);
        }
    }
}

static void
s3_accel_write_fifo(s3_t *s3, uint32_t addr, uint8_t val)
{
    const svga_t *svga = &s3->svga;

    if (s3->packed_mmio) {
        int addr_lo = addr & 1;
        if (svga->crtc[0x53] & 0x08) {
            if ((addr >= 0x08000) && (addr <= 0x0803f))
                s3_pci_write(0, addr & 0xff, val, s3);
        }

        switch (addr & 0x1fffe) {
            case 0x8100:
                addr = 0x82e8;
                break; /*ALT_CURXY*/
            case 0x8102:
                addr = 0x86e8;
                break;

            case 0x8104:
                addr = 0x82ea;
                break; /*ALT_CURXY2*/
            case 0x8106:
                addr = 0x86ea;
                break;

            case 0x8108:
                addr = 0x8ae8;
                break; /*ALT_STEP*/
            case 0x810a:
                addr = 0x8ee8;
                break;

            case 0x810c:
                addr = 0x8aea;
                break; /*ALT_STEP2*/
            case 0x810e:
                addr = 0x8eea;
                break;

            case 0x8110:
                addr = 0x92e8;
                break; /*ALT_ERR*/
            case 0x8112:
                addr = 0x92ee;
                break;

            case 0x8118:
                addr = 0x9ae8;
                break; /*ALT_CMD*/
            case 0x811a:
                addr = 0x9aea;
                break;

            case 0x811c:
                addr = 0x9ee8;
                break;

            case 0x8120:
            case 0x8122: /*BKGD_COLOR*/
                WRITE8(addr, s3->accel.bkgd_color, val);
                return;

            case 0x8124:
            case 0x8126: /*FRGD_COLOR*/
                WRITE8(addr, s3->accel.frgd_color, val);
                return;

            case 0x8128:
            case 0x812a: /*WRT_MASK*/
                WRITE8(addr, s3->accel.wrt_mask, val);
                return;

            case 0x812c:
            case 0x812e: /*RD_MASK*/
                WRITE8(addr, s3->accel.rd_mask, val);
                return;

            case 0x8130:
            case 0x8132: /*COLOR_CMP*/
                WRITE8(addr, s3->accel.color_cmp, val);
                return;

            case 0x8134:
                addr = 0xb6e8;
                break; /*ALT_MIX*/
            case 0x8136:
                addr = 0xbae8;
                break;

            case 0x8138: /*SCISSORS_T*/
                WRITE8(addr & 1, s3->accel.multifunc[1], val);
                return;
            case 0x813a: /*SCISSORS_L*/
                WRITE8(addr & 1, s3->accel.multifunc[2], val);
                return;
            case 0x813c: /*SCISSORS_B*/
                WRITE8(addr & 1, s3->accel.multifunc[3], val);
                return;
            case 0x813e: /*SCISSORS_R*/
                WRITE8(addr & 1, s3->accel.multifunc[4], val);
                return;

            case 0x8140: /*PIX_CNTL*/
                WRITE8(addr & 1, s3->accel.multifunc[0xa], val);
                return;
            case 0x8142: /*MULT_MISC2*/
                WRITE8(addr & 1, s3->accel.multifunc[0xd], val);
                return;
            case 0x8144: /*MULT_MISC*/
                WRITE8(addr & 1, s3->accel.multifunc[0xe], val);
                return;
            case 0x8146: /*READ_SEL*/
                WRITE8(addr & 1, s3->accel.multifunc[0xf], val);
                return;

            case 0x8148: /*ALT_PCNT*/
                WRITE8(addr & 1, s3->accel.multifunc[0], val);
                return;
            case 0x814a:
                addr = 0x96e8;
                break;
            case 0x814c:
                addr = 0x96ea;
                break;

            case 0x8150:
                addr = 0xd2e8;
                break;

            case 0x8154:
                addr = 0x8ee8;
                break;
            case 0x8156:
                addr = 0x96e8;
                break;

            case 0x8164:
            case 0x8166:
                WRITE8(addr, s3->accel.pat_bg_color, val);
                return;

            case 0x8168:
                addr = 0xeae8;
                break;
            case 0x816a:
                addr = 0xeaea;
                break;

            case 0x816c:
            case 0x816e:
                WRITE8(addr, s3->accel.pat_fg_color, val);
                return;

            default:
                break;
        }
        addr |= addr_lo;
    }

    if (svga->crtc[0x53] & 0x08) {
        if ((addr & 0x1ffff) < 0x8000) {
            if (s3->accel.cmd & 0x100) {
                if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 2)) {
                    if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40))
                        s3->accel_start(8, 1, val | (val << 8) | (val << 16) | (val << 24), 0, s3);
                    else
                        s3->accel_start(1, 1, 0xffffffff, val | (val << 8) | (val << 16) | (val << 24), s3);
                } else
                    s3->accel_start(1, 1, 0xffffffff, val | (val << 8) | (val << 16) | (val << 24), s3);
            }
        } else {
            s3_accel_out_fifo(s3, addr & 0xffff, val);
        }
    } else {
        if (addr & 0x8000) {
            if ((addr == 0xe2e8) || (addr == 0xe2e9) || (addr == 0xe2ea) || (addr == 0xe2eb)) {
                if ((s3->chip == S3_86C801) || (s3->chip == S3_86C805))
                    goto mmio_byte_write;
                else
                    s3_accel_out_fifo(s3, addr & 0xffff, val);
            } else
                s3_accel_out_fifo(s3, addr & 0xffff, val);
        } else {
mmio_byte_write:
            if (s3->accel.cmd & 0x100) {
                if ((s3->accel.cmd & 0x600) == 0x200) {
                    if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 2)) {
                        if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40))
                            s3->accel_start(16, 1, val | (val << 8) | (val << 16) | (val << 24), 0, s3);
                        else
                            s3->accel_start(2, 1, 0xffffffff, val | (val << 8) | (val << 16) | (val << 24), s3);
                    } else
                        s3->accel_start(2, 1, 0xffffffff, val | (val << 8) | (val << 16) | (val << 24), s3);
                } else {
                    if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 2)) {
                        if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40))
                            s3->accel_start(8, 1, val | (val << 8) | (val << 16) | (val << 24), 0, s3);
                        else
                            s3->accel_start(1, 1, 0xffffffff, val | (val << 8) | (val << 16) | (val << 24), s3);
                    } else
                        s3->accel_start(1, 1, 0xffffffff, val | (val << 8) | (val << 16) | (val << 24), s3);
                }
            }
        }
    }
}

static void
s3_accel_write_fifo_w(s3_t *s3, uint32_t addr, uint16_t val)
{
    if ((addr & 0x1fffe) < 0x8000) {
        s3_accel_out_pixtrans_w(s3, val);
    } else if ((addr & 0x1fffe) == 0x811c) {
        s3_accel_out_fifo_w(s3, 0x9ee8, val);
    } else if (((addr & 0x1fffe) >= 0x8000) && (addr & 0x1fffe) < 0x10000) {
        if (((addr & 0x1fffe) == 0xe2e8) || ((addr & 0x1fffe) == 0xe2ea)) {
            if ((s3->chip == S3_86C801) || (s3->chip == S3_86C805) || (s3->chip == S3_86C928) || (s3->chip == S3_86C928PCI))
                s3_accel_out_pixtrans_w(s3, val);
            else {
                s3_accel_write_fifo(s3, addr, val);
                s3_accel_write_fifo(s3, addr + 1, val >> 8);
            }
        } else {
            s3_accel_write_fifo(s3, addr, val);
            s3_accel_write_fifo(s3, addr + 1, val >> 8);
        }
    }
}

static void
s3_accel_write_fifo_l(s3_t *s3, uint32_t addr, uint32_t val)
{
    if ((addr & 0x1fffc) >= 0x10000 && (addr & 0x1fffc) < 0x18000) {
        s3_visionx68_video_engine_op(val, s3);
    } else if ((addr & 0x1fffc) < 0x8000) {
        s3_accel_out_pixtrans_l(s3, val);
    } else if ((addr & 0x1fffc) >= 0x18000) {
        switch (addr & 0x1fffc) {
            case 0x18080:
                s3->videoengine.nop = 1;
                break;

            case 0x18088:
                s3->videoengine.cntl                 = val;
                s3->videoengine.dda_init_accumulator = val & 0xfff;
                s3->videoengine.odf                  = (val >> 16) & 7;
                s3->videoengine.yuv                  = !!(val & (1 << 19));
                s3->videoengine.idf                  = (val >> 20) & 7;
                s3->videoengine.dither               = !!(val & (1 << 29));
                s3->videoengine.dm_index             = (val >> 23) & 7;
                break;

            case 0x1808c:
                s3->videoengine.stretch_filt_const = val;
                s3->videoengine.k2                 = val & 0x7ff;
                s3->videoengine.k1                 = (val >> 16) & 0x7ff;
                s3->videoengine.host_data          = !!(val & (1 << 30));
                s3->videoengine.scale_down         = !!(val & (1 << 31));
                break;

            case 0x18090:
                s3->videoengine.src_dst_step = val;
                s3->videoengine.dst_step     = val & 0x1fff;
                s3->videoengine.src_step     = (val >> 16) & 0x1fff;
                break;

            case 0x18094:
                s3->videoengine.crop  = val;
                s3->videoengine.len   = val & 0xfff;
                s3->videoengine.start = (val >> 16) & 0xfff;
                s3->videoengine.input = 1;
                break;

            case 0x18098:
                s3->videoengine.src_base = val & 0xffffff;
                break;

            case 0x1809c:
                s3->videoengine.dest_base = val & 0xffffff;
                break;

            default:
                break;
        }
    } else if (((addr & 0x1fffc) >= 0x8000) && ((addr & 0x1fffc) < 0x10000)) {
        if ((addr & 0x1fffc) == 0xe2e8) {
            if ((s3->chip == S3_86C928) || (s3->chip == S3_86C928PCI))
                s3_accel_out_pixtrans_l(s3, val);
            else {
                s3_accel_write_fifo(s3, addr, val);
                s3_accel_write_fifo(s3, addr + 1, val >> 8);
                s3_accel_write_fifo(s3, addr + 2, val >> 16);
                s3_accel_write_fifo(s3, addr + 3, val >> 24);
            }
        } else {
            s3_accel_write_fifo(s3, addr, val);
            s3_accel_write_fifo(s3, addr + 1, val >> 8);
            s3_accel_write_fifo(s3, addr + 2, val >> 16);
            s3_accel_write_fifo(s3, addr + 3, val >> 24);
        }
    }
}


static void
s3_vblank_start(svga_t *svga)
{
    s3_t *s3 = (s3_t *) svga->priv;

    s3->subsys_stat |= INT_VSY;
    s3_update_irqs(s3);
}

static uint32_t
s3_hwcursor_convert_addr(svga_t *svga)
{
    if ((svga->bpp == 8) && (((svga->gdcreg[5] & 0x60) == 0x20) || (svga->crtc[0x3a] & 0x10)) && (svga->crtc[0x45] & 0x10)) {
        if (svga->crtc[0x3a] & 0x10)
            return ((svga->hwcursor_latch.addr & 0xfffff1ff) | ((svga->hwcursor_latch.addr & 0x200) << 2)) | 0x600;
        else if ((svga->gdcreg[5] & 0x60) == 0x20)
            return ((svga->hwcursor_latch.addr & 0xfffff0ff) | ((svga->hwcursor_latch.addr & 0x300) << 2)) | 0x300;
        else
            return svga->hwcursor_latch.addr;
    } else
        return svga->hwcursor_latch.addr;
}

static void
s3_hwcursor_draw(svga_t *svga, int displine)
{
    const s3_t *s3 = (s3_t *) svga->priv;
    int         shift = 1;
    int         width = 16;
    uint16_t    dat[4] = { 0, 0, 0, 0 };
    int         xx;
    int         offset = svga->hwcursor_latch.x - svga->hwcursor_latch.xoff;
    uint32_t    fg;
    uint32_t    bg;
    uint32_t    real_addr;
    uint32_t    remapped_addr;

    switch (svga->bpp) {
        case 15:
            fg = video_15to32[s3->hwc_fg_col & 0xffff];
            bg = video_15to32[s3->hwc_bg_col & 0xffff];
            if ((s3->chip >= S3_86C928) && (s3->chip <= S3_86C805)) {
                if (!s3->color_16bit) {
                    if ((s3->card_type != S3_MIROCRYSTAL10SD_805) && (s3->card_type != S3_MIROCRYSTAL8S_805)) {
                        if (!(svga->crtc[0x45] & 0x04)) {
                            shift = 2;
                            width = 8;
                        }
                    }
                } else {
                    shift = 2;
                    width = 8;
                    fg = svga->pallook[svga->crtc[0xe]];
                    bg = svga->pallook[svga->crtc[0xf]];
                }
            } else if (s3->chip <= S3_86C924) {
                shift = 2;
                width = 8;
                fg = svga->pallook[svga->crtc[0xe]];
                bg = svga->pallook[svga->crtc[0xf]];
            }
            break;

        case 16:
            fg = video_16to32[s3->hwc_fg_col & 0xffff];
            bg = video_16to32[s3->hwc_bg_col & 0xffff];
            if ((s3->chip >= S3_86C928) && (s3->chip <= S3_86C805)) {
                if (!s3->color_16bit) {
                    if ((s3->card_type != S3_MIROCRYSTAL10SD_805) && (s3->card_type != S3_MIROCRYSTAL8S_805)) {
                        if (!(svga->crtc[0x45] & 0x04)) {
                            shift = 2;
                            width = 8;
                        }
                    } else if (s3->card_type == S3_MIROCRYSTAL10SD_805) {
                        if (!(svga->crtc[0x45] & 0x04))
                            offset <<= 1;
                    }
                } else {
                    shift = 2;
                    width = 8;
                    fg = svga->pallook[svga->crtc[0xe]];
                    bg = svga->pallook[svga->crtc[0xf]];
                }
            } else if (s3->chip <= S3_86C924) {
                shift = 2;
                width = 8;
                fg = svga->pallook[svga->crtc[0xe]];
                bg = svga->pallook[svga->crtc[0xf]];
            }
            break;

        case 24:
            if (s3->chip <= S3_86C805) {
                fg = svga->pallook[svga->crtc[0xe]];
                bg = svga->pallook[svga->crtc[0xf]];
            } else {
                fg = s3->hwc_fg_col;
                bg = s3->hwc_bg_col;
            }
            break;

        case 32:
            fg = s3->hwc_fg_col;
            bg = s3->hwc_bg_col;
            break;

        default:
            if (s3->chip >= S3_TRIO32) {
                fg = svga->pallook[s3->hwc_fg_col & 0xff];
                bg = svga->pallook[s3->hwc_bg_col & 0xff];
            } else {
                fg = svga->pallook[svga->crtc[0xe]];
                bg = svga->pallook[svga->crtc[0xf]];
            }
            break;
    }

    if (svga->interlace && svga->hwcursor_oddeven)
        svga->hwcursor_latch.addr += 16;

    real_addr = s3_hwcursor_convert_addr(svga);

    for (uint8_t x = 0; x < 64; x += 16) {
        remapped_addr = dword_remap(svga, real_addr);
        if (((svga->bpp == 15) || (svga->bpp == 16)) && s3->color_16bit) {
            dat[0] = svga->vram[remapped_addr & s3->vram_mask];
            dat[1] = svga->vram[(remapped_addr + 1) & s3->vram_mask];
            dat[2] = svga->vram[(remapped_addr + 2) & s3->vram_mask];
            dat[3] = svga->vram[(remapped_addr + 3) & s3->vram_mask];

            /*Windows*/
            for (xx = 0; xx < 8; xx++) {
                if (offset >= 0) {
                    if (!(dat[(xx & 4) ? 2 : 0] & 0x80))
                        buffer32->line[displine][offset + svga->x_add] = (dat[(xx & 4) ? 3 : 1] & 0x80) ? fg : bg;
                    else if (dat[(xx & 4) ? 3 : 1] & 0x80)
                        buffer32->line[displine][offset + svga->x_add] ^= 0xffffff;
                }

                offset++;
                s3_log("Up: Data0=%04x, Data1=%04x, Data2=%04x, Data3=%04x, xx=%d addr=%06x.\n", dat[0], dat[1], dat[2], dat[3], xx, remapped_addr);
                dat[(xx & 4) ? 2 : 0] <<= 2;
                dat[(xx & 4) ? 3 : 1] <<= 2;
            }
        } else {
            dat[0] = (svga->vram[remapped_addr & s3->vram_mask] << 8) | svga->vram[(remapped_addr + 1) & s3->vram_mask];
            dat[1] = (svga->vram[(remapped_addr + 2) & s3->vram_mask] << 8) | svga->vram[(remapped_addr + 3) & s3->vram_mask];

            if (svga->crtc[0x55] & 0x10) {
                /*X11*/
                for (xx = 0; xx < 16; xx++) {
                    if (offset >= 0) {
                        if (dat[0] & 0x8000)
                            buffer32->line[displine][offset + svga->x_add] = (dat[1] & 0x8000) ? fg : bg;
                    }

                    offset++;
                    dat[0] <<= shift;
                    dat[1] <<= shift;
                }
            } else {
                /*Windows*/
                for (xx = 0; xx < width; xx++) {
                    if (offset >= 0) {
                        if (!(dat[0] & 0x8000))
                            buffer32->line[displine][offset + svga->x_add] = (dat[1] & 0x8000) ? fg : bg;
                        else if (dat[1] & 0x8000)
                            buffer32->line[displine][offset + svga->x_add] ^= 0xffffff;
                    }

                    offset++;
                    dat[0] <<= shift;
                    dat[1] <<= shift;
                }
            }
        }
        svga->hwcursor_latch.addr += 4;
        real_addr = s3_hwcursor_convert_addr(svga);
    }

    if (svga->interlace && !svga->hwcursor_oddeven)
        svga->hwcursor_latch.addr += 16;
}

#define CLAMP(x)                      \
    do {                              \
        if ((x) & ~0xff)              \
            x = ((x) < 0) ? 0 : 0xff; \
    } while (0)

#define DECODE_YCbCr()                      \
    do {                                    \
        int c;                              \
                                            \
        for (c = 0; c < 2; c++) {           \
            uint8_t y1, y2;                 \
            int8_t  Cr, Cb;                 \
            int     dR, dG, dB;             \
                                            \
            y1 = src[0];                    \
            Cr = src[1] - 0x80;             \
            y2 = src[2];                    \
            Cb = src[3] - 0x80;             \
            src += 4;                       \
                                            \
            dR = (359 * Cr) >> 8;           \
            dG = (88 * Cb + 183 * Cr) >> 8; \
            dB = (453 * Cb) >> 8;           \
                                            \
            r[x_write] = y1 + dR;           \
            CLAMP(r[x_write]);              \
            g[x_write] = y1 - dG;           \
            CLAMP(g[x_write]);              \
            b[x_write] = y1 + dB;           \
            CLAMP(b[x_write]);              \
                                            \
            r[x_write + 1] = y2 + dR;       \
            CLAMP(r[x_write + 1]);          \
            g[x_write + 1] = y2 - dG;       \
            CLAMP(g[x_write + 1]);          \
            b[x_write + 1] = y2 + dB;       \
            CLAMP(b[x_write + 1]);          \
                                            \
            x_write = (x_write + 2) & 7;    \
        }                                   \
    } while (0)

/*Both YUV formats are untested*/
#define DECODE_YUV211()                  \
    do {                                 \
        uint8_t y1, y2, y3, y4;          \
        int8_t  U, V;                    \
        int     dR, dG, dB;              \
                                         \
        U  = src[0] - 0x80;              \
        y1 = (298 * (src[1] - 16)) >> 8; \
        y2 = (298 * (src[2] - 16)) >> 8; \
        V  = src[3] - 0x80;              \
        y3 = (298 * (src[4] - 16)) >> 8; \
        y4 = (298 * (src[5] - 16)) >> 8; \
        src += 6;                        \
                                         \
        dR = (309 * V) >> 8;             \
        dG = (100 * U + 208 * V) >> 8;   \
        dB = (516 * U) >> 8;             \
                                         \
        r[x_write] = y1 + dR;            \
        CLAMP(r[x_write]);               \
        g[x_write] = y1 - dG;            \
        CLAMP(g[x_write]);               \
        b[x_write] = y1 + dB;            \
        CLAMP(b[x_write]);               \
                                         \
        r[x_write + 1] = y2 + dR;        \
        CLAMP(r[x_write + 1]);           \
        g[x_write + 1] = y2 - dG;        \
        CLAMP(g[x_write + 1]);           \
        b[x_write + 1] = y2 + dB;        \
        CLAMP(b[x_write + 1]);           \
                                         \
        r[x_write + 2] = y3 + dR;        \
        CLAMP(r[x_write + 2]);           \
        g[x_write + 2] = y3 - dG;        \
        CLAMP(g[x_write + 2]);           \
        b[x_write + 2] = y3 + dB;        \
        CLAMP(b[x_write + 2]);           \
                                         \
        r[x_write + 3] = y4 + dR;        \
        CLAMP(r[x_write + 3]);           \
        g[x_write + 3] = y4 - dG;        \
        CLAMP(g[x_write + 3]);           \
        b[x_write + 3] = y4 + dB;        \
        CLAMP(b[x_write + 3]);           \
                                         \
        x_write = (x_write + 4) & 7;     \
    } while (0)

#define DECODE_YUV422()                      \
    do {                                     \
        int c;                               \
                                             \
        for (c = 0; c < 2; c++) {            \
            uint8_t y1, y2;                  \
            int8_t  U, V;                    \
            int     dR, dG, dB;              \
                                             \
            U  = src[0] - 0x80;              \
            y1 = (298 * (src[1] - 16)) >> 8; \
            V  = src[2] - 0x80;              \
            y2 = (298 * (src[3] - 16)) >> 8; \
            src += 4;                        \
                                             \
            dR = (309 * V) >> 8;             \
            dG = (100 * U + 208 * V) >> 8;   \
            dB = (516 * U) >> 8;             \
                                             \
            r[x_write] = y1 + dR;            \
            CLAMP(r[x_write]);               \
            g[x_write] = y1 - dG;            \
            CLAMP(g[x_write]);               \
            b[x_write] = y1 + dB;            \
            CLAMP(b[x_write]);               \
                                             \
            r[x_write + 1] = y2 + dR;        \
            CLAMP(r[x_write + 1]);           \
            g[x_write + 1] = y2 - dG;        \
            CLAMP(g[x_write + 1]);           \
            b[x_write + 1] = y2 + dB;        \
            CLAMP(b[x_write + 1]);           \
                                             \
            x_write = (x_write + 2) & 7;     \
        }                                    \
    } while (0)

#define DECODE_RGB555()                                                      \
    do {                                                                     \
        int c;                                                               \
                                                                             \
        for (c = 0; c < 4; c++) {                                            \
            uint16_t dat;                                                    \
                                                                             \
            dat = *(uint16_t *) src;                                         \
            src += 2;                                                        \
                                                                             \
            r[x_write + c] = ((dat & 0x001f) << 3) | ((dat & 0x001f) >> 2);  \
            g[x_write + c] = ((dat & 0x03e0) >> 2) | ((dat & 0x03e0) >> 7);  \
            b[x_write + c] = ((dat & 0x7c00) >> 7) | ((dat & 0x7c00) >> 12); \
        }                                                                    \
        x_write = (x_write + 4) & 7;                                         \
    } while (0)

#define DECODE_RGB565()                                                      \
    do {                                                                     \
        int c;                                                               \
                                                                             \
        for (c = 0; c < 4; c++) {                                            \
            uint16_t dat;                                                    \
                                                                             \
            dat = *(uint16_t *) src;                                         \
            src += 2;                                                        \
                                                                             \
            r[x_write + c] = ((dat & 0x001f) << 3) | ((dat & 0x001f) >> 2);  \
            g[x_write + c] = ((dat & 0x07e0) >> 3) | ((dat & 0x07e0) >> 9);  \
            b[x_write + c] = ((dat & 0xf800) >> 8) | ((dat & 0xf800) >> 13); \
        }                                                                    \
        x_write = (x_write + 4) & 7;                                         \
    } while (0)

#define DECODE_RGB888()              \
    do {                             \
        int c;                       \
                                     \
        for (c = 0; c < 4; c++) {    \
            r[x_write + c] = src[0]; \
            g[x_write + c] = src[1]; \
            b[x_write + c] = src[2]; \
            src += 3;                \
        }                            \
        x_write = (x_write + 4) & 7; \
    } while (0)

#define DECODE_XRGB8888()            \
    do {                             \
        int c;                       \
                                     \
        for (c = 0; c < 4; c++) {    \
            r[x_write + c] = src[0]; \
            g[x_write + c] = src[1]; \
            b[x_write + c] = src[2]; \
            src += 4;                \
        }                            \
        x_write = (x_write + 4) & 7; \
    } while (0)

#define OVERLAY_SAMPLE()            \
    do {                            \
        switch (s3->streams.sdif) { \
            case 1:                 \
                DECODE_YCbCr();     \
                break;              \
            case 2:                 \
                DECODE_YUV422();    \
                break;              \
            case 3:                 \
                DECODE_RGB555();    \
                break;              \
            case 4:                 \
                DECODE_YUV211();    \
                break;              \
            case 5:                 \
                DECODE_RGB565();    \
                break;              \
            case 6:                 \
                DECODE_RGB888();    \
                break;              \
            case 7:                 \
            default:                \
                DECODE_XRGB8888();  \
                break;              \
        }                           \
    } while (0)

static void
s3_trio64v_overlay_draw(svga_t *svga, int displine)
{
    const s3_t *s3     = (s3_t *) svga->priv;
    int         offset = (s3->streams.sec_x - s3->streams.pri_x) + 1;
    int         r[8];
    int         g[8];
    int         b[8];
    int         x_size;
    int         x_read = 4;
    int         x_write = 4;
    uint32_t   *p;
    uint8_t    *src = &svga->vram[svga->overlay_latch.addr];

    p = &((uint32_t *)buffer32->line[displine])[offset + svga->x_add];

    if ((offset + s3->streams.sec_w) > s3->streams.pri_w)
        x_size = (s3->streams.pri_w - s3->streams.sec_x) + 1;
    else
        x_size = s3->streams.sec_w + 1;

    OVERLAY_SAMPLE();

    for (int x = 0; x < x_size; x++) {
        *p++ = r[x_read] | (g[x_read] << 8) | (b[x_read] << 16);

        svga->overlay_latch.h_acc += s3->streams.k1_horiz_scale;
        if (svga->overlay_latch.h_acc >= 0) {
            if ((x_read ^ (x_read + 1)) & ~3)
                OVERLAY_SAMPLE();

            x_read = (x_read + 1) & 7;

            svga->overlay_latch.h_acc += (s3->streams.k2_horiz_scale - s3->streams.k1_horiz_scale);
        }
    }

    svga->overlay_latch.v_acc += s3->streams.k1_vert_scale;
    if (svga->overlay_latch.v_acc >= 0) {
        svga->overlay_latch.v_acc += (s3->streams.k2_vert_scale - s3->streams.k1_vert_scale);
        svga->overlay_latch.addr += s3->streams.sec_stride;
    }
}

static void
s3_io_remove_alt(s3_t *s3)
{
    if (!s3->translate)
        return;

    io_removehandler(0x4148, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0x4548, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0x4948, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0x8148, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0x8548, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0x8948, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0x8d48, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0x9148, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0x9548, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0x9948, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0x9d48, 0x0002, s3_accel_in, s3_accel_in_w, NULL, s3_accel_out, s3_accel_out_w, NULL, s3);
    io_removehandler(0xa148, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0xa548, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0xa948, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0xad48, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    if (s3->chip >= S3_86C928)
        io_removehandler(0xb148, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    else
        io_removehandler(0xb148, 0x0002, s3_accel_in, s3_accel_in_w, NULL, s3_accel_out, s3_accel_out_w, s3_accel_out_l, s3);
    io_removehandler(0xb548, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0xb948, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0xbd48, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0xd148, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0xe148, 0x0004, s3_accel_in, s3_accel_in_w, s3_accel_in_l, s3_accel_out, s3_accel_out_w, s3_accel_out_l, s3);
    io_removehandler(0xe548, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0xe948, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0xed48, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
}

static void
s3_io_remove(s3_t *s3)
{
    io_removehandler(0x03c0, 0x0020, s3_in, NULL, NULL, s3_out, NULL, NULL, s3);

    io_removehandler(0x42e8, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0x46e8, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0x4ae8, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0x82e8, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0x86e8, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0x8ae8, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0x8ee8, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0x92e8, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0x96e8, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0x9ae8, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0x9ee8, 0x0002, s3_accel_in, s3_accel_in_w, NULL, s3_accel_out, s3_accel_out_w, NULL, s3);
    io_removehandler(0xa2e8, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0xa6e8, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0xaae8, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0xaee8, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    if (s3->chip >= S3_86C928)
        io_removehandler(0xb2e8, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    else
        io_removehandler(0xb2e8, 0x0002, s3_accel_in, s3_accel_in_w, NULL, s3_accel_out, s3_accel_out_w, s3_accel_out_l, s3);
    io_removehandler(0xb6e8, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0xbae8, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0xbee8, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0xcae8, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0xd2e8, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0xe2e8, 0x0004, s3_accel_in, s3_accel_in_w, s3_accel_in_l, s3_accel_out, s3_accel_out_w, s3_accel_out_l, s3);
    io_removehandler(0xe6e8, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0xeae8, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0xeee8, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_removehandler(0xfee8, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);

    s3_io_remove_alt(s3);
}

static void
s3_io_set_alt(s3_t *s3)
{
    const svga_t *svga = &s3->svga;

    if (!s3->translate)
        return;

    if ((s3->chip == S3_VISION968 || s3->chip == S3_VISION868) && (svga->seqregs[9] & 0x80)) {
        return;
    }

    io_sethandler(0x4148, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_sethandler(0x4548, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_sethandler(0x4948, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    if (s3->chip == S3_TRIO64 || s3->chip >= S3_TRIO64V || s3->chip == S3_VISION968 || s3->chip == S3_VISION868) {
        io_sethandler(0x8148, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
        io_sethandler(0x8548, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
        io_sethandler(0x8948, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
        io_sethandler(0x8d48, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
        io_sethandler(0x9148, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
        io_sethandler(0x9548, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    } else {
        io_sethandler(0x8148, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
        io_sethandler(0x8548, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
        io_sethandler(0x8948, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
        io_sethandler(0x8d48, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
        io_sethandler(0x9148, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
        io_sethandler(0x9548, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    }
    if (s3->chip == S3_VISION968 || s3->chip == S3_VISION868)
        io_sethandler(0x9948, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    else
        io_sethandler(0x9948, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_sethandler(0x9d48, 0x0002, s3_accel_in, s3_accel_in_w, NULL, s3_accel_out, s3_accel_out_w, NULL, s3);
    io_sethandler(0xa148, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_sethandler(0xa548, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_sethandler(0xa948, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_sethandler(0xad48, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    if (s3->chip >= S3_86C928)
        io_sethandler(0xb148, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    else
        io_sethandler(0xb148, 0x0002, s3_accel_in, s3_accel_in_w, NULL, s3_accel_out, s3_accel_out_w, s3_accel_out_l, s3);
    io_sethandler(0xb548, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_sethandler(0xb948, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_sethandler(0xbd48, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_sethandler(0xe148, 0x0004, s3_accel_in, s3_accel_in_w, s3_accel_in_l, s3_accel_out, s3_accel_out_w, s3_accel_out_l, s3);
    if (s3->chip == S3_VISION968 || s3->chip == S3_VISION868) {
        io_sethandler(0xd148, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
        io_sethandler(0xe548, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
        io_sethandler(0xe948, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
        io_sethandler(0xed48, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    }
}

static void
s3_io_set(s3_t *s3)
{
    const svga_t *svga = &s3->svga;

    s3_io_remove(s3);

    io_sethandler(0x03c0, 0x0020, s3_in, NULL, NULL, s3_out, NULL, NULL, s3);

    if ((s3->chip == S3_VISION968 || s3->chip == S3_VISION868) && (svga->seqregs[9] & 0x80)) {
        return;
    }

    io_sethandler(0x42e8, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_sethandler(0x46e8, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_sethandler(0x4ae8, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    if (s3->chip == S3_TRIO64 || s3->chip >= S3_TRIO64V || s3->chip == S3_VISION968 || s3->chip == S3_VISION868) {
        io_sethandler(0x82e8, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
        io_sethandler(0x86e8, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
        io_sethandler(0x8ae8, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
        io_sethandler(0x8ee8, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
        io_sethandler(0x92e8, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
        io_sethandler(0x96e8, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    } else {
        io_sethandler(0x82e8, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
        io_sethandler(0x86e8, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
        io_sethandler(0x8ae8, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
        io_sethandler(0x8ee8, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
        io_sethandler(0x92e8, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
        io_sethandler(0x96e8, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    }
    if (s3->chip == S3_VISION968 || s3->chip == S3_VISION868)
        io_sethandler(0x9ae8, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    else
        io_sethandler(0x9ae8, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_sethandler(0x9ee8, 0x0002, s3_accel_in, s3_accel_in_w, NULL, s3_accel_out, s3_accel_out_w, NULL, s3);
    io_sethandler(0xa2e8, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_sethandler(0xa6e8, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_sethandler(0xaae8, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_sethandler(0xaee8, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    if (s3->chip >= S3_86C928)
        io_sethandler(0xb2e8, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    else
        io_sethandler(0xb2e8, 0x0002, s3_accel_in, s3_accel_in_w, NULL, s3_accel_out, s3_accel_out_w, s3_accel_out_l, s3);
    io_sethandler(0xb6e8, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_sethandler(0xbae8, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_sethandler(0xbee8, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_sethandler(0xcae8, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    io_sethandler(0xe2e8, 0x0004, s3_accel_in, s3_accel_in_w, s3_accel_in_l, s3_accel_out, s3_accel_out_w, s3_accel_out_l, s3);
    if (s3->chip == S3_VISION968 || s3->chip == S3_VISION868) {
        io_sethandler(0xd2e8, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
        io_sethandler(0xe6e8, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
        io_sethandler(0xeae8, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
        io_sethandler(0xeee8, 0x0004, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);
    }
    io_sethandler(0xfee8, 0x0002, s3_accel_in, NULL, NULL, s3_accel_out, NULL, NULL, s3);

    s3_io_set_alt(s3);
}

static void
s3_out(uint16_t addr, uint8_t val, void *priv)
{
    s3_t   *s3   = (s3_t *) priv;
    svga_t *svga = &s3->svga;
    uint8_t old;
    int     rs2;
    int     rs3;

    if (((addr & 0xfff0) == 0x3d0 || (addr & 0xfff0) == 0x3b0) && !(svga->miscout & 1))
        addr ^= 0x60;

    switch (addr) {
        case 0x3c2:
            if ((s3->chip == S3_VISION964) || (s3->chip == S3_VISION968) || (s3->chip == S3_86C928)) {
                if ((s3->card_type != S3_SPEA_MERCURY_P64V) && (s3->card_type != S3_MIROVIDEO40SV_ERGO_968)) {
                    if (((val >> 2) & 3) != 3)
                        icd2061_write(svga->clock_gen, (val >> 2) & 3);
                }
            }
            break;

        case 0x3c5:
            if (s3->chip == S3_TRIO64V2) {
                if (svga->seqaddr >= 0x10) {
                    svga->seqregs[svga->seqaddr] = val;
                    svga_recalctimings(svga);
                    return;
                }
            } else {
                if (svga->seqaddr >= 0x10 && svga->seqaddr < 0x20) {
                    svga->seqregs[svga->seqaddr] = val;
                    switch (svga->seqaddr) {
                        case 0x12:
                        case 0x13:
                            svga_recalctimings(svga);
                            return;

                        default:
                            break;
                    }
                }
            }
            if (svga->seqaddr == 4) /*Chain-4 - update banking*/
            {
                if (val & 0x08)
                    svga->write_bank = svga->read_bank = s3->bank << 16;
                else
                    svga->write_bank = svga->read_bank = s3->bank << 14;

                svga_recalctimings(svga);
            } else if (svga->seqaddr == 9) {
                svga->seqregs[9] = val & 0x80;
                s3_io_set(s3);
                return;
            } else if (svga->seqaddr == 0xa) {
                svga->seqregs[0xa] = val & 0x80;
                return;
            } else if (s3->chip >= S3_VISION964) {
                if (svga->seqaddr == 8) {
                    svga->seqregs[8] = val & 0x0f;
                    return;
                } else if ((svga->seqaddr == 0xd) && (svga->seqregs[8] == 0x06)) {
                    svga->seqregs[0xd] = val;
                    svga->dpms                   = ((s3->chip >= S3_VISION964) && (svga->seqregs[0x0d] & 0x50)) || (svga->crtc[0x56] & ((s3->chip >= S3_TRIO32) ? 0x06 : 0x20));
                    svga_recalctimings(svga);
                    return;
                }
            }
            break;

        case 0x3C6:
        case 0x3C7:
        case 0x3C8:
        case 0x3C9:
        case 0x3CA: /*0x3c6 alias*/
        case 0x3CB: /*0x3c7 alias*/
            rs2 = (svga->crtc[0x55] & 0x01) || !!(svga->crtc[0x43] & 2);
            if (s3->chip >= S3_TRIO32)
                svga_out(addr, val, svga);
            else if ((s3->chip == S3_VISION964 && s3->card_type != S3_ELSAWIN2KPROX_964) || (s3->chip == S3_86C928)) {
                rs3 = !!(svga->crtc[0x55] & 0x02);
                bt48x_ramdac_out(addr, rs2, rs3, val, svga->ramdac, svga);
            } else if ((s3->chip == S3_VISION964 && s3->card_type == S3_ELSAWIN2KPROX_964) || (s3->chip == S3_VISION968 && (s3->card_type == S3_DIAMOND_STEALTH64_968 || s3->card_type == S3_ELSAWIN2KPROX || s3->card_type == S3_PHOENIX_VISION968 || s3->card_type == S3_NUMBER9_9FX_771)))
                ibm_rgb528_ramdac_out(addr, rs2, val, svga->ramdac, svga);
            else if (s3->chip == S3_VISION968 && (s3->card_type == S3_SPEA_MERCURY_P64V || s3->card_type == S3_MIROVIDEO40SV_ERGO_968)) {
                rs3 = !!(svga->crtc[0x55] & 0x02);
                tvp3026_ramdac_out(addr, rs2, rs3, val, svga->ramdac, svga);
            } else if (((s3->chip == S3_86C801) || (s3->chip == S3_86C805)) &&
                ((s3->card_type != S3_MIROCRYSTAL10SD_805) && (s3->card_type != S3_MIROCRYSTAL8S_805)))
                att49x_ramdac_out(addr, rs2, val, svga->ramdac, svga);
            else if (s3->chip <= S3_86C924) {
                sc1148x_ramdac_out(addr, rs2, val, svga->ramdac, svga);
            } else if (s3->card_type == S3_NUMBER9_9FX_531)
                att498_ramdac_out(addr, rs2, val, svga->ramdac, svga);
            else if ((s3->chip == S3_86C928PCI) && (s3->card_type == S3_SPEA_MERCURY_LITE_PCI))
                sc1502x_ramdac_out(addr, val, svga->ramdac, svga);
            else
                sdac_ramdac_out(addr, rs2, val, svga->ramdac, svga);
            return;

        case 0x3D4:
            svga->crtcreg = (s3->chip == S3_TRIO64V2) ? val : (val & 0x7f);
            return;
        case 0x3D5:
            if ((svga->crtcreg < 7) && (svga->crtc[0x11] & 0x80))
                return;
            if ((svga->crtcreg == 7) && (svga->crtc[0x11] & 0x80))
                val = (svga->crtc[7] & ~0x10) | (val & 0x10);
            if ((svga->crtcreg >= 0x20) && (svga->crtcreg < 0x40) && (svga->crtcreg != 0x36) && (svga->crtcreg != 0x38) && (svga->crtcreg != 0x39) && ((svga->crtc[0x38] & 0xcc) != 0x48))
                return;
            if ((svga->crtcreg >= 0x40) && ((svga->crtc[0x39] & 0xe0) != 0xa0))
                return;
            if ((svga->crtcreg == 0x36) && (svga->crtc[0x39] != 0xa5))
                return;
            if ((s3->chip <= S3_86C924) && (svga->crtcreg >= 0x50))
                return;

            old                       = svga->crtc[svga->crtcreg];
            svga->crtc[svga->crtcreg] = val;

            switch (svga->crtcreg) {
                case 0x31:
                    s3->ma_ext             = (s3->ma_ext & 0x1c) | ((val & 0x30) >> 4);
                    svga->force_dword_mode = !!(val & 0x08);
                    break;

                case 0x40:
                    s3->enable_8514 = val & 0x01;
                    break;

                case 0x50:
                    s3->bpp = (svga->crtc[0x50] >> 4) & 3;
                    s3_log("S3 BPP=%d.\n", s3->bpp);
                    if (s3->bpp == 3) {
                        if (!(s3->accel.multifunc[0xe] & 0x200)) /*On True Color mode change, reset bit 4 of Misc Index register*/
                            s3->accel.multifunc[0xe] &= ~0x10;
                    }
                    break;

                case 0x5c:
                    if ((val & 0xa0) == 0x80)
                        i2c_gpio_set(s3->i2c, !!(val & 0x40), !!(val & 0x10));
                    if (s3->card_type == S3_PHOENIX_VISION868 || s3->card_type == S3_PHOENIX_VISION968) {
                        if ((val & 0x20) && (!(svga->crtc[0x55] & 0x01) && !(svga->crtc[0x43] & 2)))
                            svga->dac_addr |= 0x20;
                    } else if (s3->card_type == S3_MIROVIDEO40SV_ERGO_968) {
                        if ((val & 0x80) && (!(svga->crtc[0x55] & 0x01) && !(svga->crtc[0x43] & 2)))
                            svga->dac_addr |= 0x02;
                    }
                    break;

                case 0x69:
                    if (s3->chip >= S3_VISION964)
                        s3->ma_ext = val & 0x1f;
                    break;

                case 0x35:
                    s3->bank = (s3->bank & 0x70) | (val & 0xf);
                    if (svga->chain4)
                        svga->write_bank = svga->read_bank = s3->bank << 16;
                    else
                        svga->write_bank = svga->read_bank = s3->bank << 14;

                    s3_log("CRTC35 Chain4=%x, Bank=%02x.\n", svga->chain4, s3->bank);
                    break;

                case 0x51:
                    if (s3->chip == S3_86C801 || s3->chip == S3_86C805) {
                        s3->bank   = (s3->bank & 0x6f) | ((val & 0x4) << 2);
                        s3->ma_ext = (s3->ma_ext & ~0x4) | ((val & 1) << 2);
                    } else {
                        s3->bank   = (s3->bank & 0x4f) | ((val & 0xc) << 2);
                        s3->ma_ext = (s3->ma_ext & ~0xc) | ((val & 3) << 2);
                    }
                    if (svga->chain4)
                        svga->write_bank = svga->read_bank = s3->bank << 16;
                    else
                        svga->write_bank = svga->read_bank = s3->bank << 14;
                    break;

                case 0x6a:
                    if (s3->chip >= S3_VISION964) {
                        s3->bank = val;
                        if (svga->chain4)
                            svga->write_bank = svga->read_bank = s3->bank << 16;
                        else
                            svga->write_bank = svga->read_bank = s3->bank << 14;
                    }
                    break;

                case 0x45:
                    if ((s3->chip == S3_VISION964) || (s3->chip == S3_VISION968))
                        break;
                    svga->hwcursor.ena = val & 1;
                    break;
                case 0x46:
                case 0x47:
                case 0x48:
                case 0x49:
                case 0x4c:
                case 0x4d:
                case 0x4e:
                case 0x4f:
                    if ((s3->chip == S3_VISION964) || (s3->chip == S3_VISION968))
                        break;
                    svga->hwcursor.x = ((svga->crtc[0x46] << 8) | svga->crtc[0x47]) & 0x7ff;
                    if (svga->bpp == 32)
                        svga->hwcursor.x >>= 1;
                    svga->hwcursor.y    = ((svga->crtc[0x48] << 8) | svga->crtc[0x49]) & 0x7ff;
                    svga->hwcursor.xoff = svga->crtc[0x4e] & 0x3f;
                    svga->hwcursor.yoff = svga->crtc[0x4f] & 0x3f;
                    svga->hwcursor.addr = ((((svga->crtc[0x4c] << 8) | svga->crtc[0x4d]) & 0xfff) * 1024) + (svga->hwcursor.yoff * 16);
                    if ((s3->chip >= S3_TRIO32) && (svga->bpp == 32))
                        svga->hwcursor.x <<= 1;
                    else if ((s3->chip >= S3_86C928 && s3->chip <= S3_86C805) && ((svga->bpp == 15) || (svga->bpp == 16))) {
                        if ((s3->card_type == S3_MIROCRYSTAL10SD_805) && !(svga->crtc[0x45] & 0x04) && (svga->bpp == 16))
                            svga->hwcursor.x >>= 2;
                        else
                            svga->hwcursor.x >>= 1;
                    } else if ((s3->chip >= S3_86C928) && (s3->chip <= S3_86C805) && (svga->bpp == 24))
                        svga->hwcursor.x /= 3;
                    else if ((s3->chip <= S3_86C805) && s3->color_16bit)
                        svga->hwcursor.x >>= 1;
                    else if ((s3->chip == S3_TRIO32) && ((svga->bpp == 15) || (svga->bpp == 16)))
                        svga->hwcursor.x >>= 1;
                    break;

                case 0x4a:
                    switch (s3->hwc_col_stack_pos) {
                        case 0:
                            s3->hwc_fg_col = (s3->hwc_fg_col & 0xffff00) | val;
                            break;
                        case 1:
                            s3->hwc_fg_col = (s3->hwc_fg_col & 0xff00ff) | (val << 8);
                            break;
                        case 2:
                            s3->hwc_fg_col = (s3->hwc_fg_col & 0x00ffff) | (val << 16);
                            break;

                        default:
                            break;
                    }
                    s3->hwc_col_stack_pos = (s3->hwc_col_stack_pos + 1) & 3;
                    break;
                case 0x4b:
                    switch (s3->hwc_col_stack_pos) {
                        case 0:
                            s3->hwc_bg_col = (s3->hwc_bg_col & 0xffff00) | val;
                            break;
                        case 1:
                            s3->hwc_bg_col = (s3->hwc_bg_col & 0xff00ff) | (val << 8);
                            break;
                        case 2:
                            s3->hwc_bg_col = (s3->hwc_bg_col & 0x00ffff) | (val << 16);
                            break;

                        default:
                            break;
                    }
                    s3->hwc_col_stack_pos = (s3->hwc_col_stack_pos + 1) & 3;
                    break;

                case 0x53:
                case 0x58:
                case 0x59:
                case 0x5a:
                    s3_log("[%04X:%08X]: Write CRTC%02x=%02x.\n", CS, cpu_state.pc, svga->crtcreg, svga->crtc[svga->crtcreg]);
                    s3_updatemapping(s3);
                    break;

                case 0x55:
                    s3_log("[%04X:%08X]: Write CRTC%02x=%02x.\n", CS, cpu_state.pc, svga->crtcreg, svga->crtc[svga->crtcreg]);
                    if (s3->chip == S3_86C928) {
                        if (val & 0x28) {
                            svga->hwcursor_draw     = NULL;
                            svga->dac_hwcursor_draw = bt48x_hwcursor_draw;
                        } else {
                            svga->hwcursor_draw     = s3_hwcursor_draw;
                            svga->dac_hwcursor_draw = NULL;
                        }
                    }
                    break;

                case 0x42:
                    if ((s3->chip == S3_VISION964) || (s3->chip == S3_VISION968) || (s3->chip == S3_86C928)) {
                        if (((svga->miscout >> 2) & 3) == 3)
                            icd2061_write(svga->clock_gen, svga->crtc[0x42] & 0x0f);
                    }
                    break;

                case 0x43:
                    if (s3->chip < S3_VISION964) {
                        if (s3->chip <= S3_86C805)
                            svga_recalctimings(svga);

                        s3_io_remove_alt(s3);
                        s3->translate = !!(val & 0x10);
                        s3_io_set_alt(s3);
                    }
                    break;

                case 0x56:
                    svga->dpms = ((s3->chip >= S3_VISION964) && (svga->seqregs[0x0d] & 0x50)) || (svga->crtc[0x56] & ((s3->chip >= S3_TRIO32) ? 0x06 : 0x20));
                    old        = ~val; /* force recalc */
                    break;

                case 0x67:
                    if (s3->chip >= S3_TRIO32) {
                        switch (val >> 4) {
                            case 3:
                                svga->bpp = 15;
                                break;
                            case 5:
                                svga->bpp = 16;
                                break;
                            case 7:
                                svga->bpp = 24;
                                break;
                            case 13:
                                svga->bpp = 32;
                                break;
                            default:
                                svga->bpp = 8;
                                break;
                        }
                    }
                    break;

                default:
                    break;
            }
            if (old != val) {
                if (svga->crtcreg < 0xe || svga->crtcreg > 0x10) {
                    if ((svga->crtcreg == 0xc) || (svga->crtcreg == 0xd)) {
                        svga->fullchange = 3;
                        svga->memaddr_latch   = ((svga->crtc[0xc] << 8) | svga->crtc[0xd]) + ((svga->crtc[8] & 0x60) >> 5);
                        if ((((svga->crtc[0x67] & 0xc) != 0xc) && (s3->chip >= S3_TRIO64V)) || (s3->chip < S3_TRIO64V))
                            svga->memaddr_latch |= (s3->ma_ext << 16);
                    } else {
                        svga->fullchange = svga->monitor->mon_changeframecount;
                        svga_recalctimings(svga);
                    }
                }
            }
            break;

        default:
            break;
    }
    svga_out(addr, val, svga);
}

static uint8_t
s3_in(uint16_t addr, void *priv)
{
    s3_t   *s3   = (s3_t *) priv;
    svga_t *svga = &s3->svga;
    int     rs2;
    int     rs3;
    uint8_t temp;

    if (((addr & 0xfff0) == 0x3d0 || (addr & 0xfff0) == 0x3b0) && !(svga->miscout & 1))
        addr ^= 0x60;

    switch (addr) {
        case 0x3c1:
            if (svga->attraddr > 0x14)
                return 0xff;
            break;

        case 0x3c2:
            if (s3->chip <= S3_86C924)
                return svga_in(addr, svga) | 0x10;
            break;

        case 0x3c5:
            if ((svga->seqaddr >= 0x10 && svga->seqaddr < 0x20) && (s3->chip < S3_TRIO64V2)) {
                temp = svga->seqregs[svga->seqaddr];
                /* This is needed for the Intel Advanced/ATX's built-in S3 Trio64V+ BIOS to not
                   get stuck in an infinite loop. */
                if (((s3->card_type == S3_STB_POWERGRAPH_64_VIDEO) ||
                    (s3->card_type == S3_PHOENIX_TRIO64VPLUS_ONBOARD) ||
                    (s3->card_type == S3_CARDEX_TRIO64VPLUS)) && (svga->seqaddr == 0x17))
                    svga->seqregs[svga->seqaddr] ^= 0x01;
                return temp;
            } if ((svga->seqaddr >= 0x10) && (s3->chip >= S3_TRIO64V2)) {
                return svga->seqregs[svga->seqaddr];
            } else if ((svga->seqaddr >= 5) && (svga->seqaddr < 8))
                return 0xff;
            else
                return svga->seqregs[svga->seqaddr];
            break;

        case 0x3c6:
        case 0x3c7:
        case 0x3c8:
        case 0x3c9:
        case 0x3ca: /*0x3c6 alias*/
        case 0x3cb: /*0x3c7 alias*/
            rs2 = (svga->crtc[0x55] & 0x01) || !!(svga->crtc[0x43] & 2);
            if (s3->chip >= S3_TRIO32)
                return svga_in(addr, svga);
            else if ((s3->chip == S3_VISION964 && s3->card_type != S3_ELSAWIN2KPROX_964) || (s3->chip == S3_86C928)) {
                if (s3->chip == S3_86C928)
                    rs3 = !!(svga->crtc[0x55] & 0x28) || !!(svga->crtc[0x45] & 0x20) || !!(svga->crtc[0x55] & 0x02); /*Quite insane but Win95's S3 driver wants it set at all costs for 8bpp+ mode*/
                else
                    rs3 = !!(svga->crtc[0x55] & 0x02);
                temp = bt48x_ramdac_in(addr, rs2, rs3, svga->ramdac, svga);
                return temp;
            } else if ((s3->chip == S3_VISION964 && s3->card_type == S3_ELSAWIN2KPROX_964) || (s3->chip == S3_VISION968 && (s3->card_type == S3_DIAMOND_STEALTH64_968 || s3->card_type == S3_ELSAWIN2KPROX || s3->card_type == S3_PHOENIX_VISION968 || s3->card_type == S3_NUMBER9_9FX_771)))
                return ibm_rgb528_ramdac_in(addr, rs2, svga->ramdac, svga);
            else if (s3->chip == S3_VISION968 && (s3->card_type == S3_SPEA_MERCURY_P64V || s3->card_type == S3_MIROVIDEO40SV_ERGO_968)) {
                rs3 = !!(svga->crtc[0x55] & 0x02);
                return tvp3026_ramdac_in(addr, rs2, rs3, svga->ramdac, svga);
            } else if (((s3->chip == S3_86C801) || (s3->chip == S3_86C805)) &&
                ((s3->card_type != S3_MIROCRYSTAL10SD_805) && (s3->card_type != S3_MIROCRYSTAL8S_805)))
                return att49x_ramdac_in(addr, rs2, svga->ramdac, svga);
            else if (s3->chip <= S3_86C924)
                return sc1148x_ramdac_in(addr, rs2, svga->ramdac, svga);
            else if (s3->card_type == S3_NUMBER9_9FX_531)
                return att498_ramdac_in(addr, rs2, svga->ramdac, svga);
            else if ((s3->chip == S3_86C928PCI) && (s3->card_type == S3_SPEA_MERCURY_LITE_PCI))
                return sc1502x_ramdac_in(addr, svga->ramdac, svga);
            else
                return sdac_ramdac_in(addr, rs2, svga->ramdac, svga);
            break;

        case 0x3d4:
            return svga->crtcreg;
        case 0x3d5:
            switch (svga->crtcreg) {
                case 0x2d:
                    return (s3->chip == S3_TRIO64V2) ? 0x89 : 0x88; /*Extended chip ID*/
                case 0x2e:
                    return s3->id_ext; /*New chip ID*/
                case 0x2f: switch (s3->chip) { /*Revision level*/
                    case S3_TRIO64V:
                        return 0x40;
                    case S3_TRIO64V2:
                        return 0x16; /*Confirmed on an onboard 64V2/DX*/
                    default:
                        return 0x00;
                    }
                    break;
                case 0x30:
                    return s3->id; /*Chip ID*/
                case 0x31:
                    return (svga->crtc[0x31] & 0xcf) | ((s3->ma_ext & 3) << 4);
                case 0x35:
                    return (svga->crtc[0x35] & 0xf0) | (s3->bank & 0xf);
                case 0x45:
                    s3->hwc_col_stack_pos = 0;
                    return svga->crtc[0x45];
                case 0x51:
                    return (svga->crtc[0x51] & 0xf0) | ((s3->bank >> 2) & 0xc) | ((s3->ma_ext >> 2) & 3);
                case 0x5c: /* General Output Port Register */
                    temp = svga->crtc[0x5c] & 0xa0;
                    if (((svga->miscout >> 2) & 3) == 3)
                        temp |= svga->crtc[0x42] & 0x0f;
                    else
                        temp |= ((svga->miscout >> 2) & 3);
                    if ((temp & 0xa0) == 0xa0) {
                        if ((svga->crtc[0x5c] & 0x40) && i2c_gpio_get_scl(s3->i2c))
                            temp |= 0x40;
                        if ((svga->crtc[0x5c] & 0x10) && i2c_gpio_get_sda(s3->i2c))
                            temp |= 0x10;
                    }
                    return temp;
                case 0x69:
                    return s3->ma_ext;
                case 0x6a:
                    return s3->bank;
                /* Phoenix S3 video BIOS'es seem to expect CRTC registers 6B and 6C
                   to be mirrors of 59 and 5A. */
                case 0x6b:
                    s3_log("[%04X:%08X]: Read CRTC6b=%02x.\n", CS, cpu_state.pc, svga->crtc[0x6b]);
                    if (s3->chip != S3_TRIO64V2) {
                        if (svga->crtc[0x53] & 0x08)
                            return (s3->chip == S3_TRIO64V) ? (svga->crtc[0x59] & 0xfc) : (svga->crtc[0x59] & 0xfe);
                        else
                            return svga->crtc[0x59];
                    } else
                        return svga->crtc[0x6b];
                    break;
                case 0x6c:
                    s3_log("[%04X:%08X]: Read CRTC6c=%02x.\n", CS, cpu_state.pc, svga->crtc[0x6c]);
                    if (s3->chip != S3_TRIO64V2) {
                        if (svga->crtc[0x53] & 0x08)
                            return 0x00;
                        else
                            return (svga->crtc[0x5a] & 0x80);
                    } else
                        return svga->crtc[0x6c];
                    break;

                default:
                    break;
            }
            return svga->crtc[svga->crtcreg];

        default:
            break;
    }
    return svga_in(addr, svga);
}

uint32_t
s3_decode_addr(svga_t *svga, uint32_t addr, int write)
{
    int memory_map_mode = (svga->gdcreg[6] >> 2) & 3;

    s3_log("CRTC31 bit 3=%x, map=%x, write=%x, wrtbank=%x, chain4=%x, vrammask=%08x.\n", svga->crtc[0x31] & 0x08, memory_map_mode, write, svga->write_bank, svga->chain4, svga->vram_display_mask);

    if (svga->crtc[0x31] & 0x08)
        memory_map_mode = 1;

    addr &= 0x1ffff;

    switch (memory_map_mode) {
        case 0:
            break;
        case 1:
            if (addr >= 0x10000)
                return 0xffffffff;
            break;
        case 2:
            addr -= 0x10000;
            if (addr >= 0x8000)
                return 0xffffffff;
            break;
        default:
        case 3:
            addr -= 0x18000;
            if (addr >= 0x8000)
                return 0xffffffff;
            break;
    }

    if (memory_map_mode <= 1) {
        if (write)
            addr += svga->write_bank;
        else
            addr += svga->read_bank;
    }

    return addr;
}

static void
s3_write(uint32_t addr, uint8_t val, void *priv)
{
    s3_t *s3 = (s3_t *) priv;
    svga_t *svga = &s3->svga;

    xga_write_test(addr, val, svga);
    addr = s3_decode_addr(svga, addr, 1);
    if (addr == 0xffffffff)
        return;

    svga_write_linear(addr, val, svga);
}

static void
s3_writew(uint32_t addr, uint16_t val, void *priv)
{
    s3_t *s3 = (s3_t *) priv;
    svga_t *svga = &s3->svga;

    xga_write_test(addr, val, svga);
    xga_write_test(addr + 1, val >> 8, svga);
    addr = s3_decode_addr(svga, addr, 1);
    if (addr == 0xffffffff)
        return;

    svga_writew_linear(addr, val, svga);
}

static void
s3_writel(uint32_t addr, uint32_t val, void *priv)
{
    s3_t *s3 = (s3_t *) priv;
    svga_t *svga = &s3->svga;

    xga_write_test(addr, val, svga);
    xga_write_test(addr + 1, val >> 8, svga);
    xga_write_test(addr + 2, val >> 16, svga);
    xga_write_test(addr + 3, val >> 24, svga);
    addr = s3_decode_addr(svga, addr, 1);
    if (addr == 0xffffffff)
        return;

    svga_writel_linear(addr, val, svga);
}

static uint8_t
s3_read(uint32_t addr, void *priv)
{
    s3_t *s3 = (s3_t *) priv;
    svga_t *svga = &s3->svga;

    (void) xga_read_test(addr, svga);
    addr = s3_decode_addr(svga, addr, 0);
    if (addr == 0xffffffff)
        return 0xff;

    return svga_read_linear(addr, svga);
}

static uint16_t
s3_readw(uint32_t addr, void *priv)
{
    s3_t *s3 = (s3_t *) priv;
    svga_t *svga = &s3->svga;

    (void) xga_read_test(addr, svga);
    (void) xga_read_test(addr + 1, svga);
    addr = s3_decode_addr(svga, addr, 0);
    if (addr == 0xffffffff)
        return 0xffff;

    return svga_readw_linear(addr, svga);
}

static uint32_t
s3_readl(uint32_t addr, void *priv)
{
    s3_t *s3 = (s3_t *) priv;
    svga_t *svga = &s3->svga;

    (void) xga_read_test(addr, svga);
    (void) xga_read_test(addr + 1, svga);
    (void) xga_read_test(addr + 2, svga);
    (void) xga_read_test(addr + 3, svga);
    addr = s3_decode_addr(svga, addr, 0);
    if (addr == 0xffffffff)
        return 0xffffffff;

    return svga_readl_linear(addr, svga);
}

static void
s3_recalctimings(svga_t *svga)
{
    s3_t *s3      = (s3_t *) svga->priv;
    int   clk_sel = (svga->miscout >> 2) & 3;
    uint8_t mask = 0xc0;

    if (svga->crtc[0x33] & 0x20) {
        /* In this mode, the dots per clock are always 8 or 16, never 9 or 18. */
        if (!svga->scrblank && svga->attr_palette_enable)
            svga->dots_per_clock = (svga->seqregs[1] & 8) ? 16 : 8;
    } else {
        if (!svga->scrblank && svga->attr_palette_enable && (svga->crtc[0x43] & 0x80)) {
            /* TODO: In case of bug reports, disable 9-dots-wide character clocks in graphics modes. */
            svga->dots_per_clock = ((svga->seqregs[1] & 1) ? 16 : 18);
        }
    }

    svga->hdisp = svga->hdisp_old;
    svga->memaddr_latch |= (s3->ma_ext << 16);

    svga->lowres = (!!(svga->attrregs[0x10] & 0x40) && !(svga->crtc[0x3a] & 0x10));

    if (s3->chip >= S3_86C928) {
        if (svga->crtc[0x5d] & 0x01)
            svga->htotal |= 0x100;
        if (svga->crtc[0x5d] & 0x02) {
            svga->hdisp_time |= 0x100;
            svga->hdisp |= (0x100 * svga->dots_per_clock);
        }
        if (svga->crtc[0x5e] & 0x01)
            svga->vtotal |= 0x400;
        if (svga->crtc[0x5e] & 0x02)
            svga->dispend |= 0x400;
        if (svga->crtc[0x5e] & 0x04)
            svga->vblankstart |= 0x400;
        else if ((svga->crtc[0x3a] & 0x10) && !svga->lowres)
            svga->vblankstart = svga->dispend; /*Applies only to Enhanced modes*/
        if (svga->crtc[0x5e] & 0x10)
            svga->vsyncstart |= 0x400;
        if (svga->crtc[0x5e] & 0x40)
            svga->split |= 0x400;
        if (s3->accel.advfunc_cntl & 0x01)
            svga->split = 0x7fff;
        if (svga->crtc[0x51] & 0x30)
            svga->rowoffset |= (svga->crtc[0x51] & 0x30) << 4;
        else if (svga->crtc[0x43] & 0x04)
            svga->rowoffset |= 0x100;
    } else if (svga->crtc[0x43] & 0x04)
        svga->rowoffset |= 0x100;
    if (!svga->rowoffset)
        svga->rowoffset = 0x100;

    if ((s3->chip == S3_VISION964) || (s3->chip == S3_86C928)) {
        if (s3->card_type == S3_ELSAWIN2KPROX_964)
            ibm_rgb528_recalctimings(svga->ramdac, svga);
        else {
            bt48x_recalctimings(svga->ramdac, svga);
            svga->interlace |= (!!(svga->crtc[0x42] & 0x20));
        }
    } else if (s3->chip == S3_VISION968) {
        if ((s3->card_type == S3_SPEA_MERCURY_P64V) || (s3->card_type == S3_MIROVIDEO40SV_ERGO_968))
            tvp3026_recalctimings(svga->ramdac, svga);
        else
            ibm_rgb528_recalctimings(svga->ramdac, svga);
    } else
        svga->interlace = !!(svga->crtc[0x42] & 0x20);

    if ((((svga->miscout >> 2) & 3) == 3) && (s3->chip < S3_TRIO32))
        clk_sel = svga->crtc[0x42] & 0x0f;

    svga->clock = (cpuclock * (double) (1ULL << 32)) / svga->getclock(clk_sel, svga->clock_gen);

    switch (svga->crtc[0x67] >> 4) {
        case 3:
        case 5:
        case 7:
            svga->clock /= 2;
            break;

        default:
            break;
    }

    if (s3->chip <= S3_86C805) {
        s3->color_16bit = !!(svga->crtc[0x43] & 0x08);
        s3_log("Color 16bit=%x, bpp=%d, 256color=%x.\n", s3->color_16bit, svga->bpp, (svga->attrregs[0x10] & 0x40));
        if ((svga->bpp == 24) || (svga->bpp == 8))
            s3->color_16bit = 0;

        if (s3->color_16bit)
            s3->width = 1024;
        else {
            if (s3->chip <= S3_86C924)
                s3->width = 1024;
        }
    }

    if (s3->chip >= S3_86C928) {
        if (s3->chip != S3_86C801)
            mask |= 0x01;

        switch (svga->crtc[0x50] & mask) {
            case 0x00:
                s3->width = (svga->crtc[0x31] & 0x02) ? 2048 : 1024;
                if (s3->color_16bit)
                    s3->width = 1024;
                break;
            case 0x01:
                s3->width = 1152;
                break;
            case 0x40:
                s3->width = 640;
                break;
            case 0x80:
                s3->width = ((s3->chip > S3_86C805) && (s3->accel.advfunc_cntl & 0x04)) ? 1600 : 800;
                break;
            case 0x81:
                s3->width = 1600;
                break;
            case 0xc0:
                s3->width = 1280;
                break;

            default:
                break;
        }
    }

    if (svga->crtc[0x33] & 0x20) {
        /* The S3 version of the Cirrus' special blanking mode, with identical behavior. */
        svga->hblankstart = (((svga->crtc[0x5d] & 0x02) >> 1) << 8) + svga->crtc[1]/* +
                            ((svga->crtc[3] >> 5) & 3) + 1*/;
        svga->hblank_end_val = svga->htotal - 1 /* + ((svga->crtc[3] >> 5) & 3)*/;

        svga->monitor->mon_overscan_y = 0;
        svga->monitor->mon_overscan_x = 0;

        /* Also make sure vertical blanking starts on display end. */
        svga->vblankstart = svga->dispend;

        if (s3->chip >= S3_VISION964)
            svga->hblank_end_mask  = 0x7f;
    } else if (s3->chip >= S3_86C801) {
        svga->hblankstart    = (((svga->crtc[0x5d] & 0x04) >> 2) << 8) + svga->crtc[2];

        if (s3->chip >= S3_VISION964) {
            /* NOTE: The S3 Trio64V+ datasheet says this is bit 7, but then where is bit 6?
                     The datasheets for the pre-Trio64V+ cards say +64, which implies bit 6,
                     and, contrary to VGADOC, it also exists on Trio32, Trio64, Vision868,
                     and Vision968. */
            svga->hblank_end_val  = (svga->crtc[3] & 0x1f) | (((svga->crtc[5] & 0x80) >> 7) << 5) |
                                    (((svga->crtc[0x5d] & 0x08) >> 3) << 6);
            svga->hblank_end_mask  = 0x7f;
        }
    }

    if ((svga->crtc[0x3a] & 0x10) && !svga->lowres) {
        s3_log("BPP=%d, pitch=%d, width=%02x, double?=%x, 16bit?=%d, highres?=%d, "
               "attr=%02x, hdisp=%d.\n", svga->bpp, s3->width, svga->crtc[0x50],
               svga->crtc[0x31] & 0x02, s3->color_16bit, s3->accel.advfunc_cntl & 4,
               svga->attrregs[0x10] & 0x40, svga->hdisp);
        switch (svga->bpp) {
            case 8:
                svga->render = svga_render_8bpp_highres;
                switch (s3->chip) {
                    case S3_86C928:
                        switch (s3->card_type) {
                            case S3_METHEUS_86C928:
                                switch (s3->width) {
                                    case 1280: /*Account for the 1280x1024 resolution*/
                                        switch (svga->hdisp) {
                                            case 320:
                                                svga->hdisp <<= 2;
                                                svga->dots_per_clock <<= 2;
                                                break;
                                            case 640:
                                                svga->hdisp <<= 1;
                                                svga->dots_per_clock <<= 1;
                                                break;
                                            default:
                                                break;
                                        }
                                        break;
                                    case 2048: /*Account for the 1280x1024 resolution*/
                                        switch (svga->hdisp) {
                                            case 320:
                                                svga->hdisp <<= 2;
                                                svga->dots_per_clock <<= 2;
                                                break;
                                            case 640:
                                                svga->hdisp <<= 1;
                                                svga->dots_per_clock <<= 1;
                                                break;
                                            default:
                                                break;
                                        }
                                        break;
                                    default:
                                        break;
                                }
                                break;

                            default:
                                break;
                        }
                        break;
                    case S3_86C928PCI:
                        switch (s3->card_type) {
                            case S3_SPEA_MERCURY_LITE_PCI:
                                switch (s3->width) {
                                    case 640:
                                        svga->hdisp >>= 1;
                                        svga->dots_per_clock >>= 1;
                                        break;
                                    default:
                                        break;
                                }
                                break;

                            default:
                                break;
                        }
                        break;
                    case S3_VISION964:
                        switch (s3->card_type) {
                            case S3_ELSAWIN2KPROX_964:
                                switch (s3->width) {
                                    case 1280:
                                    case 1600:
                                        svga->hdisp <<= 1;
                                        svga->dots_per_clock <<= 1;
                                    default:
                                        break;
                                }
                                break;

                            default:
                                break;
                        }
                        break;
                    case S3_VISION968:
                        switch (s3->card_type) {
                            case S3_MIROVIDEO40SV_ERGO_968:
                                if (svga->hdisp == 832)
                                    svga->hdisp -= 32;
                                break;
                            case S3_DIAMOND_STEALTH64_968:
                            case S3_NUMBER9_9FX_771:
                            case S3_PHOENIX_VISION968:
                            case S3_SPEA_MERCURY_P64V:
                                svga->hdisp <<= 1;
                                svga->dots_per_clock <<= 1;
                                if (svga->hdisp == 832)
                                    svga->hdisp -= 32;
                                break;
                            case S3_ELSAWIN2KPROX:
                                switch (s3->width) {
                                    case 1280:
                                    case 1600:
                                        svga->hdisp <<= 1;
                                        svga->dots_per_clock <<= 1;
                                    default:
                                        break;
                                }
                                break;

                            default:
                                break;
                        }
                        break;

                    default:
                        break;
                }
                break;
            case 15:
                svga->render = svga_render_15bpp_highres;
                switch (s3->chip) {
                    case S3_86C911:
                    case S3_86C924:
                        svga->hdisp >>= 1;
                        svga->dots_per_clock >>= 1;
                        break;

                    case S3_86C801:
                        switch (s3->card_type) {
                            case S3_PHOENIX_86C801:
                                svga->hdisp >>= 1;
                                svga->dots_per_clock >>= 1;
                                break;

                            default:
                                break;
                        }
                        break;
                    case S3_86C805:
                        switch (s3->card_type) {
                            case S3_MIROCRYSTAL8S_805:
                            case S3_MIROCRYSTAL10SD_805:
                            case S3_WINNER1000_805:
                            case S3_PHOENIX_86C805:
                            case S3_86C805_ONBOARD:
                                svga->hdisp >>= 1;
                                svga->dots_per_clock >>= 1;
                                break;

                            case S3_SPEA_MIRAGE_86C805:
                                svga->hdisp >>= 1;
                                svga->dots_per_clock >>= 1;
                                switch (s3->width) {
                                    case 800:
                                    case 1024:
                                        if (svga->hdisp == 400) {
                                            /*SPEA specific drivers + its VBE RAM BIOS...*/
                                            svga->hdisp <<= 1;
                                            svga->dots_per_clock <<= 1;
                                        }
                                        break;
                                    default:
                                        break;
                                }
                                break;

                            default:
                                break;
                        }
                        break;
                    case S3_86C928:
                        switch (s3->card_type) {
                            case S3_METHEUS_86C928:
                                if (!s3->color_16bit) {
                                    svga->hdisp <<= 1;
                                    svga->dots_per_clock <<= 1;
                                }
                                switch (svga->hdisp) { /*This might be a driver issue*/
                                    case 800:
                                        s3->width = 1024;
                                        break;
                                    case 1280:
                                        s3->width = 2048;
                                        break;
                                    default:
                                        break;
                                }
                                break;

                            default:
                                break;
                        }
                        break;
                    case S3_86C928PCI:
                        switch (s3->card_type) {
                            case S3_SPEA_MERCURY_LITE_PCI:
                                switch (s3->width) {
                                    case 640:
                                        svga->hdisp >>= 1;
                                        svga->dots_per_clock >>= 1;
                                        break;
                                    default:
                                        break;
                                }
                                break;

                            default:
                                break;
                        }
                        break;
                    case S3_VISION864:
                        svga->hdisp >>= 1;
                        svga->dots_per_clock >>= 1;
                        break;
                    case S3_VISION964:
                        switch (s3->card_type) {
                            case S3_ELSAWIN2KPROX_964:
                                switch (s3->width) {
                                    case 1280:
                                    case 1600:
                                        svga->hdisp <<= 1;
                                        svga->dots_per_clock <<= 1;
                                        break;
                                    default:
                                        break;
                                }
                                break;

                            default:
                                break;
                        }
                        break;
                    case S3_VISION868:
                        switch (s3->card_type) {
                            case S3_PHOENIX_VISION868:
                            case S3_NUMBER9_9FX_531:
                                svga->hdisp >>= 1;
                                svga->dots_per_clock >>= 1;
                                break;

                            default:
                                break;
                        }
                        break;
                    case S3_VISION968:
                        switch (s3->card_type) {
                            case S3_MIROVIDEO40SV_ERGO_968:
                                if (svga->hdisp == 832)
                                    svga->hdisp -= 32;
                                break;
                            case S3_DIAMOND_STEALTH64_968:
                            case S3_NUMBER9_9FX_771:
                            case S3_PHOENIX_VISION968:
                            case S3_SPEA_MERCURY_P64V:
                                svga->hdisp <<= 1;
                                svga->dots_per_clock <<= 1;
                                /* TODO: Is this still needed? */
                                if (svga->hdisp == 832)
                                    svga->hdisp -= 32;
                                break;

                            case S3_ELSAWIN2KPROX:
                                switch (s3->width) {
                                    case 1280:
                                    case 1600:
                                        svga->hdisp <<= 1;
                                        svga->dots_per_clock <<= 1;
                                        break;
                                    default:
                                        break;
                                }
                                break;
                            default:
                                break;
                        }
                        break;

                    case S3_TRIO64:
                    case S3_TRIO32:
                        svga->hdisp >>= 1;
                        svga->dots_per_clock >>= 1;
                        break;

                    default:
                        break;
                }
                break;
            case 16:
                svga->render = svga_render_16bpp_highres;
                switch (s3->chip) {
                    case S3_86C911:
                    case S3_86C924:
                        svga->hdisp >>= 1;
                        svga->dots_per_clock >>= 1;
                        break;

                    case S3_86C801:
                        switch (s3->card_type) {
                            case S3_PHOENIX_86C801:
                                svga->hdisp >>= 1;
                                svga->dots_per_clock >>= 1;
                                break;

                            default:
                                break;
                        }
                        break;
                    case S3_86C805:
                        switch (s3->card_type) {
                            case S3_MIROCRYSTAL8S_805:
                            case S3_MIROCRYSTAL10SD_805:
                            case S3_WINNER1000_805:
                            case S3_PHOENIX_86C805:
                            case S3_86C805_ONBOARD:
                                svga->hdisp >>= 1;
                                svga->dots_per_clock >>= 1;
                                break;

                            case S3_SPEA_MIRAGE_86C805:
                                svga->hdisp >>= 1;
                                switch (s3->width) {
                                    case 800:
                                    case 1024:
                                        if (svga->hdisp == 400) {
                                            /*SPEA specific drivers + its VBE RAM BIOS...*/
                                            svga->hdisp <<= 1;
                                            svga->dots_per_clock <<= 1;
                                        }
                                        break;
                                    default:
                                        break;
                                }
                                break;

                            default:
                                break;
                        }
                        break;
                    case S3_86C928:
                        switch (s3->card_type) {
                            case S3_METHEUS_86C928:
                                svga->hdisp <<= 1;
                                svga->dots_per_clock <<= 1;
                                switch (svga->hdisp) { /*This might be a driver issue*/
                                    case 800:
                                        s3->width = 1024;
                                        break;
                                    case 1280:
                                        s3->width = 2048;
                                        break;
                                    default:
                                        break;
                                }
                                break;

                            default:
                                break;
                        }
                        break;
                    case S3_86C928PCI:
                        switch (s3->card_type) {
                            case S3_SPEA_MERCURY_LITE_PCI:
                                switch (s3->width) {
                                    case 640:
                                        svga->hdisp >>= 1;
                                        svga->dots_per_clock >>= 1;
                                      break;
                                    default:
                                        break;
                                }
                                break;

                            default:
                                break;
                        }
                        break;
                    case S3_VISION864:
                        svga->hdisp >>= 1;
                        svga->dots_per_clock >>= 1;
                        break;
                    case S3_VISION868:
                        switch (s3->card_type) {
                            case S3_PHOENIX_VISION868:
                            case S3_NUMBER9_9FX_531:
                                svga->hdisp >>= 1;
                                svga->dots_per_clock >>= 1;
                                break;

                            default:
                                break;
                        }
                        break;
                    case S3_VISION964:
                        switch (s3->card_type) {
                            case S3_ELSAWIN2KPROX_964:
                                switch (s3->width) {
                                    case 1280:
                                    case 1600:
                                        svga->hdisp <<= 1;
                                        svga->dots_per_clock <<= 1;
                                        break;
                                    default:
                                        break;
                                }
                                break;

                            default:
                                break;
                        }
                        break;
                    case S3_VISION968:
                        switch (s3->card_type) {
                            case S3_MIROVIDEO40SV_ERGO_968:
                                if (svga->hdisp == 832)
                                    svga->hdisp -= 32;
                                break;
                            case S3_DIAMOND_STEALTH64_968:
                            case S3_NUMBER9_9FX_771:
                            case S3_PHOENIX_VISION968:
                            case S3_SPEA_MERCURY_P64V:
                                svga->hdisp <<= 1;
                                svga->dots_per_clock <<= 1;
                                /* TODO: Is this still needed? */
                                if (svga->hdisp == 832)
                                    svga->hdisp -= 32;
                                break;

                            case S3_ELSAWIN2KPROX:
                                switch (s3->width) {
                                    case 1280:
                                    case 1600:
                                        svga->hdisp <<= 1;
                                        svga->dots_per_clock <<= 1;
                                        break;
                                    default:
                                        break;
                                }
                                break;
                            default:
                                break;
                        }
                        break;

                    case S3_TRIO64:
                    case S3_TRIO32:
                        svga->hdisp >>= 1;
                        svga->dots_per_clock >>= 1;
                        break;

                    default:
                        break;
                }
                break;
            case 24:
                svga->render = svga_render_24bpp_highres;
                switch (s3->chip) {
                    case S3_86C924:
                        switch (s3->card_type) {
                            case S3_AMI_86C924:
                                svga->hdisp = (svga->hdisp << 1) / 3;
                                svga->dots_per_clock = (svga->dots_per_clock << 1) / 3;
                                /* TODO: Is this still needed? */
                                if (svga->hdisp == 645)
                                    svga->hdisp -= 5;
                                break;
                            default:
                                break;
                        }
                        break;
                    case S3_86C801:
                        switch (s3->card_type) {
                            case S3_PHOENIX_86C801:
                            case S3_SPEA_MIRAGE_86C801:
                                svga->hdisp = (svga->hdisp << 1) / 3;
                                svga->dots_per_clock = (svga->dots_per_clock << 1) / 3;
                                break;
                            default:
                                break;
                        }
                        break;
                    case S3_86C805:
                        switch (s3->card_type) {
                            case S3_MIROCRYSTAL8S_805:
                            case S3_MIROCRYSTAL10SD_805:
                            case S3_WINNER1000_805:
                            case S3_PHOENIX_86C805:
                            case S3_SPEA_MIRAGE_86C805:
                            case S3_86C805_ONBOARD:
                                svga->hdisp = (svga->hdisp << 1) / 3;
                                svga->dots_per_clock = (svga->dots_per_clock << 1) / 3;
                                break;
                            default:
                                break;
                        }
                        break;
                    case S3_86C928PCI:
                        switch (s3->card_type) {
                            case S3_SPEA_MERCURY_LITE_PCI:
                                svga->hdisp = (svga->hdisp << 1) / 3;
                                svga->dots_per_clock = (svga->dots_per_clock << 1) / 3;
                                break;
                            default:
                                break;
                        }
                        break;
                    case S3_VISION864:
                        svga->hdisp = (svga->hdisp << 1) / 3;
                        svga->dots_per_clock = (svga->dots_per_clock << 1) / 3;
                        break;

                    case S3_VISION968:
                        switch (s3->card_type) {
                            case S3_MIROVIDEO40SV_ERGO_968:
                                svga->hdisp = (svga->hdisp / 3) << 2;
                                svga->dots_per_clock = (svga->hdisp / 3) << 2;
                                break;
                            default:
                                break;
                        }
                        break;

                    case S3_TRIO64:
                    case S3_TRIO32:
                        svga->hdisp /= 3;
                        svga->dots_per_clock /= 3;
                        break;

                    default:
                        break;
                }
                break;
            case 32:
                svga->render = svga_render_32bpp_highres;
                switch (s3->chip) {
                    case S3_VISION864:
                        svga->hdisp >>= 2;
                        svga->dots_per_clock >>= 2;
                        break;

                    case S3_VISION868:
                        switch (s3->card_type) {
                            case S3_PHOENIX_VISION868:
                            case S3_NUMBER9_9FX_531:
                                svga->hdisp >>= 1;
                                svga->dots_per_clock >>= 1;
                                break;
                            default:
                                break;
                        }
                        break;
                    case S3_VISION964:
                        switch (s3->card_type) {
                            case S3_MIROCRYSTAL20SV_964:
                                switch (s3->width) {
                                    case 800:
                                    case 1024:
                                        svga->hdisp >>= 1;
                                        svga->dots_per_clock >>= 1;
                                        break;
                                    default:
                                        break;
                                }
                                break;
                            case S3_ELSAWIN2KPROX_964:
                                switch (s3->width) {
                                    case 1280:
                                    case 1600:
                                        svga->hdisp <<= 1;
                                        svga->dots_per_clock <<= 1;
                                        break;
                                    default:
                                        break;
                                }
                                break;
                            default:
                                break;
                        }
                        break;
                    case S3_VISION968:
                        switch (s3->card_type) {
                            case S3_MIROVIDEO40SV_ERGO_968:
                                if (svga->hdisp == 832)
                                    svga->hdisp -= 32;
                                break;
                            case S3_DIAMOND_STEALTH64_968:
                            case S3_NUMBER9_9FX_771:
                            case S3_PHOENIX_VISION968:
                            case S3_SPEA_MERCURY_P64V:
                                svga->hdisp <<= 1;
                                svga->dots_per_clock <<= 1;
                                /* TODO: Is this still needed? */
                                if (svga->hdisp == 832)
                                    svga->hdisp -= 32;
                                break;

                            case S3_ELSAWIN2KPROX:
                                switch (s3->width) {
                                    case 1280:
                                    case 1600:
                                        svga->hdisp <<= 1;
                                        svga->dots_per_clock <<= 1;
                                        break;
                                    default:
                                        break;
                                }
                                break;
                            default:
                                break;
                        }
                        break;

                    default:
                        break;
                }
                break;

            default:
                break;
        }
        svga->vram_display_mask = s3->vram_mask;
    } else {
        if (svga->crtc[0x31] & 0x08) {
            if (!(svga->crtc[0x5e] & 0x04))
                svga->vblankstart = svga->dispend; /*Applies only to Enhanced modes*/

            /*Enhanced 4bpp mode, just like the 8bpp mode per the spec. */
            svga->render = svga_render_8bpp_highres;
            svga->rowoffset <<= 1;
            svga->vram_display_mask = s3->vram_mask;
        } else {
            svga->vram_display_mask = (svga->crtc[0x32] & 0x40) ? 0x3ffff : s3->vram_mask;
            s3_log("CRTC31 bit 0=%x, CRTC32 bit 6=%02x.\n", (svga->crtc[0x31] & 0x01), svga->crtc[0x32] & 0x40);
            if (!(svga->crtc[0x31] & 0x01)) { /*Bank Enable bit*/
                svga->write_bank = 0;
                svga->read_bank = 0;
            }
        }
    }

    if ((s3->chip == S3_TRIO32) || (s3->chip == S3_TRIO64) || (s3->chip == S3_VISION864) || (s3->chip == S3_VISION868) || (s3->chip == S3_VISION968))
        svga->hoverride = 1;
    else
        svga->hoverride = 0;

    if (svga->render == svga_render_2bpp_lowres)
        svga->render = svga_render_2bpp_s3_lowres;
    else if (svga->render == svga_render_2bpp_highres)
        svga->render = svga_render_2bpp_s3_highres;
}

static void
s3_trio64v_recalctimings(svga_t *svga)
{
    s3_t *s3            = (s3_t *) svga->priv;
    int         clk_sel = (svga->miscout >> 2) & 3;

    if (!svga->scrblank && svga->attr_palette_enable && (svga->crtc[0x43] & 0x80)) {
        /* TODO: In case of bug reports, disable 9-dots-wide character clocks in graphics modes. */
        svga->dots_per_clock = ((svga->seqregs[1] & 1) ? 16 : 18);
    }

    if ((svga->crtc[0x33] & 0x20) ||((svga->crtc[0x67] & 0xc) == 0xc)) {
        /* In this mode, the dots per clock are always 8 or 16, never 9 or 18. */
        if (!svga->scrblank && svga->attr_palette_enable)
            svga->dots_per_clock = (svga->seqregs[1] & 8) ? 16 : 8;
    }

    svga->hdisp = svga->hdisp_old;
    if (svga->crtc[0x5d] & 0x01)
        svga->htotal |= 0x100;
    if (svga->crtc[0x5d] & 0x02) {
        svga->hdisp_time |= 0x100;
        svga->hdisp |= (0x100 * svga->dots_per_clock);
    }
    if (svga->crtc[0x5e] & 0x01)
        svga->vtotal |= 0x400;
    if (svga->crtc[0x5e] & 0x02)
        svga->dispend |= 0x400;
    if (svga->crtc[0x5e] & 0x04)
        svga->vblankstart |= 0x400;
    if (svga->crtc[0x5e] & 0x10)
        svga->vsyncstart |= 0x400;
    if (svga->crtc[0x5e] & 0x40)
        svga->split |= 0x400;
    svga->interlace = svga->crtc[0x42] & 0x20;

    svga->clock = (cpuclock * (double) (1ULL << 32)) / svga->getclock(clk_sel, svga->clock_gen);

    switch (svga->crtc[0x50] & 0xc1) {
        case 0x00:
            s3->width = (svga->crtc[0x31] & 2) ? 2048 : 1024;
            break;
        case 0x01:
            s3->width = 1152;
            break;
        case 0x40:
            s3->width = 640;
            break;
        case 0x80:
            s3->width = (s3->accel.advfunc_cntl & 4) ? 1600 : 800;
            break;
        case 0x81:
            s3->width = 1600;
            break;
        case 0xc0:
            s3->width = 1280;
            break;

        default:
            break;
    }

    if ((svga->crtc[0x33] & 0x20) || ((svga->crtc[0x67] & 0xc) == 0xc)) {
        /* The S3 version of the Cirrus' special blanking mode, with identical behavior. */
        svga->hblankstart = (((svga->crtc[0x5d] & 0x02) >> 1) << 8) + svga->crtc[1]/* +
                            ((svga->crtc[3] >> 5) & 3)*/;
        svga->hblank_end_val = svga->htotal - 1 /* + ((svga->crtc[3] >> 5) & 3)*/;

        svga->monitor->mon_overscan_y = 0;
        svga->monitor->mon_overscan_x = 0;

        /* Also make sure vertical blanking starts on display end. */
        svga->vblankstart = svga->dispend;
    } else {
        svga->hblankstart    = (((svga->crtc[0x5d] & 0x04) >> 2) << 8) + svga->crtc[2];

        /* NOTE: The S3 Trio64V+ datasheet says this is bit 7, but then where is bit 6?
                 The datasheets for the pre-Trio64V+ cards say +64, which implies bit 6,
                 and, contrary to VGADOC, it also exists on Trio32, Trio64, Vision868,
                 and Vision968. */
        svga->hblank_end_val = (svga->crtc[3] & 0x1f) | (((svga->crtc[5] & 0x80) >> 7) << 5) |
                               (((svga->crtc[0x5d] & 0x08) >> 3) << 6);
        svga->hblank_end_mask = 0x7f;
    }

    if ((svga->crtc[0x67] & 0xc) != 0xc) /*VGA mode*/
    {
        svga->memaddr_latch |= (s3->ma_ext << 16);
        if (svga->crtc[0x51] & 0x30)
            svga->rowoffset |= (svga->crtc[0x51] & 0x30) << 4;
        else if (svga->crtc[0x43] & 0x04)
            svga->rowoffset |= 0x100;
        if (!svga->rowoffset)
            svga->rowoffset = 256;

        svga->lowres = (!!(svga->attrregs[0x10] & 0x40) && !(svga->crtc[0x3a] & 0x10));

        if ((svga->crtc[0x3a] & 0x10) && !svga->lowres) {
            switch (svga->bpp) {
                case 8:
                    svga->render = svga_render_8bpp_highres;
                    break;
                case 15:
                    svga->render = svga_render_15bpp_highres;
                    svga->hdisp >>= 1;
                    svga->dots_per_clock >>= 1;
                    break;
                case 16:
                    svga->render = svga_render_16bpp_highres;
                    svga->hdisp >>= 1;
                    svga->dots_per_clock >>= 1;
                    break;
                case 24:
                    svga->render = svga_render_24bpp_highres;
                    svga->hdisp /= 3;
                    svga->dots_per_clock /= 3;
                    break;
                case 32:
                    svga->render = svga_render_32bpp_highres;
                    break;

                default:
                    break;
            }
            svga->vram_display_mask = s3->vram_mask;
        } else {
            svga->vram_display_mask = (svga->crtc[0x32] & 0x40) ? 0x3ffff : s3->vram_mask;
            if (!(svga->crtc[0x31] & 0x01)) { /*Bank Enable bit*/
                svga->write_bank = 0;
                svga->read_bank = 0;
            }
        }
    } else /*Streams mode*/
    {
        if (s3->streams.buffer_ctrl & 1)
            svga->memaddr_latch = s3->streams.pri_fb1 >> 2;
        else
            svga->memaddr_latch = s3->streams.pri_fb0 >> 2;

        svga->hdisp = s3->streams.pri_w + 1;
        if (s3->streams.pri_h < svga->dispend)
            svga->dispend = s3->streams.pri_h;

        svga->overlay.x         = s3->streams.sec_x - s3->streams.pri_x;
        svga->overlay.y         = s3->streams.sec_y - s3->streams.pri_y;
        svga->overlay.cur_ysize = s3->streams.sec_h;

        if (s3->streams.buffer_ctrl & 2)
            svga->overlay.addr = s3->streams.sec_fb1;
        else
            svga->overlay.addr = s3->streams.sec_fb0;

        svga->overlay.ena   = (svga->overlay.x >= 0);
        svga->overlay.h_acc = s3->streams.dda_horiz_accumulator;
        svga->overlay.v_acc = s3->streams.dda_vert_accumulator;
        svga->rowoffset     = s3->streams.pri_stride >> 3;

        if (svga->overlay.ena) {
            svga->overlay.ena = (((s3->streams.blend_ctrl >> 24) & 7) == 0b000) ||
                                (((s3->streams.blend_ctrl >> 24) & 7) == 0b101);
        }
        switch ((s3->streams.pri_ctrl >> 24) & 0x7) {
            case 0: /*RGB-8 (CLUT)*/
                svga->render = svga_render_8bpp_highres;
                break;
            case 3: /*KRGB-16 (1.5.5.5)*/
                svga->render = svga_render_15bpp_highres;
                break;
            case 5: /*RGB-16 (5.6.5)*/
                svga->render = svga_render_16bpp_highres;
                break;
            case 6: /*RGB-24 (8.8.8)*/
                svga->render = svga_render_24bpp_highres;
                break;
            case 7: /*XRGB-32 (X.8.8.8)*/
                svga->render = svga_render_32bpp_highres;
                break;

            default:
                break;
        }
        svga->vram_display_mask = s3->vram_mask;
    }

    svga->hoverride = 1;

    if (svga->render == svga_render_2bpp_lowres)
        svga->render = svga_render_2bpp_s3_lowres;
    else if (svga->render == svga_render_2bpp_highres)
        svga->render = svga_render_2bpp_s3_highres;
}

static void
s3_updatemapping(s3_t *s3)
{
    svga_t *svga = &s3->svga;
    xga_t  *xga  = (xga_t *) svga->xga;

    if (s3->pci && !(s3->pci_regs[PCI_REG_COMMAND] & PCI_COMMAND_MEM)) {
        mem_mapping_disable(&svga->mapping);
        mem_mapping_disable(&s3->linear_mapping);
        mem_mapping_disable(&s3->mmio_mapping);
        mem_mapping_disable(&s3->new_mmio_mapping);
        return;
    }

    /*Banked framebuffer*/
    if (svga->crtc[0x31] & 0x08) /*Enhanced mode mappings*/
    {
        s3_log("Enhanced Mode Mapping.\n");
        /* Enhanced mode forces 64kb at 0xa0000*/
        mem_mapping_set_addr(&svga->mapping, 0xa0000, 0x10000);
        svga->banked_mask = 0xffff;
        if (xga_active && (svga->xga != NULL)) {
            xga->on = 0;
            mem_mapping_set_handler(&svga->mapping, svga->read, svga->readw, svga->readl, svga->write, svga->writew, svga->writel);
        }
    } else
        switch (svga->gdcreg[6] & 0xc) { /*VGA mapping*/
            case 0x0: /*128k at A0000*/
                mem_mapping_set_addr(&svga->mapping, 0xa0000, 0x20000);
                svga->banked_mask = 0xffff;
                break;
            case 0x4: /*64k at A0000*/
                mem_mapping_set_addr(&svga->mapping, 0xa0000, 0x10000);
                svga->banked_mask = 0xffff;
                if (xga_active && (svga->xga != NULL)) {
                    xga->on = 0;
                    mem_mapping_set_handler(&svga->mapping, svga->read, svga->readw, svga->readl, svga->write, svga->writew, svga->writel);
                }
                break;
            case 0x8: /*32k at B0000*/
                mem_mapping_set_addr(&svga->mapping, 0xb0000, 0x08000);
                svga->banked_mask = 0x7fff;
                break;
            case 0xC: /*32k at B8000*/
                mem_mapping_set_addr(&svga->mapping, 0xb8000, 0x08000);
                svga->banked_mask = 0x7fff;
                break;

            default:
                break;
        }

    if (s3->chip >= S3_86C928) {
        s3->linear_base = (svga->crtc[0x5a] << 16) | (svga->crtc[0x59] << 24);

        if (s3->chip >= S3_86C928 && s3->chip <= S3_86C805) {
            if (s3->vlb)
                s3->linear_base &= 0x03ffffff;
            else
                s3->linear_base &= 0x00ffffff;
        }
        if ((svga->crtc[0x58] & 0x10) || (s3->accel.advfunc_cntl & 0x10)) {
            /*Linear framebuffer*/
            mem_mapping_disable(&svga->mapping);

            switch (svga->crtc[0x58] & 3) {
                case 0: /*64k*/
                    s3->linear_size = 0x10000;
                    break;
                case 1: /*1mb*/
                    s3->linear_size = 0x100000;
                    break;
                case 2: /*2mb*/
                    s3->linear_size = 0x200000;
                    break;
                case 3:                 /*8mb*/
                    switch (s3->chip) { /* Not on video cards that don't support 4MB*/
                        case S3_TRIO64:
                        case S3_TRIO64V:
                        case S3_TRIO64V2:
                        case S3_86C928:
                        case S3_86C928PCI:
                            s3->linear_size = 0x400000;
                            break;
                        default:
                            s3->linear_size = 0x800000;
                            break;
                    }
                    break;

                default:
                    break;
            }
            s3->linear_base &= ~(s3->linear_size - 1);
            if (s3->linear_base == 0xa0000) {
                mem_mapping_disable(&s3->linear_mapping);
                if (!(svga->crtc[0x53] & 0x10)) {
                    mem_mapping_set_addr(&svga->mapping, s3->linear_base, 0x10000);
                    svga->banked_mask = 0xffff;
                }
            } else {
                if (s3->chip >= S3_TRIO64V)
                    s3->linear_base &= 0xfc000000;
                else if ((s3->chip == S3_VISION968) || (s3->chip == S3_VISION868))
                    s3->linear_base &= 0xfe000000;

                s3_log("LinearBase update=%x, size=%x.\n", s3->linear_base, s3->linear_size);
                if (s3->linear_base)
                    mem_mapping_set_addr(&s3->linear_mapping, s3->linear_base, s3->linear_size);
                else
                    mem_mapping_disable(&s3->linear_mapping);
            }
            svga->fb_only = 1;
        } else {
            svga->fb_only = 0;
            mem_mapping_disable(&s3->linear_mapping);
        }

        /* Memory mapped I/O. */
        s3_log("CRTC53=%02x, SEQREG9=%02x.\n", svga->crtc[0x53] & 0x18, svga->seqregs[9] & 0x80);
        if ((svga->crtc[0x53] & 0x10) || (s3->accel.advfunc_cntl & 0x20)) {
            mem_mapping_disable(&svga->mapping);
            if (s3->chip >= S3_TRIO64V) {
                if (svga->crtc[0x53] & 0x20)
                    mem_mapping_set_addr(&s3->mmio_mapping, 0xb8000, 0x8000);
                else
                    mem_mapping_set_addr(&s3->mmio_mapping, 0xa0000, 0x10000);
            } else
                mem_mapping_enable(&s3->mmio_mapping);
        } else {
            mem_mapping_disable(&s3->mmio_mapping);
        }

        /* New MMIO. */
        if (svga->crtc[0x53] & 0x08) {
            if (s3->linear_base)
                mem_mapping_set_addr(&s3->new_mmio_mapping, s3->linear_base + 0x1000000, 0x20000);
            else
                mem_mapping_disable(&s3->new_mmio_mapping);
        } else
            mem_mapping_disable(&s3->new_mmio_mapping);
    }
}

static float
s3_trio64_getclock(int clock, void *priv)
{
    const s3_t   *s3   = (s3_t *) priv;
    const svga_t *svga = &s3->svga;
    float         t;
    int           m;
    int           n1;
    int           n2;

    if (clock == 0)
        return 25175000.0;
    if (clock == 1)
        return 28322000.0;

    m  = svga->seqregs[0x13] + 2;
    n1 = (svga->seqregs[0x12] & 0x1f) + 2;
    n2 = ((svga->seqregs[0x12] >> 5) & 0x07);
    t  = (14318184.0 * ((float) m / (float) n1)) / (float) (1 << n2);
    return t;
}

static void
s3_accel_out(uint16_t port, uint8_t val, void *priv)
{
    s3_t   *s3   = (s3_t *) priv;
    svga_t *svga = &s3->svga;

    if (port >= 0x8000) {
        if (!s3->enable_8514)
            return;

        if (s3_enable_fifo(s3))
            s3_queue(s3, port, val, FIFO_OUT_BYTE);
        else
            s3_accel_out_fifo(s3, port, val);
    } else {
        switch (port) {
            case 0x4148:
            case 0x42e8:
                s3->subsys_stat &= ~val;
                s3->accel.subsys_cntl = (s3->accel.subsys_cntl & 0xff00) | val;
                s3_update_irqs(s3);
                break;
            case 0x4149:
            case 0x42e9:
                s3->accel.subsys_cntl = (s3->accel.subsys_cntl & 0xff) | (val << 8);
                s3_update_irqs(s3);
                break;
            case 0x4548:
            case 0x46e8:
                s3->accel.setup_md = val;
                break;
            case 0x4948:
            case 0x4ae8:
                s3->accel.advfunc_cntl = val;
                if ((s3->chip > S3_86C805) && ((svga->crtc[0x50] & 0xc1) == 0x80)) {
                    s3->width        = (val & 4) ? 1600 : 800;
                    svga->fullchange = svga->monitor->mon_changeframecount;
                    svga_recalctimings(svga);
                } else if (s3->chip <= S3_86C805) {
                    svga->fullchange = svga->monitor->mon_changeframecount;
                    svga_recalctimings(svga);
                }
                s3_updatemapping(s3);
                break;

            default:
                break;
        }
    }
}

static void
s3_accel_out_w(uint16_t port, uint16_t val, void *priv)
{
    s3_t *s3 = (s3_t *) priv;

    if (!s3->enable_8514)
        return;

    if (s3_enable_fifo(s3))
        s3_queue(s3, port, val, FIFO_OUT_WORD);
    else
        s3_accel_out_fifo_w(s3, port, val);
}

static void
s3_accel_out_l(uint16_t port, uint32_t val, void *priv)
{
    s3_t *s3 = (s3_t *) priv;

    if (!s3->enable_8514)
        return;

    if (s3_enable_fifo(s3))
        s3_queue(s3, port, val, FIFO_OUT_DWORD);
    else
        s3_accel_out_fifo_l(s3, port, val);
}

static uint8_t
s3_accel_in(uint16_t port, void *priv)
{
    s3_t   *s3   = (s3_t *) priv;
    svga_t *svga = &s3->svga;
    int     temp;
    uint8_t temp2 = 0x00;

    if (!s3->enable_8514)
        return 0xff;

    switch (port) {
        case 0x4148:
        case 0x42e8:
            return s3->subsys_stat;
        case 0x4149:
        case 0x42e9:
            return s3->accel.subsys_cntl >> 8;

        case 0x8148:
        case 0x82e8:
            if (s3_enable_fifo(s3))
                s3_wait_fifo_idle(s3);
            return s3->accel.cur_y & 0xff;
        case 0x8149:
        case 0x82e9:
            if (s3_enable_fifo(s3))
                s3_wait_fifo_idle(s3);
            return s3->accel.cur_y >> 8;

        case 0x8548:
        case 0x86e8:
            if (s3_enable_fifo(s3))
                s3_wait_fifo_idle(s3);
            return s3->accel.cur_x & 0xff;
        case 0x8549:
        case 0x86e9:
            if (s3_enable_fifo(s3))
                s3_wait_fifo_idle(s3);
            return s3->accel.cur_x >> 8;

        case 0x8948:
        case 0x8ae8:
            if (s3->chip >= S3_86C928) {
                if (s3_enable_fifo(s3))
                    s3_wait_fifo_idle(s3);
                return s3->accel.desty_axstp & 0xff;
            }
            break;
        case 0x8949:
        case 0x8ae9:
            if (s3->chip >= S3_86C928) {
                if (s3_enable_fifo(s3))
                    s3_wait_fifo_idle(s3);
                return s3->accel.desty_axstp >> 8;
            }
            break;

        case 0x8d48:
        case 0x8ee8:
            if (s3->chip >= S3_86C928) {
                if (s3_enable_fifo(s3))
                    s3_wait_fifo_idle(s3);
                return s3->accel.destx_distp & 0xff;
            }
            break;
        case 0x8d49:
        case 0x8ee9:
            if (s3->chip >= S3_86C928) {
                if (s3_enable_fifo(s3))
                    s3_wait_fifo_idle(s3);
                return s3->accel.destx_distp >> 8;
            }
            break;

        case 0x9148:
        case 0x92e8:
            if (s3_enable_fifo(s3))
                s3_wait_fifo_idle(s3);
            return s3->accel.err_term & 0xff;
        case 0x9149:
        case 0x92e9:
            if (s3_enable_fifo(s3))
                s3_wait_fifo_idle(s3);
            return s3->accel.err_term >> 8;

        case 0x9548:
        case 0x96e8:
            if (s3->chip >= S3_86C928) {
                if (s3_enable_fifo(s3))
                    s3_wait_fifo_idle(s3);
                return s3->accel.maj_axis_pcnt & 0xff;
            }
            break;
        case 0x9549:
        case 0x96e9:
            if (s3->chip >= S3_86C928) {
                if (s3_enable_fifo(s3))
                    s3_wait_fifo_idle(s3);
                return s3->accel.maj_axis_pcnt >> 8;
            }
            break;

        case 0x8118:
        case 0x9948:
        case 0x9ae8:
            temp = 0; /* FIFO empty */
            if (s3_enable_fifo(s3)) {
                if (!s3->blitter_busy)
                    wake_fifo_thread(s3);
                if (FIFO_FULL)
                    temp = 0xff;
            }
            return temp;
        case 0x8119:
        case 0x9949:
        case 0x9ae9:
            temp = 0;
            if (s3_enable_fifo(s3)) {
                if (!s3->blitter_busy)
                    wake_fifo_thread(s3);

                if (!FIFO_EMPTY || s3->force_busy)
                    temp |= 0x02; /*Hardware busy*/
                else
                    temp |= 0x04; /*FIFO empty*/

                s3->force_busy = 0;

                if (s3->chip >= S3_VISION964) {
                    if (FIFO_FULL)
                        temp |= 0xf8; /*FIFO full*/
                }

                if (s3->data_available) {
                    temp |= 0x01; /*Read Data available*/
                    s3->data_available = 0;
                }
            } else {
                if (s3->force_busy)
                    temp |= 0x02; /*Hardware busy*/
                else {
                    switch (s3->accel.cmd >> 13) { /*Some drivers may not set FIFO on but may still turn on FIFO empty bits!*/
                        case 0:
                            if (!s3->accel.ssv_len)
                                temp |= 0x04;
                            break;
                        case 1:
                            if (!s3->accel.sy)
                                temp |= 0x04;
                            break;
                        case 2:
                        case 6:
                        case 7:
                            if (s3->accel.sy < 0)
                                temp |= 0x04;
                            break;

                        default:
                            break;
                    }
                }

                s3->force_busy = 0;
                if (s3->data_available) {
                    temp |= 0x01; /*Read Data available*/
                    s3->data_available = 0;
                }
            }
            s3_log("FIFO Status Temp=%02x.\n", temp);
            return temp;

        case 0x9d48:
        case 0x9ee8:
            if (s3->chip >= S3_86C928) {
                if (s3_enable_fifo(s3))
                    s3_wait_fifo_idle(s3);
                return s3->accel.short_stroke & 0xff;
            }
            break;
        case 0x9d49:
        case 0x9ee9:
            if (s3->chip >= S3_86C928) {
                if (s3_enable_fifo(s3))
                    s3_wait_fifo_idle(s3);
                return s3->accel.short_stroke >> 8;
            }
            break;

        case 0xa148:
        case 0xa2e8:
            if (s3->chip >= S3_86C928) {
                if (s3_enable_fifo(s3))
                    s3_wait_fifo_idle(s3);

                if (s3->bpp == 3) {
                    if (s3->chip < S3_VISION964) {
                        if (s3->accel.multifunc[0xe] & 0x10)
                            temp2 = s3->accel.bkgd_color >> 16;
                        else
                            temp2 = s3->accel.bkgd_color & 0xff;
                    } else {
                        if ((s3->accel.multifunc[0xe] & 0x10) && !(s3->accel.multifunc[0xe] & 0x200))
                            temp2 = s3->accel.bkgd_color >> 16;
                        else
                            temp2 = s3->accel.bkgd_color & 0xff;
                    }
                } else
                    temp2 = s3->accel.bkgd_color & 0xff;

                return temp2;
            }
            break;
        case 0xa149:
        case 0xa2e9:
            if (s3->chip >= S3_86C928) {
                if (s3_enable_fifo(s3))
                    s3_wait_fifo_idle(s3);

                if (s3->bpp == 3) {
                    if (s3->chip < S3_VISION964) {
                        if (s3->accel.multifunc[0xe] & 0x10)
                            temp2 = s3->accel.bkgd_color >> 24;
                        else
                            temp2 = s3->accel.bkgd_color >> 8;

                        s3->accel.multifunc[0xe] ^= 0x10;
                    } else {
                        if ((s3->accel.multifunc[0xe] & 0x10) && !(s3->accel.multifunc[0xe] & 0x200))
                            temp2 = s3->accel.bkgd_color >> 24;
                        else
                            temp2 = s3->accel.bkgd_color >> 8;

                        if (!(s3->accel.multifunc[0xe] & 0x200))
                            s3->accel.multifunc[0xe] ^= 0x10;
                    }
                } else {
                    temp2 = s3->accel.bkgd_color >> 8;

                    if (s3->chip < S3_VISION964)
                        s3->accel.multifunc[0xe] ^= 0x10;
                    else {
                        if (!(s3->accel.multifunc[0xe] & 0x200))
                            s3->accel.multifunc[0xe] ^= 0x10;
                    }
                }
                return temp2;
            }
            break;
        case 0xa14a:
        case 0xa2ea:
            if (s3_enable_fifo(s3))
                s3_wait_fifo_idle(s3);

            if (s3->accel.multifunc[0xe] & 0x200)
                temp2 = s3->accel.bkgd_color >> 16;
            else if (s3->bpp == 3) {
                if (s3->accel.multifunc[0xe] & 0x10)
                    temp2 = s3->accel.bkgd_color & 0xff;
                else
                    temp2 = s3->accel.bkgd_color >> 16;
            }
            return temp2;
        case 0xa14b:
        case 0xa2eb:
            if (s3_enable_fifo(s3))
                s3_wait_fifo_idle(s3);

            if (s3->accel.multifunc[0xe] & 0x200)
                temp2 = s3->accel.bkgd_color >> 24;
            else if (s3->bpp == 3) {
                if (s3->accel.multifunc[0xe] & 0x10)
                    temp2 = s3->accel.bkgd_color >> 8;
                else
                    temp2 = s3->accel.bkgd_color >> 24;

                s3->accel.multifunc[0xe] ^= 0x10;
            }
            return temp2;
        case 0xa548:
        case 0xa6e8:
            if (s3->chip >= S3_86C928) {
                if (s3_enable_fifo(s3))
                    s3_wait_fifo_idle(s3);

                if (s3->bpp == 3) {
                    if (s3->chip < S3_VISION964) {
                        if (s3->accel.multifunc[0xe] & 0x10)
                            temp2 = s3->accel.frgd_color >> 16;
                        else
                            temp2 = s3->accel.frgd_color & 0xff;
                    } else {
                        if ((s3->accel.multifunc[0xe] & 0x10) && !(s3->accel.multifunc[0xe] & 0x200))
                            temp2 = s3->accel.frgd_color >> 16;
                        else
                            temp2 = s3->accel.frgd_color & 0xff;
                    }
                } else
                    temp2 = s3->accel.frgd_color & 0xff;

                return temp2;
            }
            break;
        case 0xa549:
        case 0xa6e9:
            if (s3->chip >= S3_86C928) {
                if (s3_enable_fifo(s3))
                    s3_wait_fifo_idle(s3);

                if (s3->bpp == 3) {
                    if (s3->chip < S3_VISION964) {
                        if (s3->accel.multifunc[0xe] & 0x10)
                            temp2 = s3->accel.frgd_color >> 24;
                        else
                            temp2 = s3->accel.frgd_color >> 8;

                        s3->accel.multifunc[0xe] ^= 0x10;
                    } else {
                        if ((s3->accel.multifunc[0xe] & 0x10) && !(s3->accel.multifunc[0xe] & 0x200))
                            temp2 = s3->accel.frgd_color >> 24;
                        else
                            temp2 = s3->accel.frgd_color >> 8;

                        if (!(s3->accel.multifunc[0xe] & 0x200))
                            s3->accel.multifunc[0xe] ^= 0x10;
                    }
                } else {
                    temp2 = s3->accel.frgd_color >> 8;

                    if (s3->chip < S3_VISION964)
                        s3->accel.multifunc[0xe] ^= 0x10;
                    else {
                        if (!(s3->accel.multifunc[0xe] & 0x200))
                            s3->accel.multifunc[0xe] ^= 0x10;
                    }
                }
                return temp2;
            }
            break;
        case 0xa54a:
        case 0xa6ea:
            if (s3_enable_fifo(s3))
                s3_wait_fifo_idle(s3);

            if (s3->accel.multifunc[0xe] & 0x200)
                temp2 = s3->accel.frgd_color >> 16;
            else if (s3->bpp == 3) {
                if (s3->accel.multifunc[0xe] & 0x10)
                    temp2 = s3->accel.frgd_color & 0xff;
                else
                    temp2 = s3->accel.frgd_color >> 16;
            }
            return temp2;
        case 0xa54b:
        case 0xa6eb:
            if (s3_enable_fifo(s3))
                s3_wait_fifo_idle(s3);

            if (s3->accel.multifunc[0xe] & 0x200)
                temp2 = s3->accel.frgd_color >> 24;
            else if (s3->bpp == 3) {
                if (s3->accel.multifunc[0xe] & 0x10)
                    temp2 = s3->accel.frgd_color >> 8;
                else
                    temp2 = s3->accel.frgd_color >> 24;

                s3->accel.multifunc[0xe] ^= 0x10;
            }
            return temp2;

        case 0xa948:
        case 0xaae8:
            if (s3->chip >= S3_86C928) {
                if (s3_enable_fifo(s3))
                    s3_wait_fifo_idle(s3);

                if (s3->bpp == 3) {
                    if (s3->chip < S3_VISION964) {
                        if (s3->accel.multifunc[0xe] & 0x10)
                            temp2 = s3->accel.wrt_mask >> 16;
                        else
                            temp2 = s3->accel.wrt_mask & 0xff;
                    } else {
                        if ((s3->accel.multifunc[0xe] & 0x10) && !(s3->accel.multifunc[0xe] & 0x200))
                            temp2 = s3->accel.wrt_mask >> 16;
                        else
                            temp2 = s3->accel.wrt_mask & 0xff;
                    }
                } else
                    temp2 = s3->accel.wrt_mask & 0xff;

                return temp2;
            }
            break;
        case 0xa949:
        case 0xaae9:
            if (s3->chip >= S3_86C928) {
                if (s3_enable_fifo(s3))
                    s3_wait_fifo_idle(s3);

                if (s3->bpp == 3) {
                    if (s3->chip < S3_VISION964) {
                        if (s3->accel.multifunc[0xe] & 0x10)
                            temp2 = s3->accel.wrt_mask >> 24;
                        else
                            temp2 = s3->accel.wrt_mask >> 8;

                        s3->accel.multifunc[0xe] ^= 0x10;
                    } else {
                        if ((s3->accel.multifunc[0xe] & 0x10) && !(s3->accel.multifunc[0xe] & 0x200))
                            temp2 = s3->accel.wrt_mask >> 24;
                        else
                            temp2 = s3->accel.wrt_mask >> 8;

                        if (!(s3->accel.multifunc[0xe] & 0x200))
                            s3->accel.multifunc[0xe] ^= 0x10;
                    }
                } else {
                    temp2 = s3->accel.wrt_mask >> 8;

                    if (s3->chip < S3_VISION964)
                        s3->accel.multifunc[0xe] ^= 0x10;
                    else {
                        if (!(s3->accel.multifunc[0xe] & 0x200))
                            s3->accel.multifunc[0xe] ^= 0x10;
                    }
                }
                return temp2;
            }
            break;
        case 0xa94a:
        case 0xaaea:
            if (s3_enable_fifo(s3))
                s3_wait_fifo_idle(s3);

            if (s3->accel.multifunc[0xe] & 0x200)
                temp2 = s3->accel.wrt_mask >> 16;
            else if (s3->bpp == 3) {
                if (s3->accel.multifunc[0xe] & 0x10)
                    temp2 = s3->accel.wrt_mask & 0xff;
                else
                    temp2 = s3->accel.wrt_mask >> 16;
            }
            return temp2;
        case 0xa94b:
        case 0xaaeb:
            if (s3_enable_fifo(s3))
                s3_wait_fifo_idle(s3);

            if (s3->accel.multifunc[0xe] & 0x200)
                temp2 = s3->accel.wrt_mask >> 24;
            else if (s3->bpp == 3) {
                if (s3->accel.multifunc[0xe] & 0x10)
                    temp2 = s3->accel.wrt_mask >> 8;
                else
                    temp2 = s3->accel.wrt_mask >> 24;

                s3->accel.multifunc[0xe] ^= 0x10;
            }
            return temp2;

        case 0xad48:
        case 0xaee8:
            if (s3->chip >= S3_86C928) {
                if (s3_enable_fifo(s3))
                    s3_wait_fifo_idle(s3);

                if (s3->bpp == 3) {
                    if (s3->chip < S3_VISION964) {
                        if (s3->accel.multifunc[0xe] & 0x10)
                            temp2 = s3->accel.rd_mask >> 16;
                        else
                            temp2 = s3->accel.rd_mask & 0xff;
                    } else {
                        if ((s3->accel.multifunc[0xe] & 0x10) && !(s3->accel.multifunc[0xe] & 0x200))
                            temp2 = s3->accel.rd_mask >> 16;
                        else
                            temp2 = s3->accel.rd_mask & 0xff;
                    }
                } else
                    temp2 = s3->accel.rd_mask & 0xff;

                return temp2;
            }
            break;
        case 0xad49:
        case 0xaee9:
            if (s3->chip >= S3_86C928) {
                if (s3_enable_fifo(s3))
                    s3_wait_fifo_idle(s3);

                if (s3->bpp == 3) {
                    if (s3->chip < S3_VISION964) {
                        if (s3->accel.multifunc[0xe] & 0x10)
                            temp2 = s3->accel.rd_mask >> 24;
                        else
                            temp2 = s3->accel.rd_mask >> 8;

                        s3->accel.multifunc[0xe] ^= 0x10;
                    } else {
                        if ((s3->accel.multifunc[0xe] & 0x10) && !(s3->accel.multifunc[0xe] & 0x200))
                            temp2 = s3->accel.rd_mask >> 24;
                        else
                            temp2 = s3->accel.rd_mask >> 8;

                        if (!(s3->accel.multifunc[0xe] & 0x200))
                            s3->accel.multifunc[0xe] ^= 0x10;
                    }
                } else {
                    temp2 = s3->accel.rd_mask >> 8;

                    if (s3->chip < S3_VISION964)
                        s3->accel.multifunc[0xe] ^= 0x10;
                    else {
                        if (!(s3->accel.multifunc[0xe] & 0x200))
                            s3->accel.multifunc[0xe] ^= 0x10;
                    }
                }

                return temp2;
            }
            break;
        case 0xad4a:
        case 0xaeea:
            if (s3_enable_fifo(s3))
                s3_wait_fifo_idle(s3);

            if (s3->accel.multifunc[0xe] & 0x200)
                temp2 = s3->accel.rd_mask >> 16;
            else if (s3->bpp == 3) {
                if (s3->accel.multifunc[0xe] & 0x10)
                    temp2 = s3->accel.rd_mask & 0xff;
                else
                    temp2 = s3->accel.rd_mask >> 16;
            }
            return temp2;
        case 0xad4b:
        case 0xaeeb:
            if (s3_enable_fifo(s3))
                s3_wait_fifo_idle(s3);

            if (s3->accel.multifunc[0xe] & 0x200)
                temp2 = s3->accel.rd_mask >> 24;
            else if (s3->bpp == 3) {
                if (s3->accel.multifunc[0xe] & 0x10)
                    temp2 = s3->accel.rd_mask >> 8;
                else
                    temp2 = s3->accel.rd_mask >> 24;

                s3->accel.multifunc[0xe] ^= 0x10;
            }
            return temp2;

        case 0xb148:
        case 0xb2e8:
            if (s3->chip >= S3_86C928) {
                if (s3_enable_fifo(s3))
                    s3_wait_fifo_idle(s3);
                if ((s3->bpp == 3) && (s3->accel.multifunc[0xe] & 0x10) && !(s3->accel.multifunc[0xe] & 0x200))
                    temp2 = s3->accel.color_cmp >> 16;
                else
                    temp2 = s3->accel.color_cmp & 0xff;

                return temp2;
            }
            break;
        case 0xb149:
        case 0xb2e9:
            if (s3->chip >= S3_86C928) {
                if (s3_enable_fifo(s3))
                    s3_wait_fifo_idle(s3);
                if ((s3->bpp == 3) && (s3->accel.multifunc[0xe] & 0x10) && !(s3->accel.multifunc[0xe] & 0x200))
                    temp2 = s3->accel.color_cmp >> 24;
                else
                    temp2 = s3->accel.color_cmp >> 8;

                if (!(s3->accel.multifunc[0xe] & 0x200))
                    s3->accel.multifunc[0xe] ^= 0x10;
                return temp2;
            }
            break;
        case 0xb14a:
        case 0xb2ea:
            if (s3_enable_fifo(s3))
                s3_wait_fifo_idle(s3);
            if ((s3->bpp == 3) && (s3->accel.multifunc[0xe] & 0x10) && !(s3->accel.multifunc[0xe] & 0x200))
                temp2 = s3->accel.color_cmp & 0xff;
            else
                temp2 = s3->accel.color_cmp >> 16;

            return temp2;
        case 0xb14b:
        case 0xb2eb:
            if (s3_enable_fifo(s3))
                s3_wait_fifo_idle(s3);
            if ((s3->bpp == 3) && (s3->accel.multifunc[0xe] & 0x10) && !(s3->accel.multifunc[0xe] & 0x200))
                temp2 = s3->accel.color_cmp >> 8;
            else
                temp2 = s3->accel.color_cmp >> 24;

            if (!(s3->accel.multifunc[0xe] & 0x200))
                s3->accel.multifunc[0xe] ^= 0x10;
            return temp2;

        case 0xb548:
        case 0xb6e8:
            if (s3->chip >= S3_86C928) {
                if (s3_enable_fifo(s3))
                    s3_wait_fifo_idle(s3);
                return s3->accel.bkgd_mix;
            }
            break;

        case 0xb948:
        case 0xbae8:
            if (s3->chip >= S3_86C928) {
                if (s3_enable_fifo(s3))
                    s3_wait_fifo_idle(s3);
                return s3->accel.frgd_mix;
            }
            break;

        case 0xbd48:
        case 0xbee8:
            if (s3->chip >= S3_86C928) {
                if (s3_enable_fifo(s3))
                    s3_wait_fifo_idle(s3);
                if (s3->chip >= S3_VISION964) {
                    temp = s3->accel.multifunc[0xf] & 0xf;
                    switch (temp) {
                        case 0x0:
                            return s3->accel.multifunc[0x0] & 0xff;
                        case 0x1:
                            return s3->accel.multifunc[0x1] & 0xff;
                        case 0x2:
                            return s3->accel.multifunc[0x2] & 0xff;
                        case 0x3:
                            return s3->accel.multifunc[0x3] & 0xff;
                        case 0x4:
                            return s3->accel.multifunc[0x4] & 0xff;
                        case 0x5:
                            return s3->accel.multifunc[0xa] & 0xff;
                        case 0x6:
                            return s3->accel.multifunc[0xe] & 0xff;
                        case 0x7:
                            return s3->accel.cmd & 0xff;
                        case 0x8:
                            return s3->accel.subsys_cntl & 0xff;
                        case 0x9:
                            return s3->accel.setup_md & 0xff;
                        case 0xa:
                            return s3->accel.multifunc[0xd] & 0xff;

                        default:
                            break;
                    }
                } else {
                    temp = s3->accel.multifunc[0xf] & 7;
                    switch (temp) {
                        case 0x0:
                            return s3->accel.multifunc[0x0] & 0xff;
                        case 0x1:
                            return s3->accel.multifunc[0x1] & 0xff;
                        case 0x2:
                            return s3->accel.multifunc[0x2] & 0xff;
                        case 0x3:
                            return s3->accel.multifunc[0x3] & 0xff;
                        case 0x4:
                            return s3->accel.multifunc[0x4] & 0xff;
                        case 0x5:
                            return s3->accel.multifunc[0xa] & 0xff;
                        case 0x6:
                            return s3->accel.multifunc[0xe] & 0xff;
                        case 0x7:
                            return s3->accel.cmd & 0xff;

                        default:
                            break;
                    }
                }
                return 0xff;
            }
            break;
        case 0xbd49:
        case 0xbee9:
            if (s3->chip >= S3_86C928) {
                if (s3_enable_fifo(s3))
                    s3_wait_fifo_idle(s3);
                if (s3->chip >= S3_VISION964) {
                    temp = s3->accel.multifunc[0xf] & 0xf;
                    s3->accel.multifunc[0xf] = (s3->accel.multifunc[0xf] + 1) & 0xf;
                    switch (temp) {
                        case 0x0:
                            return s3->accel.multifunc[0x0] >> 8;
                        case 0x1:
                            return s3->accel.multifunc[0x1] >> 8;
                        case 0x2:
                            return s3->accel.multifunc[0x2] >> 8;
                        case 0x3:
                            return s3->accel.multifunc[0x3] >> 8;
                        case 0x4:
                            return s3->accel.multifunc[0x4] >> 8;
                        case 0x5:
                            return s3->accel.multifunc[0xa] >> 8;
                        case 0x6:
                            return s3->accel.multifunc[0xe] >> 8;
                        case 0x7:
                            return s3->accel.cmd >> 8;
                        case 0x8:
                            return (s3->accel.subsys_cntl >> 8) & ~0xe000;
                        case 0x9:
                            return (s3->accel.setup_md >> 8) & ~0xf000;
                        case 0xa:
                            return s3->accel.multifunc[0xd] >> 8;

                        default:
                            break;
                    }
                } else {
                    temp = s3->accel.multifunc[0xf] & 7;
                    s3->accel.multifunc[0xf] = (s3->accel.multifunc[0xf] + 1) & 7;
                    switch (temp) {
                        case 0x0:
                            return s3->accel.multifunc[0x0] >> 8;
                        case 0x1:
                            return s3->accel.multifunc[0x1] >> 8;
                        case 0x2:
                            return s3->accel.multifunc[0x2] >> 8;
                        case 0x3:
                            return s3->accel.multifunc[0x3] >> 8;
                        case 0x4:
                            return s3->accel.multifunc[0x4] >> 8;
                        case 0x5:
                            return s3->accel.multifunc[0xa] >> 8;
                        case 0x6:
                            return s3->accel.multifunc[0xe] >> 8;
                        case 0x7:
                            return s3->accel.cmd >> 8;

                        default:
                            break;
                    }
                }
                return 0xff;
            }
            break;

        case 0xd148:
        case 0xd2e8:
            if (s3_enable_fifo(s3))
                s3_wait_fifo_idle(s3);

            return s3->accel.ropmix & 0xff;

        case 0xd149:
        case 0xd2e9:
            if (s3_enable_fifo(s3))
                s3_wait_fifo_idle(s3);

            return s3->accel.ropmix >> 8;

        case 0xe548:
        case 0xe6e8:
            if (s3_enable_fifo(s3))
                s3_wait_fifo_idle(s3);

            if (s3->bpp == 3) {
                if ((s3->accel.multifunc[0xe] & 0x10) && !(s3->accel.multifunc[0xe] & 0x200))
                    temp2 = s3->accel.pat_bg_color >> 16;
                else
                    temp2 = s3->accel.pat_bg_color & 0xff;
            } else
                temp2 = s3->accel.pat_bg_color & 0xff;

            return temp2;

        case 0xe549:
        case 0xe6e9:
            if (s3_enable_fifo(s3))
                s3_wait_fifo_idle(s3);

            if (s3->bpp == 3) {
                if ((s3->accel.multifunc[0xe] & 0x10) && !(s3->accel.multifunc[0xe] & 0x200))
                    temp2 = s3->accel.pat_bg_color >> 24;
                else
                    temp2 = s3->accel.pat_bg_color >> 8;

                if (!(s3->accel.multifunc[0xe] & 0x200))
                    s3->accel.multifunc[0xe] ^= 0x10;
            } else
                temp2 = s3->accel.pat_bg_color >> 8;

            if (!(s3->accel.multifunc[0xe] & 0x200))
                s3->accel.multifunc[0xe] ^= 0x10;

            return temp2;

        case 0xe54a:
        case 0xe6ea:
            if (s3_enable_fifo(s3))
                s3_wait_fifo_idle(s3);

            if (s3->accel.multifunc[0xe] & 0x200)
                temp2 = s3->accel.pat_bg_color >> 16;
            else if (s3->bpp == 3) {
                if (s3->accel.multifunc[0xe] & 0x10)
                    temp2 = s3->accel.pat_bg_color & 0xff;
                else
                    temp2 = s3->accel.pat_bg_color >> 16;
            }
            return temp2;

        case 0xe54b:
        case 0xe6eb:
            if (s3_enable_fifo(s3))
                s3_wait_fifo_idle(s3);

            if (s3->accel.multifunc[0xe] & 0x200)
                temp2 = s3->accel.pat_bg_color >> 24;
            else if (s3->bpp == 3) {
                if (s3->accel.multifunc[0xe] & 0x10)
                    temp2 = s3->accel.pat_bg_color >> 8;
                else
                    temp2 = s3->accel.pat_bg_color >> 24;

                s3->accel.multifunc[0xe] ^= 0x10;
            }
            return temp2;

        case 0xe948:
        case 0xeae8:
            if (s3_enable_fifo(s3))
                s3_wait_fifo_idle(s3);
            return s3->accel.pat_y & 0xff;

        case 0xe949:
        case 0xeae9:
            if (s3_enable_fifo(s3))
                s3_wait_fifo_idle(s3);
            return s3->accel.pat_y >> 8;

        case 0xe94a:
        case 0xeaea:
            if (s3_enable_fifo(s3))
                s3_wait_fifo_idle(s3);
            return s3->accel.pat_x & 0xff;

        case 0xe94b:
        case 0xeaeb:
            if (s3_enable_fifo(s3))
                s3_wait_fifo_idle(s3);
            return s3->accel.pat_x >> 8;

        case 0xed48:
        case 0xeee8:
            if (s3_enable_fifo(s3))
                s3_wait_fifo_idle(s3);

            if (s3->bpp == 3) {
                if ((s3->accel.multifunc[0xe] & 0x10) && !(s3->accel.multifunc[0xe] & 0x200))
                    temp2 = s3->accel.pat_fg_color >> 16;
                else
                    temp2 = s3->accel.pat_fg_color & 0xff;
            } else
                temp2 = s3->accel.pat_fg_color & 0xff;

            return temp2;

        case 0xed49:
        case 0xeee9:
            if (s3_enable_fifo(s3))
                s3_wait_fifo_idle(s3);

            if (s3->bpp == 3) {
                if ((s3->accel.multifunc[0xe] & 0x10) && !(s3->accel.multifunc[0xe] & 0x200))
                    temp2 = s3->accel.pat_fg_color >> 24;
                else
                    temp2 = s3->accel.pat_fg_color >> 8;

                if (!(s3->accel.multifunc[0xe] & 0x200))
                    s3->accel.multifunc[0xe] ^= 0x10;
            } else
                temp2 = s3->accel.pat_fg_color >> 8;

            if (!(s3->accel.multifunc[0xe] & 0x200))
                s3->accel.multifunc[0xe] ^= 0x10;

            return temp2;

        case 0xed4a:
        case 0xeeea:
            if (s3_enable_fifo(s3))
                s3_wait_fifo_idle(s3);

            if (s3->accel.multifunc[0xe] & 0x200)
                temp2 = s3->accel.pat_fg_color >> 16;
            else if (s3->bpp == 3) {
                if (s3->accel.multifunc[0xe] & 0x10)
                    temp2 = s3->accel.pat_fg_color & 0xff;
                else
                    temp2 = s3->accel.pat_fg_color >> 16;
            }
            return temp2;

        case 0xed4b:
        case 0xeeeb:
            if (s3_enable_fifo(s3))
                s3_wait_fifo_idle(s3);

            if (s3->accel.multifunc[0xe] & 0x200)
                temp2 = s3->accel.pat_fg_color >> 24;
            else if (s3->bpp == 3) {
                if (s3->accel.multifunc[0xe] & 0x10)
                    temp2 = s3->accel.pat_fg_color >> 8;
                else
                    temp2 = s3->accel.pat_fg_color >> 24;

                s3->accel.multifunc[0xe] ^= 0x10;
            }
            return temp2;

        case 0xe148:
        case 0xe2e8:
            if (!s3_cpu_dest(s3))
                break;
            READ_PIXTRANS_BYTE_IO(0);
            if (s3->accel.cmd & 0x100) {
                switch (s3->accel.cmd & 0x600) {
                    case 0x000:
                        if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 0x02)) {
                            if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40))
                                s3->accel_start(8, 1, s3->accel.pix_trans[0], 0, s3);
                            else
                                s3->accel_start(1, 1, 0xffffffff, s3->accel.pix_trans[0], s3);
                        } else
                            s3->accel_start(1, 1, 0xffffffff, s3->accel.pix_trans[0], s3);
                        break;
                    case 0x200:
                        if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 0x02)) {
                            if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40))
                                s3->accel_start(16, 1, s3->accel.pix_trans[0], 0, s3);
                            else
                                s3->accel_start(2, 1, 0xffffffff, s3->accel.pix_trans[0], s3);
                        } else
                            s3->accel_start(2, 1, 0xffffffff, s3->accel.pix_trans[0], s3);
                        break;

                    default:
                        break;
                }
            }
            return s3->accel.pix_trans[0];

        case 0xe149:
        case 0xe2e9:
            if (!s3_cpu_dest(s3))
                break;
            READ_PIXTRANS_BYTE_IO(1);
            if (s3->accel.cmd & 0x100) {
                switch (s3->accel.cmd & 0x600) {
                    case 0x000:
                        if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 0x02)) {
                            if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40))
                                s3->accel_start(8, 1, s3->accel.pix_trans[0] | (s3->accel.pix_trans[1] << 8), 0, s3);
                            else
                                s3->accel_start(1, 1, 0xffffffff, s3->accel.pix_trans[0] | (s3->accel.pix_trans[1] << 8), s3);
                        } else
                            s3->accel_start(1, 1, 0xffffffff, s3->accel.pix_trans[0] | (s3->accel.pix_trans[1] << 8), s3);
                        break;
                    case 0x200:
                        if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 0x02)) {
                            if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40)) {
                                if (s3->accel.cmd & 0x1000)
                                    s3->accel_start(16, 1, s3->accel.pix_trans[1] | (s3->accel.pix_trans[0] << 8), 0, s3);
                                else
                                    s3->accel_start(16, 1, s3->accel.pix_trans[0] | (s3->accel.pix_trans[1] << 8), 0, s3);
                            } else {
                                if (s3->accel.cmd & 0x1000)
                                    s3->accel_start(2, 1, 0xffffffff, s3->accel.pix_trans[1] | (s3->accel.pix_trans[0] << 8), s3);
                                else
                                    s3->accel_start(2, 1, 0xffffffff, s3->accel.pix_trans[0] | (s3->accel.pix_trans[1] << 8), s3);
                            }
                        } else {
                            if (s3->accel.cmd & 0x1000)
                                s3->accel_start(2, 1, 0xffffffff, s3->accel.pix_trans[1] | (s3->accel.pix_trans[0] << 8), s3);
                            else
                                s3->accel_start(2, 1, 0xffffffff, s3->accel.pix_trans[0] | (s3->accel.pix_trans[1] << 8), s3);
                        }
                        break;

                    default:
                        break;
                }
            }
            return s3->accel.pix_trans[1];

        case 0xe14a:
        case 0xe2ea:
            if (!s3_cpu_dest(s3))
                break;
            READ_PIXTRANS_BYTE_IO(2);
            return s3->accel.pix_trans[2];

        case 0xe14b:
        case 0xe2eb:
            if (!s3_cpu_dest(s3))
                break;
            READ_PIXTRANS_BYTE_IO(3);
            if (s3->accel.cmd & 0x100) {
                switch (s3->accel.cmd & 0x600) {
                    case 0x000:
                        if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 2)) {
                            if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40))
                                s3->accel_start(8, 1, s3->accel.pix_trans[0] | (s3->accel.pix_trans[1] << 8) | (s3->accel.pix_trans[2] << 16) | (s3->accel.pix_trans[3] << 24), 0, s3);
                            else
                                s3->accel_start(1, 1, 0xffffffff, s3->accel.pix_trans[0] | (s3->accel.pix_trans[1] << 8) | (s3->accel.pix_trans[2] << 16) | (s3->accel.pix_trans[3] << 24), s3);
                        } else
                            s3->accel_start(1, 1, 0xffffffff, s3->accel.pix_trans[0] | (s3->accel.pix_trans[1] << 8) | (s3->accel.pix_trans[2] << 16) | (s3->accel.pix_trans[3] << 24), s3);
                        break;
                    case 0x200:
                        if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 2)) {
                            if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40)) {
                                if (s3->accel.cmd & 0x1000)
                                    s3->accel_start(16, 1, s3->accel.pix_trans[3] | (s3->accel.pix_trans[2] << 8) | (s3->accel.pix_trans[1] << 16) | (s3->accel.pix_trans[0] << 24), 0, s3);
                                else
                                    s3->accel_start(16, 1, s3->accel.pix_trans[0] | (s3->accel.pix_trans[1] << 8) | (s3->accel.pix_trans[2] << 16) | (s3->accel.pix_trans[3] << 24), 0, s3);
                            } else {
                                if (s3->accel.cmd & 0x1000)
                                    s3->accel_start(2, 1, 0xffffffff, s3->accel.pix_trans[3] | (s3->accel.pix_trans[2] << 8) | (s3->accel.pix_trans[1] << 16) | (s3->accel.pix_trans[0] << 24), s3);
                                else
                                    s3->accel_start(2, 1, 0xffffffff, s3->accel.pix_trans[0] | (s3->accel.pix_trans[1] << 8) | (s3->accel.pix_trans[2] << 16) | (s3->accel.pix_trans[3] << 24), s3);
                            }
                        } else {
                            if (s3->accel.cmd & 0x1000)
                                s3->accel_start(2, 1, 0xffffffff, s3->accel.pix_trans[3] | (s3->accel.pix_trans[2] << 8) | (s3->accel.pix_trans[1] << 16) | (s3->accel.pix_trans[0] << 24), s3);
                            else
                                s3->accel_start(2, 1, 0xffffffff, s3->accel.pix_trans[0] | (s3->accel.pix_trans[1] << 8) | (s3->accel.pix_trans[2] << 16) | (s3->accel.pix_trans[3] << 24), s3);
                        }
                        break;

                    default:
                        break;
                }
            }
            return s3->accel.pix_trans[3];

        case 0xff20:
        case 0xff21:
            temp = s3->serialport & ~(SERIAL_PORT_SCR | SERIAL_PORT_SDR);
            if ((s3->serialport & SERIAL_PORT_SCW) && i2c_gpio_get_scl(s3->i2c))
                temp |= SERIAL_PORT_SCR;
            if ((s3->serialport & SERIAL_PORT_SDW) && i2c_gpio_get_sda(s3->i2c))
                temp |= SERIAL_PORT_SDR;
            return temp;

        default:
            break;
    }

    return 0xff;
}

static uint16_t
s3_accel_in_w(uint16_t port, void *priv)
{
    s3_t           *s3     = (s3_t *) priv;
    svga_t         *svga   = &s3->svga;
    uint16_t        temp   = 0x0000;
    uint16_t        temp1  = 0x0000;
    uint16_t        temp2  = 0x0000;
    const uint16_t *vram_w = (uint16_t *) svga->vram;

    if (!s3->enable_8514)
        return 0xffff;

    if (port != 0x9ee8 && port != 0x9d48) {
        if (s3_cpu_dest(s3)) {
            READ_PIXTRANS_WORD

            switch (s3->accel.cmd & 0x600) {
                case 0x000:
                    if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 0x02)) {
                        if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40)) {
                            if (s3->accel.cmd & 0x1000)
                                temp = (temp >> 8) | (temp << 8);

                            s3->accel_start(8, 1, temp | (temp << 16), 0, s3);
                        } else {
                            if ((s3->bpp == 0) && s3->color_16bit) {
                                if (s3->accel.rd_mask_16bit_check) {
                                    if (s3->accel.cur_x & 0x400)
                                        temp = (temp >> 8) | (temp << 8);
                                }
                                s3->accel_start(2, 1, 0xffffffff, temp | (temp << 16), s3);
                            } else
                                s3->accel_start(1, 1, 0xffffffff, temp | (temp << 16), s3);
                        }
                    } else {
                        if ((s3->bpp == 0) && s3->color_16bit) {
                            if (s3->accel.rd_mask_16bit_check) {
                                if (s3->accel.cur_x & 0x400)
                                    temp = (temp >> 8) | (temp << 8);
                            }
                            s3->accel_start(2, 1, 0xffffffff, temp | (temp << 16), s3);
                        } else
                            s3->accel_start(1, 1, 0xffffffff, temp | (temp << 16), s3);
                    }
                    break;
                case 0x200:
                    if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 0x02)) {
                        if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40)) {
                            if (s3->accel.cmd & 0x1000)
                                temp = (temp >> 8) | (temp << 8);

                            s3->accel_start(16, 1, temp | (temp << 16), 0, s3);
                        } else
                            s3->accel_start(2, 1, 0xffffffff, temp | (temp << 16), s3);
                    } else {
                        if (s3->accel.rd_mask_16bit_check) {
                            if (s3->accel.cmd == 0x53b0) {
                                temp1 = vram_w[dword_remap_w(svga, s3->accel.dest + s3->accel.cx - s3->accel.minus) & (s3->vram_mask >> 1)];
                                temp2 = vram_w[dword_remap_w(svga, s3->accel.dest + s3->accel.cx - s3->accel.minus + 1) & (s3->vram_mask >> 1)];
                                if (s3->accel.cur_x & 0x400) {
                                    temp = temp1 >> 8;
                                    temp |= (temp2 >> 8) << 8;
                                } else {
                                    temp = temp1 & 0xff;
                                    temp |= ((temp2 & 0xff) << 8);
                                }
                                s3->accel_start(4, 1, 0xffffffff, temp | (temp << 16), s3);
                            } else
                                s3->accel_start(2, 1, 0xffffffff, temp | (temp << 16), s3);
                        } else
                            s3->accel_start(2, 1, 0xffffffff, temp | (temp << 16), s3);
                    }
                    break;

                default:
                    break;
            }
        }
    } else {
        if (s3_enable_fifo(s3))
            s3_wait_fifo_idle(s3);

        temp = s3->accel.short_stroke;
    }

    return temp;
}

static uint32_t
s3_accel_in_l(UNUSED(uint16_t port), void *priv)
{
    s3_t           *s3     = (s3_t *) priv;
    svga_t         *svga   = &s3->svga;
    uint32_t        temp   = 0x00000000;
    const uint16_t *vram_w = (uint16_t *) svga->vram;

    if (!s3->enable_8514)
        return 0xffffffff;

    if (s3_cpu_dest(s3)) {
        READ_PIXTRANS_LONG

        switch (s3->accel.cmd & 0x600) {
            case 0x000:
                if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 2)) {
                    if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40)) {
                        if (s3->accel.cmd & 0x1000)
                            temp = ((temp & 0xff00ff00) >> 8) | ((temp & 0x00ff00ff) << 8);
                        s3->accel_start(8, 1, temp, 0, s3);
                        s3->accel_start(8, 1, temp >> 16, 0, s3);
                    } else {
                        s3->accel_start(1, 1, 0xffffffff, temp, s3);
                        s3->accel_start(1, 1, 0xffffffff, temp >> 16, s3);
                    }
                } else {
                    s3->accel_start(1, 1, 0xffffffff, temp, s3);
                    s3->accel_start(1, 1, 0xffffffff, temp >> 16, s3);
                }
                break;
            case 0x200:
                if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 2)) {
                    if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40)) {
                        if (s3->accel.cmd & 0x1000)
                            temp = ((temp & 0xff00ff00) >> 8) | ((temp & 0x00ff00ff) << 8);
                        s3->accel_start(16, 1, temp, 0, s3);
                        s3->accel_start(16, 1, temp >> 16, 0, s3);
                    } else {
                        s3->accel_start(2, 1, 0xffffffff, temp, s3);
                        s3->accel_start(2, 1, 0xffffffff, temp >> 16, s3);
                    }
                } else {
                    s3->accel_start(2, 1, 0xffffffff, temp, s3);
                    s3->accel_start(2, 1, 0xffffffff, temp >> 16, s3);
                }
                break;

            default:
                break;
        }
    }

    return temp;
}

static void
s3_accel_write(uint32_t addr, uint8_t val, void *priv)
{
    s3_t         *s3   = (s3_t *) priv;
    const svga_t *svga = &s3->svga;
    uint32_t addr_mask = (svga->crtc[0x53] & 0x08) ? 0x1ffff : 0xffff;

    if (!s3->enable_8514)
        return;

    if (s3_enable_fifo(s3)) {
        if ((addr & addr_mask) < 0x8000) {
            s3_queue(s3, addr & addr_mask, val, FIFO_WRITE_BYTE);
        } else {
            switch (addr & addr_mask) {
                case 0x83b0 ... 0x83df:
                    s3_out(addr & 0x3ff, val, s3);
                    break;
                case 0x8504:
                    s3->subsys_stat &= ~val;
                    s3_update_irqs(s3);
                    break;
                case 0x8505:
                    s3->subsys_cntl = val;
                    s3_update_irqs(s3);
                    break;
                case 0x850c:
                    s3->accel.advfunc_cntl = val;
                    s3_updatemapping(s3);
                    break;
                case 0xff20:
                    s3->serialport = val;
                    i2c_gpio_set(s3->i2c, !!(val & SERIAL_PORT_SCW), !!(val & SERIAL_PORT_SDW));
                    break;
                default:
                    s3_queue(s3, addr & addr_mask, val, FIFO_WRITE_BYTE);
                    break;
            }
        }
    } else
        s3_accel_write_fifo(s3, addr & 0xffff, val);
}

static void
s3_accel_write_w(uint32_t addr, uint16_t val, void *priv)
{
    s3_t         *s3   = (s3_t *) priv;
    const svga_t *svga = &s3->svga;
    uint32_t addr_mask = (svga->crtc[0x53] & 0x08) ? 0x1ffff : 0xffff;

    if (!s3->enable_8514)
        return;

    if (s3_enable_fifo(s3)) {
        if ((addr & (addr_mask - 1)) < 0x8000) {
            s3_queue(s3, addr & addr_mask, val, FIFO_WRITE_WORD);
        } else {
            switch (addr & (addr_mask - 1)) {
                case 0x83d4:
                    s3_accel_write_fifo(s3, addr, val);
                    s3_accel_write_fifo(s3, addr + 1, val >> 8);
                    break;
                case 0xff20:
                    s3_accel_write_fifo(s3, addr, val);
                    break;
                default:
                    s3_queue(s3, addr & addr_mask, val, FIFO_WRITE_WORD);
                    break;
            }
        }
    } else
        s3_accel_write_fifo_w(s3, addr & 0xffff, val);
}

static void
s3_accel_write_l(uint32_t addr, uint32_t val, void *priv)
{
    s3_t         *s3   = (s3_t *) priv;
    svga_t       *svga = &s3->svga;
    uint32_t addr_mask = (svga->crtc[0x53] & 0x08) ? 0x1ffff : 0xffff;

    if (!s3->enable_8514)
        return;

    if (s3_enable_fifo(s3)) {
        if ((addr & (addr_mask - 3)) < 0x8000) {
            s3_queue(s3, addr & addr_mask, val, FIFO_WRITE_DWORD);
        } else {
            switch (addr & (addr_mask - 3)) {
                case 0x8180:
                    s3->streams.pri_ctrl = val;
                    svga_recalctimings(svga);
                    svga->fullchange = svga->monitor->mon_changeframecount;;
                    break;
                case 0x8184:
                    s3->streams.chroma_ctrl = val;
                    break;
                case 0x8190:
                    s3->streams.sec_ctrl = val;
                    s3->streams.dda_horiz_accumulator = val & 0xfff;
                    if (val & 0x1000)
                        s3->streams.dda_horiz_accumulator |= ~0xfff;

                    s3->streams.sdif = (val >> 24) & 7;
                    break;
                case 0x8194:
                    s3->streams.chroma_upper_bound = val;
                    break;
                case 0x8198:
                    s3->streams.sec_filter = val;
                    s3->streams.k1_horiz_scale = val & 0x7ff;
                    if (val & 0x800)
                        s3->streams.k1_horiz_scale |= ~0x7ff;

                    s3->streams.k2_horiz_scale = (val >> 16) & 0x7ff;
                    if ((val >> 16) & 0x800)
                        s3->streams.k2_horiz_scale |= ~0x7ff;

                    svga_recalctimings(svga);
                    svga->fullchange = svga->monitor->mon_changeframecount;;
                    break;
                case 0x81a0:
                    s3->streams.blend_ctrl = val;
                    svga_recalctimings(svga);
                    svga->fullchange = svga->monitor->mon_changeframecount;;
                    break;
                case 0x81c0:
                    s3->streams.pri_fb0 = val & 0x3fffff;
                    svga_recalctimings(svga);
                    svga->fullchange = svga->monitor->mon_changeframecount;;
                    break;
                case 0x81c4:
                    s3->streams.pri_fb1 = val & 0x3fffff;
                    svga_recalctimings(svga);
                    svga->fullchange = svga->monitor->mon_changeframecount;;
                    break;
                case 0x81c8:
                    s3->streams.pri_stride = val & 0xfff;
                    svga_recalctimings(svga);
                    svga->fullchange = svga->monitor->mon_changeframecount;;
                    break;
                case 0x81cc:
                    s3->streams.buffer_ctrl = val;
                    svga_recalctimings(svga);
                    svga->fullchange = svga->monitor->mon_changeframecount;;
                    break;
                case 0x81d0:
                    s3->streams.sec_fb0 = val;
                    svga_recalctimings(svga);
                    svga->fullchange = svga->monitor->mon_changeframecount;;
                    break;
                case 0x81d4:
                    s3->streams.sec_fb1 = val;
                    svga_recalctimings(svga);
                    svga->fullchange = svga->monitor->mon_changeframecount;;
                    break;
                case 0x81d8:
                    s3->streams.sec_stride = val;
                    svga_recalctimings(svga);
                    svga->fullchange = svga->monitor->mon_changeframecount;;
                    break;
                case 0x81dc:
                    s3->streams.overlay_ctrl = val;
                    break;
                case 0x81e0:
                    s3->streams.k1_vert_scale = val & 0x7ff;
                    if (val & 0x800)
                        s3->streams.k1_vert_scale |= ~0x7ff;
                    break;
                case 0x81e4:
                    s3->streams.k2_vert_scale = val & 0x7ff;
                    if (val & 0x800)
                        s3->streams.k2_vert_scale |= ~0x7ff;
                    break;
                case 0x81e8:
                    s3->streams.dda_vert_accumulator = val & 0xfff;
                    if (val & 0x1000)
                        s3->streams.dda_vert_accumulator |= ~0xfff;

                    svga_recalctimings(svga);
                    svga->fullchange = svga->monitor->mon_changeframecount;;
                    break;
                case 0x81ec:
                    s3->streams.fifo_ctrl = val;
                    break;
                case 0x81f0:
                    s3->streams.pri_start = val;
                    s3->streams.pri_x = (val >> 16) & 0x7ff;
                    s3->streams.pri_y = val & 0x7ff;
                    svga_recalctimings(svga);
                    svga->fullchange = svga->monitor->mon_changeframecount;;
                    break;
                case 0x81f4:
                    s3->streams.pri_size = val;
                    s3->streams.pri_w = (val >> 16) & 0x7ff;
                    s3->streams.pri_h = val & 0x7ff;
                    svga_recalctimings(svga);
                    svga->fullchange = svga->monitor->mon_changeframecount;;
                    break;
                case 0x81f8:
                    s3->streams.sec_start = val;
                    s3->streams.sec_x = (val >> 16) & 0x7ff;
                    s3->streams.sec_y = val & 0x7ff;
                    svga_recalctimings(svga);
                    svga->fullchange = svga->monitor->mon_changeframecount;;
                    break;
                case 0x81fc:
                    s3->streams.sec_size = val;
                    s3->streams.sec_w = (val >> 16) & 0x7ff;
                    s3->streams.sec_h = val & 0x7ff;
                    svga_recalctimings(svga);
                    svga->fullchange = svga->monitor->mon_changeframecount;;
                    break;

                case 0x8504:
                    s3->subsys_stat &= ~(val & 0xff);
                    s3->subsys_cntl = (val >> 8);
                    s3_update_irqs(s3);
                    break;

                case 0x850c:
                    s3->accel.advfunc_cntl = val & 0xff;
                    s3_updatemapping(s3);
                    break;

                case 0xff20:
                    s3_accel_write_fifo(s3, addr, val);
                    break;

                default:
                    s3_queue(s3, addr & addr_mask, val, FIFO_WRITE_DWORD);
                    break;
            }
        }
    } else
        s3_accel_write_fifo_l(s3, addr & 0xffff, val);
}

static uint8_t
s3_accel_read(uint32_t addr, void *priv)
{
    s3_t   *s3   = (s3_t *) priv;
    svga_t *svga = &s3->svga;
    uint8_t temp = 0x00;

    if (!s3->enable_8514)
        return 0xff;

    if (svga->crtc[0x53] & 0x08) {
        if ((addr >= 0x08000) && (addr <= 0x0803f))
            return s3_pci_read(0, addr & 0xff, s3);
        switch (addr & 0x1ffff) {
            case 0x83b0:
            case 0x83b1:
            case 0x83b2:
            case 0x83b3:
            case 0x83b4:
            case 0x83b5:
            case 0x83b6:
            case 0x83b7:
            case 0x83b8:
            case 0x83b9:
            case 0x83ba:
            case 0x83bb:
            case 0x83bc:
            case 0x83bd:
            case 0x83be:
            case 0x83bf:
            case 0x83c0:
            case 0x83c1:
            case 0x83c2:
            case 0x83c3:
            case 0x83c4:
            case 0x83c5:
            case 0x83c6:
            case 0x83c7:
            case 0x83c8:
            case 0x83c9:
            case 0x83ca:
            case 0x83cb:
            case 0x83cc:
            case 0x83cd:
            case 0x83ce:
            case 0x83cf:
            case 0x83d0:
            case 0x83d1:
            case 0x83d2:
            case 0x83d3:
            case 0x83d4:
            case 0x83d5:
            case 0x83d6:
            case 0x83d7:
            case 0x83d8:
            case 0x83d9:
            case 0x83da:
            case 0x83db:
            case 0x83dc:
            case 0x83dd:
            case 0x83de:
            case 0x83df:
                return s3_in(addr & 0x3ff, s3);
            case 0x8504:
                return s3->subsys_stat;
            case 0x8505:
                return s3->subsys_cntl;
            default:
                return s3_accel_in(addr & 0xffff, priv);
        }
        return 0xff;
    } else {
        if (addr & 0x8000) {
            temp = s3_accel_in(addr & 0xffff, priv);
        } else if (s3_cpu_dest(s3)) {
            READ_PIXTRANS_BYTE_MM

            switch (s3->accel.cmd & 0x600) {
                case 0x000:
                    if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 2)) {
                        if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40))
                            s3->accel_start(8, 1, temp | (temp << 8) | (temp << 16) | (temp << 24), 0, s3);
                        else
                            s3->accel_start(1, 1, 0xffffffff, temp | (temp << 8) | (temp << 16) | (temp << 24), s3);
                    } else
                        s3->accel_start(1, 1, 0xffffffff, temp | (temp << 8) | (temp << 16) | (temp << 24), s3);
                    break;
                case 0x200:
                    if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 2)) {
                        if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40))
                            s3->accel_start(16, 1, temp | (temp << 8) | (temp << 16) | (temp << 24), 0, s3);
                        else
                            s3->accel_start(2, 1, 0xffffffff, temp | (temp << 8) | (temp << 16) | (temp << 24), s3);
                    } else
                        s3->accel_start(2, 1, 0xffffffff, temp | (temp << 8) | (temp << 16) | (temp << 24), s3);
                    break;

                default:
                    break;
            }
        }
    }

    return temp;
}

static uint16_t
s3_accel_read_w(uint32_t addr, void *priv)
{
    s3_t           *s3     = (s3_t *) priv;
    svga_t         *svga   = &s3->svga;
    uint16_t        temp   = 0x0000;
    const uint16_t *vram_w = (uint16_t *) svga->vram;

    if (!s3->enable_8514)
        return 0xffff;

    if (svga->crtc[0x53] & 0x08) {
        switch (addr & 0x1fffe) {
            case 0x811c:
                if (s3_enable_fifo(s3))
                    s3_wait_fifo_idle(s3);
                return s3->accel.short_stroke;

            default:
                return s3_accel_read(addr, priv) | s3_accel_read(addr + 1, priv) << 8;
        }
        return 0xffff;
    } else {
        if (addr & 0x8000) {
            if (addr == 0x811c) {
                if (s3_enable_fifo(s3))
                    s3_wait_fifo_idle(s3);
                temp = s3->accel.short_stroke;
            } else {
                temp = s3_accel_read((addr & 0xfffe), priv);
                temp |= s3_accel_read((addr & 0xfffe) + 1, priv) << 8;
            }
        } else if (s3_cpu_dest(s3)) {
            READ_PIXTRANS_WORD

            switch (s3->accel.cmd & 0x600) {
                case 0x000:
                    if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 2)) {
                        if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40))
                            s3->accel_start(8, 1, temp | (temp << 16), 0, s3);
                        else
                            s3->accel_start(1, 1, 0xffffffff, temp | (temp << 16), s3);
                    } else
                        s3->accel_start(1, 1, 0xffffffff, temp | (temp << 16), s3);
                    break;
                case 0x200:
                    if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 2)) {
                        if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40))
                            s3->accel_start(16, 1, temp | (temp << 16), 0, s3);
                        else
                            s3->accel_start(2, 1, 0xffffffff, temp | (temp << 16), s3);
                    } else
                        s3->accel_start(2, 1, 0xffffffff, temp | (temp << 16), s3);
                    break;

                default:
                    break;
            }
        }
    }

    return temp;
}

static uint32_t
s3_accel_read_l(uint32_t addr, void *priv)
{
    s3_t           *s3     = (s3_t *) priv;
    svga_t         *svga   = &s3->svga;
    uint32_t        temp   = 0x00000000;
    const uint16_t *vram_w = (uint16_t *) svga->vram;

    if (!s3->enable_8514)
        return 0xffffffff;

    if (svga->crtc[0x53] & 0x08) {
        switch (addr & 0x1fffc) {
            case 0x8180:
                temp = s3->streams.pri_ctrl;
                break;
            case 0x8184:
                temp = s3->streams.chroma_ctrl;
                break;
            case 0x8190:
                temp = s3->streams.sec_ctrl;
                break;
            case 0x8194:
                temp = s3->streams.chroma_upper_bound;
                break;
            case 0x8198:
                temp = s3->streams.sec_filter;
                break;
            case 0x81a0:
                temp = s3->streams.blend_ctrl;
                break;
            case 0x81c0:
                temp = s3->streams.pri_fb0;
                break;
            case 0x81c4:
                temp = s3->streams.pri_fb1;
                break;
            case 0x81c8:
                temp = s3->streams.pri_stride;
                break;
            case 0x81cc:
                temp = s3->streams.buffer_ctrl;
                break;
            case 0x81d0:
                temp = s3->streams.sec_fb0;
                break;
            case 0x81d4:
                temp = s3->streams.sec_fb1;
                break;
            case 0x81d8:
                temp = s3->streams.sec_stride;
                break;
            case 0x81dc:
                temp = s3->streams.overlay_ctrl;
                break;
            case 0x81e0:
                temp = s3->streams.k1_vert_scale;
                break;
            case 0x81e4:
                temp = s3->streams.k2_vert_scale;
                break;
            case 0x81e8:
                temp = s3->streams.dda_vert_accumulator;
                break;
            case 0x81ec:
                temp = s3->streams.fifo_ctrl;
                break;
            case 0x81f0:
                temp = s3->streams.pri_start;
                break;
            case 0x81f4:
                temp = s3->streams.pri_size;
                break;
            case 0x81f8:
                temp = s3->streams.sec_start;
                break;
            case 0x81fc:
                temp = s3->streams.sec_size;
                break;

            case 0x18080:
                if (s3_enable_fifo(s3))
                    s3_wait_fifo_idle(s3);
                temp = 0;
                break;
            case 0x18088:
                if (s3_enable_fifo(s3))
                    s3_wait_fifo_idle(s3);
                temp = s3->videoengine.cntl;
                if (s3->bpp == 1) { /*The actual bpp is decided by the guest when idf is the same as odf*/
                    if (s3->videoengine.idf == 0 && s3->videoengine.odf == 0) {
                        if (svga->bpp == 15)
                            temp |= 0x600000;
                        else
                            temp |= 0x700000;
                    }
                } else if (s3->bpp > 1) {
                    if (s3->videoengine.idf == 0 && s3->videoengine.odf == 0)
                        temp |= 0x300000;
                }
                break;
            case 0x1808c:
                if (s3_enable_fifo(s3))
                    s3_wait_fifo_idle(s3);
                temp = s3->videoengine.stretch_filt_const;
                break;
            case 0x18090:
                if (s3_enable_fifo(s3))
                    s3_wait_fifo_idle(s3);
                temp = s3->videoengine.src_dst_step;
                break;
            case 0x18094:
                if (s3_enable_fifo(s3))
                    s3_wait_fifo_idle(s3);
                temp = s3->videoengine.crop;
                break;
            case 0x18098:
                if (s3_enable_fifo(s3))
                    s3_wait_fifo_idle(s3);
                temp = s3->videoengine.src_base;
                break;
            case 0x1809c:
                if (s3_enable_fifo(s3))
                    s3_wait_fifo_idle(s3);
                temp = s3->videoengine.dest_base;
                if (s3->videoengine.busy) {
                    temp |= (1 << 31);
                } else {
                    temp &= ~(1 << 31);
                }
                break;

            default:
                temp = s3_accel_read_w(addr, priv) | (s3_accel_read_w(addr + 2, priv) << 16);
                break;
        }
    } else {
        if (addr & 0x8000) {
            temp = s3_accel_read((addr & 0xfffc), priv);
            temp |= s3_accel_read((addr & 0xfffc) + 1, priv) << 8;
            temp |= s3_accel_read((addr & 0xfffc) + 2, priv) << 16;
            temp |= s3_accel_read((addr & 0xfffc) + 3, priv) << 24;
        } else if (s3_cpu_dest(s3)) {
            READ_PIXTRANS_LONG

            switch (s3->accel.cmd & 0x600) {
                case 0x000:
                    if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 2)) {
                        if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40)) {
                            s3->accel_start(8, 1, temp, 0, s3);
                            s3->accel_start(8, 1, temp >> 16, 0, s3);
                        } else {
                            s3->accel_start(1, 1, 0xffffffff, temp, s3);
                            s3->accel_start(1, 1, 0xffffffff, temp >> 16, s3);
                        }
                    } else {
                        s3->accel_start(1, 1, 0xffffffff, temp, s3);
                        s3->accel_start(1, 1, 0xffffffff, temp >> 16, s3);
                    }
                    break;
                case 0x200:
                    if (((s3->accel.multifunc[0xa] & 0xc0) == 0x80) || (s3->accel.cmd & 2)) {
                        if (((s3->accel.frgd_mix & 0x60) != 0x40) || ((s3->accel.bkgd_mix & 0x60) != 0x40)) {
                            s3->accel_start(16, 1, temp, 0, s3);
                            s3->accel_start(16, 1, temp >> 16, 0, s3);
                        } else {
                            s3->accel_start(2, 1, 0xffffffff, temp, s3);
                            s3->accel_start(2, 1, 0xffffffff, temp >> 16, s3);
                        }
                    } else {
                        s3->accel_start(2, 1, 0xffffffff, temp, s3);
                        s3->accel_start(2, 1, 0xffffffff, temp >> 16, s3);
                    }
                    break;

                default:
                    break;
            }
        }
    }

    return temp;
}

static void
polygon_setup(s3_t *s3)
{
    if (s3->accel.point_1_updated) {
        int start_x = s3->accel.poly_cx;
        int start_y = s3->accel.poly_cy;
        int end_x   = s3->accel.destx_distp << 20;
        int end_y   = s3->accel.desty_axstp;

        if (end_y - start_y)
            s3->accel.poly_dx1 = (end_x - start_x) / (end_y - start_y);
        else
            s3->accel.poly_dx1 = 0;

        s3->accel.point_1_updated = 0;

        if (end_y == s3->accel.poly_cy) {
            s3->accel.poly_cx = end_x;
            s3->accel.poly_x  = end_x >> 20;
        }
    }
    if (s3->accel.point_2_updated) {
        int start_x = s3->accel.poly_cx2;
        int start_y = s3->accel.poly_cy2;
        int end_x   = s3->accel.x2 << 20;
        int end_y   = s3->accel.desty_axstp2;

        if (end_y - start_y)
            s3->accel.poly_dx2 = (end_x - start_x) / (end_y - start_y);
        else
            s3->accel.poly_dx2 = 0;

        s3->accel.point_2_updated = 0;

        if (end_y == s3->accel.poly_cy)
            s3->accel.poly_cx2 = end_x;
    }
}

#define READ(addr, dat)                                                 \
    if (((s3->bpp == 0) && !s3->color_16bit) || (s3->bpp == 2))         \
        dat = svga->vram[dword_remap(svga, addr) & s3->vram_mask];      \
    else if ((s3->bpp == 1) || s3->color_16bit)                         \
        dat = vram_w[dword_remap_w(svga, addr) & (s3->vram_mask >> 1)]; \
    else                                                                \
        dat = vram_l[dword_remap_l(svga, addr) & (s3->vram_mask >> 2)];

#define MIX_READ                                                                                  \
    {                                                                                             \
        switch ((mix_dat & mix_mask) ? (s3->accel.frgd_mix & 0xf) : (s3->accel.bkgd_mix & 0xf)) { \
            case 0x0:                                                                             \
                dest_dat = ~dest_dat;                                                             \
                break;                                                                            \
            case 0x1:                                                                             \
                dest_dat = 0;                                                                     \
                break;                                                                            \
            case 0x2:                                                                             \
                dest_dat = ~0;                                                                    \
                break;                                                                            \
            case 0x3:                                                                             \
                dest_dat = dest_dat;                                                              \
                break;                                                                            \
            case 0x4:                                                                             \
                dest_dat = ~src_dat;                                                              \
                break;                                                                            \
            case 0x5:                                                                             \
                dest_dat = src_dat ^ dest_dat;                                                    \
                break;                                                                            \
            case 0x6:                                                                             \
                dest_dat = ~(src_dat ^ dest_dat);                                                 \
                break;                                                                            \
            case 0x7:                                                                             \
                dest_dat = src_dat;                                                               \
                break;                                                                            \
            case 0x8:                                                                             \
                dest_dat = ~(src_dat & dest_dat);                                                 \
                break;                                                                            \
            case 0x9:                                                                             \
                dest_dat = ~src_dat | dest_dat;                                                   \
                break;                                                                            \
            case 0xa:                                                                             \
                dest_dat = src_dat | ~dest_dat;                                                   \
                break;                                                                            \
            case 0xb:                                                                             \
                dest_dat = src_dat | dest_dat;                                                    \
                break;                                                                            \
            case 0xc:                                                                             \
                dest_dat = src_dat & dest_dat;                                                    \
                break;                                                                            \
            case 0xd:                                                                             \
                dest_dat = src_dat & ~dest_dat;                                                   \
                break;                                                                            \
            case 0xe:                                                                             \
                dest_dat = ~src_dat & dest_dat;                                                   \
                break;                                                                            \
            case 0xf:                                                                             \
                dest_dat = ~(src_dat | dest_dat);                                                 \
                break;                                                                            \
        }                                                                                         \
    }

#define MIX                                                            \
    {                                                                  \
        MIX_READ                                                       \
        dest_dat = (dest_dat & wrt_mask) | (old_dest_dat & ~wrt_mask); \
    }

#define ROPMIX_READ(D, P, S)                       \
    {                                              \
        switch (rop) {                             \
            case 0x00:                             \
                out = 0;                           \
                break;                             \
            case 0x01:                             \
                out = ~(D | (P | S));              \
                break;                             \
            case 0x02:                             \
                out = D & ~(P | S);                \
                break;                             \
            case 0x03:                             \
                out = ~(P | S);                    \
                break;                             \
            case 0x04:                             \
                out = S & ~(D | P);                \
                break;                             \
            case 0x05:                             \
                out = ~(D | P);                    \
                break;                             \
            case 0x06:                             \
                out = ~(P | ~(D ^ S));             \
                break;                             \
            case 0x07:                             \
                out = ~(P | (D & S));              \
                break;                             \
            case 0x08:                             \
                out = S & (D & ~P);                \
                break;                             \
            case 0x09:                             \
                out = ~(P | (D ^ S));              \
                break;                             \
            case 0x0a:                             \
                out = D & ~P;                      \
                break;                             \
            case 0x0b:                             \
                out = ~(P | (S & ~D));             \
                break;                             \
            case 0x0c:                             \
                out = S & ~P;                      \
                break;                             \
            case 0x0d:                             \
                out = ~(P | (D & ~S));             \
                break;                             \
            case 0x0e:                             \
                out = ~(P | ~(D | S));             \
                break;                             \
            case 0x0f:                             \
                out = ~P;                          \
                break;                             \
            case 0x10:                             \
                out = P & ~(D | S);                \
                break;                             \
            case 0x11:                             \
                out = ~(D | S);                    \
                break;                             \
            case 0x12:                             \
                out = ~(S | ~(D ^ P));             \
                break;                             \
            case 0x13:                             \
                out = ~(S | (D & P));              \
                break;                             \
            case 0x14:                             \
                out = ~(D | ~(P ^ S));             \
                break;                             \
            case 0x15:                             \
                out = ~(D | (P & S));              \
                break;                             \
            case 0x16:                             \
                out = P ^ (S ^ (D & ~(P & S)));    \
                break;                             \
            case 0x17:                             \
                out = ~(S ^ ((S ^ P) & (D ^ S)));  \
                break;                             \
            case 0x18:                             \
                out = (S ^ P) & (P ^ D);           \
                break;                             \
            case 0x19:                             \
                out = ~(S ^ (D & ~(P & S)));       \
                break;                             \
            case 0x1a:                             \
                out = P ^ (D | (S & P));           \
                break;                             \
            case 0x1b:                             \
                out = ~(S ^ (D & (P ^ S)));        \
                break;                             \
            case 0x1c:                             \
                out = P ^ (S | (D & P));           \
                break;                             \
            case 0x1d:                             \
                out = ~(D ^ (S & (P ^ D)));        \
                break;                             \
            case 0x1e:                             \
                out = P ^ (D | S);                 \
                break;                             \
            case 0x1f:                             \
                out = ~(P & (D | S));              \
                break;                             \
            case 0x20:                             \
                out = D & (P & ~S);                \
                break;                             \
            case 0x21:                             \
                out = ~(S | (D ^ P));              \
                break;                             \
            case 0x22:                             \
                out = D & ~S;                      \
                break;                             \
            case 0x23:                             \
                out = ~(S | (P & ~D));             \
                break;                             \
            case 0x24:                             \
                out = (S ^ P) & (D ^ S);           \
                break;                             \
            case 0x25:                             \
                out = ~(P ^ (D & ~(S & P)));       \
                break;                             \
            case 0x26:                             \
                out = S ^ (D | (P & S));           \
                break;                             \
            case 0x27:                             \
                out = S ^ (D | ~(P ^ S));          \
                break;                             \
            case 0x28:                             \
                out = D & (P ^ S);                 \
                break;                             \
            case 0x29:                             \
                out = ~(P ^ (S ^ (D | (P & S))));  \
                break;                             \
            case 0x2a:                             \
                out = D & ~(P & S);                \
                break;                             \
            case 0x2b:                             \
                out = ~(S ^ ((S ^ P) & (P ^ D)));  \
                break;                             \
            case 0x2c:                             \
                out = S ^ (P & (D | S));           \
                break;                             \
            case 0x2d:                             \
                out = P ^ (S | ~D);                \
                break;                             \
            case 0x2e:                             \
                out = P ^ (S | (D ^ P));           \
                break;                             \
            case 0x2f:                             \
                out = ~(P & (S | ~D));             \
                break;                             \
            case 0x30:                             \
                out = P & ~S;                      \
                break;                             \
            case 0x31:                             \
                out = ~(S | (D & ~P));             \
                break;                             \
            case 0x32:                             \
                out = S ^ (D | (P | S));           \
                break;                             \
            case 0x33:                             \
                out = ~S;                          \
                break;                             \
            case 0x34:                             \
                out = S ^ (P | (D & S));           \
                break;                             \
            case 0x35:                             \
                out = S ^ (P | ~(D ^ S));          \
                break;                             \
            case 0x36:                             \
                out = S ^ (D | P);                 \
                break;                             \
            case 0x37:                             \
                out = ~(S & (D | P));              \
                break;                             \
            case 0x38:                             \
                out = P ^ (S & (D | P));           \
                break;                             \
            case 0x39:                             \
                out = S ^ (P | ~D);                \
                break;                             \
            case 0x3a:                             \
                out = S ^ (P | (D ^ S));           \
                break;                             \
            case 0x3b:                             \
                out = ~(S & (P | ~D));             \
                break;                             \
            case 0x3c:                             \
                out = P ^ S;                       \
                break;                             \
            case 0x3d:                             \
                out = S ^ (P | ~(D | S));          \
                break;                             \
            case 0x3e:                             \
                out = S ^ (P | (D & ~S));          \
                break;                             \
            case 0x3f:                             \
                out = ~(P & S);                    \
                break;                             \
            case 0x40:                             \
                out = P & (S & ~D);                \
                break;                             \
            case 0x41:                             \
                out = ~(D | (P ^ S));              \
                break;                             \
            case 0x42:                             \
                out = (S ^ D) & (P ^ D);           \
                break;                             \
            case 0x43:                             \
                out = ~(S ^ (P & ~(D & S)));       \
                break;                             \
            case 0x44:                             \
                out = S & ~D;                      \
                break;                             \
            case 0x45:                             \
                out = ~(D | (P & ~S));             \
                break;                             \
            case 0x46:                             \
                out = D ^ (S | (P & D));           \
                break;                             \
            case 0x47:                             \
                out = ~(P ^ (S & (D ^ P)));        \
                break;                             \
            case 0x48:                             \
                out = S & (D ^ P);                 \
                break;                             \
            case 0x49:                             \
                out = ~(P ^ (D ^ (S | (P & D))));  \
                break;                             \
            case 0x4a:                             \
                out = D ^ (P & (S | D));           \
                break;                             \
            case 0x4b:                             \
                out = P ^ (D | ~S);                \
                break;                             \
            case 0x4c:                             \
                out = S & ~(D & P);                \
                break;                             \
            case 0x4d:                             \
                out = ~(S ^ ((S ^ P) | (D ^ S)));  \
                break;                             \
            case 0x4e:                             \
                out = P ^ (D | (S ^ P));           \
                break;                             \
            case 0x4f:                             \
                out = ~(P & (D | ~S));             \
                break;                             \
            case 0x50:                             \
                out = P & ~D;                      \
                break;                             \
            case 0x51:                             \
                out = ~(D | (S & ~P));             \
                break;                             \
            case 0x52:                             \
                out = D ^ (P | (S & D));           \
                break;                             \
            case 0x53:                             \
                out = ~(S ^ (P & (D ^ S)));        \
                break;                             \
            case 0x54:                             \
                out = ~(D | ~(P | S));             \
                break;                             \
            case 0x55:                             \
                out = ~D;                          \
                break;                             \
            case 0x56:                             \
                out = D ^ (P | S);                 \
                break;                             \
            case 0x57:                             \
                out = ~(D & (P | S));              \
                break;                             \
            case 0x58:                             \
                out = P ^ (D & (S | P));           \
                break;                             \
            case 0x59:                             \
                out = D ^ (P | ~S);                \
                break;                             \
            case 0x5a:                             \
                out = D ^ P;                       \
                break;                             \
            case 0x5b:                             \
                out = D ^ (P | ~(S | D));          \
                break;                             \
            case 0x5c:                             \
                out = D ^ (P | (S ^ D));           \
                break;                             \
            case 0x5d:                             \
                out = ~(D & (P | ~S));             \
                break;                             \
            case 0x5e:                             \
                out = D ^ (P | (S & ~D));          \
                break;                             \
            case 0x5f:                             \
                out = ~(D & P);                    \
                break;                             \
            case 0x60:                             \
                out = P & (D ^ S);                 \
                break;                             \
            case 0x61:                             \
                out = ~(D ^ (S ^ (P | (D & S))));  \
                break;                             \
            case 0x62:                             \
                out = D ^ (S & (P | D));           \
                break;                             \
            case 0x63:                             \
                out = S ^ (D | ~P);                \
                break;                             \
            case 0x64:                             \
                out = S ^ (D & (P | S));           \
                break;                             \
            case 0x65:                             \
                out = D ^ (S | ~P);                \
                break;                             \
            case 0x66:                             \
                out = D ^ S;                       \
                break;                             \
            case 0x67:                             \
                out = S ^ (D | ~(P | S));          \
                break;                             \
            case 0x68:                             \
                out = ~(D ^ (S ^ (P | ~(D | S)))); \
                break;                             \
            case 0x69:                             \
                out = ~(P ^ (D ^ S));              \
                break;                             \
            case 0x6a:                             \
                out = D ^ (P & S);                 \
                break;                             \
            case 0x6b:                             \
                out = ~(P ^ (S ^ (D & (P | S))));  \
                break;                             \
            case 0x6c:                             \
                out = S ^ (D & P);                 \
                break;                             \
            case 0x6d:                             \
                out = ~(P ^ (D ^ (S & (P | D))));  \
                break;                             \
            case 0x6e:                             \
                out = S ^ (D & (P | ~S));          \
                break;                             \
            case 0x6f:                             \
                out = ~(P & ~(D ^ S));             \
                break;                             \
            case 0x70:                             \
                out = P & ~(D & S);                \
                break;                             \
            case 0x71:                             \
                out = ~(S ^ ((S ^ D) & (P ^ D)));  \
                break;                             \
            case 0x72:                             \
                out = S ^ (D | (P ^ S));           \
                break;                             \
            case 0x73:                             \
                out = ~(S & (D | ~P));             \
                break;                             \
            case 0x74:                             \
                out = D ^ (S | (P ^ D));           \
                break;                             \
            case 0x75:                             \
                out = ~(D & (S | ~P));             \
                break;                             \
            case 0x76:                             \
                out = S ^ (D | (P & ~S));          \
                break;                             \
            case 0x77:                             \
                out = ~(D & S);                    \
                break;                             \
            case 0x78:                             \
                out = P ^ (D & S);                 \
                break;                             \
            case 0x79:                             \
                out = ~(D ^ (S ^ (P & (D | S))));  \
                break;                             \
            case 0x7a:                             \
                out = D ^ (P & (S | ~D));          \
                break;                             \
            case 0x7b:                             \
                out = ~(S & ~(D ^ P));             \
                break;                             \
            case 0x7c:                             \
                out = S ^ (P & (D | ~S));          \
                break;                             \
            case 0x7d:                             \
                out = ~(D & ~(P ^ S));             \
                break;                             \
            case 0x7e:                             \
                out = (S ^ P) | (D ^ S);           \
                break;                             \
            case 0x7f:                             \
                out = ~(D & (P & S));              \
                break;                             \
            case 0x80:                             \
                out = D & (P & S);                 \
                break;                             \
            case 0x81:                             \
                out = ~((S ^ P) | (D ^ S));        \
                break;                             \
            case 0x82:                             \
                out = D & ~(P ^ S);                \
                break;                             \
            case 0x83:                             \
                out = ~(S ^ (P & (D | ~S)));       \
                break;                             \
            case 0x84:                             \
                out = S & ~(D ^ P);                \
                break;                             \
            case 0x85:                             \
                out = ~(P ^ (D & (S | ~P)));       \
                break;                             \
            case 0x86:                             \
                out = D ^ (S ^ (P & (D | S)));     \
                break;                             \
            case 0x87:                             \
                out = ~(P ^ (D & S));              \
                break;                             \
            case 0x88:                             \
                out = D & S;                       \
                break;                             \
            case 0x89:                             \
                out = ~(S ^ (D | (P & ~S)));       \
                break;                             \
            case 0x8a:                             \
                out = D & (S | ~P);                \
                break;                             \
            case 0x8b:                             \
                out = ~(D ^ (S | (P ^ D)));        \
                break;                             \
            case 0x8c:                             \
                out = S & (D | ~P);                \
                break;                             \
            case 0x8d:                             \
                out = ~(S ^ (D | (P ^ S)));        \
                break;                             \
            case 0x8e:                             \
                out = S ^ ((S ^ D) & (P ^ D));     \
                break;                             \
            case 0x8f:                             \
                out = ~(P & ~(D & S));             \
                break;                             \
            case 0x90:                             \
                out = P & ~(D ^ S);                \
                break;                             \
            case 0x91:                             \
                out = ~(S ^ (D & (P | ~S)));       \
                break;                             \
            case 0x92:                             \
                out = D ^ (P ^ (S & (D | P)));     \
                break;                             \
            case 0x93:                             \
                out = ~(S ^ (P & D));              \
                break;                             \
            case 0x94:                             \
                out = P ^ (S ^ (D & (P | S)));     \
                break;                             \
            case 0x95:                             \
                out = ~(D ^ (P & S));              \
                break;                             \
            case 0x96:                             \
                out = D ^ (P ^ S);                 \
                break;                             \
            case 0x97:                             \
                out = P ^ (S ^ (D | ~(P | S)));    \
                break;                             \
            case 0x98:                             \
                out = ~(S ^ (D | ~(P | S)));       \
                break;                             \
            case 0x99:                             \
                out = ~(D ^ S);                    \
                break;                             \
            case 0x9a:                             \
                out = D ^ (P & ~S);                \
                break;                             \
            case 0x9b:                             \
                out = ~(S ^ (D & (P | S)));        \
                break;                             \
            case 0x9c:                             \
                out = S ^ (P & ~D);                \
                break;                             \
            case 0x9d:                             \
                out = ~(D ^ (S & (P | D)));        \
                break;                             \
            case 0x9e:                             \
                out = D ^ (S ^ (P | (D & S)));     \
                break;                             \
            case 0x9f:                             \
                out = ~(P & (D ^ S));              \
                break;                             \
            case 0xa0:                             \
                out = D & P;                       \
                break;                             \
            case 0xa1:                             \
                out = ~(P ^ (D | (S & ~P)));       \
                break;                             \
            case 0xa2:                             \
                out = D & (P | ~S);                \
                break;                             \
            case 0xa3:                             \
                out = ~(D ^ (P | (S ^ D)));        \
                break;                             \
            case 0xa4:                             \
                out = ~(P ^ (D | ~(S | P)));       \
                break;                             \
            case 0xa5:                             \
                out = ~(P ^ D);                    \
                break;                             \
            case 0xa6:                             \
                out = D ^ (S & ~P);                \
                break;                             \
            case 0xa7:                             \
                out = ~(P ^ (D & (S | P)));        \
                break;                             \
            case 0xa8:                             \
                out = D & (P | S);                 \
                break;                             \
            case 0xa9:                             \
                out = ~(D ^ (P | S));              \
                break;                             \
            case 0xaa:                             \
                out = D;                           \
                break;                             \
            case 0xab:                             \
                out = D | ~(P | S);                \
                break;                             \
            case 0xac:                             \
                out = S ^ (P & (D ^ S));           \
                break;                             \
            case 0xad:                             \
                out = ~(D ^ (P | (S & D)));        \
                break;                             \
            case 0xae:                             \
                out = D | (S & ~P);                \
                break;                             \
            case 0xaf:                             \
                out = D | ~P;                      \
                break;                             \
            case 0xb0:                             \
                out = P & (D | ~S);                \
                break;                             \
            case 0xb1:                             \
                out = ~(P ^ (D | (S ^ P)));        \
                break;                             \
            case 0xb2:                             \
                out = S ^ ((S ^ P) | (D ^ S));     \
                break;                             \
            case 0xb3:                             \
                out = ~(S & ~(D & P));             \
                break;                             \
            case 0xb4:                             \
                out = P ^ (S & ~D);                \
                break;                             \
            case 0xb5:                             \
                out = ~(D ^ (P & (S | D)));        \
                break;                             \
            case 0xb6:                             \
                out = D ^ (P ^ (S | (D & P)));     \
                break;                             \
            case 0xb7:                             \
                out = ~(S & (D ^ P));              \
                break;                             \
            case 0xb8:                             \
                out = P ^ (S & (D ^ P));           \
                break;                             \
            case 0xb9:                             \
                out = ~(D ^ (S | (P & D)));        \
                break;                             \
            case 0xba:                             \
                out = D | (P & ~S);                \
                break;                             \
            case 0xbb:                             \
                out = D | ~S;                      \
                break;                             \
            case 0xbc:                             \
                out = S ^ (P & ~(D & S));          \
                break;                             \
            case 0xbd:                             \
                out = ~((S ^ D) & (P ^ D));        \
                break;                             \
            case 0xbe:                             \
                out = D | (P ^ S);                 \
                break;                             \
            case 0xbf:                             \
                out = D | ~(P & S);                \
                break;                             \
            case 0xc0:                             \
                out = P & S;                       \
                break;                             \
            case 0xc1:                             \
                out = ~(S ^ (P | (D & ~S)));       \
                break;                             \
            case 0xc2:                             \
                out = ~(S ^ (P | ~(D | S)));       \
                break;                             \
            case 0xc3:                             \
                out = ~(P ^ S);                    \
                break;                             \
            case 0xc4:                             \
                out = S & (P | ~D);                \
                break;                             \
            case 0xc5:                             \
                out = ~(S ^ (P | (D ^ S)));        \
                break;                             \
            case 0xc6:                             \
                out = S ^ (D & ~P);                \
                break;                             \
            case 0xc7:                             \
                out = ~(P ^ (S & (D | P)));        \
                break;                             \
            case 0xc8:                             \
                out = S & (D | P);                 \
                break;                             \
            case 0xc9:                             \
                out = ~(S ^ (P | D));              \
                break;                             \
            case 0xca:                             \
                out = D ^ (P & (S ^ D));           \
                break;                             \
            case 0xcb:                             \
                out = ~(S ^ (P | (D & S)));        \
                break;                             \
            case 0xcc:                             \
                out = S;                           \
                break;                             \
            case 0xcd:                             \
                out = S | ~(D | P);                \
                break;                             \
            case 0xce:                             \
                out = S | (D & ~P);                \
                break;                             \
            case 0xcf:                             \
                out = S | ~P;                      \
                break;                             \
            case 0xd0:                             \
                out = P & (S | ~D);                \
                break;                             \
            case 0xd1:                             \
                out = ~(P ^ (S | (D ^ P)));        \
                break;                             \
            case 0xd2:                             \
                out = P ^ (D & ~S);                \
                break;                             \
            case 0xd3:                             \
                out = ~(S ^ (P & (D | S)));        \
                break;                             \
            case 0xd4:                             \
                out = S ^ ((S ^ P) & (P ^ D));     \
                break;                             \
            case 0xd5:                             \
                out = ~(D & ~(P & S));             \
                break;                             \
            case 0xd6:                             \
                out = P ^ (S ^ (D | (P & S)));     \
                break;                             \
            case 0xd7:                             \
                out = ~(D & (P ^ S));              \
                break;                             \
            case 0xd8:                             \
                out = P ^ (D & (S ^ P));           \
                break;                             \
            case 0xd9:                             \
                out = ~(S ^ (D | (P & S)));        \
                break;                             \
            case 0xda:                             \
                out = D ^ (P & ~(S & D));          \
                break;                             \
            case 0xdb:                             \
                out = ~((S ^ P) & (D ^ S));        \
                break;                             \
            case 0xdc:                             \
                out = S | (P & ~D);                \
                break;                             \
            case 0xdd:                             \
                out = S | ~D;                      \
                break;                             \
            case 0xde:                             \
                out = S | (D ^ P);                 \
                break;                             \
            case 0xdf:                             \
                out = S | ~(D & P);                \
                break;                             \
            case 0xe0:                             \
                out = P & (D | S);                 \
                break;                             \
            case 0xe1:                             \
                out = ~(P ^ (D | S));              \
                break;                             \
            case 0xe2:                             \
                out = D ^ (S & (P ^ D));           \
                break;                             \
            case 0xe3:                             \
                out = ~(P ^ (S | (D & P)));        \
                break;                             \
            case 0xe4:                             \
                out = S ^ (D & (P ^ S));           \
                break;                             \
            case 0xe5:                             \
                out = ~(P ^ (D | (S & P)));        \
                break;                             \
            case 0xe6:                             \
                out = S ^ (D & ~(P & S));          \
                break;                             \
            case 0xe7:                             \
                out = ~((S ^ P) & (P ^ D));        \
                break;                             \
            case 0xe8:                             \
                out = S ^ ((S ^ P) & (D ^ S));     \
                break;                             \
            case 0xe9:                             \
                out = ~(D ^ (S ^ (P & ~(D & S)))); \
                break;                             \
            case 0xea:                             \
                out = D | (P & S);                 \
                break;                             \
            case 0xeb:                             \
                out = D | ~(P ^ S);                \
                break;                             \
            case 0xec:                             \
                out = S | (D & P);                 \
                break;                             \
            case 0xed:                             \
                out = S | ~(D ^ P);                \
                break;                             \
            case 0xee:                             \
                out = D | S;                       \
                break;                             \
            case 0xef:                             \
                out = S | (D | ~P);                \
                break;                             \
            case 0xf0:                             \
                out = P;                           \
                break;                             \
            case 0xf1:                             \
                out = P | ~(D | S);                \
                break;                             \
            case 0xf2:                             \
                out = P | (D & ~S);                \
                break;                             \
            case 0xf3:                             \
                out = P | ~S;                      \
                break;                             \
            case 0xf4:                             \
                out = P | (S & ~D);                \
                break;                             \
            case 0xf5:                             \
                out = P | ~D;                      \
                break;                             \
            case 0xf6:                             \
                out = P | (D ^ S);                 \
                break;                             \
            case 0xf7:                             \
                out = P | ~(D & S);                \
                break;                             \
            case 0xf8:                             \
                out = P | (D & S);                 \
                break;                             \
            case 0xf9:                             \
                out = P | ~(D ^ S);                \
                break;                             \
            case 0xfa:                             \
                out = D | P;                       \
                break;                             \
            case 0xfb:                             \
                out = D | (P | ~S);                \
                break;                             \
            case 0xfc:                             \
                out = P | S;                       \
                break;                             \
            case 0xfd:                             \
                out = P | (S | ~D);                \
                break;                             \
            case 0xfe:                             \
                out = D | (P | S);                 \
                break;                             \
            case 0xff:                             \
                out = ~0;                          \
                break;                             \
        }                                          \
    }

#define ROPMIX                                                                   \
    {                                                                            \
        old_dest_dat = dest_dat;                                                 \
        ROPMIX_READ(dest_dat, pat_dat, src_dat);                                 \
        out = (out & s3->accel.wrt_mask) | (old_dest_dat & ~s3->accel.wrt_mask); \
    }

#define WRITE(addr, dat)                                                                                                   \
    if (((s3->bpp == 0) && !s3->color_16bit) || (s3->bpp == 2)) {                                                          \
        svga->vram[dword_remap(svga, addr) & s3->vram_mask] = dat;                                                         \
        svga->changedvram[(dword_remap(svga, addr) & s3->vram_mask) >> 12] = svga->monitor->mon_changeframecount;          \
    } else if ((s3->bpp == 1) || s3->color_16bit) {                                                                        \
        vram_w[dword_remap_w(svga, addr) & (s3->vram_mask >> 1)]                    = dat;                                 \
        svga->changedvram[(dword_remap_w(svga, addr) & (s3->vram_mask >> 1)) >> 11] = svga->monitor->mon_changeframecount; \
    } else {                                                                                                               \
        vram_l[dword_remap_l(svga, addr) & (s3->vram_mask >> 2)] = dat;                                                    \
        svga->changedvram[(dword_remap_l(svga, addr) & (s3->vram_mask >> 2)) >> 10] = svga->monitor->mon_changeframecount; \
    }

static __inline void
convert_to_rgb32(int idf, int is_yuv, uint32_t val, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *r2, uint8_t *g2, uint8_t *b2)
{
    static double dr = 0.0;
    static double dg = 0.0;
    static double db = 0.0;
    static double dY1 = 0.0;
    static double dCr = 0.0;
    static double dY2 = 0.0;
    static double dCb = 0.0;
    static double dU = 0.0;
    static double dV = 0.0;

    switch (idf) {
        case 0: /* 8 bpp, RGB 3-3-2 */
            dr = (double) ((val >> 5) & 0x07);
            dg = (double) ((val >> 2) & 0x07);
            db = (double) (val & 0x03);
            dr = (dr / 7.0) * 255.0;
            dg = (dg / 7.0) * 255.0;
            db = (db / 3.0) * 255.0;
            break;
        case 3: /* 32bpp, RGB 8-8-8 */
            dr = (double) ((val >> 16) & 0xff);
            dg = (double) ((val >> 8) & 0xff);
            db = (double) (val & 0xff);
            break;
        case 4: /* YCbCr */
            if (is_yuv) {
                dU  = ((double) (val & 0xff)) - 128.0;
                dY1 = (double) ((val >> 8) & 0xff);
                dY1 = (298.0 * (dY1 - 16.0)) / 256.0;
                dV  = ((double) ((val >> 16) & 0xff)) - 128.0;
                dY2 = (double) ((val >> 24) & 0xff);
                dY2 = (298.0 * (dY2 - 16.0)) / 256.0;

                dr = (309.0 * dV) / 256.0;
                dg = ((100.0 * dU) + (208.0 * dV)) / 256.0;
                db = (516.0 * dU) / 256.0;
            } else {
                dY1 = (double) (val & 0xff);
                dCr = ((double) ((val >> 8) & 0xff)) - 128.0;
                dY2 = (double) ((val >> 16) & 0xff);
                dCb = ((double) ((val >> 24) & 0xff)) - 128.0;

                dr = (359.0 * dCr) / 256.0;
                dg = ((88.0 * dCb) + (183.0 * dCr)) / 2560.0;
                db = (453.0 * dCr) / 256.0;
            }

            *r = (uint8_t) round(dY1 + dr);
            CLAMP(*r);
            *g = (uint8_t) round(dY1 - dg);
            CLAMP(*g);
            *b = (uint8_t) round(dY1 + db);
            CLAMP(*b);

            *r2 = (uint8_t) round(dY2 + dr);
            CLAMP(*r2);
            *g2 = (uint8_t) round(dY2 - dg);
            CLAMP(*g2);
            *b2 = (uint8_t) round(dY2 + db);
            CLAMP(*b2);
            return;
        case 5: /* 16bpp, raw */
        case 7: /* 16bpp, RGB 5-6-5 */
            dr = (double) ((val >> 11) & 0x1f);
            dg = (double) ((val >> 5) & 0x03f);
            db = (double) (val & 0x1f);
            dr = (dr / 31.0) * 255.0;
            dg = (dg / 63.0) * 255.0;
            db = (db / 31.0) * 255.0;
            break;
        case 6: /* 15bpp, RGB 5-5-5 */
            dr = (double) ((val >> 10) & 0x1f);
            dg = (double) ((val >> 5) & 0x01f);
            db = (double) (val & 0x1f);
            dr = (dr / 31.0) * 255.0;
            dg = (dg / 31.0) * 255.0;
            db = (db / 31.0) * 255.0;
            break;

        default:
            break;
    }

    *r = (uint8_t) round(dr);
    *g = (uint8_t) round(dg);
    *b = (uint8_t) round(db);
}

static __inline void
convert_from_rgb32(int idf, int odf, int is_yuv, uint32_t *val, uint8_t r, uint8_t g, uint8_t b, uint8_t r2, uint8_t g2, uint8_t b2)
{
    static double dr = 0.0;
    static double dg = 0.0;
    static double db = 0.0;
    static double dr2 = 0.0;
    static double dg2 = 0.0;
    static double db2 = 0.0;
    static double dY1 = 0.0;
    static double dCr = 0.0;
    static double dY2 = 0.0;
    static double dCb = 0.0;
    static double dU = 0.0;
    static double dV = 0.0;

    dr = (double) r;
    dg = (double) g;
    db = (double) b;

    switch (odf) {
        case 0: /* 8 bpp, RGB 3-3-2 */
            switch (idf) {
                case 3:
                    *val = (((uint32_t) round(dr)) << 16) + (((uint32_t) round(dg)) << 8) + ((uint32_t) round(db));
                    break;
                case 5:
                case 7:
                    dr   = (dr / 255.0) * 31.0;
                    dg   = (dg / 255.0) * 63.0;
                    db   = (db / 255.0) * 31.0;
                    *val = (((uint32_t) round(dr)) << 11) + (((uint32_t) round(dg)) << 5) + ((uint32_t) round(db));
                    break;
                case 6:
                    dr   = (dr / 255.0) * 31.0;
                    dg   = (dg / 255.0) * 31.0;
                    db   = (db / 255.0) * 31.0;
                    *val = (((uint32_t) round(dr)) << 10) + (((uint32_t) round(dg)) << 5) + ((uint32_t) round(db));
                    break;
                case 0:
                default:
                    dr   = (dr / 255.0) * 7.0;
                    dg   = (dg / 255.0) * 7.0;
                    db   = (db / 255.0) * 3.0;
                    *val = (((uint32_t) round(dr)) << 5) + (((uint32_t) round(dg)) << 2) + ((uint32_t) round(db));
                    break;
            }
            break;
        case 3: /* 32bpp, RGB 8-8-8 */
            *val = (((uint32_t) round(dr)) << 16) + (((uint32_t) round(dg)) << 8) + ((uint32_t) round(db));
            break;
        case 4: /* YCbCr */
            dr2 = (double) r2;
            dg2 = (double) g2;
            db2 = (double) b2;

            if (is_yuv) {
                dU  = ((113046.0 * dg2) - (71552.0 * dr2) - (69488.0 * db2)) / 28509.0;
                dV  = ((3328.0 * dr2) + (800.0 * db2) - (4128.0 * dg2)) / 663.0;
                dY1 = dr - ((309 * dV) / 256.0);
                dY2 = dr2 - ((309 * dV) / 256.0);

                *val = ((uint32_t) round(dU)) + (((uint32_t) round(dY1)) << 8) + (((uint32_t) round(dV)) << 16) + (((uint32_t) round(dY2)) << 24);
            } else {
                dCr = ((128.0 * db2) - (128.0 * dr2)) / 47.0;
                dCb = ((128.0 * dr2) - (128.0 * dg2) - (271.0 * dCr)) / 44.0;
                dY1 = dr - ((359.0 * dCr) / 256.0);
                dY2 = dr2 - ((359.0 * dCr) / 256.0);

                *val = ((uint32_t) round(dY1)) + (((uint32_t) round(dCr)) << 8) + (((uint32_t) round(dY2)) << 16) + (((uint32_t) round(dCb)) << 24);
            }
            return;
        case 5: /* 16bpp, raw */
        case 7: /* 16bpp, RGB 5-6-5 */
            dr   = (dr / 255.0) * 31.0;
            dg   = (dg / 255.0) * 63.0;
            db   = (db / 255.0) * 31.0;
            *val = (((uint32_t) round(dr)) << 11) + (((uint32_t) round(dg)) << 5) + ((uint32_t) round(db));
            break;
        case 6: /* 15bpp, RGB 5-5-5 */
            dr   = (dr / 255.0) * 31.0;
            dg   = (dg / 255.0) * 31.0;
            db   = (db / 255.0) * 31.0;
            *val = (((uint32_t) round(dr)) << 10) + (((uint32_t) round(dg)) << 5) + ((uint32_t) round(db));
            break;

        default:
            break;
    }
}

/*To Do: Dithering, color space conversion.*/
static void
s3_visionx68_video_engine_op(uint32_t cpu_dat, s3_t *s3)
{
    svga_t   *svga = &s3->svga;
    int       idf;
    int       odf;
    int       host;
    int       is_yuv;
    uint32_t  src;
    uint32_t  dest = 0x00000000;
    uint8_t   r = 0x00;
    uint8_t   g = 0x00;
    uint8_t   b = 0x00;
    uint8_t   r2 = 0x00;
    uint8_t   g2 = 0x00;
    uint8_t   b2 = 0x00;
    uint16_t *vram_w = (uint16_t *) svga->vram;
    uint32_t *vram_l = (uint32_t *) svga->vram;
    uint32_t  k2 = 0;
    uint32_t dda = 0;
    uint32_t diff = 0;
    int       count = -1;

    idf    = s3->videoengine.idf;
    odf    = s3->videoengine.odf;
    is_yuv = s3->videoengine.yuv;
    host   = s3->videoengine.host_data;

    k2   = s3->videoengine.k2 - 0x700;
    dda  = s3->videoengine.dda_init_accumulator - 0xf00;
    diff = 0xff - k2;

    s3->videoengine.busy = 1;

    if (host) {
        if (idf == 0 && odf == 0) {
            if (s3->bpp == 0)
                count = 4;
            else if (s3->bpp == 1)
                count = 2;
            else
                count = 1;
        } else {
            if (idf == 0)
                count = 4;
            else if (idf == 3)
                count = 1;
            else
                count = 2;
        }
    }

    if (s3->videoengine.input == 1) {
        if (s3->videoengine.scale_down) {
            if (s3->bpp > 1) {
                s3->videoengine.sx        = k2 - dda + diff;
                s3->videoengine.sx_backup = s3->videoengine.len - s3->videoengine.start;
            } else {
                s3->videoengine.sx        = k2 - dda + diff - 1;
                s3->videoengine.sx_backup = s3->videoengine.len - s3->videoengine.start - 1;
            }
            s3->videoengine.sx_scale_inc = (double) (s3->videoengine.sx_backup >> 1);
            s3->videoengine.sx_scale_inc = s3->videoengine.sx_scale_inc / (double) (s3->videoengine.sx >> 1);
        } else {
            s3->videoengine.sx_scale     = (double) (s3->videoengine.k1 - 2);
            s3->videoengine.sx_scale_dec = (s3->videoengine.sx_scale / (double) (s3->videoengine.len - s3->videoengine.start - 2));

            if (s3->videoengine.sx_scale_dec >= 0.5) {
                s3->videoengine.sx_scale++;
            }
        }

        if (s3->bpp == 0) {
            s3->videoengine.dest = s3->videoengine.dest_base + s3->width;
            s3->videoengine.src  = s3->videoengine.src_base + s3->width;
        } else if (s3->bpp == 1) {
            s3->videoengine.dest = (s3->videoengine.dest_base >> 1) + s3->width;
            s3->videoengine.src  = (s3->videoengine.src_base >> 1) + s3->width;
        } else {
            s3->videoengine.dest = (s3->videoengine.dest_base >> 2) + s3->width;
            s3->videoengine.src  = (s3->videoengine.src_base >> 2) + s3->width;
        }
        s3->videoengine.input = 2;
        s3->videoengine.cx    = 0.0;
        s3->videoengine.dx    = 0.0;
    }

    while (count) {
        if (host) { /*Source data is CPU*/
            src = cpu_dat;
        } else { /*Source data is display memory*/
            READ(s3->videoengine.src + lround(s3->videoengine.cx), src);
        }

        convert_to_rgb32(idf, is_yuv, src, &r, &g, &b, &r2, &g2, &b2);

        convert_from_rgb32(idf, odf, is_yuv, &dest, r, g, b, r2, g2, b2);

        WRITE(s3->videoengine.dest + lround(s3->videoengine.dx), dest);

        if (s3->videoengine.scale_down) { /*Data shrink*/
            s3->videoengine.dx += s3->videoengine.sx_scale_inc;
            if (!host)
                s3->videoengine.cx += s3->videoengine.sx_scale_inc;

            s3->videoengine.sx--;

            if (host) {
                if (s3->bpp == 0) {
                    cpu_dat >>= 8;
                } else {
                    cpu_dat >>= 16;
                }
                count--;
            }

            if (s3->videoengine.sx < 0) {
                if (s3->bpp > 1) {
                    s3->videoengine.sx        = k2 - dda + diff;
                    s3->videoengine.sx_backup = s3->videoengine.len - s3->videoengine.start;
                } else {
                    s3->videoengine.sx        = k2 - dda + diff - 1;
                    s3->videoengine.sx_backup = s3->videoengine.len - s3->videoengine.start - 1;
                }
                s3->videoengine.sx_scale_inc = (double) (s3->videoengine.sx_backup >> 1);
                s3->videoengine.sx_scale_inc = s3->videoengine.sx_scale_inc / (double) (s3->videoengine.sx >> 1);

                s3->videoengine.cx = 0.0;
                s3->videoengine.dx = 0.0;

                if (s3->bpp == 0) {
                    s3->videoengine.dest = s3->videoengine.dest_base + s3->width;
                    s3->videoengine.src  = s3->videoengine.src_base + s3->width;
                } else if (s3->bpp == 1) {
                    s3->videoengine.dest = (s3->videoengine.dest_base >> 1) + s3->width;
                    s3->videoengine.src  = (s3->videoengine.src_base >> 1) + s3->width;
                } else {
                    s3->videoengine.dest = (s3->videoengine.dest_base >> 2) + s3->width;
                    s3->videoengine.src  = (s3->videoengine.src_base >> 2) + s3->width;
                }

                if (s3->videoengine.input >= 1) {
                    s3->videoengine.busy = 0;
                    return;
                }
            }
        } else { /*Data stretch*/
            s3->videoengine.dx++;

            s3->videoengine.sx_scale -= s3->videoengine.sx_scale_dec;
            s3->videoengine.sx_scale_backup = (s3->videoengine.sx_scale - s3->videoengine.sx_scale_dec);

            s3->videoengine.sx           = lround(s3->videoengine.sx_scale);
            s3->videoengine.sx_scale_int = lround(s3->videoengine.sx_scale_backup);

            if (s3->videoengine.sx > s3->videoengine.sx_scale_int) {
                if (host) {
                    if (s3->bpp == 0)
                        cpu_dat >>= 8;
                    else
                        cpu_dat >>= 16;
                    count--;
                } else {
                    s3->videoengine.cx++;
                }
            }

            if (s3->videoengine.sx < 0) {
                s3->videoengine.sx_scale     = (double) (s3->videoengine.k1 - 2);
                s3->videoengine.sx_scale_dec = (s3->videoengine.sx_scale / (double) (s3->videoengine.len - s3->videoengine.start - 2));

                if (s3->videoengine.sx_scale_dec >= 0.5) {
                    s3->videoengine.sx_scale++;
                }

                s3->videoengine.cx = 0.0;
                s3->videoengine.dx = 0.0;

                if (s3->bpp == 0) {
                    s3->videoengine.dest = s3->videoengine.dest_base + s3->width;
                    s3->videoengine.src  = s3->videoengine.src_base + s3->width;
                } else if (s3->bpp == 1) {
                    s3->videoengine.dest = (s3->videoengine.dest_base >> 1) + s3->width;
                    s3->videoengine.src  = (s3->videoengine.src_base >> 1) + s3->width;
                } else {
                    s3->videoengine.dest = (s3->videoengine.dest_base >> 2) + s3->width;
                    s3->videoengine.src  = (s3->videoengine.src_base >> 2) + s3->width;
                }

                if (s3->videoengine.input >= 1) {
                    s3->videoengine.busy = 0;
                    return;
                }
            }
        }
    }
}

void
s3_short_stroke_start(s3_t *s3, uint8_t ssv)
{
    s3->accel.ssv_len  = ssv & 0x0f;
    s3->accel.ssv_dir  = ssv & 0xe0;
    s3->accel.ssv_draw = !!(ssv & 0x10);

    if (s3_cpu_src(s3))
        return; /*Wait for data from CPU*/

    s3->accel_start(-1, 0, -1, 0, s3);
}

void
s3_accel_start(int count, int cpu_input, uint32_t mix_dat, uint32_t cpu_dat, void *priv)
{
    s3_t     *s3      = (s3_t *)priv;
    svga_t   *svga    = &s3->svga;
    uint32_t  src_dat = 0;
    uint32_t  dest_dat;
    uint32_t  old_dest_dat;
    uint32_t  out;
    uint32_t  pat_dat = 0;
    int       frgd_mix;
    int       bkgd_mix;
    int       clip_t       = s3->accel.multifunc[1] & 0xfff;
    int       clip_l       = s3->accel.multifunc[2] & 0xfff;
    int       clip_b       = s3->accel.multifunc[3] & 0xfff;
    int       clip_r       = s3->accel.multifunc[4] & 0xfff;
    int       vram_mask    = (s3->accel.multifunc[0xa] & 0xc0) == 0xc0;
    uint32_t  mix_mask     = 0;
    uint16_t *vram_w       = (uint16_t *) svga->vram;
    uint32_t *vram_l       = (uint32_t *) svga->vram;
    uint32_t  compare      = s3->accel.color_cmp;
    uint8_t   rop          = s3->accel.ropmix & 0xff;
    uint32_t  rd_mask      = s3->accel.rd_mask;
    uint32_t  wrt_mask     = s3->accel.wrt_mask;
    uint32_t  frgd_color   = s3->accel.frgd_color;
    uint32_t  bkgd_color   = s3->accel.bkgd_color;
    int       cmd          = s3->accel.cmd >> 13;
    int       update       = 1;
    uint32_t  srcbase;
    uint32_t  dstbase;

    frgd_mix = (s3->accel.frgd_mix >> 5) & 3;
    bkgd_mix = (s3->accel.bkgd_mix >> 5) & 3;

    if (((s3->chip >= S3_TRIO64) || (s3->chip == S3_VISION968) || (s3->chip == S3_VISION868)) && (s3->accel.cmd & (1 << 11)))
        cmd |= 0x08;

    // SRC-BASE/DST-BASE
    if (((s3->accel.multifunc[0xd] >> 4) & 7) && (s3->chip >= S3_VISION964))
        srcbase = 0x100000 * ((s3->accel.multifunc[0xd] >> 4) & 7);
    else
        srcbase = 0x100000 * ((s3->accel.multifunc[0xe] >> 2) & 3);

    if (((s3->accel.multifunc[0xd] >> 0) & 7) && (s3->chip >= S3_VISION964))
        dstbase = 0x100000 * ((s3->accel.multifunc[0xd] >> 0) & 7);
    else
        dstbase = 0x100000 * ((s3->accel.multifunc[0xe] >> 0) & 3);

    if ((s3->bpp == 1) || s3->color_16bit) {
        srcbase >>= 1;
        dstbase >>= 1;
    } else if (s3->bpp == 3) {
        srcbase >>= 2;
        dstbase >>= 2;
    }

    if ((s3->accel.cmd & 0x100) && (s3_cpu_src(s3) || (s3_cpu_dest(s3))) && (!cpu_input || (s3_enable_fifo(s3) == 0)))
        s3->force_busy = 1;

    if (!cpu_input)
        s3->accel.dat_count = 0;

    if (cpu_input && (((s3->accel.multifunc[0xa] & 0xc0) != 0x80) || (!(s3->accel.cmd & 0x02)))) {
        if ((s3->bpp == 3) && (count == 2)) {
            if (s3->accel.dat_count) {
                cpu_dat             = ((cpu_dat & 0xffff) << 16) | s3->accel.dat_buf;
                count               = 4;
                s3->accel.dat_count = 0;
            } else {
                s3->accel.dat_buf   = cpu_dat & 0xffff;
                s3->accel.dat_count = 1;
            }
        }
        if ((s3->bpp == 1) || s3->color_16bit)
            count >>= 1;
        else if (s3->bpp == 3)
            count >>= 2;
    }

    if ((s3->bpp == 0) && !s3->color_16bit)
        rd_mask &= 0xff;
    else if ((s3->bpp == 1) || s3->color_16bit)
        rd_mask &= 0xffff;

    if ((s3->bpp == 0) && !s3->color_16bit)
        compare &= 0xff;
    else if ((s3->bpp == 1) || s3->color_16bit)
        compare &= 0xffff;

    switch (s3->accel.cmd & 0x600) {
        case 0x000:
            mix_mask = 0x80;
            break;
        case 0x200:
            mix_mask = 0x8000;
            break;
        case 0x400:
            mix_mask = 0x80000000;
            break;
        case 0x600:
            mix_mask = (s3->chip == S3_TRIO32 || s3->chip >= S3_TRIO64V || s3->chip == S3_VISION968 || s3->chip == S3_VISION868) ? 0x80 : 0x80000000;
            break;

        default:
            break;
    }

    /*Bit 4 of the Command register is the draw yes bit, which enables writing to memory/reading from memory when enabled.
      When this bit is disabled, no writing to memory/reading from memory is allowed. (This bit is almost meaningless on
      the NOP command)*/
    s3_log("CMD=%d, full=%04x, s3bpp=%x, clr=%d, clb=%d, sourcedisplay=%02x, mmio=%02x, srcbase=%08x, dstbase=%08x, cpu=%04x, mix=%04x, count=%d, rd_mask=%04x, wrt_mask=%04x, width=%d, s=%d,%d, c=%d,%d, d=%d,%d, 16bitcolor=%x, frgdcolor=%04x, bkgdcolor=%04x, frgdsel=%d, bkgdsel=%d, frgdmix=%02x, curx=%d, cury=%d, cll=%d, b2e8pix=%x.\n", cmd, s3->accel.cmd, s3->bpp, clip_r, clip_b, s3->accel.multifunc[0x0a] & 0xc4, svga->crtc[0x53] & 0x18, srcbase, dstbase, cpu_dat & 0xffff, mix_dat & 0xffff, count, rd_mask, wrt_mask, s3->width, s3->accel.sx, s3->accel.sy, s3->accel.cx, s3->accel.cy, s3->accel.dx, s3->accel.dy, s3->color_16bit, frgd_color, bkgd_color, frgd_mix, bkgd_mix, s3->accel.frgd_mix & 0x0f, s3->accel.cur_x, s3->accel.cur_y, clip_l, s3->accel.b2e8_pix);

    switch (cmd) {
        case 0: /*NOP (Short Stroke Vectors)*/
            if (s3->accel.ssv_state == 0)
                break;

            if (s3->accel.cmd & 0x08) { /*Radial*/
                while (count-- && s3->accel.ssv_len >= 0) {
                    if ((s3->accel.cx & 0xfff) >= clip_l && (s3->accel.cx & 0xfff) <= clip_r && (s3->accel.cy & 0xfff) >= clip_t && (s3->accel.cy & 0xfff) <= clip_b) {
                        switch ((mix_dat & mix_mask) ? frgd_mix : bkgd_mix) {
                            case 0:
                                src_dat = bkgd_color;
                                break;
                            case 1:
                                src_dat = frgd_color;
                                break;
                            case 2:
                                src_dat = cpu_dat;
                                break;
                            case 3:
                                src_dat = 0;
                                break;

                            default:
                                break;
                        }

                        if (s3->accel.multifunc[0xe] & 0x100) {
                            if (s3->accel.multifunc[0xe] & 0x80) {
                                if (src_dat != compare)
                                    update = 0;
                                else
                                    update = 1;
                            } else {
                                if (src_dat == compare)
                                    update = 0;
                                else
                                    update = 1;
                            }
                        } else
                            update = 1;

                        if (update) {
                            READ((s3->accel.cy * s3->width) + s3->accel.cx, dest_dat);

                            old_dest_dat = dest_dat;
                            MIX

                            if (s3->accel.ssv_draw) {
                                WRITE((s3->accel.cy * s3->width) + s3->accel.cx, dest_dat);
                            }
                        }
                    }

                    mix_dat <<= 1;
                    mix_dat |= 1;
                    if ((s3->bpp == 0) && !s3->color_16bit)
                        cpu_dat >>= 8;
                    else
                        cpu_dat >>= 16;

                    if (!s3->accel.ssv_len) {
                        s3->accel.cur_x = s3->accel.cx & 0xfff;
                        s3->accel.cur_y = s3->accel.cy & 0xfff;
                        break;
                    }

                    switch (s3->accel.ssv_dir & 0xe0) {
                        case 0x00:
                            s3->accel.cx++;
                            break;
                        case 0x20:
                            s3->accel.cx++;
                            s3->accel.cy--;
                            break;
                        case 0x40:
                            s3->accel.cy--;
                            break;
                        case 0x60:
                            s3->accel.cx--;
                            s3->accel.cy--;
                            break;
                        case 0x80:
                            s3->accel.cx--;
                            break;
                        case 0xa0:
                            s3->accel.cx--;
                            s3->accel.cy++;
                            break;
                        case 0xc0:
                            s3->accel.cy++;
                            break;
                        case 0xe0:
                            s3->accel.cx++;
                            s3->accel.cy++;
                            break;

                        default:
                            break;
                    }

                    s3->accel.ssv_len--;
                    s3->accel.cx &= 0xfff;
                    s3->accel.cy &= 0xfff;
                }
            }
            break;

        case 1: /*Draw line*/
            if (!cpu_input) {
                s3->accel.rd_mask_16bit_check = 0;
                s3->accel.minus = 0;
                s3->accel.color_16bit_check_pixtrans = 0;
                s3->accel.cx = s3->accel.cur_x & 0xfff;
                s3->accel.cy = s3->accel.cur_y & 0xfff;

                s3->accel.sy = s3->accel.maj_axis_pcnt;
                if ((s3->bpp == 0) && s3->color_16bit) {
                    s3->accel.rd_mask_16bit_check = ((rd_mask & 0xff00) != 0xff00) && rd_mask;
                    if (s3->accel.rd_mask_16bit_check) {
                        if ((s3->accel.cur_x_overflow & 0xc00) == 0xc00)
                            s3->accel.start = 1;
                        else {
                            if (s3->accel.start) {
                                s3->accel.start = 0;
                                s3->accel.minus = 0x400;
                            } else {
                                s3->accel.start = 0;
                                if (s3->accel.cur_x_overflow & 0x400)
                                    s3->accel.minus = 0x400;
                            }
                        }
                    }
                }

                if (s3_cpu_src(s3))
                    return; /*Wait for data from CPU*/
            }

            if (s3->accel.cmd & 0x08) { /*Radial*/
                if ((s3->bpp == 0) && s3->color_16bit) {
                    if (s3->accel.rd_mask_16bit_check) {
                        if (s3->accel.minus) {
                            wrt_mask = (s3->accel.wrt_mask_actual[1] << 8);
                            frgd_color = (s3->accel.frgd_color_actual[1] << 8);
                            bkgd_color = (s3->accel.bkgd_color_actual[1] << 8);
                        } else {
                            wrt_mask = s3->accel.wrt_mask_actual[0];
                            frgd_color = s3->accel.frgd_color_actual[0];
                            bkgd_color = s3->accel.bkgd_color_actual[0];
                        }
                        rd_mask &= 0x00ff;
                    } else if (!s3->accel.rd_mask_16bit_check && (s3->accel.cur_x & 0x400))
                        break;
                }

                while (count-- && s3->accel.sy >= 0) {
                    if ((s3->accel.cx & 0xfff) >= clip_l && (s3->accel.cx & 0xfff) <= clip_r && (s3->accel.cy & 0xfff) >= clip_t && (s3->accel.cy & 0xfff) <= clip_b) {
                        switch ((mix_dat & mix_mask) ? frgd_mix : bkgd_mix) {
                            case 0:
                                src_dat = bkgd_color;
                                break;
                            case 1:
                                src_dat = frgd_color;
                                break;
                            case 2:
                                src_dat = cpu_dat;
                                break;
                            case 3:
                                src_dat = 0;
                                break;

                            default:
                                break;
                        }

                        if (s3->accel.multifunc[0xe] & 0x100) {
                            if (s3->accel.multifunc[0xe] & 0x80) {
                                if (src_dat != compare)
                                    update = 0;
                                else
                                    update = 1;
                            } else {
                                if (src_dat == compare)
                                    update = 0;
                                else
                                    update = 1;
                            }
                        } else
                            update = 1;

                        if (update) {
                            READ((s3->accel.cy * s3->width) + s3->accel.cx - s3->accel.minus, dest_dat);

                            old_dest_dat = dest_dat;
                            MIX

                            WRITE((s3->accel.cy * s3->width) + s3->accel.cx - s3->accel.minus, dest_dat);
                        }
                    }

                    mix_dat <<= 1;
                    mix_dat |= 1;
                    if ((s3->bpp == 0) && !s3->color_16bit)
                        cpu_dat >>= 8;
                    else
                        cpu_dat >>= 16;

                    if (!s3->accel.sy) {
                        if (s3->accel.rd_mask_16bit_check)  {
                            if (s3->accel.minus)
                                s3->accel.color_16bit_check = 0;
                            else
                                s3->accel.color_16bit_check = 1;
                        }
                        break;
                    }

                    switch (s3->accel.cmd & 0xe0) {
                        case 0x00:
                            s3->accel.cx++;
                            break;
                        case 0x20:
                            s3->accel.cx++;
                            s3->accel.cy--;
                            break;
                        case 0x40:
                            s3->accel.cy--;
                            break;
                        case 0x60:
                            s3->accel.cx--;
                            s3->accel.cy--;
                            break;
                        case 0x80:
                            s3->accel.cx--;
                            break;
                        case 0xa0:
                            s3->accel.cx--;
                            s3->accel.cy++;
                            break;
                        case 0xc0:
                            s3->accel.cy++;
                            break;
                        case 0xe0:
                            s3->accel.cx++;
                            s3->accel.cy++;
                            break;

                        default:
                            break;
                    }
                    s3->accel.sy--;
                    s3->accel.cx &= 0xfff;
                    s3->accel.cy &= 0xfff;
                }
                s3->accel.cur_x = s3->accel.cx & 0xfff;
                s3->accel.cur_y = s3->accel.cy & 0xfff;
            } else { /*Bresenham*/
                if (s3->accel.b2e8_pix && s3_cpu_src(s3) && (count == 16)) { /*Pattern on pixtrans (911/924)*/
                    count              = s3->accel.maj_axis_pcnt + 1;
                    s3->accel.temp_cnt = 16;
                    if ((s3->bpp == 0) && s3->color_16bit) {
                        if (s3->accel.rd_mask_16bit_check) {
                            if (s3->accel.minus) {
                                wrt_mask = (s3->accel.wrt_mask_actual[1] << 8);
                                frgd_color = (s3->accel.frgd_color_actual[1] << 8);
                                bkgd_color = (s3->accel.bkgd_color_actual[1] << 8);
                            } else {
                                wrt_mask = s3->accel.wrt_mask_actual[0];
                                frgd_color = s3->accel.frgd_color_actual[0];
                                bkgd_color = s3->accel.bkgd_color_actual[0];
                            }
                            rd_mask &= 0x00ff;
                        } else if (!s3->accel.rd_mask_16bit_check && (s3->accel.cur_x & 0x400))
                            break;
                    }
                } else {
                    if ((s3->bpp == 0) && s3->color_16bit) {
                        if (s3->accel.rd_mask_16bit_check) {
                            if (s3->accel.minus) {
                                wrt_mask = (s3->accel.wrt_mask_actual[1] << 8);
                                frgd_color = (s3->accel.frgd_color_actual[1] << 8);
                                bkgd_color = (s3->accel.bkgd_color_actual[1] << 8);
                            } else {
                                wrt_mask = s3->accel.wrt_mask_actual[0];
                                frgd_color = s3->accel.frgd_color_actual[0];
                                bkgd_color = s3->accel.bkgd_color_actual[0];
                            }
                            rd_mask &= 0x00ff;
                        } else if (!s3->accel.rd_mask_16bit_check && (s3->accel.cur_x & 0x400))
                            break;
                    }
                }

                if (!s3->accel.b2e8_pix)
                    s3_log("CMDFULL=%04x, FRGDMIX=%x, FRGDCOLR=%04x, RDMASK=%04x, MINUS=%d, WRTMASK=%04X, MIX=%04x, CX=%d, CY=%d, SX=%d, SY=%d, PIXCNTL=%02x, 16BITCOLOR=%x, RDCHECK=%x, CLIPL=%d, CLIPR=%d, CLIPT=%d, CLIPB=%d.\n", s3->accel.cmd, frgd_mix, s3->accel.frgd_color, rd_mask, s3->accel.minus, wrt_mask, mix_dat & 0xffff, s3->accel.cx, s3->accel.cy, s3->accel.sx, s3->accel.sy, s3->accel.multifunc[0x0a] & 0xc4, s3->accel.color_16bit_check, s3->accel.rd_mask_16bit_check, clip_l, clip_r, clip_t, clip_b);

                while (count-- && (s3->accel.sy >= 0)) {
                    if (s3->accel.b2e8_pix && s3_cpu_src(s3) && !s3->accel.temp_cnt) {
                        mix_dat >>= 16;
                        s3->accel.temp_cnt = 16;
                    }

                    if (s3->accel.minus)
                        s3_log("Total pixel cx=%d, cy=%d.\n", s3->accel.cx - s3->accel.minus, s3->accel.cy);

                    if ((s3->accel.cx & 0xfff) >= clip_l && (s3->accel.cx & 0xfff) <= clip_r && (s3->accel.cy & 0xfff) >= clip_t && (s3->accel.cy & 0xfff) <= clip_b) {
                        switch ((mix_dat & mix_mask) ? frgd_mix : bkgd_mix) {
                            case 0:
                                src_dat = bkgd_color;
                                break;
                            case 1:
                                src_dat = frgd_color;
                                break;
                            case 2:
                                src_dat = cpu_dat;
                                break;
                            case 3:
                                src_dat = 0;
                                break;

                            default:
                                break;
                        }

                        if (s3->accel.multifunc[0xe] & 0x100) {
                            if (s3->accel.multifunc[0xe] & 0x80) {
                                if (src_dat != compare)
                                    update = 0;
                                else
                                    update = 1;
                            } else {
                                if (src_dat == compare)
                                    update = 0;
                                else
                                    update = 1;
                            }
                        } else
                            update = 1;

                        if (update) {
                            READ((s3->accel.cy * s3->width) + s3->accel.cx - s3->accel.minus, dest_dat);

                            old_dest_dat = dest_dat;
                            MIX

                            WRITE((s3->accel.cy * s3->width) + s3->accel.cx - s3->accel.minus, dest_dat);
                        }
                    }

                    if (s3->accel.b2e8_pix && s3_cpu_src(s3)) {
                        if (s3->accel.temp_cnt > 0) {
                            s3->accel.temp_cnt--;
                            mix_dat <<= 1;
                            mix_dat |= 1;
                        }
                    } else {
                        mix_dat <<= 1;
                        mix_dat |= 1;
                    }

                    if ((s3->bpp == 0) && !s3->color_16bit)
                        cpu_dat >>= 8;
                    else
                        cpu_dat >>= 16;

                    if (!s3->accel.sy) {
                        if (s3->accel.rd_mask_16bit_check)  {
                            if (s3->accel.minus)
                                s3->accel.color_16bit_check = 0;
                            else
                                s3->accel.color_16bit_check = 1;
                        }
                        break;
                    }

                    if (s3->accel.cmd & 0x40) {
                        if (s3->accel.cmd & 0x80)
                            s3->accel.cy++;
                        else
                            s3->accel.cy--;

                        if (s3->accel.err_term >= s3->accel.maj_axis_pcnt) {
                            s3->accel.err_term += s3->accel.destx_distp;
                            if (s3->accel.cmd & 0x20)
                                s3->accel.cx++;
                            else
                                s3->accel.cx--;
                        } else
                            s3->accel.err_term += s3->accel.desty_axstp;
                    } else {
                        if (s3->accel.cmd & 0x20)
                            s3->accel.cx++;
                        else
                            s3->accel.cx--;

                        if (s3->accel.err_term >= s3->accel.maj_axis_pcnt) {
                            s3->accel.err_term += s3->accel.destx_distp;
                            if (s3->accel.cmd & 0x80)
                                s3->accel.cy++;
                            else
                                s3->accel.cy--;
                        } else
                            s3->accel.err_term += s3->accel.desty_axstp;
                    }

                    s3->accel.sy--;
                    s3->accel.cx &= 0xfff;
                    s3->accel.cy &= 0xfff;
                }
                s3->accel.cur_x = s3->accel.cx & 0xfff;
                s3->accel.cur_y = s3->accel.cy & 0xfff;
            }
            break;

        case 2: /*Rectangle fill*/
            if (!cpu_input) /*!cpu_input is trigger to start operation*/
            {
                s3->accel.minus = 0;
                s3->accel.mix_dat_upper = 0;
                s3->accel.color_16bit_check_pixtrans = 0;
                s3->accel.sx = s3->accel.maj_axis_pcnt & 0xfff;
                s3->accel.sy = s3->accel.multifunc[0] & 0xfff;
                s3->accel.cx = s3->accel.cur_x & 0xfff;
                s3->accel.cy = s3->accel.cur_y & 0xfff;

                s3->accel.dest = dstbase + s3->accel.cy * s3->width;

                if ((s3->bpp == 0) && s3->color_16bit) {
                    s3->accel.rd_mask_16bit_check = ((rd_mask & 0xff00) != 0xff00) && rd_mask;
                    if (s3->accel.rd_mask_16bit_check) {
                        if (s3->accel.cmd == 0x41b3) {
                            if (frgd_mix == 0) {
                                if (!(s3->accel.cur_x & 0x400))
                                    s3->accel.color_16bit_check = 0;
                            } else {
                                if ((s3->accel.cur_x_overflow & 0xc00) == 0xc00)
                                    s3->accel.start = 1;
                                else {
                                    if (s3->accel.start) {
                                        s3->accel.start = 0;
                                        s3->accel.minus = 0x400;
                                    } else {
                                        s3->accel.start = 0;
                                        if (s3->accel.cur_x_overflow & 0x400)
                                            s3->accel.minus = 0x400;
                                    }
                                }
                            }
                        } else {
                            if ((s3->accel.cur_x_overflow & 0xc00) == 0xc00)
                                s3->accel.start = 1;
                            else {
                                if (s3->accel.start) {
                                    s3->accel.start = 0;
                                    s3->accel.minus = 0x400;
                                } else {
                                    s3->accel.start = 0;
                                    if (s3->accel.cur_x_overflow & 0x400)
                                        s3->accel.minus = 0x400;
                                }
                            }
                        }
                    } else {
                        if (s3->accel.cmd & 0x100) {
                            if (mix_mask == 0x80) {
                                if (s3->accel.cur_x & 0x400)
                                    s3->accel.minus = 0x400;
                                else
                                    s3->accel.minus = 0;
                            }
                        }
                    }
                } else
                    s3->accel.rd_mask_16bit_check = 0;

                if (s3->accel.cur_x & 0x400)
                    s3_log("Rectangle Fill + 1024 FULLCMD=%04x: frgdcolor=%04x, s=%d,%d, c=%d,%d, frmix=%x, bkmix=%x, pixcntl=%02x, m2=%d, m4=%d.\n", s3->accel.cmd, s3->accel.frgd_color_actual[1] << 8, s3->accel.sx, s3->accel.sy, s3->accel.cx, s3->accel.cy, frgd_mix, bkgd_mix, s3->accel.multifunc[0xa] & 0xc0, s3->accel.multifunc[2], s3->accel.multifunc[4]);
                else
                    s3_log("Rectangle Fill + 0 FULLCMD=%04x: frgdcolor=%04x, s=%d,%d, c=%d,%d, frmix=%x, bkmix=%x, pixcntl=%02x, m2=%d, m4=%d.\n", s3->accel.cmd, s3->accel.frgd_color_actual[0], s3->accel.sx, s3->accel.sy, s3->accel.cx, s3->accel.cy, frgd_mix, bkgd_mix, s3->accel.multifunc[0xa] & 0xc0, s3->accel.multifunc[2], s3->accel.multifunc[4]);

                if (s3_cpu_src(s3)) {
                    s3->data_available = 0;
                    return; /*Wait for data from CPU*/
                } else if (s3_cpu_dest(s3)) {
                    s3->data_available = 1;
                    return;
                }
            }

            if (s3->accel.b2e8_pix && s3_cpu_src(s3) && (count == 16)) { /*Pattern on pixtrans (911/924)*/
                count              = s3->accel.maj_axis_pcnt + 1;
                s3->accel.temp_cnt = 16;
                if (s3->accel.rd_mask_16bit_check) {
                    if (s3->accel.minus) {
                        wrt_mask = (s3->accel.wrt_mask_actual[1] << 8);
                        frgd_color = (s3->accel.frgd_color_actual[1] << 8);
                        bkgd_color = (s3->accel.bkgd_color_actual[1] << 8);
                    } else {
                        wrt_mask = s3->accel.wrt_mask_actual[0];
                        frgd_color = s3->accel.frgd_color_actual[0];
                        bkgd_color = s3->accel.bkgd_color_actual[0];
                    }
                    rd_mask &= 0x00ff;
                } else if (!s3->accel.rd_mask_16bit_check && (s3->accel.cur_x & 0x400))
                    break;
            } else {
                if ((s3->bpp == 0) && s3->color_16bit) {
                    if (s3->accel.cmd == 0x41b3) {
                        if (frgd_mix != 0) {
                            if (s3->accel.rd_mask_16bit_check) {
                                if (s3->accel.minus) {
                                    wrt_mask = (s3->accel.wrt_mask_actual[1] << 8);
                                    frgd_color = (s3->accel.frgd_color_actual[1] << 8);
                                    bkgd_color = (s3->accel.bkgd_color_actual[1] << 8);
                                } else {
                                    wrt_mask = s3->accel.wrt_mask_actual[0];
                                    frgd_color = s3->accel.frgd_color_actual[0];
                                    bkgd_color = s3->accel.bkgd_color_actual[0];
                                }
                                rd_mask &= 0x00ff;
                            } else if (!s3->accel.rd_mask_16bit_check && (s3->accel.cur_x & 0x400))
                                break;
                        } else {
                            if (s3->accel.rd_mask_16bit_check) {
                                if (s3->accel.minus) {
                                    wrt_mask = (s3->accel.wrt_mask_actual[1] << 8);
                                    frgd_color = (s3->accel.frgd_color_actual[1] << 8);
                                    bkgd_color = (s3->accel.bkgd_color_actual[1] << 8);
                                } else {
                                    wrt_mask = s3->accel.wrt_mask_actual[0];
                                    frgd_color = s3->accel.frgd_color_actual[0];
                                    bkgd_color = s3->accel.bkgd_color_actual[0];
                                    s3->accel.mix_dat_upper = !!(mix_dat & 0xff00);
                                }
                                rd_mask &= 0x00ff;
                            }
                        }
                    } else {
                        if (s3->accel.rd_mask_16bit_check) {
                            if (s3->accel.minus) {
                                wrt_mask = (s3->accel.wrt_mask_actual[1] << 8);
                                frgd_color = (s3->accel.frgd_color_actual[1] << 8);
                                bkgd_color = (s3->accel.bkgd_color_actual[1] << 8);
                            } else {
                                wrt_mask = s3->accel.wrt_mask_actual[0];
                                frgd_color = s3->accel.frgd_color_actual[0];
                                bkgd_color = s3->accel.bkgd_color_actual[0];
                            }
                            rd_mask &= 0x00ff;
                        } else {
                            if ((s3_cpu_src(s3)) && !(s3->accel.cmd & 0x200)) {
                                s3_log("FIXME: S3 911/924 15/16bpp documentation needed.\n");
                            } else {
                                if (!cpu_input && (s3->accel.cur_x & 0x400)) {
                                    s3_log("No Input.\n");
                                    break;
                                } else if (cpu_input && (s3->accel.cmd == 0x53b3) && (s3->accel.cur_x & 0x400))
                                    break;
                            }
                        }
                    }
                }
            }

            s3_log("CMDFULL=%04x, FRGDSEL=%x, BKGDSEL=%x, FRGDMIX=%02x, BKGDMIX=%02x, MASKCHECK=%x, RDMASK=%04x, MINUS=%d, WRTMASK=%04X, MIX=%04x, CX=%d, CY=%d, DX=%d, DY=%d, SX=%d, SY=%d, PIXCNTL=%02x, 16BITCOLOR=%x, RDCHECK=%x, CLIPL=%d, CLIPR=%d, OVERFLOW=%d, pitch=%d.\n", s3->accel.cmd, frgd_mix, bkgd_mix, s3->accel.frgd_mix & 0x0f, s3->accel.bkgd_mix & 0x0f, s3->accel.rd_mask_16bit_check, rd_mask, s3->accel.minus, wrt_mask, mix_dat & 0xffff, s3->accel.cx, s3->accel.cy, s3->accel.dx, s3->accel.dy, s3->accel.sx, s3->accel.sy, s3->accel.multifunc[0x0a] & 0xc4, s3->accel.color_16bit_check, s3->accel.rd_mask_16bit_check, clip_l, clip_r, (s3->accel.destx_overflow & 0xc00) == 0xc00, s3->width);

            while (count-- && (s3->accel.sy >= 0)) {
                if (s3->accel.b2e8_pix && s3_cpu_src(s3) && !s3->accel.temp_cnt) {
                    mix_dat >>= 16;
                    s3->accel.temp_cnt = 16;
                }

                if ((((s3->accel.cx >= clip_l) && (s3->accel.cx <= clip_r) && (s3->accel.cy >= clip_t) && (s3->accel.cy <= clip_b)) && !(s3->accel.multifunc[0xe] & 0x20)) ||
                    (((s3->accel.cx < clip_l) && (s3->accel.cx > clip_r) && (s3->accel.cy < clip_t) && (s3->accel.cy > clip_b)) && (s3->accel.multifunc[0xe] & 0x20)) ) {
                    if (s3_cpu_dest(s3) && ((s3->accel.multifunc[0xa] & 0xc0) == 0x00)) {
                        mix_dat = mix_mask; /* Mix data = forced to foreground register. */
                    } else if (s3_cpu_dest(s3) && vram_mask) {
                        /* Mix data = current video memory value. */
                        READ(s3->accel.dest + s3->accel.cx, mix_dat);
                        mix_dat = ((mix_dat & rd_mask) == rd_mask);
                        mix_dat = mix_dat ? mix_mask : 0;
                    }

                    if (s3_cpu_dest(s3)) {
                        READ(s3->accel.dest + s3->accel.cx - s3->accel.minus, src_dat);
                        if (vram_mask)
                            src_dat = ((src_dat & rd_mask) == rd_mask);
                    } else {
                        switch ((mix_dat & mix_mask) ? frgd_mix : bkgd_mix) {
                            case 0:
                                src_dat = bkgd_color;
                                break;
                            case 1:
                                src_dat = frgd_color;
                                break;
                            case 2:
                                src_dat = cpu_dat;
                                break;
                            case 3:
                                src_dat = 0;
                                break;

                            default:
                                break;
                        }
                    }

                    if (s3->accel.multifunc[0xe] & 0x100) {
                        if (s3->accel.multifunc[0xe] & 0x80) {
                            if (src_dat != compare)
                                update = 0;
                            else
                                update = 1;
                        } else {
                            if (src_dat == compare)
                                update = 0;
                            else
                                update = 1;
                        }
                    } else
                        update = 1;

                    if (update) {
                        READ(s3->accel.dest + s3->accel.cx - s3->accel.minus, dest_dat);

                        if (s3_cpu_dest(s3)) {
                            if (vram_mask) {
                                old_dest_dat = dest_dat;
                                MIX
                            }
                        } else {
                            old_dest_dat = dest_dat;
                            MIX
                        }

                        if (s3->accel.cmd & 0x10) {
                            if (!s3->accel.color_16bit_check_pixtrans) {
                                WRITE(s3->accel.dest + s3->accel.cx - s3->accel.minus, dest_dat);
                            }
                        }
                    }
                }

                if (s3->accel.b2e8_pix && s3_cpu_src(s3)) {
                    if (s3->accel.temp_cnt > 0) {
                        s3->accel.temp_cnt--;
                        mix_dat <<= 1;
                        mix_dat |= 1;
                    }
                } else {
                    mix_dat <<= 1;
                    mix_dat |= 1;
                }

                if ((s3->bpp == 0) && !s3->color_16bit)
                    cpu_dat >>= 8;
                else
                    cpu_dat >>= 16;

                if (s3->accel.cmd & 0x20)
                    s3->accel.cx++;
                else
                    s3->accel.cx--;

                s3->accel.cx &= 0xfff;
                s3->accel.sx--;
                if (s3->accel.sx < 0) {
                    s3->accel.sx = s3->accel.maj_axis_pcnt & 0xfff;

                    if (s3->accel.cmd & 0x20)
                        s3->accel.cx -= (s3->accel.sx + 1);
                    else
                        s3->accel.cx += (s3->accel.sx + 1);

                    if (s3->accel.cmd & 0x80)
                        s3->accel.cy++;
                    else
                        s3->accel.cy--;

                    s3->accel.cy &= 0xfff;
                    s3->accel.dest = dstbase + s3->accel.cy * s3->width;

                    s3->accel.sy--;

                    if (cpu_input) {
                        if (s3->accel.sy < 0) {
                            if ((s3->bpp == 0) && s3->color_16bit)  {
                                if (s3->accel.rd_mask_16bit_check) {
                                    if (s3->accel.minus)
                                        s3->accel.color_16bit_check = 0;
                                    else
                                        s3->accel.color_16bit_check = 1;

                                    if ((s3->accel.cmd == 0x41b3) && (frgd_mix == 0))
                                        s3->accel.color_16bit_check = 0;
                                }
                            }
                        }
                        if (s3->accel.b2e8_pix) {
                            s3->accel.cur_x = s3->accel.cx;
                            s3->accel.cur_y = s3->accel.cy;
                        }
                        return;
                    }
                    if (s3->accel.sy < 0) {
                        if ((s3->bpp == 0) && s3->color_16bit)  {
                            if (s3->accel.rd_mask_16bit_check) {
                                if (s3->accel.minus)
                                    s3->accel.color_16bit_check = 0;
                                else
                                    s3->accel.color_16bit_check = 1;
                            }
                        }
                        s3->accel.cur_x = s3->accel.cx;
                        s3->accel.cur_y = s3->accel.cy;
                        return;
                    }
                }
            }
            break;

        case 3: /*Polygon Fill Solid (Vision868/968 and Trio64 only)*/
            {
                int end_y1;
                int end_y2;

                if (s3->chip != S3_TRIO64 && s3->chip != S3_VISION968 && s3->chip != S3_VISION868)
                    break;

                polygon_setup(s3);

                if ((s3->accel.cmd & 0x100) && !cpu_input)
                    return; /*Wait for data from CPU*/

                end_y1 = s3->accel.desty_axstp;
                end_y2 = s3->accel.desty_axstp2;

                while ((s3->accel.poly_cy < end_y1) && (s3->accel.poly_cy2 < end_y2)) {
                    int y       = s3->accel.poly_cy;
                    int x_count = ABS((s3->accel.poly_cx2 >> 20) - s3->accel.poly_x) + 1;

                    s3->accel.dest = dstbase + y * s3->width;

                    while (x_count-- && count--) {
                        if ((s3->accel.poly_x & 0xfff) >= clip_l && (s3->accel.poly_x & 0xfff) <= clip_r && (s3->accel.poly_cy & 0xfff) >= clip_t && (s3->accel.poly_cy & 0xfff) <= clip_b) {
                            switch (frgd_mix) {
                                case 0:
                                    src_dat = s3->accel.bkgd_color;
                                    break;
                                case 1:
                                    src_dat = s3->accel.frgd_color;
                                    break;
                                case 2:
                                    src_dat = cpu_dat;
                                    break;
                                case 3:
                                    src_dat = 0; /*Not supported?*/
                                    break;

                                default:
                                    break;
                            }

                            if (s3->accel.multifunc[0xe] & 0x100) {
                                if (s3->accel.multifunc[0xe] & 0x80) {
                                    if (src_dat != compare)
                                        update = 0;
                                    else
                                        update = 1;
                                } else {
                                    if (src_dat == compare)
                                        update = 0;
                                    else
                                        update = 1;
                                }
                            } else
                                update = 1;

                            if (update) {
                                READ(s3->accel.dest + s3->accel.poly_x, dest_dat);

                                old_dest_dat = dest_dat;
                                MIX

                                if (s3->accel.cmd & 0x10) {
                                    WRITE(s3->accel.dest + s3->accel.poly_x, dest_dat);
                                }
                            }
                        }
                        if (s3->bpp == 0)
                            cpu_dat >>= 8;
                        else
                            cpu_dat >>= 16;

                        if (s3->accel.poly_x < (s3->accel.poly_cx2 >> 20))
                            s3->accel.poly_x++;
                        else
                            s3->accel.poly_x--;
                    }

                    s3->accel.poly_cx += s3->accel.poly_dx1;
                    s3->accel.poly_cx2 += s3->accel.poly_dx2;
                    s3->accel.poly_x = s3->accel.poly_cx >> 20;

                    s3->accel.poly_cy++;
                    s3->accel.poly_cy2++;

                    if (!count)
                        break;
                }

                s3->accel.cur_x  = s3->accel.poly_cx & 0xfff;
                s3->accel.cur_y  = s3->accel.poly_cy & 0xfff;
                s3->accel.cur_x2 = s3->accel.poly_cx2 & 0xfff;
                s3->accel.cur_y2 = s3->accel.poly_cy2 & 0xfff;
            }
            break;

        case 6: /*BitBlt*/
            if (!cpu_input) { /*!cpu_input is trigger to start operation*/
                s3->accel.minus = 0;
                s3->accel.minus_src_24bpp = 0;
                s3->accel.sx = s3->accel.maj_axis_pcnt & 0xfff;
                s3->accel.sy = s3->accel.multifunc[0] & 0xfff;

                s3->accel.dx = s3->accel.destx_distp & 0xfff;
                s3->accel.dy = s3->accel.desty_axstp & 0xfff;

                s3->accel.cx = s3->accel.cur_x & 0xfff;
                s3->accel.cy = s3->accel.cur_y & 0xfff;

                if ((s3->bpp == 0) && s3->color_16bit) {
                    s3->accel.rd_mask_16bit_check = ((rd_mask & 0xff00) != 0xff00) && rd_mask;
                    if (s3->accel.rd_mask_16bit_check) {
                        if (!(clip_r & 0x400))
                            s3->accel.start = 1;
                        else {
                            if (s3->accel.start) {
                                s3->accel.start = 0;
                                s3->accel.minus = 0x400;
                            } else {
                                s3->accel.start = 0;
                                if (s3->accel.destx_distp & 0x400)
                                    s3->accel.minus = 0x400;
                            }
                        }
                    }
                }

                if (s3->accel.destx_distp & 0x400) {
                    s3_log("BitBLT + 1024 FULLCMD=%04x: frgdcolor=%04x, s=%d,%d, d=%d,%d, frmix=%x, bkmix=%x, pixcntl=%02x.\n", s3->accel.cmd, frgd_color, s3->accel.sx, s3->accel.sy, s3->accel.dx, s3->accel.dy, frgd_mix, bkgd_mix, s3->accel.multifunc[0xa] & 0xc0);
                } else {
                    s3_log("BitBLT + 0 FULLCMD=%04x: frgdcolor=%04x, s=%d,%d, d=%d,%d, frmix=%x, bkmix=%x, pixcntl=%02x.\n", s3->accel.cmd, frgd_color, s3->accel.sx, s3->accel.sy, s3->accel.dx, s3->accel.dy, frgd_mix, bkgd_mix, s3->accel.multifunc[0xa] & 0xc0);
                }
                s3->accel.src  = srcbase + s3->accel.cy * s3->width;
                s3->accel.dest = dstbase + s3->accel.dy * s3->width;
                s3_log("BitBLT: D(%d,%d).\n", s3->accel.dx, s3->accel.dy);
            }

            if ((s3->accel.cmd & 0x100) && !cpu_input)
                return; /*Wait for data from CPU*/

            if ((s3->bpp == 0) && s3->color_16bit) {
                if (s3->accel.rd_mask_16bit_check) {
                    if (s3->accel.minus) {
                        wrt_mask = (s3->accel.wrt_mask_actual[1] << 8);
                        frgd_color = (s3->accel.frgd_color_actual[1] << 8);
                        bkgd_color = (s3->accel.bkgd_color_actual[1] << 8);
                    } else {
                        wrt_mask = s3->accel.wrt_mask_actual[0];
                        frgd_color = s3->accel.frgd_color_actual[0];
                        bkgd_color = s3->accel.bkgd_color_actual[0];
                    }
                    rd_mask &= 0x00ff;
                } else if (!s3->accel.rd_mask_16bit_check && (s3->accel.destx_distp & 0x400))
                    break;
            }

            s3_log("CMDFULL=%04x, FRGDSEL=%x, BKGDSEL=%x, FRGDMIX=%02x, BKGDMIX=%02x, MASKCHECK=%x, RDMASK=%04x, MINUS=%d, WRTMASK=%04X, MIX=%04x, CX=%d, CY=%d, DX=%d, DY=%d, SX=%d, SY=%d, PIXCNTL=%02x, 16BITCOLOR=%x, RDCHECK=%x, CLIPL=%d, CLIPR=%d, OVERFLOW=%d, pitch=%d.\n", s3->accel.cmd, frgd_mix, bkgd_mix, s3->accel.frgd_mix & 0x0f, s3->accel.bkgd_mix & 0x0f, s3->accel.rd_mask_16bit_check, rd_mask, s3->accel.minus, wrt_mask, mix_dat & 0xffff, s3->accel.cx, s3->accel.cy, s3->accel.dx, s3->accel.dy, s3->accel.sx, s3->accel.sy, s3->accel.multifunc[0x0a] & 0xc4, s3->accel.color_16bit_check, s3->accel.rd_mask_16bit_check, clip_l, clip_r, (s3->accel.destx_overflow & 0xc00) == 0xc00, s3->width);

            if (!cpu_input && (frgd_mix == 3) && !vram_mask && !(s3->accel.multifunc[0xe] & 0x100) && ((s3->accel.cmd & 0xa0) == 0xa0) && ((s3->accel.frgd_mix & 0xf) == 7) && ((s3->accel.bkgd_mix & 0xf) == 7)) {
                s3_log("Special BitBLT.\n");
                while (1) {
                    if ((s3->accel.dx >= clip_l) && (s3->accel.dx <= clip_r) && (s3->accel.dy >= clip_t) && (s3->accel.dy <= clip_b)) {
                        READ(s3->accel.src + s3->accel.cx - s3->accel.minus, src_dat);
                        READ(s3->accel.dest + s3->accel.dx - s3->accel.minus, dest_dat);
                        dest_dat = (src_dat & wrt_mask) | (dest_dat & ~wrt_mask);

                        if (s3->accel.cmd & 0x10) {
                            WRITE(s3->accel.dest + s3->accel.dx - s3->accel.minus, dest_dat);
                        }
                    }

                    s3->accel.cx++;
                    s3->accel.dx++;
                    s3->accel.sx--;
                    s3->accel.dx &= 0xfff;

                    if (s3->accel.sx < 0) {
                        s3->accel.cx -= (s3->accel.maj_axis_pcnt & 0xfff) + 1;
                        s3->accel.dx -= (s3->accel.maj_axis_pcnt & 0xfff) + 1;
                        s3->accel.sx = s3->accel.maj_axis_pcnt & 0xfff;

                        s3->accel.cy++;
                        s3->accel.dy++;

                        s3->accel.src  = srcbase + (s3->accel.cy * s3->width);
                        s3->accel.dest = dstbase + (s3->accel.dy * s3->width);

                        s3->accel.sy--;

                        if (s3->accel.sy < 0) {
                            if (s3->accel.rd_mask_16bit_check)  {
                                if (s3->accel.minus)
                                    s3->accel.color_16bit_check = 0;
                                else
                                    s3->accel.color_16bit_check = 1;
                            }
                            s3->accel.destx_distp = s3->accel.dx;
                            s3->accel.desty_axstp = s3->accel.dy;
                            return;
                        }
                    }
                }
            } else {
                s3_log("Normal blit, srcbase=%08x, dstbase=%08x, full=%04x, wrt_mask=%08x, extmultifunc0e=%03x, frgdmixval=%02x.\n", srcbase, dstbase, s3->accel.cmd, wrt_mask, s3->accel.multifunc[0x0e] & 0x180, s3->accel.frgd_mix);
                while (count-- && (s3->accel.sy >= 0)) {
                    if ((s3->accel.dx >= clip_l) && (s3->accel.dx <= clip_r) && (s3->accel.dy >= clip_t) && (s3->accel.dy <= clip_b)) {
                        if (vram_mask && (s3->accel.cmd & 0x10)) {
                            READ(s3->accel.src + s3->accel.cx, mix_dat);
                            mix_dat = ((mix_dat & rd_mask) == rd_mask);
                            mix_dat = mix_dat ? mix_mask : 0;
                        }
                        switch ((mix_dat & mix_mask) ? frgd_mix : bkgd_mix) {
                            case 0:
                                src_dat = bkgd_color;
                                break;
                            case 1:
                                src_dat = frgd_color;
                                break;
                            case 2:
                                src_dat = cpu_dat;
                                break;
                            case 3:
                                READ(s3->accel.src + s3->accel.cx - s3->accel.minus, src_dat);
                                if (vram_mask && (s3->accel.cmd & 0x10))
                                    src_dat = ((src_dat & rd_mask) == rd_mask);
                                break;

                            default:
                                break;
                        }

                        if (s3->accel.multifunc[0xe] & 0x100) {
                            if (s3->accel.multifunc[0xe] & 0x80) {
                                if (src_dat != compare)
                                    update = 0;
                                else
                                    update = 1;
                            } else {
                                if (src_dat == compare)
                                    update = 0;
                                else
                                    update = 1;
                            }
                        } else
                            update = 1;

                        if (update) {
                            READ(s3->accel.dest + s3->accel.dx - s3->accel.minus, dest_dat);

                            old_dest_dat = dest_dat;
                            MIX

                            if ((!(s3->accel.cmd & 0x10) && vram_mask) || (s3->accel.cmd & 0x10)) {
                                WRITE(s3->accel.dest + s3->accel.dx - s3->accel.minus, dest_dat);
                            }
                        }
                    }

                    mix_dat <<= 1;
                    mix_dat |= 1;

                    if ((s3->bpp == 0) && !s3->color_16bit)
                        cpu_dat >>= 8;
                    else
                        cpu_dat >>= 16;

                    if (s3->accel.cmd & 0x20) {
                        s3->accel.cx++;
                        s3->accel.dx++;
                    } else {
                        s3->accel.cx--;
                        s3->accel.dx--;
                    }
                    if (s3->accel.rd_mask_16bit_check)
                        s3->accel.dx &= 0x7ff;
                    else
                        s3->accel.dx &= 0xfff;

                    s3->accel.sx--;
                    if (s3->accel.sx < 0) {
                        if (s3->accel.cmd & 0x20) {
                            s3->accel.cx -= ((s3->accel.maj_axis_pcnt & 0xfff) + 1);
                            s3->accel.dx -= ((s3->accel.maj_axis_pcnt & 0xfff) + 1);
                        } else {
                            s3->accel.cx += ((s3->accel.maj_axis_pcnt & 0xfff) + 1);
                            s3->accel.dx += ((s3->accel.maj_axis_pcnt & 0xfff) + 1);
                        }
                        s3->accel.sx = s3->accel.maj_axis_pcnt & 0xfff;

                        if (s3->accel.cmd & 0x80) {
                            s3->accel.cy++;
                            s3->accel.dy++;
                        } else {
                            s3->accel.cy--;
                            s3->accel.dy--;
                        }

                        s3->accel.src  = srcbase + s3->accel.cy * s3->width;
                        s3->accel.dest = dstbase + s3->accel.dy * s3->width;

                        s3->accel.sy--;

                        if (cpu_input) {
                            if (s3->accel.sy < 0) {
                                if (s3->accel.rd_mask_16bit_check)  {
                                    if (s3->accel.minus)
                                        s3->accel.color_16bit_check = 0;
                                    else
                                        s3->accel.color_16bit_check = 1;
                                }
                            }
                            return;
                        }

                        if (s3->accel.sy < 0) {
                            if (s3->accel.rd_mask_16bit_check)  {
                                if (s3->accel.minus)
                                    s3->accel.color_16bit_check = 0;
                                else
                                    s3->accel.color_16bit_check = 1;

                                if (s3->accel.mix_dat_upper && !vram_mask && (frgd_mix == 3))
                                    s3->accel.color_16bit_check = 0;
                            }
                            s3->accel.destx_distp = s3->accel.dx;
                            s3->accel.desty_axstp = s3->accel.dy;
                            return;
                        }
                    }
                }
            }
            break;

        case 7: /*Pattern fill - BitBlt but with source limited to 8x8*/
            if (!cpu_input) /*!cpu_input is trigger to start operation*/
            {
                s3->accel.sx = s3->accel.maj_axis_pcnt & 0xfff;
                s3->accel.sy = s3->accel.multifunc[0] & 0xfff;

                s3->accel.dx = s3->accel.destx_distp & 0xfff;
                s3->accel.dy = s3->accel.desty_axstp & 0xfff;

                s3->accel.cx = s3->accel.cur_x & 0xfff;
                s3->accel.cy = s3->accel.cur_y & 0xfff;

                /*Align source with destination*/
                s3->accel.pattern = (s3->accel.cy * s3->width) + s3->accel.cx;
                s3->accel.dest    = dstbase + s3->accel.dy * s3->width;

                s3->accel.cx = s3->accel.dx & 7;
                s3->accel.cy = s3->accel.dy & 7;

                s3->accel.src = srcbase + s3->accel.pattern + (s3->accel.cy * s3->width);
            }

            if ((s3->accel.cmd & 0x100) && !cpu_input)
                return; /*Wait for data from CPU*/

            while (count-- && (s3->accel.sy >= 0)) {
                if ((s3->accel.dx >= clip_l) && (s3->accel.dx <= clip_r) && (s3->accel.dy >= clip_t) && (s3->accel.dy <= clip_b)) {
                    if (vram_mask) {
                        READ(s3->accel.src + s3->accel.cx, mix_dat);
                        mix_dat = ((mix_dat & rd_mask) == rd_mask);
                        mix_dat = mix_dat ? mix_mask : 0;
                    }
                    switch ((mix_dat & mix_mask) ? frgd_mix : bkgd_mix) {
                        case 0:
                            src_dat = bkgd_color;
                            break;
                        case 1:
                            src_dat = frgd_color;
                            break;
                        case 2:
                            src_dat = cpu_dat;
                            break;
                        case 3:
                            READ(s3->accel.src + s3->accel.cx, src_dat);
                            if (vram_mask)
                                src_dat = ((src_dat & rd_mask) == rd_mask);
                            break;

                        default:
                            break;
                    }

                    if (s3->accel.multifunc[0xe] & 0x100) {
                        if (s3->accel.multifunc[0xe] & 0x80) {
                            if (src_dat != compare)
                                update = 0;
                            else
                                update = 1;
                        } else {
                            if (src_dat == compare)
                                update = 0;
                            else
                                update = 1;
                        }
                    } else
                        update = 1;

                    if (update) {
                        READ(s3->accel.dest + s3->accel.dx, dest_dat);

                        old_dest_dat = dest_dat;
                        MIX

                        if (s3->accel.cmd & 0x10) {
                            WRITE(s3->accel.dest + s3->accel.dx, dest_dat);
                        }
                    }
                }

                mix_dat <<= 1;
                mix_dat |= 1;
                if ((s3->bpp == 0) && !s3->color_16bit)
                    cpu_dat >>= 8;
                else
                    cpu_dat >>= 16;

                if (s3->accel.cmd & 0x20) {
                    s3->accel.cx = ((s3->accel.cx + 1) & 7) | (s3->accel.cx & ~7);
                    s3->accel.dx++;
                } else {
                    s3->accel.cx = ((s3->accel.cx - 1) & 7) | (s3->accel.cx & ~7);
                    s3->accel.dx--;
                }
                s3->accel.dx &= 0xfff;
                s3->accel.sx--;
                if (s3->accel.sx < 0) {
                    if (s3->accel.cmd & 0x20) {
                        s3->accel.cx = ((s3->accel.cx - (((s3->accel.maj_axis_pcnt & 0xfff) + 1))) & 7) | (s3->accel.cx & ~7);
                        s3->accel.dx -= ((s3->accel.maj_axis_pcnt & 0xfff) + 1);
                    } else {
                        s3->accel.cx = ((s3->accel.cx + (((s3->accel.maj_axis_pcnt & 0xfff) + 1))) & 7) | (s3->accel.cx & ~7);
                        s3->accel.dx += ((s3->accel.maj_axis_pcnt & 0xfff) + 1);
                    }
                    s3->accel.sx = s3->accel.maj_axis_pcnt & 0xfff;

                    if (s3->accel.cmd & 0x80) {
                        s3->accel.cy = ((s3->accel.cy + 1) & 7) | (s3->accel.cy & ~7);
                        s3->accel.dy++;
                    } else {
                        s3->accel.cy = ((s3->accel.cy - 1) & 7) | (s3->accel.cy & ~7);
                        s3->accel.dy--;
                    }

                    s3->accel.src  = srcbase + s3->accel.pattern + (s3->accel.cy * s3->width);
                    s3->accel.dest = dstbase + s3->accel.dy * s3->width;

                    s3->accel.sy--;

                    if (cpu_input)
                        return;

                    if (s3->accel.sy < 0) {
                        s3->accel.destx_distp = s3->accel.dx;
                        s3->accel.desty_axstp = s3->accel.dy;
                        return;
                    }
                }
            }
            break;

        case 9: /*Polyline/2-Point Line (Vision868/968 and Trio64 only)*/
            {
                int error;

                if (s3->chip != S3_TRIO64 && s3->chip != S3_VISION968 && s3->chip != S3_VISION868)
                    break;

                if (!cpu_input) {
                    s3->accel.dx = ABS(s3->accel.destx_distp - s3->accel.cur_x);
                    if (s3->accel.destx_distp & 0x1000)
                        s3->accel.dx |= ~0xfff;
                    s3->accel.dy = ABS(s3->accel.desty_axstp - s3->accel.cur_y);
                    if (s3->accel.desty_axstp & 0x1000)
                        s3->accel.dy |= ~0xfff;

                    s3->accel.cx = s3->accel.cur_x & 0xfff;
                    s3->accel.cy = s3->accel.cur_y & 0xfff;
                }

                if ((s3->accel.cmd & 0x100) && !cpu_input)
                    return; /*Wait for data from CPU*/

                if (s3->accel.dx > s3->accel.dy) {
                    error = s3->accel.dx / 2;
                    while (s3->accel.cx != s3->accel.destx_distp && count--) {
                        if ((s3->accel.cx & 0xfff) >= clip_l && (s3->accel.cx & 0xfff) <= clip_r && (s3->accel.cy & 0xfff) >= clip_t && (s3->accel.cy & 0xfff) <= clip_b) {
                            src_dat = s3->accel.frgd_color;

                            if (s3->accel.multifunc[0xe] & 0x100) {
                                if (s3->accel.multifunc[0xe] & 0x80) {
                                    if (src_dat != compare)
                                        update = 0;
                                    else
                                        update = 1;
                                } else {
                                    if (src_dat == compare)
                                        update = 0;
                                    else
                                        update = 1;
                                }
                            } else
                                update = 1;

                            if (update) {
                                READ((s3->accel.cy * s3->width) + s3->accel.cx, dest_dat);

                                old_dest_dat = dest_dat;
                                MIX

                                if (s3->accel.cmd & 0x10) {
                                    WRITE((s3->accel.cy * s3->width) + s3->accel.cx, dest_dat);
                                }
                            }
                        }

                        error -= s3->accel.dy;
                        if (error < 0) {
                            error += s3->accel.dx;
                            if (s3->accel.desty_axstp > s3->accel.cur_y)
                                s3->accel.cy++;
                            else
                                s3->accel.cy--;

                            s3->accel.cy &= 0xfff;
                        }
                        if (s3->accel.destx_distp > s3->accel.cur_x)
                            s3->accel.cx++;
                        else
                            s3->accel.cx--;

                        s3->accel.cx &= 0xfff;
                    }
                } else {
                    error = s3->accel.dy / 2;
                    while (s3->accel.cy != s3->accel.desty_axstp && count--) {
                        if ((s3->accel.cx & 0xfff) >= clip_l && (s3->accel.cx & 0xfff) <= clip_r && (s3->accel.cy & 0xfff) >= clip_t && (s3->accel.cy & 0xfff) <= clip_b) {
                            src_dat = s3->accel.frgd_color;

                            if (s3->accel.multifunc[0xe] & 0x100) {
                                if (s3->accel.multifunc[0xe] & 0x80) {
                                    if (src_dat != compare)
                                        update = 0;
                                    else
                                        update = 1;
                                } else {
                                    if (src_dat == compare)
                                        update = 0;
                                    else
                                        update = 1;
                                }
                            } else
                                update = 1;

                            if (update) {
                                READ((s3->accel.cy * s3->width) + s3->accel.cx, dest_dat);

                                old_dest_dat = dest_dat;
                                MIX

                                if (s3->accel.cmd & 0x10) {
                                    WRITE((s3->accel.cy * s3->width) + s3->accel.cx, dest_dat);
                                }
                            }
                        }

                        error -= s3->accel.dx;
                        if (error < 0) {
                            error += s3->accel.dy;
                            if (s3->accel.destx_distp > s3->accel.cur_x)
                                s3->accel.cx++;
                            else
                                s3->accel.cx--;

                            s3->accel.cx &= 0xfff;
                        }
                        if (s3->accel.desty_axstp > s3->accel.cur_y)
                            s3->accel.cy++;
                        else
                            s3->accel.cy--;

                        s3->accel.cy &= 0xfff;
                    }
                }
                s3->accel.cur_x = s3->accel.cx;
                s3->accel.cur_y = s3->accel.cy;
            }
            break;

        case 11: /*Polygon Fill Pattern (Vision868/968 and Trio64 only)*/
            {
                int end_y1;
                int end_y2;

                if (s3->chip != S3_TRIO64 && s3->chip != S3_VISION968 && s3->chip != S3_VISION868)
                    break;

                polygon_setup(s3);

                if ((s3->accel.cmd & 0x100) && !cpu_input)
                    return; /*Wait for data from CPU*/

                end_y1 = s3->accel.desty_axstp;
                end_y2 = s3->accel.desty_axstp2;

                while ((s3->accel.poly_cy < end_y1) && (s3->accel.poly_cy2 < end_y2)) {
                    int y       = s3->accel.poly_cy;
                    int x_count = ABS((s3->accel.poly_cx2 >> 20) - s3->accel.poly_x) + 1;

                    s3->accel.src  = srcbase + s3->accel.pattern + ((y & 7) * s3->width);
                    s3->accel.dest = dstbase + y * s3->width;

                    while (x_count-- && count--) {
                        int pat_x = s3->accel.poly_x & 7;

                        if ((s3->accel.poly_x & 0xfff) >= clip_l && (s3->accel.poly_x & 0xfff) <= clip_r && (s3->accel.poly_cy & 0xfff) >= clip_t && (s3->accel.poly_cy & 0xfff) <= clip_b) {
                            if (vram_mask) {
                                READ(s3->accel.src + pat_x, mix_dat);
                                mix_dat = ((mix_dat & rd_mask) == rd_mask);
                                mix_dat = mix_dat ? mix_mask : 0;
                            }
                            switch ((mix_dat & mix_mask) ? frgd_mix : bkgd_mix) {
                                case 0:
                                    src_dat = s3->accel.bkgd_color;
                                    break;
                                case 1:
                                    src_dat = s3->accel.frgd_color;
                                    break;
                                case 2:
                                    src_dat = cpu_dat;
                                    break;
                                case 3:
                                    READ(s3->accel.src + pat_x, src_dat);
                                    if (vram_mask)
                                        src_dat = ((src_dat & rd_mask) == rd_mask);
                                    break;

                                default:
                                    break;
                            }

                            if (s3->accel.multifunc[0xe] & 0x100) {
                                if (s3->accel.multifunc[0xe] & 0x80) {
                                    if (src_dat != compare)
                                        update = 0;
                                    else
                                        update = 1;
                                } else {
                                    if (src_dat == compare)
                                        update = 0;
                                    else
                                        update = 1;
                                }
                            } else
                                update = 1;

                            if (update) {
                                READ(s3->accel.dest + s3->accel.poly_x, dest_dat);

                                old_dest_dat = dest_dat;
                                MIX

                                if (s3->accel.cmd & 0x10) {
                                    WRITE(s3->accel.dest + s3->accel.poly_x, dest_dat);
                                }
                            }
                        }
                        if (s3->bpp == 0)
                            cpu_dat >>= 8;
                        else
                            cpu_dat >>= 16;

                        mix_dat <<= 1;
                        mix_dat |= 1;

                        if (s3->accel.poly_x < (s3->accel.poly_cx2 >> 20))
                            s3->accel.poly_x++;
                        else
                            s3->accel.poly_x--;
                    }

                    s3->accel.poly_cx += s3->accel.poly_dx1;
                    s3->accel.poly_cx2 += s3->accel.poly_dx2;
                    s3->accel.poly_x = s3->accel.poly_cx >> 20;

                    s3->accel.poly_cy++;
                    s3->accel.poly_cy2++;

                    if (!count)
                        break;
                }

                s3->accel.cur_x  = s3->accel.poly_cx & 0xfff;
                s3->accel.cur_y  = s3->accel.poly_cy & 0xfff;
                s3->accel.cur_x2 = s3->accel.poly_cx2 & 0xfff;
                s3->accel.cur_y2 = s3->accel.poly_cy & 0xfff;
            }
            break;

        case 14: /*ROPBlt (Vision868/968 only)*/
            if (s3->chip != S3_VISION968 && s3->chip != S3_VISION868)
                break;

            if (!cpu_input) /*!cpu_input is trigger to start operation*/
            {
                s3->accel.sx = s3->accel.maj_axis_pcnt & 0xfff;
                s3->accel.sy = s3->accel.multifunc[0] & 0xfff;

                s3->accel.dx = s3->accel.destx_distp & 0xfff;
                if (s3->accel.destx_distp & 0x1000)
                    s3->accel.dx |= ~0xfff;
                s3->accel.dy = s3->accel.desty_axstp & 0xfff;
                if (s3->accel.desty_axstp & 0x1000)
                    s3->accel.dy |= ~0xfff;

                s3->accel.cx = s3->accel.cur_x & 0xfff;
                s3->accel.cy = s3->accel.cur_y & 0xfff;

                s3->accel.px = s3->accel.pat_x & 0xfff;
                s3->accel.py = s3->accel.pat_y & 0xfff;

                s3->accel.dest    = dstbase + (s3->accel.dy * s3->width);
                s3->accel.src     = srcbase + (s3->accel.cy * s3->width);
                s3->accel.pattern = (s3->accel.py * s3->width);
            }

            if ((s3->accel.cmd & 0x100) && !cpu_input)
                return; /*Wait for data from CPU*/

            while (count-- && s3->accel.sy >= 0) {
                if ((s3->accel.dx & 0xfff) >= clip_l && (s3->accel.dx & 0xfff) <= clip_r && (s3->accel.dy & 0xfff) >= clip_t && (s3->accel.dy & 0xfff) <= clip_b) {
                    switch ((mix_dat & mix_mask) ? frgd_mix : bkgd_mix) {
                        case 0:
                            src_dat = s3->accel.bkgd_color;
                            break;
                        case 1:
                            src_dat = s3->accel.frgd_color;
                            break;
                        case 2:
                            src_dat = cpu_dat;
                            break;
                        case 3:
                            READ(s3->accel.src + s3->accel.cx, src_dat);
                            break;

                        default:
                            break;
                    }

                    if (s3->accel.ropmix & 0x100) {
                        switch ((mix_dat & mix_mask) ? frgd_mix : bkgd_mix) {
                            case 0:
                                pat_dat = s3->accel.pat_bg_color;
                                break;
                            case 1:
                                pat_dat = s3->accel.pat_fg_color;
                                break;
                            case 2:
                                pat_dat = cpu_dat;
                                break;
                            case 3:
                                READ(s3->accel.pattern + s3->accel.px, pat_dat);
                                break;

                            default:
                                break;
                        }
                    } else {
                        switch ((mix_dat & mix_mask) ? frgd_mix : bkgd_mix) {
                            case 0:
                                pat_dat = s3->accel.bkgd_color;
                                break;
                            case 1:
                                pat_dat = s3->accel.frgd_color;
                                break;
                            case 2:
                                pat_dat = cpu_dat;
                                break;
                            case 3:
                                READ(s3->accel.pattern + s3->accel.px, pat_dat);
                                break;

                            default:
                                break;
                        }
                    }

                    if (s3->accel.multifunc[0xe] & 0x100) {
                        if (s3->accel.multifunc[0xe] & 0x80) {
                            if (src_dat != compare)
                                update = 0;
                            else
                                update = 1;
                        } else {
                            if (src_dat == compare)
                                update = 0;
                            else
                                update = 1;
                        }
                    } else
                        update = 1;

                    if (update) {
                        READ(s3->accel.dest + s3->accel.dx, dest_dat);

                        ROPMIX

                        if (s3->accel.cmd & 0x10) {
                            WRITE(s3->accel.dest + s3->accel.dx, out);
                        }
                    }
                }

                mix_dat <<= 1;
                mix_dat |= 1;
                if (s3->bpp == 0)
                    cpu_dat >>= 8;
                else
                    cpu_dat >>= 16;

                if (s3->accel.cmd & 0x20) {
                    s3->accel.cx++;
                    s3->accel.dx++;
                    s3->accel.px++;
                } else {
                    s3->accel.cx--;
                    s3->accel.dx--;
                    s3->accel.px--;
                }
                s3->accel.sx--;
                if (s3->accel.sx < 0) {
                    if (s3->accel.cmd & 0x20) {
                        s3->accel.cx -= ((s3->accel.maj_axis_pcnt & 0xfff) + 1);
                        s3->accel.dx -= ((s3->accel.maj_axis_pcnt & 0xfff) + 1);
                        s3->accel.px -= ((s3->accel.maj_axis_pcnt & 0xfff) + 1);
                    } else {
                        s3->accel.cx += ((s3->accel.maj_axis_pcnt & 0xfff) + 1);
                        s3->accel.dx += ((s3->accel.maj_axis_pcnt & 0xfff) + 1);
                        s3->accel.px += ((s3->accel.maj_axis_pcnt & 0xfff) + 1);
                    }
                    s3->accel.sx = s3->accel.maj_axis_pcnt & 0xfff;

                    if (s3->accel.cmd & 0x80) {
                        s3->accel.cy++;
                        s3->accel.dy++;
                        s3->accel.py++;
                    } else {
                        s3->accel.cy--;
                        s3->accel.dy--;
                        s3->accel.py--;
                    }
                    s3->accel.src     = srcbase + (s3->accel.cy * s3->width);
                    s3->accel.dest    = dstbase + (s3->accel.dy * s3->width);
                    s3->accel.pattern = (s3->accel.py * s3->width);

                    s3->accel.sy--;

                    if (cpu_input /* && (s3->accel.multifunc[0xa] & 0xc0) == 0x80*/)
                        return;
                    if (s3->accel.sy < 0) {
                        return;
                    }
                }
            }
            break;

        default:
            break;
    }
}

static uint8_t
s3_pci_read(UNUSED(int func), int addr, void *priv)
{
    const s3_t   *s3   = (s3_t *) priv;
    const svga_t *svga = &s3->svga;

    switch (addr) {
        case 0x00:
            return 0x33; /*'S3'*/
        case 0x01:
            return 0x53;

        case 0x02:
            return s3->id_ext_pci;
        case 0x03:
            return (s3->chip == S3_TRIO64V2) ? 0x89 : 0x88;

        case PCI_REG_COMMAND:
            if (s3->chip == S3_VISION968 || s3->chip == S3_VISION868)
                return s3->pci_regs[PCI_REG_COMMAND] | 0x80; /*Respond to IO and memory accesses*/
            else
                return s3->pci_regs[PCI_REG_COMMAND]; /*Respond to IO and memory accesses*/
            break;

        case 0x07:
            return (s3->chip == S3_TRIO64V2) ? (s3->pci_regs[0x07] & 0x36) : (1 << 1); /*Medium DEVSEL timing*/

        case 0x08:
            switch (s3->chip) { /*Revision ID*/
                case S3_TRIO64V:
                    return 0x40;
                case S3_TRIO64V2:
                    return 0x16; /*Confirmed on an onboard 64V2/DX*/
                default:
                    return 0x00;
            }
            break;
        case 0x09:
            return 0; /*Programming interface*/

        case 0x0a:
            if (s3->chip >= S3_TRIO32 || s3->chip == S3_VISION968 || s3->chip == S3_VISION868)
                return 0x00; /*Supports VGA interface*/
            else
                return 0x01;
            break;
        case 0x0b:
            if (s3->chip >= S3_TRIO32 || s3->chip == S3_VISION968 || s3->chip == S3_VISION868)
                return 0x03;
            else
                return 0x00;
            break;

        case 0x0d:
            return (s3->chip == S3_TRIO64V2) ? (s3->pci_regs[0x0d] & 0xf8) : 0x00;

        case 0x12:
            return ((s3->chip == S3_VISION868) || (s3->chip == S3_VISION968) || (s3->chip >= S3_TRIO64V)) ? 0x00 :
                       (svga->crtc[0x5a] & 0x80);

        case 0x13:
            return ((s3->chip == S3_VISION868) || (s3->chip == S3_VISION968) || (s3->chip >= S3_TRIO64V)) ?
                       (svga->crtc[0x59] & 0xfc) : svga->crtc[0x59];

        case 0x30:
            return s3->has_bios ? (s3->pci_regs[0x30] & 0x01) : 0x00; /*BIOS ROM address*/
        case 0x31:
            return 0x00;
        case 0x32:
            return s3->has_bios ? s3->pci_regs[0x32] : 0x00;
        case 0x33:
            return s3->has_bios ? s3->pci_regs[0x33] : 0x00;

        case 0x3c:
            return s3->int_line;
        case 0x3d:
            return PCI_INTA;

        case 0x3e:
            return (s3->chip == S3_TRIO64V2) ? 0x04 : 0x00;
        case 0x3f:
            return (s3->chip == S3_TRIO64V2) ? 0xff : 0x00;

        default:
            break;
    }
    return 0;
}

static void
s3_pci_write(UNUSED(int func), int addr, uint8_t val, void *priv)
{
    s3_t   *s3   = (s3_t *) priv;
    svga_t *svga = &s3->svga;

    switch (addr) {
        case 0x00:
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x08:
        case 0x09:
        case 0x0a:
        case 0x0b:
        case 0x3d:
        case 0x3e:
        case 0x3f:
            if (s3->chip == S3_TRIO64V2)
                return;
            break;

        case PCI_REG_COMMAND:
            if (val & PCI_COMMAND_IO) {
                s3_io_set(s3);
            } else
                s3_io_remove(s3);

            if (s3->chip >= S3_TRIO64V)
                s3->pci_regs[PCI_REG_COMMAND] = (val & 0x27);
            else
                s3->pci_regs[PCI_REG_COMMAND] = (val & 0x23);

            s3_updatemapping(s3);
            return;

        case 0x07:
            if (s3->chip == S3_TRIO64V2) {
                s3->pci_regs[0x07] &= ~(val & 0x30);
                return;
            }
            break;

        case 0x0d:
            if (s3->chip == S3_TRIO64V2) {
                s3->pci_regs[0x0d] = val & 0xf8;
                return;
            }
            break;

        case 0x12:
            if (s3->chip < S3_TRIO64V) {
                svga->crtc[0x5a] = val & 0x80;
                s3_updatemapping(s3);
            }
            break;

        case 0x13:
            svga->crtc[0x59] = (s3->chip >= S3_TRIO64V) ? (val & 0xfc) : val;
            s3_updatemapping(s3);
            break;

        case 0x30:
        case 0x32:
        case 0x33:
            if (!s3->has_bios)
                return;
            s3->pci_regs[addr] = val;
            if (s3->pci_regs[0x30] & 0x01) {
                uint32_t biosaddr = (s3->pci_regs[0x32] << 16) | (s3->pci_regs[0x33] << 24);
                mem_mapping_set_addr(&s3->bios_rom.mapping, biosaddr, 0x8000);
            } else {
                mem_mapping_disable(&s3->bios_rom.mapping);
            }
            return;

        case 0x3c:
            s3->int_line = val;
            return;

        default:
            break;
    }
}

static void
fifo_thread(void *param)
{
    s3_t    *s3 = (s3_t *) param;
    uint64_t start_time;
    uint64_t end_time;

    while (s3->fifo_thread_run) {
        thread_set_event(s3->fifo_not_full_event);
        thread_wait_event(s3->wake_fifo_thread, -1);
        thread_reset_event(s3->wake_fifo_thread);
        s3->blitter_busy = 1;
        while (!FIFO_EMPTY) {
            start_time         = plat_timer_read();
            fifo_entry_t *fifo = &s3->fifo[s3->fifo_read_idx & FIFO_MASK];

            switch (fifo->addr_type & FIFO_TYPE) {
                case FIFO_WRITE_BYTE:
                    s3_accel_write_fifo(s3, fifo->addr_type & FIFO_ADDR, fifo->val);
                    break;
                case FIFO_WRITE_WORD:
                    s3_accel_write_fifo_w(s3, fifo->addr_type & FIFO_ADDR, fifo->val);
                    break;
                case FIFO_WRITE_DWORD:
                    s3_accel_write_fifo_l(s3, fifo->addr_type & FIFO_ADDR, fifo->val);
                    break;
                case FIFO_OUT_BYTE:
                    s3_accel_out_fifo(s3, fifo->addr_type & FIFO_ADDR, fifo->val);
                    break;
                case FIFO_OUT_WORD:
                    s3_accel_out_fifo_w(s3, fifo->addr_type & FIFO_ADDR, fifo->val);
                    break;
                case FIFO_OUT_DWORD:
                    s3_accel_out_fifo_l(s3, fifo->addr_type & FIFO_ADDR, fifo->val);
                    break;

                default:
                    break;
            }

            s3->fifo_read_idx++;
            fifo->addr_type = FIFO_INVALID;

            if (FIFO_ENTRIES > 0xe000)
                thread_set_event(s3->fifo_not_full_event);

            end_time = plat_timer_read();
            s3->blitter_time += (end_time - start_time);
        }
        s3->blitter_busy = 0;
        s3->subsys_stat |= INT_FIFO_EMP;
        s3_update_irqs(s3);
    }
}

static int vram_sizes[] = {
    7, /*512 kB*/
    6, /*1 MB*/
    4, /*2 MB*/
    0,
    0, /*4 MB*/
    0,
    0, /*6 MB*/
    0,
    3 /*8 MB*/
};

static void
s3_disable_handlers(s3_t *s3)
{
    if (s3->pci)
        s3_io_remove(s3);

    mem_mapping_disable(&s3->linear_mapping);
    mem_mapping_disable(&s3->mmio_mapping);
    mem_mapping_disable(&s3->new_mmio_mapping);
    mem_mapping_disable(&s3->svga.mapping);
    if (s3->pci)
        mem_mapping_disable(&s3->bios_rom.mapping);

    /* Save all the mappings and the timers because they are part of linked lists. */
    reset_state->linear_mapping   = s3->linear_mapping;
    reset_state->mmio_mapping     = s3->mmio_mapping;
    reset_state->new_mmio_mapping = s3->new_mmio_mapping;
    reset_state->svga.mapping     = s3->svga.mapping;
    reset_state->bios_rom.mapping = s3->bios_rom.mapping;

    reset_state->svga.timer       = s3->svga.timer;

    memset(s3->svga.vram, 0x00, s3->svga.vram_max + 8);
    memset(s3->svga.changedvram, 0x00, (s3->svga.vram_max >> 12) + 1);
}

static void
s3_reset(void *priv)
{
    s3_t *s3 = (s3_t *) priv;

    if (reset_state != NULL) {
        s3->accel.multifunc[0xe] &= ~(0x200 | 0x10);
        s3_disable_handlers(s3);
        s3->force_busy = 0;
        s3->blitter_busy = 0;
        s3->fifo_read_idx = 0;
        s3->fifo_write_idx = 0;
        if (s3->pci)
            reset_state->pci_slot = s3->pci_slot;

        *s3 = *reset_state;
    }
}

static void *
s3_init(const device_t *info)
{
    const char *bios_fn;
    int         chip;
    int         stepping;
    s3_t       *s3    = calloc(1, sizeof(s3_t));
    reset_state       = calloc(1, sizeof(s3_t));
    svga_t     *svga  = &s3->svga;
    int         vram;
    uint32_t    vram_size;

    switch (info->local) {
        case S3_ORCHID_86C911:
            bios_fn = ROM_ORCHID_86C911;
            chip    = S3_86C911;
            video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_86c911);
            break;
        case S3_DIAMOND_STEALTH_VRAM:
            bios_fn = ROM_DIAMOND_STEALTH_VRAM;
            chip    = S3_86C911;
            video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_86c911);
            break;
        case S3_AMI_86C924:
            bios_fn = ROM_AMI_86C924;
            chip    = S3_86C924;
            video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_86c911);
            break;
        case S3_SPEA_MIRAGE_86C801:
            bios_fn = ROM_SPEA_MIRAGE_86C801;
            chip    = S3_86C801;
            video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_86c801);
            break;
        case S3_WINNER1000_805:
            bios_fn = ROM_WINNER1000_805;
            chip    = S3_86C801;
            video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_86c801);
            break;
        case S3_86C805_ONBOARD:
            bios_fn = NULL;
            chip    = S3_86C805;
            video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_86c805);
            break;
        case S3_SPEA_MIRAGE_86C805:
            bios_fn = ROM_SPEA_MIRAGE_86C805;
            chip    = S3_86C805;
            video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_86c805);
            break;
        case S3_MIROCRYSTAL8S_805:
            bios_fn = ROM_MIROCRYSTAL8S_805;
            chip    = S3_86C805;
            video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_86c805);
            break;
        case S3_MIROCRYSTAL10SD_805:
            bios_fn = ROM_MIROCRYSTAL10SD_805;
            chip    = S3_86C805;
            video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_86c805);
            break;
        case S3_PHOENIX_86C801:
            bios_fn = ROM_PHOENIX_86C80X;
            chip    = S3_86C801;
            video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_86c801);
            break;
        case S3_PHOENIX_86C805:
            bios_fn = ROM_PHOENIX_86C80X;
            chip    = S3_86C805;
            video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_86c805);
            break;
        case S3_METHEUS_86C928:
            bios_fn = ROM_METHEUS_86C928;
            chip    = S3_86C928;
            if (info->flags & DEVICE_VLB)
                video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_86c805);
            else
                video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_86c801);
            break;
        case S3_SPEA_MERCURY_LITE_PCI:
            bios_fn = ROM_SPEA_MERCURY_LITE_PCI;
            chip    = S3_86C928PCI;
            video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_86c928pci);
            break;
        case S3_MIROCRYSTAL20SD_864:
            bios_fn = ROM_MIROCRYSTAL20SD_864_VLB;
            chip    = S3_VISION864;
            video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_vision864_vlb);
            break;
        case S3_PARADISE_BAHAMAS64:
            bios_fn = ROM_PARADISE_BAHAMAS64;
            chip    = S3_VISION864;
            if (info->flags & DEVICE_PCI)
                video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_vision864_pci);
            else
                video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_vision864_vlb);
            break;
        case S3_PHOENIX_VISION864:
            bios_fn = ROM_PHOENIX_VISION864;
            chip    = S3_VISION864;
            if (info->flags & DEVICE_PCI)
                video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_vision864_pci);
            else
                video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_vision864_vlb);
            break;
        case S3_NUMBER9_9FX_531:
            bios_fn = ROM_NUMBER9_9FX_531;
            chip    = S3_VISION868;
            video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_vision868_pci);
            break;
        case S3_PHOENIX_VISION868:
            bios_fn = ROM_PHOENIX_VISION868;
            chip    = S3_VISION868;
            video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_vision868_pci);
            break;
        case S3_DIAMOND_STEALTH64_964:
            bios_fn = ROM_DIAMOND_STEALTH64_964;
            chip    = S3_VISION964;
            if (info->flags & DEVICE_PCI)
                video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_vision964_pci);
            else
                video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_vision964_vlb);
            break;
        case S3_MIROCRYSTAL20SV_964:
            chip = S3_VISION964;
            if (info->flags & DEVICE_PCI) {
                bios_fn = ROM_MIROCRYSTAL20SV_964_PCI;
                video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_vision964_pci);
            } else {
                bios_fn = ROM_MIROCRYSTAL20SV_964_VLB;
                video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_vision964_vlb);
            }
            break;
        case S3_DIAMOND_STEALTH64_968:
            bios_fn = ROM_DIAMOND_STEALTH64_968;
            chip    = S3_VISION968;
            if (info->flags & DEVICE_PCI)
                video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_vision968_pci);
            else
                video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_vision968_vlb);
            break;
        case S3_MIROVIDEO40SV_ERGO_968:
            bios_fn = ROM_MIROVIDEO40SV_ERGO_968_PCI;
            chip    = S3_VISION968;
            video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_vision968_pci);
            break;
        case S3_NUMBER9_9FX_771:
            bios_fn = ROM_NUMBER9_9FX_771;
            chip    = S3_VISION968;
            video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_vision968_pci);
            break;
        case S3_PHOENIX_VISION968:
            bios_fn = ROM_PHOENIX_VISION968;
            chip    = S3_VISION968;
            video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_vision968_pci);
            break;
        case S3_ELSAWIN2KPROX_964:
            bios_fn = ROM_ELSAWIN2KPROX_964;
            chip    = S3_VISION964;
            video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_vision964_pci);
            break;
        case S3_ELSAWIN2KPROX:
            bios_fn = ROM_ELSAWIN2KPROX;
            chip    = S3_VISION968;
            video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_vision968_pci);
            break;
        case S3_SPEA_MERCURY_P64V:
            bios_fn = ROM_SPEA_MERCURY_P64V;
            chip    = S3_VISION968;
            video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_vision968_pci);
            break;
        case S3_PHOENIX_TRIO32:
            bios_fn = ROM_PHOENIX_TRIO32;
            chip    = S3_TRIO32;
            if (info->flags & DEVICE_PCI)
                video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_trio32_pci);
            else
                video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_trio32_vlb);
            break;
        case S3_PHOENIX_TRIO32_ONBOARD:
            bios_fn = NULL;
            chip    = S3_TRIO32;
            if (info->flags & DEVICE_PCI)
                video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_trio32_pci);
            else
                video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_trio32_vlb);
            break;
        case S3_DIAMOND_STEALTH_SE:
            bios_fn = ROM_DIAMOND_STEALTH_SE;
            chip    = S3_TRIO32;
            if (info->flags & DEVICE_PCI)
                video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_trio32_pci);
            else
                video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_trio32_vlb);
            break;
        case S3_PHOENIX_TRIO64:
            bios_fn = ROM_PHOENIX_TRIO64;
            chip    = S3_TRIO64;
            if (info->flags & DEVICE_PCI)
                video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_trio64_pci);
            else
                video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_trio64_vlb);
            break;
        case S3_SPEA_MIRAGE_P64:
            bios_fn = ROM_SPEA_MIRAGE_P64;
            chip    = S3_TRIO64;
            video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_trio64_vlb);
            break;
        case S3_PHOENIX_TRIO64_ONBOARD:
            bios_fn = NULL;
            chip    = S3_TRIO64;
            if (info->flags & DEVICE_PCI)
                video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_trio64_pci);
            else
                video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_trio64_vlb);
            break;
        case S3_STB_POWERGRAPH_64_VIDEO:
            bios_fn = ROM_STB_POWERGRAPH_64_VIDEO;
            chip    = S3_TRIO64V;
            if (info->flags & DEVICE_PCI)
                video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_trio64_pci);
            else
                video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_trio64_vlb);
            break;
        case S3_PHOENIX_TRIO64VPLUS:
            bios_fn = ROM_PHOENIX_TRIO64VPLUS;
            chip    = S3_TRIO64V;
            if (info->flags & DEVICE_PCI)
                video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_trio64_pci);
            else
                video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_trio64_vlb);
            break;
        case S3_PHOENIX_TRIO64VPLUS_ONBOARD:
            bios_fn = NULL;
            chip    = S3_TRIO64V;
            if (info->flags & DEVICE_PCI)
                video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_trio64_pci);
            else
                video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_trio64_vlb);
            break;
        case S3_CARDEX_TRIO64VPLUS:
            bios_fn = ROM_CARDEX_TRIO64VPLUS;
            chip    = S3_TRIO64V;
            video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_trio64vp_cardex_pci);
            break;
        case S3_DIAMOND_STEALTH64_764:
            bios_fn = ROM_DIAMOND_STEALTH64_764;
            chip    = S3_TRIO64;
            if (info->flags & DEVICE_PCI)
                video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_stealth64_pci);
            else
                video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_stealth64_vlb);
            break;
        case S3_NUMBER9_9FX:
            bios_fn = ROM_NUMBER9_9FX;
            chip    = S3_TRIO64;
            if (info->flags & DEVICE_PCI)
                video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_trio64_pci);
            else
                video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_trio64_vlb);
            break;
        case S3_TRIO64V2_DX:
            bios_fn = ROM_TRIO64V2_DX_VBE20;
            chip    = S3_TRIO64V2;
            video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_trio64_pci);
            break;
        case S3_TRIO64V2_DX_ONBOARD:
            bios_fn = NULL;
            chip    = S3_TRIO64V2;
            video_inform(VIDEO_FLAG_TYPE_SPECIAL, &timing_s3_trio64_pci);
            break;
        default:
            free(s3);
            return NULL;
    }

    vram = device_get_config_int("memory");

    if (vram)
        vram_size = vram << 20;
    else
        vram_size = 512 << 10;

    s3->vram_mask = vram_size - 1;
    s3->vram      = vram;

    s3->has_bios = (bios_fn != NULL);
    if (s3->has_bios) {
        rom_init(&s3->bios_rom, bios_fn, 0xc0000, 0x8000, 0x7fff, 0, MEM_MAPPING_EXTERNAL);
        if (info->flags & DEVICE_PCI)
            mem_mapping_disable(&s3->bios_rom.mapping);
    }

    s3->pci = !!(info->flags & DEVICE_PCI);
    s3->vlb = !!(info->flags & DEVICE_VLB);


    mem_mapping_add(&s3->linear_mapping, 0, 0,
                    svga_read_linear, svga_readw_linear, svga_readl_linear,
                    svga_write_linear, svga_writew_linear, svga_writel_linear,
                    NULL, MEM_MAPPING_EXTERNAL, &s3->svga);
    /*It's hardcoded to 0xa0000 before the Trio64V+ and expects so*/
    if (chip >= S3_TRIO64V)
        mem_mapping_add(&s3->mmio_mapping, 0, 0,
                        s3_accel_read, s3_accel_read_w, s3_accel_read_l,
                        s3_accel_write, s3_accel_write_w, s3_accel_write_l,
                        NULL, MEM_MAPPING_EXTERNAL, s3);
    else
        mem_mapping_add(&s3->mmio_mapping, 0xa0000, 0x10000,
                        s3_accel_read, s3_accel_read_w, s3_accel_read_l,
                        s3_accel_write, s3_accel_write_w, s3_accel_write_l,
                        NULL, MEM_MAPPING_EXTERNAL, s3);
    mem_mapping_add(&s3->new_mmio_mapping, 0, 0,
                    s3_accel_read, s3_accel_read_w, s3_accel_read_l,
                    s3_accel_write, s3_accel_write_w, s3_accel_write_l,
                    NULL, MEM_MAPPING_EXTERNAL, s3);
    mem_mapping_disable(&s3->mmio_mapping);
    mem_mapping_disable(&s3->new_mmio_mapping);

    if ((chip == S3_VISION964) || (chip == S3_VISION968))
        svga_init(info, &s3->svga, s3, vram_size,
                  s3_recalctimings,
                  s3_in, s3_out,
                  NULL,
                  NULL);
    else {
        if (chip >= S3_TRIO64V) {
            svga_init(info, svga, s3, vram_size,
                      s3_trio64v_recalctimings,
                      s3_in, s3_out,
                      s3_hwcursor_draw,
                      s3_trio64v_overlay_draw);
        } else {
            svga_init(info, svga, s3, vram_size,
                      s3_recalctimings,
                      s3_in, s3_out,
                      s3_hwcursor_draw,
                      NULL);
        }
    }
    svga->read = s3_read;
    svga->readw = s3_readw;
    svga->readl = s3_readl;
    svga->write = s3_write;
    svga->writew = s3_writew;
    svga->writel = s3_writel;
    mem_mapping_set_handler(&svga->mapping, s3_read, s3_readw, s3_readl, s3_write, s3_writew, s3_writel);
    mem_mapping_set_p(&svga->mapping, s3);

    svga->hwcursor.cur_ysize = 64;

    switch (chip) {
        case S3_VISION964:
            switch (info->local) {
                case S3_ELSAWIN2KPROX_964:
                    svga->dac_hwcursor_draw = ibm_rgb528_hwcursor_draw;
                    break;
                default:
                    svga->dac_hwcursor_draw = bt48x_hwcursor_draw;
                    break;
            }
            break;

        case S3_VISION968:
            switch (info->local) {
                case S3_DIAMOND_STEALTH64_968:
                case S3_ELSAWIN2KPROX:
                case S3_PHOENIX_VISION968:
                case S3_NUMBER9_9FX_771:
                    svga->dac_hwcursor_draw = ibm_rgb528_hwcursor_draw;
                    break;
                default:
                    svga->dac_hwcursor_draw = tvp3026_hwcursor_draw;
                    break;
            }
            break;
    }

    if (chip >= S3_VISION964) {
        switch (vram) {
            case 0: /* 512 kB */
                svga->vram_mask = (1 << 19) - 1;
                svga->vram_max  = 1 << 19;
                break;
            case 1: /* 1 MB */
                /* VRAM in first MB, mirrored in 2nd MB, 3rd and 4th MBs are open bus.

                   This works with the #9 9FX BIOS, and matches how my real Trio64 behaves,
                   but does not work with the Phoenix EDO BIOS. Possibly an FPM/EDO difference? */
                svga->vram_mask = (1 << 20) - 1;
                svga->vram_max  = 1 << 20;
                break;
            case 2:
            default: /*2 MB */
                /* VRAM in first 2 MB, 3rd and 4th MBs are open bus. */
                svga->vram_mask = (2 << 20) - 1;
                svga->vram_max  = 2 << 20;
                break;
            case 4: /*4MB*/
                svga->vram_mask = (4 << 20) - 1;
                svga->vram_max  = 4 << 20;
                break;
            case 8: /*8MB*/
                svga->vram_mask = (8 << 20) - 1;
                svga->vram_max  = 8 << 20;
                break;
        }
    }

    if (s3->pci)
        svga->crtc[0x36] = 2 | (3 << 2) | (1 << 4);
    else if (s3->vlb)
        svga->crtc[0x36] = 1 | (3 << 2) | (1 << 4);
    else
        svga->crtc[0x36] = 3 | (1 << 4);

    if (chip >= S3_86C928)
        svga->crtc[0x36] |= (vram_sizes[vram] << 5);
    else {
        svga->crtc[0x36] |= ((vram == 1) ? 0x00 : 0x20) | 0x98;
        svga->crtc[0x41] = (vram == 1) ? 0x10 : 0x00;
    }

    svga->crtc[0x37] = 1 | (7 << 5);

    if (chip >= S3_86C928)
        svga->crtc[0x37] |= 0x04;

    svga->vblank_start = s3_vblank_start;

    s3_io_set(s3);

    s3->pci_regs[PCI_REG_COMMAND] = 7;

    s3->pci_regs[0x30] = 0x00;
    s3->pci_regs[0x32] = 0x0c;
    s3->pci_regs[0x33] = 0x00;

    s3->chip = chip;

    s3->int_line = 0;

    s3->card_type = info->local;

    svga->force_old_addr = 1;

    s3->accel_start = s3_accel_start;

    switch (s3->card_type) {
        case S3_ORCHID_86C911:
        case S3_DIAMOND_STEALTH_VRAM:
            svga->decode_mask = (1 << 20) - 1;
            stepping          = 0x81; /*86C911*/
            s3->id            = stepping;
            s3->id_ext        = stepping;
            s3->id_ext_pci    = 0;
            s3->packed_mmio   = 0;

            svga->ramdac    = device_add(&sc11483_ramdac_device);
            if (s3->card_type == S3_ORCHID_86C911) {
                svga->clock_gen = device_add(&av9194_device);
                svga->getclock  = av9194_getclock;
            } else {
                /* DCS2824-0 = Diamond ICD2061A-compatible. */
                svga->clock_gen   = device_add(&icd2061_device);
                svga->getclock    = icd2061_getclock;
            }
            break;

        case S3_AMI_86C924:
            svga->decode_mask = (1 << 20) - 1;
            stepping          = 0x82; /*86C911A/86C924*/
            s3->id            = stepping;
            s3->id_ext        = stepping;
            s3->id_ext_pci    = 0;
            s3->packed_mmio   = 0;

            svga->ramdac    = device_add(&sc11483_ramdac_device);
            svga->clock_gen = device_add(&ics2494an_305_device);
            svga->getclock  = ics2494_getclock;
            break;

        case S3_MIROCRYSTAL8S_805:
        case S3_MIROCRYSTAL10SD_805:
            svga->decode_mask = (2 << 20) - 1;
            stepping          = 0xa0; /*86C801/86C805*/
            s3->id            = stepping;
            s3->id_ext        = stepping;
            s3->id_ext_pci    = 0;
            s3->packed_mmio   = 0;
            svga->crtc[0x5a]  = 0x0a;

            svga->ramdac    = device_add(&gendac_ramdac_device);
            svga->clock_gen = svga->ramdac;
            svga->getclock  = sdac_getclock;
            break;

        case S3_SPEA_MIRAGE_86C801:
        case S3_SPEA_MIRAGE_86C805:
        case S3_WINNER1000_805:
            svga->decode_mask = (2 << 20) - 1;
            stepping          = 0xa2; /*86C801/86C805*/
            s3->id            = stepping;
            s3->id_ext        = stepping;
            s3->id_ext_pci    = 0;
            s3->packed_mmio   = 0;
            svga->crtc[0x5a]  = 0x0a;

            svga->ramdac    = device_add(&att491_ramdac_device);
            svga->clock_gen = device_add(&av9194_device);
            svga->getclock  = av9194_getclock;
            break;

        case S3_86C805_ONBOARD:
            svga->decode_mask = (2 << 20) - 1;
            stepping          = 0xa0; /*86C801/86C805*/
            s3->id            = stepping;
            s3->id_ext        = stepping;
            s3->id_ext_pci    = 0;
            s3->packed_mmio   = 0;
            svga->crtc[0x5a]  = 0x0a;

            svga->ramdac    = device_add(&att490_ramdac_device);
            svga->clock_gen = device_add(&av9194_device);
            svga->getclock  = av9194_getclock;
            break;

        case S3_PHOENIX_86C801:
        case S3_PHOENIX_86C805:
            svga->decode_mask = (2 << 20) - 1;
            stepping          = 0xa0; /*86C801/86C805*/
            s3->id            = stepping;
            s3->id_ext        = stepping;
            s3->id_ext_pci    = 0;
            s3->packed_mmio   = 0;
            svga->crtc[0x5a]  = 0x0a;

            svga->ramdac    = device_add(&att492_ramdac_device);
            svga->clock_gen = device_add(&av9194_device);
            svga->getclock  = av9194_getclock;
            break;

        case S3_METHEUS_86C928:
            svga->decode_mask = (4 << 20) - 1;
            stepping          = 0x91; /*86C928D*/
            s3->id            = stepping;
            s3->id_ext        = stepping;
            s3->id_ext_pci    = 0;
            s3->packed_mmio   = 0;
            svga->crtc[0x5a]  = 0x0a;
            svga->ramdac      = device_add(&bt485_ramdac_device);
            svga->clock_gen   = device_add(&icd2061_device);
            svga->getclock    = icd2061_getclock;
            break;

        case S3_SPEA_MERCURY_LITE_PCI:
            svga->decode_mask = (4 << 20) - 1;
            stepping          = 0xb0; /*86C928PCI*/
            s3->id            = stepping;
            s3->id_ext        = stepping;
            s3->id_ext_pci    = stepping;
            s3->packed_mmio   = 0;
            svga->crtc[0x5a]  = 0x0a;
            svga->ramdac      = device_add(&sc1502x_ramdac_device);
            svga->clock_gen   = device_add(&av9194_device);
            svga->getclock    = av9194_getclock;
            break;

        case S3_PARADISE_BAHAMAS64:
        case S3_PHOENIX_VISION864:
        case S3_MIROCRYSTAL20SD_864: /*BIOS 3.xx has a SDAC ramdac.*/
            svga->decode_mask = (8 << 20) - 1;
            if (info->local == S3_PARADISE_BAHAMAS64)
                stepping = 0xc0; /*Vision864*/
            else
                stepping = 0xc1; /*Vision864P*/

            s3->id     = stepping;
            s3->id_ext = s3->id_ext_pci = stepping;
            s3->packed_mmio             = 0;
            svga->crtc[0x5a]            = 0x0a;
            svga->ramdac                = device_add(&sdac_ramdac_device);
            svga->clock_gen             = svga->ramdac;
            svga->getclock              = sdac_getclock;
            break;

        case S3_DIAMOND_STEALTH64_964:
        case S3_ELSAWIN2KPROX_964:
        case S3_MIROCRYSTAL20SV_964:
            svga->decode_mask = (8 << 20) - 1;
            stepping          = 0xd0; /*Vision964*/
            s3->id            = stepping;
            s3->id_ext = s3->id_ext_pci = stepping;
            s3->packed_mmio             = 1;
            svga->crtc[0x5a]            = 0x0a;

            switch (info->local) {
                case S3_ELSAWIN2KPROX_964:
                    svga->ramdac = device_add(&ibm_rgb528_ramdac_device);
                    svga->clock_gen = device_add(&icd2061_device);
                    svga->getclock  = icd2061_getclock;
                    break;
                default:
                    svga->ramdac = device_add(&bt485_ramdac_device);
                    svga->clock_gen = device_add(&icd2061_device);
                    svga->getclock  = icd2061_getclock;
                    break;
            }
            break;

        case S3_DIAMOND_STEALTH64_968:
        case S3_ELSAWIN2KPROX:
        case S3_SPEA_MERCURY_P64V:
        case S3_MIROVIDEO40SV_ERGO_968:
        case S3_NUMBER9_9FX_771:
        case S3_PHOENIX_VISION968:
            svga->decode_mask = (8 << 20) - 1;
            s3->id            = 0xe1; /*Vision968*/
            s3->id_ext        = 0xf0;
            s3->id_ext_pci    = s3->id_ext;
            s3->packed_mmio   = 1;
            if (s3->pci) {
                svga->crtc[0x53] = 0x18;
                svga->crtc[0x58] = 0x10;
                svga->crtc[0x59] = 0x70;
                svga->crtc[0x5a] = 0x00;
                svga->crtc[0x6c] = 1;
            } else {
                svga->crtc[0x53] = 0x00;
                svga->crtc[0x59] = 0x00;
                svga->crtc[0x5a] = 0x0a;
            }

            switch (info->local) {
                case S3_DIAMOND_STEALTH64_968:
                case S3_ELSAWIN2KPROX:
                case S3_PHOENIX_VISION968:
                case S3_NUMBER9_9FX_771:
                    svga->ramdac    = device_add(&ibm_rgb528_ramdac_device);
                    svga->clock_gen = device_add(&icd2061_device);
                    svga->getclock  = icd2061_getclock;
                    break;
                default:
                    svga->ramdac    = device_add(&tvp3026_ramdac_device);
                    svga->clock_gen = svga->ramdac;
                    svga->getclock  = tvp3026_getclock;
                    svga->conv_16to32 = tvp3026_conv_16to32;
                    break;
            }
            break;

        case S3_NUMBER9_9FX_531:
        case S3_PHOENIX_VISION868:
            svga->decode_mask = (8 << 20) - 1;
            s3->id            = 0xe1; /*Vision868*/
            s3->id_ext        = 0x90;
            s3->id_ext_pci    = 0x80;
            s3->packed_mmio   = 1;
            if (s3->pci) {
                svga->crtc[0x53] = 0x18;
                svga->crtc[0x58] = 0x10;
                svga->crtc[0x59] = 0x70;
                svga->crtc[0x5a] = 0x00;
                svga->crtc[0x6c] = 1;
            } else {
                svga->crtc[0x53] = 0x00;
                svga->crtc[0x59] = 0x00;
                svga->crtc[0x5a] = 0x0a;
            }

            if (info->local == S3_NUMBER9_9FX_531) {
                svga->ramdac    = device_add(&att498_ramdac_device);
                svga->clock_gen = device_add(&icd2061_device);
                svga->getclock  = icd2061_getclock;
            } else {
                svga->ramdac    = device_add(&sdac_ramdac_device);
                svga->clock_gen = svga->ramdac;
                svga->getclock  = sdac_getclock;
            }
            break;

        case S3_PHOENIX_TRIO32:
        case S3_PHOENIX_TRIO32_ONBOARD:
        case S3_DIAMOND_STEALTH_SE:
            svga->decode_mask = (4 << 20) - 1;
            s3->id            = 0xe1; /*Trio32*/
            s3->id_ext        = 0x10;
            s3->id_ext_pci    = 0x11;
            s3->packed_mmio   = 1;

            svga->clock_gen = s3;
            svga->getclock  = s3_trio64_getclock;
            break;

        case S3_PHOENIX_TRIO64:
        case S3_PHOENIX_TRIO64_ONBOARD:
        case S3_STB_POWERGRAPH_64_VIDEO:
        case S3_PHOENIX_TRIO64VPLUS:
        case S3_PHOENIX_TRIO64VPLUS_ONBOARD:
        case S3_CARDEX_TRIO64VPLUS:
        case S3_DIAMOND_STEALTH64_764:
        case S3_SPEA_MIRAGE_P64:
        case S3_NUMBER9_9FX:
            svga->decode_mask = (4 << 20) - 1;
            s3->id            = 0xe1; /*Trio64*/
            s3->id_ext = s3->id_ext_pci = 0x11;
            s3->packed_mmio             = 1;

            if (s3->chip == S3_TRIO64V)
                svga->crtc[0x53] = 0x08;

            svga->clock_gen = s3;
            svga->getclock  = s3_trio64_getclock;
            break;

        case S3_TRIO64V2_DX:
        case S3_TRIO64V2_DX_ONBOARD:
            svga->decode_mask = (4 << 20) - 1;
            s3->id            = 0xe1; /*Trio64V2*/
            s3->id_ext = s3->id_ext_pci = 0x01;
            s3->packed_mmio             = 1;
            svga->crtc[0x53]            = 0x08;
            svga->crtc[0x59]            = 0x70;
            svga->crtc[0x5a]            = 0x00;
            svga->crtc[0x6c]            = 1;
            s3->pci_regs[0x05]          = 0;
            s3->pci_regs[0x06]          = 0;
            s3->pci_regs[0x07]          = 2;
            s3->pci_regs[0x3d]          = 1;
            s3->pci_regs[0x3e]          = 4;
            s3->pci_regs[0x3f]          = 0xff;

            svga->clock_gen = s3;
            svga->getclock  = s3_trio64_getclock;
            break;

        default:
            return NULL;
    }

    if (s3->pci) {
        if (bios_fn == NULL)
            pci_add_card(PCI_ADD_VIDEO, s3_pci_read, s3_pci_write, s3, &s3->pci_slot);
        else
            pci_add_card(PCI_ADD_NORMAL, s3_pci_read, s3_pci_write, s3, &s3->pci_slot);
    }

    s3->i2c = i2c_gpio_init("ddc_s3");
    s3->ddc = ddc_init(i2c_gpio_get_bus(s3->i2c));
    s3->accel.multifunc[0xd] = 0xd000;
    s3->accel.multifunc[0xe] = 0xe000;

    s3->wake_fifo_thread    = thread_create_event();
    s3->fifo_not_full_event = thread_create_event();
    s3->fifo_thread_run     = 1;
    s3->fifo_thread         = thread_create(fifo_thread, s3);

    *reset_state = *s3;

    return s3;
}

static int
s3_orchid_86c911_available(void)
{
    return rom_present(ROM_ORCHID_86C911);
}

static int
s3_diamond_stealth_vram_available(void)
{
    return rom_present(ROM_DIAMOND_STEALTH_VRAM);
}

static int
s3_ami_86c924_available(void)
{
    return rom_present(ROM_AMI_86C924);
}

static int
s3_spea_mirage_86c801_available(void)
{
    return rom_present(ROM_SPEA_MIRAGE_86C801);
}

static int
s3_spea_mirage_86c805_available(void)
{
    return rom_present(ROM_SPEA_MIRAGE_86C805);
}

static int
s3_phoenix_86c80x_available(void)
{
    return rom_present(ROM_PHOENIX_86C80X);
}

static int
s3_winner1000_805_available(void)
{
    return rom_present(ROM_WINNER1000_805);
}

static int
s3_mirocrystal_8s_805_available(void)
{
    return rom_present(ROM_MIROCRYSTAL8S_805);
}

static int
s3_mirocrystal_10sd_805_available(void)
{
    return rom_present(ROM_MIROCRYSTAL10SD_805);
}

static int
s3_metheus_86c928_available(void)
{
    return rom_present(ROM_METHEUS_86C928);
}

static int
s3_spea_mercury_lite_pci_available(void)
{
    return rom_present(ROM_SPEA_MERCURY_LITE_PCI);
}

static int
s3_bahamas64_available(void)
{
    return rom_present(ROM_PARADISE_BAHAMAS64);
}

static int
s3_phoenix_vision864_available(void)
{
    return rom_present(ROM_PHOENIX_VISION864);
}

static int
s3_9fx_531_available(void)
{
    return rom_present(ROM_NUMBER9_9FX_531);
}

static int
s3_phoenix_vision868_available(void)
{
    return rom_present(ROM_PHOENIX_VISION868);
}

static int
s3_mirocrystal_20sv_964_vlb_available(void)
{
    return rom_present(ROM_MIROCRYSTAL20SV_964_VLB);
}

static int
s3_mirocrystal_20sv_964_pci_available(void)
{
    return rom_present(ROM_MIROCRYSTAL20SV_964_PCI);
}

static int
s3_diamond_stealth64_964_available(void)
{
    return rom_present(ROM_DIAMOND_STEALTH64_964);
}

static int
s3_diamond_stealth64_968_available(void)
{
    return rom_present(ROM_DIAMOND_STEALTH64_968);
}

static int
s3_mirovideo_40sv_ergo_968_pci_available(void)
{
    return rom_present(ROM_MIROVIDEO40SV_ERGO_968_PCI);
}

static int
s3_9fx_771_available(void)
{
    return rom_present(ROM_NUMBER9_9FX_771);
}

static int
s3_phoenix_vision968_available(void)
{
    return rom_present(ROM_PHOENIX_VISION968);
}

static int
s3_mirocrystal_20sd_864_vlb_available(void)
{
    return rom_present(ROM_MIROCRYSTAL20SD_864_VLB);
}

static int
s3_spea_mercury_p64v_pci_available(void)
{
    return rom_present(ROM_SPEA_MERCURY_P64V);
}

static int
s3_elsa_winner2000_pro_x_964_available(void)
{
    return rom_present(ROM_ELSAWIN2KPROX_964);
}

static int
s3_elsa_winner2000_pro_x_available(void)
{
    return rom_present(ROM_ELSAWIN2KPROX);
}

static int
s3_phoenix_trio32_available(void)
{
    return rom_present(ROM_PHOENIX_TRIO32);
}

static int
s3_diamond_stealth_se_available(void)
{
    return rom_present(ROM_DIAMOND_STEALTH_SE);
}

static int
s3_9fx_available(void)
{
    return rom_present(ROM_NUMBER9_9FX);
}

static int
s3_spea_mirage_p64_vlb_available(void)
{
    return rom_present(ROM_SPEA_MIRAGE_P64);
}

static int
s3_phoenix_trio64_available(void)
{
    return rom_present(ROM_PHOENIX_TRIO64);
}

static int
s3_stb_powergraph_64_video_available(void)
{
    return rom_present(ROM_STB_POWERGRAPH_64_VIDEO);
}

static int
s3_phoenix_trio64vplus_available(void)
{
    return rom_present(ROM_PHOENIX_TRIO64VPLUS);
}

static int
s3_cardex_trio64vplus_available(void)
{
    return rom_present(ROM_PHOENIX_TRIO64VPLUS);
}

static int
s3_diamond_stealth64_764_available(void)
{
    return rom_present(ROM_DIAMOND_STEALTH64_764);
}

static int
s3_trio64v2_dx_available(void)
{
    return rom_present(ROM_TRIO64V2_DX_VBE20);
}

static void
s3_close(void *priv)
{
    s3_t *s3 = (s3_t *) priv;

    s3->fifo_thread_run = 0;
    thread_set_event(s3->wake_fifo_thread);
    thread_wait(s3->fifo_thread);
    thread_destroy_event(s3->fifo_not_full_event);
    thread_destroy_event(s3->wake_fifo_thread);

    svga_close(&s3->svga);

    ddc_close(s3->ddc);
    i2c_gpio_close(s3->i2c);

    free(reset_state);
    reset_state = NULL;

    free(s3);
}

static void
s3_speed_changed(void *priv)
{
    s3_t *s3 = (s3_t *) priv;

    svga_recalctimings(&s3->svga);
}

static void
s3_force_redraw(void *priv)
{
    s3_t *s3 = (s3_t *) priv;

    s3->svga.fullchange = s3->svga.monitor->mon_changeframecount;
}

// clang-format off
static const device_config_t s3_orchid_86c911_config[] = {
    {
        .name           = "memory",
        .description    = "Memory size",
        .type           = CONFIG_SELECTION,
        .default_string = NULL,
        .default_int    = 1,
        .file_filter    = NULL,
        .spinner        = { 0 },
        .selection      = {
            { .description = "512 KB", .value = 0 },
            { .description = "1 MB",   .value = 1 },
            { .description = ""                   }
        },
        .bios           = { { 0 } }
    },
    { .name = "", .description = "", .type = CONFIG_END }
};

static const device_config_t s3_9fx_config[] = {
    {
        .name           = "memory",
        .description    = "Memory size",
        .type           = CONFIG_SELECTION,
        .default_string = NULL,
        .default_int    = 2,
        .file_filter    = NULL,
        .spinner        = { 0 },
        .selection      = {
             { .description = "1 MB", .value = 1 },
             { .description = "2 MB", .value = 2 },
             /* Trio64 also supports 4 MB, however the Number Nine BIOS does not */
             { .description = ""                 }
        },
        .bios           = { { 0 } }
    },
    { .name = "", .description = "", .type = CONFIG_END }
};

static const device_config_t s3_phoenix_trio32_config[] = {
    {
        .name           = "memory",
        .description    = "Memory size",
        .type           = CONFIG_SELECTION,
        .default_string = NULL,
        .default_int    = 2,
        .file_filter    = NULL,
        .spinner        = { 0 },
        .selection      = {
            { .description = "512 KB", .value = 0 },
            { .description = "1 MB",   .value = 1 },
            { .description = "2 MB",   .value = 2 },
            { .description = ""                   }
        },
        .bios           = { { 0 } }
    },
    { .name = "", .description = "", .type = CONFIG_END }
};

static const device_config_t s3_standard_config[] = {
    {
        .name           = "memory",
        .description    = "Memory size",
        .type           = CONFIG_SELECTION,
        .default_string = NULL,
        .default_int    = 4,
        .file_filter    = NULL,
        .spinner        = { 0 },
        .selection      = {
            { .description = "1 MB", .value = 1 },
            { .description = "2 MB", .value = 2 },
            { .description = "4 MB", .value = 4 },
            { .description = ""                 }
        },
        .bios           = { { 0 } }
    },
    { .name = "", .description = "", .type = CONFIG_END }
};

static const device_config_t s3_968_config[] = {
    {
        .name           = "memory",
        .description    = "Memory size",
        .type           = CONFIG_SELECTION,
        .default_string = NULL,
        .default_int    = 4,
        .file_filter    = NULL,
        .spinner        = { 0 },
        .selection      = {
            { .description = "1 MB", .value = 1 },
            { .description = "2 MB", .value = 2 },
            { .description = "4 MB", .value = 4 },
            { .description = "8 MB", .value = 8 },
            { .description = ""                 }
        },
        .bios           = { { 0 } }
    },
    { .name = "", .description = "", .type = CONFIG_END }
};

static const device_config_t s3_standard_config2[] = {
    {
        .name           = "memory",
        .description    = "Memory size",
        .type           = CONFIG_SELECTION,
        .default_string = NULL,
        .default_int    = 4,
        .file_filter    = NULL,
        .spinner        = { 0 },
        .selection      = {
            { .description = "2 MB", .value = 2 },
            { .description = "4 MB", .value = 4 },
            { .description = ""                 }
        },
        .bios           = { { 0 } }
    },
    { .name = "", .description = "", .type = CONFIG_END }
};
// clang-format on

const device_t s3_orchid_86c911_isa_device = {
    .name          = "S3 86c911 ISA (Orchid Fahrenheit 1280)",
    .internal_name = "orchid_s3_911",
    .flags         = DEVICE_ISA16,
    .local         = S3_ORCHID_86C911,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_orchid_86c911_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_orchid_86c911_config
};

const device_t s3_diamond_stealth_vram_isa_device = {
    .name          = "S3 86c911 ISA (Diamond Stealth VRAM)",
    .internal_name = "stealthvram_isa",
    .flags         = DEVICE_ISA16,
    .local         = S3_DIAMOND_STEALTH_VRAM,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_diamond_stealth_vram_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_orchid_86c911_config
};

const device_t s3_ami_86c924_isa_device = {
    .name          = "S3 86c924 ISA (AMI)",
    .internal_name = "ami_s3_924",
    .flags         = DEVICE_ISA16,
    .local         = S3_AMI_86C924,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_ami_86c924_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_orchid_86c911_config
};

const device_t s3_spea_mirage_86c801_isa_device = {
    .name          = "S3 86c801 ISA (SPEA Mirage ISA)",
    .internal_name = "px_s3_v7_801_isa",
    .flags         = DEVICE_ISA16,
    .local         = S3_SPEA_MIRAGE_86C801,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_spea_mirage_86c801_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_9fx_config
};

const device_t s3_winner1000_805_isa_device = {
    .name          = "S3 86c805 ISA (ELSA Winner 1000)",
    .internal_name = "winner1000_805_isa",
    .flags         = DEVICE_ISA16,
    .local         = S3_WINNER1000_805,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available = s3_winner1000_805_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_9fx_config
};

const device_t s3_86c805_onboard_vlb_device = {
    .name          = "S3 86c805 VLB On-Board",
    .internal_name = "px_s3_805_onboard_vlb",
    .flags         = DEVICE_VLB,
    .local         = S3_86C805_ONBOARD,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = NULL,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_9fx_config
};

const device_t s3_spea_mirage_86c805_vlb_device = {
    .name          = "S3 86c805 VLB (SPEA Mirage VL)",
    .internal_name = "px_s3_v7_805_vlb",
    .flags         = DEVICE_VLB,
    .local         = S3_SPEA_MIRAGE_86C805,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_spea_mirage_86c805_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_9fx_config
};

const device_t s3_mirocrystal_8s_805_vlb_device = {
    .name          = "S3 86c805 VLB (MiroCRYSTAL 8S)",
    .internal_name = "mirocrystal8s_vlb",
    .flags         = DEVICE_VLB,
    .local         = S3_MIROCRYSTAL8S_805,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_mirocrystal_8s_805_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_9fx_config
};

const device_t s3_mirocrystal_10sd_805_vlb_device = {
    .name          = "S3 86c805 VLB (MiroCRYSTAL 10SD)",
    .internal_name = "mirocrystal10sd_vlb",
    .flags         = DEVICE_VLB,
    .local         = S3_MIROCRYSTAL10SD_805,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_mirocrystal_10sd_805_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_9fx_config
};

const device_t s3_phoenix_86c801_isa_device = {
    .name          = "S3 86c801 ISA (Phoenix)",
    .internal_name = "px_86c801_isa",
    .flags         = DEVICE_ISA16,
    .local         = S3_PHOENIX_86C801,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_phoenix_86c80x_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_9fx_config
};

const device_t s3_phoenix_86c805_vlb_device = {
    .name          = "S3 86c805 VLB (Phoenix)",
    .internal_name = "px_86c805_vlb",
    .flags         = DEVICE_VLB,
    .local         = S3_PHOENIX_86C805,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_phoenix_86c80x_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_9fx_config
};

const device_t s3_metheus_86c928_isa_device = {
    .name          = "S3 86c928 ISA (Metheus Premier 928)",
    .internal_name = "metheus928_isa",
    .flags         = DEVICE_ISA16,
    .local         = S3_METHEUS_86C928,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_metheus_86c928_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_standard_config
};

const device_t s3_metheus_86c928_vlb_device = {
    .name          = "S3 86c928 VLB (Metheus Premier 928)",
    .internal_name = "metheus928_vlb",
    .flags         = DEVICE_VLB,
    .local         = S3_METHEUS_86C928,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_metheus_86c928_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_standard_config
};

const device_t s3_spea_mercury_lite_86c928_pci_device = {
    .name          = "S3 86c928 PCI (SPEA Mercury Lite)",
    .internal_name = "spea_mercurylite_pci",
    .flags         = DEVICE_PCI,
    .local         = S3_SPEA_MERCURY_LITE_PCI,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_spea_mercury_lite_pci_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_orchid_86c911_config
};

const device_t s3_mirocrystal_20sd_864_vlb_device = {
    .name          = "S3 Vision864 VLB (MiroCRYSTAL 20SD)",
    .internal_name = "mirocrystal20sd_vlb",
    .flags         = DEVICE_VLB,
    .local         = S3_MIROCRYSTAL20SD_864,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_mirocrystal_20sd_864_vlb_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_9fx_config
};

const device_t s3_bahamas64_vlb_device = {
    .name          = "S3 Vision864 VLB (Paradise Bahamas 64)",
    .internal_name = "bahamas64_vlb",
    .flags         = DEVICE_VLB,
    .local         = S3_PARADISE_BAHAMAS64,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_bahamas64_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_9fx_config
};

const device_t s3_bahamas64_pci_device = {
    .name          = "S3 Vision864 PCI (Paradise Bahamas 64)",
    .internal_name = "bahamas64_pci",
    .flags         = DEVICE_PCI,
    .local         = S3_PARADISE_BAHAMAS64,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_bahamas64_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_9fx_config
};

const device_t s3_mirocrystal_20sv_964_vlb_device = {
    .name          = "S3 Vision964 VLB (MiroCRYSTAL 20SV)",
    .internal_name = "mirocrystal20sv_vlb",
    .flags         = DEVICE_VLB,
    .local         = S3_MIROCRYSTAL20SV_964,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_mirocrystal_20sv_964_vlb_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_9fx_config
};

const device_t s3_mirocrystal_20sv_964_pci_device = {
    .name          = "S3 Vision964 PCI (MiroCRYSTAL 20SV)",
    .internal_name = "mirocrystal20sv_pci",
    .flags         = DEVICE_PCI,
    .local         = S3_MIROCRYSTAL20SV_964,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_mirocrystal_20sv_964_pci_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_9fx_config
};

const device_t s3_diamond_stealth64_964_vlb_device = {
    .name          = "S3 Vision964 VLB (Diamond Stealth64 VRAM)",
    .internal_name = "stealth64v_vlb",
    .flags         = DEVICE_VLB,
    .local         = S3_DIAMOND_STEALTH64_964,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_diamond_stealth64_964_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_standard_config
};

const device_t s3_diamond_stealth64_964_pci_device = {
    .name          = "S3 Vision964 PCI (Diamond Stealth64 VRAM)",
    .internal_name = "stealth64v_pci",
    .flags         = DEVICE_PCI,
    .local         = S3_DIAMOND_STEALTH64_964,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_diamond_stealth64_964_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_standard_config
};

const device_t s3_diamond_stealth64_968_vlb_device = {
    .name          = "S3 Vision968 VLB (Diamond Stealth64 Video VRAM)",
    .internal_name = "stealth64vv_vlb",
    .flags         = DEVICE_VLB,
    .local         = S3_DIAMOND_STEALTH64_968,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_diamond_stealth64_968_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_standard_config2
};

const device_t s3_diamond_stealth64_968_pci_device = {
    .name          = "S3 Vision968 PCI (Diamond Stealth64 Video VRAM)",
    .internal_name = "stealth64vv_pci",
    .flags         = DEVICE_PCI,
    .local         = S3_DIAMOND_STEALTH64_968,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_diamond_stealth64_968_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_standard_config2
};

const device_t s3_9fx_771_pci_device = {
    .name          = "S3 Vision968 PCI (Number 9 9FX 771)",
    .internal_name = "n9_9fx_771_pci",
    .flags         = DEVICE_PCI,
    .local         = S3_NUMBER9_9FX_771,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_9fx_771_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_968_config
};

const device_t s3_phoenix_vision968_pci_device = {
    .name          = "S3 Vision968 PCI (Phoenix)",
    .internal_name = "px_vision968_pci",
    .flags         = DEVICE_PCI,
    .local         = S3_PHOENIX_VISION968,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_phoenix_vision968_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_standard_config
};

const device_t s3_mirovideo_40sv_ergo_968_pci_device = {
    .name          = "S3 Vision968 PCI (MiroVIDEO 40SV Ergo)",
    .internal_name = "mirovideo40sv_pci",
    .flags         = DEVICE_PCI,
    .local         = S3_MIROVIDEO40SV_ERGO_968,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_mirovideo_40sv_ergo_968_pci_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_standard_config
};

const device_t s3_spea_mercury_p64v_pci_device = {
    .name          = "S3 Vision968 PCI (SPEA Mercury P64V)",
    .internal_name = "spea_mercury64p_pci",
    .flags         = DEVICE_PCI,
    .local         = S3_SPEA_MERCURY_P64V,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_spea_mercury_p64v_pci_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_standard_config
};

const device_t s3_9fx_vlb_device = {
    .name          = "S3 Trio64 VLB (Number 9 9FX 330)",
    .internal_name = "n9_9fx_vlb",
    .flags         = DEVICE_VLB,
    .local         = S3_NUMBER9_9FX,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_9fx_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_9fx_config
};

const device_t s3_9fx_pci_device = {
    .name          = "S3 Trio64 PCI (Number 9 9FX 330)",
    .internal_name = "n9_9fx_pci",
    .flags         = DEVICE_PCI,
    .local         = S3_NUMBER9_9FX,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_9fx_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_9fx_config
};

const device_t s3_phoenix_trio32_onboard_vlb_device = {
    .name          = "S3 Trio32 VLB On-Board (Phoenix)",
    .internal_name = "px_trio32_onboard_vlb",
    .flags         = DEVICE_VLB,
    .local         = S3_PHOENIX_TRIO32_ONBOARD,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = NULL,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_phoenix_trio32_config
};

const device_t s3_phoenix_trio32_vlb_device = {
    .name          = "S3 Trio32 VLB (Phoenix)",
    .internal_name = "px_trio32_vlb",
    .flags         = DEVICE_VLB,
    .local         = S3_PHOENIX_TRIO32,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_phoenix_trio32_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_phoenix_trio32_config
};

const device_t s3_phoenix_trio32_onboard_pci_device = {
    .name          = "S3 Trio32 PCI On-Board (Phoenix)",
    .internal_name = "px_trio32_onboard_pci",
    .flags         = DEVICE_PCI,
    .local         = S3_PHOENIX_TRIO32_ONBOARD,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = NULL,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_phoenix_trio32_config
};

const device_t s3_phoenix_trio32_pci_device = {
    .name          = "S3 Trio32 PCI (Phoenix)",
    .internal_name = "px_trio32_pci",
    .flags         = DEVICE_PCI,
    .local         = S3_PHOENIX_TRIO32,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_phoenix_trio32_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_phoenix_trio32_config
};

const device_t s3_diamond_stealth_se_vlb_device = {
    .name          = "S3 Trio32 VLB (Diamond Stealth SE)",
    .internal_name = "stealthse_vlb",
    .flags         = DEVICE_VLB,
    .local         = S3_DIAMOND_STEALTH_SE,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_diamond_stealth_se_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_phoenix_trio32_config
};

const device_t s3_diamond_stealth_se_pci_device = {
    .name          = "S3 Trio32 PCI (Diamond Stealth SE)",
    .internal_name = "stealthse_pci",
    .flags         = DEVICE_PCI,
    .local         = S3_DIAMOND_STEALTH_SE,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_diamond_stealth_se_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_phoenix_trio32_config
};

const device_t s3_phoenix_trio64_vlb_device = {
    .name          = "S3 Trio64 VLB (Phoenix)",
    .internal_name = "px_trio64_vlb",
    .flags         = DEVICE_VLB,
    .local         = S3_PHOENIX_TRIO64,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_phoenix_trio64_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_standard_config
};

const device_t s3_phoenix_trio64_onboard_pci_device = {
    .name          = "S3 Trio64 PCI On-Board (Phoenix)",
    .internal_name = "px_trio64_onboard_pci",
    .flags         = DEVICE_PCI,
    .local         = S3_PHOENIX_TRIO64_ONBOARD,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = NULL,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_standard_config
};

const device_t s3_phoenix_trio64_pci_device = {
    .name          = "S3 Trio64 PCI (Phoenix)",
    .internal_name = "px_trio64_pci",
    .flags         = DEVICE_PCI,
    .local         = S3_PHOENIX_TRIO64,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_phoenix_trio64_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_standard_config
};

const device_t s3_stb_powergraph_64_video_vlb_device = {
    .name          = "S3 Trio64V+ (STB PowerGraph 64 Video) VLB",
    .internal_name = "stb_trio64vplus_vlb",
    .flags         = DEVICE_VLB,
    .local         = S3_STB_POWERGRAPH_64_VIDEO,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_stb_powergraph_64_video_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_phoenix_trio32_config
};

const device_t s3_phoenix_trio64vplus_onboard_pci_device = {
    .name          = "S3 Trio64V+ PCI On-Board (Phoenix)",
    .internal_name = "px_trio64vplus_onboard_pci",
    .flags         = DEVICE_PCI,
    .local         = S3_PHOENIX_TRIO64VPLUS_ONBOARD,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = NULL,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_standard_config
};

const device_t s3_phoenix_trio64vplus_pci_device = {
    .name          = "S3 Trio64V+ PCI (Phoenix)",
    .internal_name = "px_trio64vplus_pci",
    .flags         = DEVICE_PCI,
    .local         = S3_PHOENIX_TRIO64VPLUS,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_phoenix_trio64vplus_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_standard_config
};

const device_t s3_cardex_trio64vplus_pci_device = {
    .name          = "S3 Trio64V+ PCI (Cardex)",
    .internal_name = "cardex_trio64vplus_pci",
    .flags         = DEVICE_PCI,
    .local         = S3_CARDEX_TRIO64VPLUS,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_cardex_trio64vplus_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_standard_config
};

const device_t s3_phoenix_vision864_vlb_device = {
    .name          = "S3 Vision864 VLB (Phoenix)",
    .internal_name = "px_vision864_vlb",
    .flags         = DEVICE_VLB,
    .local         = S3_PHOENIX_VISION864,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_phoenix_vision864_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_standard_config
};

const device_t s3_phoenix_vision864_pci_device = {
    .name          = "S3 Vision864 PCI (Phoenix)",
    .internal_name = "px_vision864_pci",
    .flags         = DEVICE_PCI,
    .local         = S3_PHOENIX_VISION864,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_phoenix_vision864_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_standard_config
};

const device_t s3_9fx_531_pci_device = {
    .name          = "S3 Vision868 PCI (Number 9 9FX 531)",
    .internal_name = "n9_9fx_531_pci",
    .flags         = DEVICE_PCI,
    .local         = S3_NUMBER9_9FX_531,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_9fx_531_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_9fx_config
};

const device_t s3_phoenix_vision868_pci_device = {
    .name          = "S3 Vision868 PCI (Phoenix)",
    .internal_name = "px_vision868_pci",
    .flags         = DEVICE_PCI,
    .local         = S3_PHOENIX_VISION868,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_phoenix_vision868_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_standard_config
};

const device_t s3_diamond_stealth64_vlb_device = {
    .name          = "S3 Trio64 VLB (Diamond Stealth64 DRAM)",
    .internal_name = "stealth64d_vlb",
    .flags         = DEVICE_VLB,
    .local         = S3_DIAMOND_STEALTH64_764,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_diamond_stealth64_764_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_9fx_config
};

const device_t s3_diamond_stealth64_pci_device = {
    .name          = "S3 Trio64 PCI (Diamond Stealth64 DRAM)",
    .internal_name = "stealth64d_pci",
    .flags         = DEVICE_PCI,
    .local         = S3_DIAMOND_STEALTH64_764,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_diamond_stealth64_764_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_9fx_config
};

const device_t s3_spea_mirage_p64_vlb_device = {
    .name          = "S3 Trio64 VLB (SPEA Mirage P64)",
    .internal_name = "spea_miragep64_vlb",
    .flags         = DEVICE_VLB,
    .local         = S3_SPEA_MIRAGE_P64,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_spea_mirage_p64_vlb_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_9fx_config
};

const device_t s3_elsa_winner2000_pro_x_964_pci_device = {
    .name          = "S3 Vision964 PCI (ELSA Winner 2000 Pro/X)",
    .internal_name = "elsawin2kprox_964_pci",
    .flags         = DEVICE_PCI,
    .local         = S3_ELSAWIN2KPROX_964,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_elsa_winner2000_pro_x_964_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_968_config
};

const device_t s3_elsa_winner2000_pro_x_pci_device = {
    .name          = "S3 Vision968 PCI (ELSA Winner 2000 Pro/X)",
    .internal_name = "elsawin2kprox_pci",
    .flags         = DEVICE_PCI,
    .local         = S3_ELSAWIN2KPROX,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_elsa_winner2000_pro_x_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_968_config
};

const device_t s3_trio64v2_dx_pci_device = {
    .name          = "S3 Trio64V2/DX PCI",
    .internal_name = "trio64v2dx_pci",
    .flags         = DEVICE_PCI,
    .local         = S3_TRIO64V2_DX,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = s3_reset,
    .available     = s3_trio64v2_dx_available,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_standard_config
};

const device_t s3_trio64v2_dx_onboard_pci_device = {
    .name          = "S3 Trio64V2/DX On-Board PCI",
    .internal_name = "trio64v2dx_onboard_pci",
    .flags         = DEVICE_PCI,
    .local         = S3_TRIO64V2_DX_ONBOARD,
    .init          = s3_init,
    .close         = s3_close,
    .reset         = NULL,
    .available     = NULL,
    .speed_changed = s3_speed_changed,
    .force_redraw  = s3_force_redraw,
    .config        = s3_standard_config
};
