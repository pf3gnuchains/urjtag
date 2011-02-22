/*
 * $Id: cmd_readmem.c 1812 2010-07-18 18:28:21Z vapier $
 *
 * Copyright (C) 2003 ETC s.r.o.
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
 * Written by Marcel Telka <marcel@telka.sk>, 2003.
 *
 */

#include <sysdep.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_LIBREADLINE
#include <readline/readline.h>
#endif

#include <urjtag/error.h>
#include <urjtag/bus.h>

#include <urjtag/cmd.h>

#include "cmd.h"

static int
cmd_readmem_run (urj_chain_t *chain, char *params[])
{
    long unsigned adr;
    long unsigned len;
    int r;
    FILE *f;

    if (urj_cmd_params (params) != 4)
    {
        urj_error_set (URJ_ERROR_SYNTAX,
                       "%s: #parameters should be %d, not %d",
                       params[0], 4, urj_cmd_params (params));
        return URJ_STATUS_FAIL;
    }

    if (!urj_bus)
    {
        urj_error_set (URJ_ERROR_ILLEGAL_STATE, _("Bus missing"));
        return URJ_STATUS_FAIL;
    }

    if (urj_cmd_get_number (params[1], &adr) != URJ_STATUS_OK
        || urj_cmd_get_number (params[2], &len) != URJ_STATUS_OK)
        return URJ_STATUS_FAIL;

    f = fopen (params[3], "wb");
    if (!f)
    {
        urj_error_IO_set (_("Unable to create file `%s'"), params[3]);
        return URJ_STATUS_FAIL;
    }
    r = urj_bus_readmem (urj_bus, f, adr, len);
    fclose (f);

    return r;
}

static void
cmd_readmem_help (void)
{
    urj_log (URJ_LOG_LEVEL_NORMAL,
             _("Usage: %s ADDR LEN FILENAME\n"
               "Copy device memory content starting with ADDR to FILENAME file.\n"
               "\n"
               "ADDR       start address of the copied memory area\n"
               "LEN        copied memory length\n"
               "FILENAME   name of the output file\n"
               "\n"
               "ADDR and LEN could be in decimal or hexadecimal (prefixed with 0x) form.\n"),
             "readmem");
}

static void
cmd_readmem_complete (urj_chain_t *chain, char ***matches, size_t *match_cnt,
                      const char *text, size_t text_len, size_t token_point)
{
    switch (token_point)
    {
    case 1: /* addr */
        break;

    case 2: /* len */
        break;

    case 3: /* filename */
    {
#ifdef HAVE_LIBREADLINE
        int state;
        char *match;

        state = 0;
        while (1)
        {
            match = rl_filename_completion_function (text, state++);
            if (!match)
                break;
            urj_completion_add_match_dupe (matches, match_cnt, match);
            free (match);
        }
#endif
        break;
    }
    }
}

const urj_cmd_t urj_cmd_readmem = {
    "readmem",
    N_("read content of the memory and write it to file"),
    cmd_readmem_help,
    cmd_readmem_run,
    cmd_readmem_complete,
};
