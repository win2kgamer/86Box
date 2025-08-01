/*
 * 86Box    A hypervisor and IBM PC system emulator that specializes in
 *          running old operating systems and software designed for IBM
 *          PC systems and compatibles from 1981 through fairly recent
 *          system designs based on the PCI bus.
 *
 *          This file is part of the 86Box distribution.
 *
 *          Common UI functions.
 *
 *
 *
 * Authors: Joakim L. Gilje <jgilje@jgilje.net>
 *          Cacodemon345
 *
 *          Copyright 2021 Joakim L. Gilje
 *          Copyright 2021-2022 Cacodemon345
 */
#include <cstdint>

#include <QDebug>
#include <QThread>
#include <QMessageBox>

#include <QStatusBar>
#include <QApplication>

#include "qt_mainwindow.hpp"
#include "qt_machinestatus.hpp"

MainWindow *main_window = nullptr;

static QString sb_text;
static QString sb_buguitext;
static QString sb_mt32lcdtext;

extern "C" {

#include "86box/86box.h"
#include <86box/plat.h>
#include <86box/ui.h>
#include <86box/mouse.h>
#include <86box/timer.h>
#include <86box/86box.h>
#include <86box/device.h>
#include <86box/fdd.h>
#include <86box/hdc.h>
#include <86box/scsi.h>
#include <86box/scsi_device.h>
#include <86box/cartridge.h>
#include <86box/cassette.h>
#include <86box/cdrom.h>
#include <86box/rdisk.h>
#include <86box/mo.h>
#include <86box/hdd.h>
#include <86box/thread.h>
#include <86box/network.h>
#include <86box/machine_status.h>

#ifdef Q_OS_WINDOWS
#    include <86box/win.h>
#endif

void
plat_delay_ms(uint32_t count)
{
#ifdef Q_OS_WINDOWS
    // On Win32 the accuracy of Sleep() depends on the timer resolution, which can be set by calling timeBeginPeriod
    // https://learn.microsoft.com/en-us/windows/win32/api/timeapi/nf-timeapi-timebeginperiod
    timeBeginPeriod(1);
    Sleep(count);
    timeEndPeriod(1);
#else
    QThread::msleep(count);
#endif
}

wchar_t *
ui_window_title(wchar_t *str)
{
    if (str == nullptr) {
        static wchar_t title[512] = { 0 };

        main_window->getTitle(title);
        str = title;
    } else
        emit main_window->setTitle(QString::fromWCharArray(str));

    return str;
}

void
ui_hard_reset_completed()
{
    emit main_window->hardResetCompleted();
}

extern "C" void
qt_blit(int x, int y, int w, int h, int monitor_index)
{
    main_window->blitToWidget(x, y, w, h, monitor_index);
}

extern "C" int vid_resize;
void
plat_resize_request(int w, int h, int monitor_index)
{
    if (video_fullscreen || is_quit)
        return;
    if (vid_resize & 2) {
        plat_resize(fixed_size_x, fixed_size_y, monitor_index);
    } else {
        plat_resize(w, h, monitor_index);
    }
}

void
plat_resize(int w, int h, int monitor_index)
{
    if (monitor_index >= 1)
        main_window->resizeContentsMonitor(w, h, monitor_index);
    else
        main_window->resizeContents(w, h);
}

#if defined _WIN32
extern HWND rw_hwnd;
#endif

void
plat_mouse_capture(int on)
{
    if (!kbd_req_capture && (mouse_type == MOUSE_TYPE_NONE) && !machine_has_mouse())
        return;

    main_window->setMouseCapture(on > 0 ? true : false);

#if defined _WIN32
    if (on) {
        QCursor cursor(Qt::BlankCursor);

        QApplication::setOverrideCursor(cursor);
        QApplication::changeOverrideCursor(cursor);

        RECT rect;

        GetWindowRect(rw_hwnd, &rect);

        ClipCursor(&rect);

    } else {
        ClipCursor(NULL);

        QApplication::restoreOverrideCursor();
    }
#endif
}

int
ui_msgbox_header(int flags, void *header, void *message)
{
    const auto hdr = (flags & MBX_ANSI) ? QString(static_cast<char *>(header)) :
                            QString::fromWCharArray(static_cast<const wchar_t *>(header));
    const auto msg = (flags & MBX_ANSI) ? QString(static_cast<char *>(message)) :
                            QString::fromWCharArray(static_cast<const wchar_t *>(message));

    // any error in early init
    if (main_window == nullptr) {
        auto msgicon = QMessageBox::Icon::Critical;
        if (flags & MBX_INFO)
            msgicon = QMessageBox::Icon::Information;
        else if (flags & MBX_QUESTION)
            msgicon = QMessageBox::Icon::Question;
        else if (flags & MBX_WARNING)
            msgicon = QMessageBox::Icon::Warning;
        QMessageBox msgBox(msgicon, hdr, msg);
        msgBox.exec();
    } else {
        // else scope it to main_window
        main_window->showMessage(flags, hdr, msg, false);
    }
    return 0;
}

void
ui_init_monitor(int monitor_index)
{
    if (QThread::currentThread() == main_window->thread()) {
        emit main_window->initRendererMonitor(monitor_index);
    } else
        emit main_window->initRendererMonitorForNonQtThread(monitor_index);
}

void
ui_deinit_monitor(int monitor_index)
{
    if (QThread::currentThread() == main_window->thread()) {
        emit main_window->destroyRendererMonitor(monitor_index);
    } else
        emit main_window->destroyRendererMonitorForNonQtThread(monitor_index);
}

int
ui_msgbox(int flags, void *message)
{
    return ui_msgbox_header(flags, nullptr, message);
}

void
ui_sb_update_text()
{
    emit main_window->statusBarMessage(!sb_mt32lcdtext.isEmpty() ? sb_mt32lcdtext : sb_text.isEmpty() ? sb_buguitext
                                                                                                      : sb_text);
}

void
ui_sb_mt32lcd(char *str)
{
    sb_mt32lcdtext = QString(str);
    ui_sb_update_text();
}

void
ui_sb_set_text_w(wchar_t *wstr)
{
    sb_text = QString::fromWCharArray(wstr);
    ui_sb_update_text();
}

void
ui_sb_set_text(char *str)
{
    sb_text = str;
    ui_sb_update_text();
}

void
ui_sb_update_tip(int arg)
{
    main_window->updateStatusBarTip(arg);
}

void
ui_sb_update_panes()
{
    main_window->updateStatusBarPanes();
}

void
ui_sb_bugui(char *str)
{
    sb_buguitext = str;
    ui_sb_update_text();
}

void
ui_sb_set_ready(int ready)
{
    if (ready == 0) {
        ui_sb_bugui(nullptr);
        ui_sb_set_text(nullptr);
    }
}

void
ui_sb_update_icon_wp(int tag, int state)
{
    const auto temp    = static_cast<unsigned int>(tag);
    const int category = static_cast<int>(temp & 0xfffffff0);
    const int item     = tag & 0xf;

    switch (category) {
        default:
            break;
        case SB_CASSETTE:
            machine_status.cassette.write_prot = state > 0 ? true : false;
            break;
        case SB_FLOPPY:
            machine_status.fdd[item].write_prot = state > 0 ? true : false;
            break;
        case SB_RDISK:
            machine_status.rdisk[item].write_prot = state > 0 ? true : false;
            break;
        case SB_MO:
            machine_status.mo[item].write_prot = state > 0 ? true : false;
            break;
    }

    if (main_window != nullptr)
        main_window->updateStatusEmptyIcons();
}

void
ui_sb_update_icon_state(int tag, int state)
{
    const auto temp    = static_cast<unsigned int>(tag);
    const int category = static_cast<int>(temp & 0xfffffff0);
    const int item     = tag & 0xf;

    switch (category) {
        default:
            break;
        case SB_CASSETTE:
            machine_status.cassette.empty = state > 0 ? true : false;
            break;
        case SB_CARTRIDGE:
            machine_status.cartridge[item].empty = state > 0 ? true : false;
            break;
        case SB_FLOPPY:
            machine_status.fdd[item].empty = state > 0 ? true : false;
            break;
        case SB_CDROM:
            machine_status.cdrom[item].empty = state > 0 ? true : false;
            break;
        case SB_RDISK:
            machine_status.rdisk[item].empty = state > 0 ? true : false;
            break;
        case SB_MO:
            machine_status.mo[item].empty = state > 0 ? true : false;
            break;
        case SB_HDD:
            break;
        case SB_NETWORK:
            machine_status.net[item].empty = state > 0 ? true : false;
            break;
        case SB_SOUND:
        case SB_TEXT:
            break;
    }

    if (main_window != nullptr)
        main_window->updateStatusEmptyIcons();
}

void
ui_sb_update_icon(int tag, int active)
{
    const auto temp    = static_cast<unsigned int>(tag);
    const int category = static_cast<int>(temp & 0xfffffff0);
    const int item     = tag & 0xf;

    switch (category) {
        default:
        case SB_CASSETTE:
        case SB_CARTRIDGE:
            break;
        case SB_FLOPPY:
            machine_status.fdd[item].active = active > 0 ? true : false;
            break;
        case SB_CDROM:
            machine_status.cdrom[item].active = active > 0 ? true : false;
            break;
        case SB_RDISK:
            machine_status.rdisk[item].active = active > 0 ? true : false;
            break;
        case SB_MO:
            machine_status.mo[item].active = active > 0 ? true : false;
            break;
        case SB_HDD:
            machine_status.hdd[item].active = active > 0 ? true : false;
            break;
        case SB_NETWORK:
            machine_status.net[item].active = active > 0 ? true : false;
            break;
        case SB_SOUND:
        case SB_TEXT:
            break;
    }
}

void
ui_sb_update_icon_write(int tag, int write)
{
    const auto temp    = static_cast<unsigned int>(tag);
    const int category = static_cast<int>(temp & 0xfffffff0);
    const int item     = tag & 0xf;

    switch (category) {
        default:
        case SB_CASSETTE:
        case SB_CARTRIDGE:
            break;
        case SB_FLOPPY:
            machine_status.fdd[item].write_active = write > 0 ? true : false;
            break;
        case SB_CDROM:
            machine_status.cdrom[item].write_active = write > 0 ? true : false;
            break;
        case SB_RDISK:
            machine_status.rdisk[item].write_active = write > 0 ? true : false;
            break;
        case SB_MO:
            machine_status.mo[item].write_active = write > 0 ? true : false;
            break;
        case SB_HDD:
            machine_status.hdd[item].write_active = write > 0 ? true : false;
            break;
        case SB_NETWORK:
            machine_status.net[item].write_active = write > 0 ? true : false;
            break;
        case SB_SOUND:
        case SB_TEXT:
            break;
    }
}

}
