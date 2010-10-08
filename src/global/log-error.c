/*
 * $Id: log-error.c 1763 2010-02-05 23:09:29Z vapier $
 *
 * Copyright (C) 2009 Rutger Hofman, VU Amsterdam
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * Written by Rutger Hofman
 */

#include <sysdep.h>

#include <stdarg.h>
#include <errno.h>
#include <string.h>

#include <urjtag/log.h>
#include <urjtag/error.h>
#include <urjtag/jtag.h>

urj_error_state_t urj_error_state;

static int stderr_vprintf (const char *fmt, va_list ap);
static int stdout_vprintf (const char *fmt, va_list ap);

urj_log_state_t urj_log_state =
    {
        .level = URJ_LOG_LEVEL_NORMAL,
        .out_vprintf = stdout_vprintf,
        .err_vprintf = stderr_vprintf,
    };

static int
stderr_vprintf(const char *fmt, va_list ap)
{
    return vfprintf (stderr, fmt, ap);
}

static int
stdout_vprintf(const char *fmt, va_list ap)
{
    int r = vfprintf (stdout, fmt, ap);

    fflush (stdout);

    return r;
}

int
urj_do_log (urj_log_level_t level, const char *fmt, ...)
{
    va_list ap;
    int r;

    if (level < urj_log_state.level)
        return 0;

    va_start (ap, fmt);
    if (level < URJ_LOG_LEVEL_WARNING)
        r = urj_log_state.out_vprintf (fmt, ap);
    else
        r = urj_log_state.err_vprintf (fmt, ap);
    va_end (ap);

    return r;
}

urj_error_t
urj_error_get (void)
{
    return urj_error_state.errnum;
}

void
urj_error_reset (void)
{
    urj_error_state.errnum = URJ_ERROR_OK;
}

const char *
urj_error_string (urj_error_t err)
{
    switch (err)
    {
    case URJ_ERROR_OK:                  return "no error";
    case URJ_ERROR_ALREADY:             return "already defined";
    case URJ_ERROR_OUT_OF_MEMORY:       return "out of memory";
    case URJ_ERROR_NO_CHAIN:            return "no chain";
    case URJ_ERROR_NO_PART:             return "no part";
    case URJ_ERROR_NO_ACTIVE_INSTRUCTION: return "no active instruction";
    case URJ_ERROR_NO_DATA_REGISTER:    return "no data register";
    case URJ_ERROR_INVALID:             return "invalid parameter";
    case URJ_ERROR_NOTFOUND:            return "not found";
    case URJ_ERROR_NO_BUS_DRIVER:       return "no bus driver";
    case URJ_ERROR_BUFFER_EXHAUSTED:    return "buffer exhausted";
    case URJ_ERROR_ILLEGAL_STATE:       return "illegal state";
    case URJ_ERROR_ILLEGAL_TRANSITION:  return "illegal state transition";
    case URJ_ERROR_OUT_OF_BOUNDS:       return "out of bounds";
    case URJ_ERROR_TIMEOUT:             return "timeout";
    case URJ_ERROR_UNSUPPORTED:         return "unsupported";
    case URJ_ERROR_SYNTAX:              return "syntax";
    case URJ_ERROR_FILEIO:              return "file I/O";

    case URJ_ERROR_IO:                  return "I/O error from OS";
    case URJ_ERROR_FTD:                 return "ftdi/ftd2xx error";
    case URJ_ERROR_USB:                 return "libusb error";

    case URJ_ERROR_BUS:                 return "bus";
    case URJ_ERROR_BUS_DMA:             return "bus DMA";

    case URJ_ERROR_FLASH:               return "flash";
    case URJ_ERROR_FLASH_DETECT:        return "flash detect";
    case URJ_ERROR_FLASH_PROGRAM:       return "flash program";
    case URJ_ERROR_FLASH_ERASE:         return "flash erase";
    case URJ_ERROR_FLASH_UNLOCK:        return "flash unlock";

    case URJ_ERROR_BSDL_VHDL:           return "vhdl subsystem";
    case URJ_ERROR_BSDL_BSDL:           return "bsdl subsystem";

    case URJ_ERROR_BFIN:                return "blackfin";

    case URJ_ERROR_PLD:                 return "pld subsystem";

    case URJ_ERROR_UNIMPLEMENTED:       return "unimplemented";
    }

    return "UNDEFINED ERROR";
}

const char *
urj_error_describe (void)
{
    static char msg[URJ_ERROR_MSG_LEN + 1024 + 256 + 20];

    if (urj_error_state.errnum == URJ_ERROR_IO)
    {
        snprintf (msg, sizeof msg, "%s:%d %s() %s: %s %s",
                  urj_error_state.file, urj_error_state.line,
                  urj_error_state.function,
                  "System error", strerror(urj_error_state.sys_errno),
                  urj_error_state.msg);
    }
    else
    {
        snprintf (msg, sizeof msg, "%s:%d %s() %s: %s",
                  urj_error_state.file, urj_error_state.line,
                  urj_error_state.function,
                  urj_error_string (urj_error_state.errnum),
                  urj_error_state.msg);
    }

    return msg;
}
