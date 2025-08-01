/*
 * 86Box    A hypervisor and IBM PC system emulator that specializes in
 *          running old operating systems and software designed for IBM
 *          PC systems and compatibles from 1981 through fairly recent
 *          system designs based on the PCI bus.
 *
 *          This file is part of the 86Box distribution.
 *
 *          Implementation of 286 and 386SX machines.
 *
 *
 *
 * Authors: Miran Grca, <mgrca8@gmail.com>
 *          EngiNerd <webmaster.crrc@yahoo.it>
 *
 *          Copyright 2016-2019 Miran Grca.
 *          Copyright 2020 EngiNerd.
 */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#define HAVE_STDARG_H
#include <86box/86box.h>
#include "cpu.h"
#include <86box/timer.h>
#include <86box/io.h>
#include <86box/device.h>
#include <86box/chipset.h>
#include <86box/keyboard.h>
#include <86box/mem.h>
#include <86box/rom.h>
#include <86box/fdd.h>
#include <86box/fdc.h>
#include <86box/fdc_ext.h>
#include <86box/hdc.h>
#include <86box/nvr.h>
#include <86box/port_6x.h>
#define USE_SIO_DETECT
#include <86box/sio.h>
#include <86box/serial.h>
#include <86box/video.h>
#include <86box/vid_cga.h>
#include <86box/flash.h>
#include <86box/machine.h>

int
machine_at_mr286_init(const machine_t *model)
{
    int ret;

    ret = bios_load_interleaved("roms/machines/mr286/V000B200-1",
                                "roms/machines/mr286/V000B200-2",
                                0x000f0000, 65536, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_common_ide_init(model);
    device_add(&kbc_at_device);

    if (fdc_current[0] == FDC_INTERNAL)
        device_add(&fdc_at_device);

    return ret;
}

static void
machine_at_headland_common_init(const machine_t *model, int type)
{
    device_add(&kbc_at_ami_device);

    if ((type != 2) && (fdc_current[0] == FDC_INTERNAL))
        device_add(&fdc_at_device);

    if (type == 2)
        device_add(&headland_ht18b_device);
    else if (type == 1)
        device_add(&headland_gc113_device);
    else
        device_add(&headland_gc10x_device);
}

int
machine_at_tg286m_init(const machine_t *model)
{
    int ret;

    ret = bios_load_linear("roms/machines/tg286m/ami.bin",
                           0x000f0000, 131072, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_common_ide_init(model);

    machine_at_headland_common_init(model, 1);

    return ret;
}

int
machine_at_ama932j_init(const machine_t *model)
{
    int ret;

    ret = bios_load_linear("roms/machines/ama932j/ami.bin",
                           0x000f0000, 131072, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_common_ide_init(model);

    if (gfxcard[0] == VID_INTERNAL)
        device_add(&oti067_ama932j_device);

    machine_at_headland_common_init(model, 2);

    device_add_params(&pc87310_device, (void *) (PC87310_ALI));

    return ret;
}

int
machine_at_quadt286_init(const machine_t *model)
{
    int ret;

    ret = bios_load_interleaved("roms/machines/quadt286/QUADT89L.ROM",
                                "roms/machines/quadt286/QUADT89H.ROM",
                                0x000f0000, 65536, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_common_init(model);
    device_add(&kbc_at_device);

    if (fdc_current[0] == FDC_INTERNAL)
        device_add(&fdc_at_device);

    device_add(&headland_gc10x_device);

    return ret;
}

int
machine_at_quadt386sx_init(const machine_t *model)
{
    int ret;

    ret = bios_load_interleaved("roms/machines/quadt386sx/QTC-SXM-EVEN-U3-05-07.BIN",
                                "roms/machines/quadt386sx/QTC-SXM-ODD-U3-05-07.BIN",
                                0x000f0000, 65536, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_common_init(model);
    device_add(&kbc_at_device);

    if (fdc_current[0] == FDC_INTERNAL)
        device_add(&fdc_at_device);

    device_add(&headland_gc10x_device);

    return ret;
}

static const device_config_t pbl300sx_config[] = {
    // clang-format off
    {
        .name = "bios",
        .description = "BIOS Version",
        .type = CONFIG_BIOS,
        .default_string = "pbl300sx",
        .default_int = 0,
        .file_filter = "",
        .spinner = { 0 },
        .bios = {
            { .name = "Phoenix ROM BIOS PLUS 1.10 - Revision 19910723091302", .internal_name = "pbl300sx_1991", .bios_type = BIOS_NORMAL, 
              .files_no = 1, .local = 0, .size = 131072, .files = { "roms/machines/pbl300sx/V1.10_1113_910723.bin", "" } },
            { .name = "Phoenix ROM BIOS PLUS 1.10 - Revision 19920910", .internal_name = "pbl300sx", .bios_type = BIOS_NORMAL, 
              .files_no = 1, .local = 0, .size = 131072, .files = { "roms/machines/pbl300sx/pb_l300sx_1992.bin", "" } },
            { .files_no = 0 }
        },
    },
    { .name = "", .description = "", .type = CONFIG_END }
    // clang-format on
};

const device_t pbl300sx_device = {
    .name          = "Packard Bell Legend 300SX",
    .internal_name = "pbl300sx_device",
    .flags         = 0,
    .local         = 0,
    .init          = NULL,
    .close         = NULL,
    .reset         = NULL,
    .available     = NULL,
    .speed_changed = NULL,
    .force_redraw  = NULL,
    .config        = pbl300sx_config
};

int
machine_at_pbl300sx_init(const machine_t *model)
{
    int ret = 0;
    const char* fn;

    /* No ROMs available */
    if (!device_available(model->device))
        return ret;

    device_context(model->device);
    fn = device_get_bios_file(machine_get_device(machine), device_get_config_bios("bios"), 0);
    ret = bios_load_linear(fn, 0x000e0000, 131072, 0);
    device_context_restore();

    if (bios_only || !ret)
        return ret;

    machine_at_common_init(model);
    device_add(&acc2036_device);

    device_add(&kbc_ps2_phoenix_device);
    device_add(&um82c862f_ide_device);

    if (gfxcard[0] == VID_INTERNAL)
        device_add(machine_get_vid_device(machine));

    return ret;
}

int
machine_at_neat_init(const machine_t *model)
{
    int ret;

    ret = bios_load_linear("roms/machines/dtk386/3cto001.bin",
                           0x000f0000, 65536, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_init(model);

    device_add(&neat_device);

    if (fdc_current[0] == FDC_INTERNAL)
        device_add(&fdc_at_device);

    return ret;
}

int
machine_at_neat_ami_init(const machine_t *model)
{
    int ret;

    ret = bios_load_linear("roms/machines/ami286/AMIC206.BIN",
                           0x000f0000, 65536, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_common_init(model);

    device_add(&neat_device);

    if (fdc_current[0] == FDC_INTERNAL)
        device_add(&fdc_at_device);

    device_add(&kbc_at_ami_device);

    return ret;
}

// TODO
// Onboard Paradise PVGA1A-JK VGA Graphics
// Data Technology Corporation DTC7187 RLL Controller (Optional)
int
machine_at_ataripc4_init(const machine_t *model)
{
    int ret;

    ret = bios_load_interleaved("roms/machines/ataripc4/AMI_PC4X_1.7_EVEN.BIN",
                                "roms/machines/ataripc4/AMI_PC4X_1.7_ODD.BIN",
                                0x000f0000, 65536, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_common_init(model);

    device_add(&neat_device);

    if (fdc_current[0] == FDC_INTERNAL)
        device_add(&fdc_at_device);

    device_add(&kbc_at_ami_device);

    return ret;
}

int
machine_at_px286_init(const machine_t *model)
{
    int ret;

    ret = bios_load_linear("roms/machines/px286/KENITEC.BIN",
                           0x000f0000, 65536, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_common_init(model);
    device_add(&kbc_at_device);

    if (fdc_current[0] == FDC_INTERNAL)
        device_add(&fdc_at_device);

    device_add(&neat_device);

    return ret;
}

static void
machine_at_ctat_common_init(const machine_t *model)
{
    machine_at_common_init(model);

    device_add(&cs8220_device);

    if (fdc_current[0] == FDC_INTERNAL)
        device_add(&fdc_at_device);

    device_add(&kbc_at_phoenix_device);
}

int
machine_at_dells200_init(const machine_t *model)
{
    int ret;

    ret = bios_load_interleaved("roms/machines/dells200/dellL200256_LO_@DIP28.BIN",
                                "roms/machines/dells200/Dell200256_HI_@DIP28.BIN",
                                0x000f0000, 65536, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_ctat_common_init(model);

    return ret;
}

int
machine_at_at122_init(const machine_t *model)
{
    int ret;

    ret = bios_load_linear("roms/machines/at122/FINAL.BIN",
                           0x000f0000, 65536, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_ctat_common_init(model);

    return ret;
}

int
machine_at_tuliptc7_init(const machine_t *model)
{
    int ret;

    ret = bios_load_interleavedr("roms/machines/tuliptc7/tc7be.bin",
                                 "roms/machines/tuliptc7/tc7bo.bin",
                                 0x000f8000, 65536, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_ctat_common_init(model);

    return ret;
}

int
machine_at_wellamerastar_init(const machine_t *model)
{
    int ret;

    ret = bios_load_interleaved("roms/machines/wellamerastar/W_3.031_L.BIN",
                                "roms/machines/wellamerastar/W_3.031_H.BIN",
                                0x000f0000, 65536, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_ctat_common_init(model);

    return ret;
}

static void
machine_at_scat_init(const machine_t *model, int is_v4, int is_ami)
{
    machine_at_common_init(model);

    if (machines[machine].bus_flags & MACHINE_BUS_PS2) {
        if (is_ami)
            device_add(&kbc_ps2_ami_device);
        else
            device_add(&kbc_ps2_device);
    } else {
        if (is_ami)
            device_add(&kbc_at_ami_device);
        else
            device_add(&kbc_at_device);
    }

    if (is_v4)
        device_add(&scat_4_device);
    else
        device_add(&scat_device);
}

static void
machine_at_scatsx_init(const machine_t *model)
{
    machine_at_common_init(model);

    device_add(&kbc_at_ami_device);

    if (fdc_current[0] == FDC_INTERNAL)
        device_add(&fdc_at_device);

    device_add(&scat_sx_device);
}

int
machine_at_award286_init(const machine_t *model)
{
    int ret;

    ret = bios_load_linear("roms/machines/award286/award.bin",
                           0x000f0000, 131072, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_scat_init(model, 0, 1);

    if (fdc_current[0] == FDC_INTERNAL)
        device_add(&fdc_at_device);

    device_add(&ide_isa_device);

    return ret;
}

int
machine_at_gdc212m_init(const machine_t *model)
{
    int ret;

    ret = bios_load_linear("roms/machines/gdc212m/gdc212m_72h.bin",
                           0x000f0000, 65536, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_scat_init(model, 0, 1);

    if (fdc_current[0] == FDC_INTERNAL)
        device_add(&fdc_at_device);

    device_add(&ide_isa_device);

    return ret;
}

int
machine_at_gw286ct_init(const machine_t *model)
{
    int ret;

    ret = bios_load_linear("roms/machines/gw286ct/2ctc001.bin",
                           0x000f0000, 131072, 0);

    if (bios_only || !ret)
        return ret;

    device_add(&f82c710_device);

    machine_at_scat_init(model, 1, 0);

    device_add(&ide_isa_device);

    return ret;
}

int
machine_at_drsm35286_init(const machine_t *model)
{
    int ret;

    ret = bios_load_linear("roms/machines/drsm35286/syab04-665821fb81363428830424.bin",
                           0x000f0000, 65536, 0);

    if (bios_only || !ret)
        return ret;
	
    device_add(&ide_isa_device);
    device_add(&fdc37c651_ide_device);
   
    machine_at_scat_init(model, 1, 0);
	
	if (gfxcard[0] == VID_INTERNAL)
        device_add(machine_get_vid_device(machine));

    return ret;
}

int
machine_at_senor_scat286_init(const machine_t *model)
{
    int ret;

    ret = bios_load_linear("roms/machines/senor286/AMI-DSC2-1115-061390-K8.rom",
                           0x000f0000, 65536, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_scat_init(model, 0, 1);

    if (fdc_current[0] == FDC_INTERNAL)
        device_add(&fdc_at_device);

    return ret;
}

int
machine_at_super286c_init(const machine_t *model)
{
    int ret;

    ret = bios_load_linear("roms/machines/super286c/hyundai_award286.bin",
                           0x000f0000, 65536, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_common_init(model);

    device_add(&kbc_at_ami_device);

    if (fdc_current[0] == FDC_INTERNAL)
        device_add(&fdc_at_device);

    device_add(&cs8220_device);

    return ret;
}

int
machine_at_super286tr_init(const machine_t *model)
{
    int ret;

    ret = bios_load_linear("roms/machines/super286tr/hyundai_award286.bin",
                           0x000f0000, 131072, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_scat_init(model, 0, 1);

    if (fdc_current[0] == FDC_INTERNAL)
        device_add(&fdc_at_device);

    return ret;
}

int
machine_at_spc4200p_init(const machine_t *model)
{
    int ret;

    ret = bios_load_linear("roms/machines/spc4200p/u8.01",
                           0x000f0000, 131072, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_scat_init(model, 0, 1);

    device_add(&f82c710_device);

    device_add(&ide_isa_device);

    return ret;
}

int
machine_at_spc4216p_init(const machine_t *model)
{
    int ret;

    ret = bios_load_interleaved("roms/machines/spc4216p/7101.U8",
                                "roms/machines/spc4216p/AC64.U10",
                                0x000f0000, 131072, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_scat_init(model, 1, 1);

    device_add(&f82c710_device);

    return ret;
}

int
machine_at_spc4620p_init(const machine_t *model)
{
    int ret;

    ret = bios_load_interleaved("roms/machines/spc4620p/31005h.u8",
                                "roms/machines/spc4620p/31005h.u10",
                                0x000f0000, 131072, 0x8000);

    if (bios_only || !ret)
        return ret;

    if (gfxcard[0] == VID_INTERNAL)
        device_add(&ati28800k_spc4620p_device);

    machine_at_scat_init(model, 1, 1);

    device_add(&f82c710_device);

    device_add(&ide_isa_device);

    return ret;
}

int
machine_at_kmxc02_init(const machine_t *model)
{
    int ret;

    ret = bios_load_linear("roms/machines/kmxc02/3ctm005.bin",
                           0x000f0000, 131072, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_scatsx_init(model);

    return ret;
}

int
machine_at_deskmaster286_init(const machine_t *model)
{
    int ret;

    ret = bios_load_linear("roms/machines/deskmaster286/SAMSUNG-DESKMASTER-28612-ROM.BIN",
                           0x000f0000, 131072, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_scat_init(model, 0, 1);

    device_add(&f82c710_device);
        
    device_add(&ide_isa_device);

    return ret;
}

int
machine_at_shuttle386sx_init(const machine_t *model)
{
    int ret;

    ret = bios_load_interleaved("roms/machines/shuttle386sx/386-Shuttle386SX-Even.BIN",
                                "roms/machines/shuttle386sx/386-Shuttle386SX-Odd.BIN",
                                0x000f0000, 131072, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_common_init(model);

    device_add(&intel_82335_device);
    device_add(&kbc_at_ami_device);

    if (fdc_current[0] == FDC_INTERNAL)
        device_add(&fdc_at_device);

    return ret;
}

int
machine_at_adi386sx_init(const machine_t *model)
{
    int ret;

    ret = bios_load_interleaved("roms/machines/adi386sx/3iip001l.bin",
                                "roms/machines/adi386sx/3iip001h.bin",
                                0x000f0000, 65536, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_common_init_ex(model, 2);
    device_add(&amstrad_megapc_nvr_device); /* NVR that is initialized to all 0x00's. */

    device_add(&intel_82335_device);
    device_add(&kbc_at_ami_device);

    if (fdc_current[0] == FDC_INTERNAL)
        device_add(&fdc_at_device);

    return ret;
}

int
machine_at_wd76c10_init(const machine_t *model)
{
    int ret;

    ret = bios_load_interleaved("roms/machines/megapc/41651-bios lo.u18",
                                "roms/machines/megapc/211253-bios hi.u19",
                                0x000f0000, 65536, 0x08000);

    if (bios_only || !ret)
        return ret;

    machine_at_common_init_ex(model, 2);

    if (gfxcard[0] == VID_INTERNAL)
        device_add(&paradise_wd90c11_megapc_device);

    device_add(&kbc_ps2_quadtel_device);

    device_add(&wd76c10_device);

    return ret;
}

int
machine_at_cmdsl386sx16_init(const machine_t *model)
{
    int ret;

    ret = bios_load_interleaved("roms/machines/cmdsl386sx16/cbm-sl386sx-bios-lo-v1.04-390914-04.bin",
                                "roms/machines/cmdsl386sx16/cbm-sl386sx-bios-hi-v1.04-390915-04.bin",
                                0x000f0000, 65536, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_common_init(model);

    device_add(&kbc_ps2_device);

    if (fdc_current[0] == FDC_INTERNAL)
        device_add(&fdc_at_device);

    device_add(&neat_device);
    /* Two serial ports - on the real hardware SL386SX-16, they are on the single UMC UM82C452. */
    device_add_inst(&ns16450_device, 1);
    device_add_inst(&ns16450_device, 2);

    return ret;
}

int
machine_at_if386sx_init(const machine_t *model)
{
    int ret;

    ret = bios_load_interleaved("roms/machines/if386sx/OKI_IF386SX_odd.bin",
                                "roms/machines/if386sx/OKI_IF386SX_even.bin",
                                0x000f0000, 65536, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_common_init_ex(model, 2);
    device_add(&amstrad_megapc_nvr_device); /* NVR that is initialized to all 0x00's. */

    device_add(&kbc_at_phoenix_device);

    device_add(&neat_sx_device);

    device_add(&if386jega_device);

    if (fdc_current[0] == FDC_INTERNAL)
        device_add(&fdc_at_device);

    /*
       One serial port - on the real hardware IF386AX, it is on the VL 16C451,
       alognside the bidirectional parallel port.
     */
    device_add_inst(&ns16450_device, 1);

    return ret;
}

static void
machine_at_scamp_common_init(const machine_t *model, int is_ps2)
{
    machine_at_common_ide_init(model);

    if (is_ps2)
        device_add(&kbc_ps2_ami_device);
    else
        device_add(&kbc_at_ami_device);

    if (fdc_current[0] == FDC_INTERNAL)
        device_add(&fdc_at_device);

    device_add(&vlsi_scamp_device);
}

int
machine_at_cmdsl386sx25_init(const machine_t *model)
{
    int ret;

    ret = bios_load_linear("roms/machines/cmdsl386sx25/f000.rom",
                           0x000f0000, 65536, 0);

    if (bios_only || !ret)
        return ret;

    if (gfxcard[0] == VID_INTERNAL)
        device_add(&gd5402_onboard_device);

    machine_at_common_init_ex(model, 2);

    device_add(&ide_isa_device);

    device_add_params(&pc87310_device, (void *) (PC87310_ALI));
    device_add(&vl82c113_device); /* The keyboard controller is part of the VL82c113. */

    device_add(&vlsi_scamp_device);

    return ret;
}

static const device_config_t dells333sl_config[] = {
    // clang-format off
    {
        .name = "bios",
        .description = "BIOS Version",
        .type = CONFIG_BIOS,
        .default_string = "dells333sl",
        .default_int = 0,
        .file_filter = "",
        .spinner = { 0 },
        .bios = {
            { .name = "Phoenix ROM BIOS PLUS 1.10 - Revision J01 (Jostens Learning Corporation OEM)", .internal_name = "dells333sl_j01", .bios_type = BIOS_NORMAL, 
              .files_no = 1, .local = 0, .size = 131072, .files = { "roms/machines/dells333sl/DELL386.BIN", "" } },
            { .name = "Phoenix ROM BIOS PLUS 1.10 - Revision A02", .internal_name = "dells333sl", .bios_type = BIOS_NORMAL, 
              .files_no = 1, .local = 0, .size = 131072, .files = { "roms/machines/dells333sl/Dell_386SX_30807_UBIOS_B400_VLSI_VL82C311_Cirrus_Logic_GD5420.bin", "" } },
            { .files_no = 0 }
        },
    },
    { .name = "", .description = "", .type = CONFIG_END }
    // clang-format on
};

const device_t dells333sl_device = {
    .name          = "Dell System 333s/L",
    .internal_name = "dells333sl_device",
    .flags         = 0,
    .local         = 0,
    .init          = NULL,
    .close         = NULL,
    .reset         = NULL,
    .available     = NULL,
    .speed_changed = NULL,
    .force_redraw  = NULL,
    .config        = dells333sl_config
};

int
machine_at_dells333sl_init(const machine_t *model)
{
    int ret = 0;
    const char* fn;

    /* No ROMs available */
    if (!device_available(model->device))
        return ret;

    device_context(model->device);
    fn = device_get_bios_file(machine_get_device(machine), device_get_config_bios("bios"), 0);
    ret = bios_load_linear(fn, 0x000e0000, 262144, 0);
    memcpy(rom, &(rom[0x00020000]), 131072);
    mem_mapping_set_addr(&bios_mapping, 0x0c0000, 0x40000);
    mem_mapping_set_exec(&bios_mapping, rom);
    device_context_restore();

    if (gfxcard[0] == VID_INTERNAL)
        device_add(machine_get_vid_device(machine));

    machine_at_common_init_ex(model, 2);

    device_add(&ide_isa_device);

    device_add(&pc87311_device);
    device_add(&vl82c113_device); /* The keyboard controller is part of the VL82c113. */

    device_add(&vlsi_scamp_device);

    return ret;
}

int
machine_at_dataexpert386sx_init(const machine_t *model)
{
    int ret;

    ret = bios_load_linear("roms/machines/dataexpert386sx/5e9f20e5ef967717086346.BIN",
                           0x000f0000, 65536, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_scamp_common_init(model, 0);

    return ret;
}

int
machine_at_spc6033p_init(const machine_t *model)
{
    int ret;

    ret = bios_load_linear("roms/machines/spc6033p/phoenix.BIN",
                           0x000f0000, 65536, 0x10000);

    if (bios_only || !ret)
        return ret;

    if (gfxcard[0] == VID_INTERNAL)
        device_add(&ati28800k_spc6033p_device);

    machine_at_scamp_common_init(model, 1);

    return ret;
}

int
machine_at_awardsx_init(const machine_t *model)
{
    int ret;

    ret = bios_load_linear("roms/machines/awardsx/Unknown 386SX OPTi291 - Award (original).BIN",
                           0x000f0000, 65536, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_init(model);

    device_add(&opti291_device);

    if (fdc_current[0] == FDC_INTERNAL)
        device_add(&fdc_at_device);

    return ret;
}

int
machine_at_acer100t_init(const machine_t *model)
{
    int ret;

    ret = bios_load_linear("roms/machines/acer100t/acer386.BIN",
                           0x000f0000, 65536, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_ps2_ide_init(model);

    device_add(&ali1409_device);
    if (gfxcard[0] == VID_INTERNAL)
        device_add(&oti077_acer100t_device);
     
    device_add_params(&pc87310_device, (void *) (PC87310_ALI));
    
    return ret;
}


int
machine_at_arb1374_init(const machine_t *model)
{
    int ret;

    ret = bios_load_linear("roms/machines/arb1374/1374s.rom",
                           0x000f0000, 65536, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_common_init(model);

    device_add(&ali1217_device);
    device_add(&w83787f_ide_en_device);
    device_add(&kbc_ps2_ami_device);

    return ret;
}

int
machine_at_sbc350a_init(const machine_t *model)
{
    int ret;

    ret = bios_load_linear("roms/machines/sbc350a/350a.rom",
                           0x000f0000, 65536, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_common_init(model);

    device_add(&ali1217_device);
    device_add(&ide_isa_device);
    device_add(&fdc37c665_ide_pri_device);
    device_add(&kbc_ps2_ami_device);

    return ret;
}

int
machine_at_flytech386_init(const machine_t *model)
{
    int ret;

    ret = bios_load_linear("roms/machines/flytech386/FLYTECH.BIO",
                           0x000f0000, 65536, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_common_init(model);

    device_add(&ali1217_device);
    device_add(&w83787f_ide_en_device);

    if (gfxcard[0] == VID_INTERNAL)
        device_add(&tvga8900d_device);

    device_add(&kbc_at_ami_device);

    return ret;
}

int
machine_at_325ax_init(const machine_t *model)
{
    int ret;

    ret = bios_load_linear("roms/machines/325ax/M27C512.BIN",
                           0x000f0000, 65536, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_common_init(model);

    device_add(&ali1217_device);
    device_add(&fdc_at_device);
    device_add(&kbc_at_ami_device);

    return ret;
}

int
machine_at_mr1217_init(const machine_t *model)
{
    int ret;

    ret = bios_load_linear("roms/machines/mr1217/mrbios.BIN",
                           0x000f0000, 65536, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_common_init(model);

    device_add(&ali1217_device);
    device_add(&fdc_at_device);
    device_add(&kbc_at_ami_device);

    return ret;
}

int
machine_at_pja511m_init(const machine_t *model)
{
    int ret;

    ret = bios_load_linear("roms/machines/pja511m/2006915102435734.rom",
                           0x000e0000, 131072, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_common_init(model);

    device_add_inst(&fdc37c669_device, 1);
    device_add_inst(&fdc37c669_device, 2);
    device_add(&kbc_ps2_ami_pci_device);
    device_add(&ali6117d_device);
    device_add(&sst_flash_29ee010_device);

    return ret;
}

int
machine_at_prox1332_init(const machine_t *model)
{
    int ret;

    ret = bios_load_linear("roms/machines/prox1332/D30B3AC1.BIN",
                           0x000e0000, 131072, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_common_init(model);

    device_add(&fdc37c669_device);
    device_add(&kbc_ps2_ami_pci_device);
    device_add(&ali6117d_device);
    device_add(&sst_flash_29ee010_device);

    return ret;
}

/*
 * Current bugs:
 * - ctrl-alt-del produces an 8042 error
 */
int
machine_at_pc8_init(const machine_t *model)
{
    int ret;

    ret = bios_load_interleaved("roms/machines/pc8/ncr_35117_u127_vers.4-2.bin",
                                "roms/machines/pc8/ncr_35116_u113_vers.4-2.bin",
                                0x000f0000, 65536, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_common_init(model);
    device_add(&kbc_at_ncr_device);

    if (fdc_current[0] == FDC_INTERNAL)
        device_add(&fdc_at_device);

    return ret;
}

int
machine_at_3302_init(const machine_t *model)
{
    int ret;

    ret = bios_load_linear("roms/machines/3302/f000-flex_drive_test.bin",
                           0x000f0000, 65536, 0);

    if (ret) {
        ret &= bios_load_aux_linear("roms/machines/3302/f800-setup_ncr3.5-013190.bin",
                                    0x000f8000, 32768, 0);
    }

    if (bios_only || !ret)
        return ret;

    machine_at_common_ide_init(model);
    device_add(&neat_device);

    if (fdc_current[0] == FDC_INTERNAL)
        device_add(&fdc_at_device);

    if (gfxcard[0] == VID_INTERNAL)
        device_add(machine_get_vid_device(machine));

    device_add(&kbc_at_ncr_device);

    return ret;
}

/*
 * Current bugs:
 * - soft-reboot after saving CMOS settings/pressing ctrl-alt-del produces an 8042 error
 */
int
machine_at_pc916sx_init(const machine_t *model)
{
    int ret;

    ret = bios_load_interleaved("roms/machines/pc916sx/ncr_386sx_u46-17_7.3.bin",
                                "roms/machines/pc916sx/ncr_386sx_u12-19_7.3.bin",
                                0x000f0000, 65536, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_common_init(model);

    device_add(&kbc_at_ncr_device);
    mem_remap_top(384);

    if (fdc_current[0] == FDC_INTERNAL)
        device_add(&fdc_at_device);

    return ret;
}

int
machine_at_m290_init(const machine_t *model)
{
    int ret;

    ret = bios_load_linear("roms/machines/m290/m290_pep3_1.25.bin",
                           0x000f0000, 65536, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_common_init_ex(model, 6);
    device_add(&amstrad_megapc_nvr_device);

    device_add(&olivetti_eva_device);
    device_add(&port_6x_olivetti_device);

    if (fdc_current[0] == FDC_INTERNAL)
        device_add(&fdc_at_device);

    device_add(&kbc_at_olivetti_device);

    return ret;
}
