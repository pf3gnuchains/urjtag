/*
 * $Id$
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
#include <string.h>

#include <urjtag/error.h>
#include <urjtag/tap.h>

#include <urjtag/cmd.h>

#include "cmd.h"

static int
cmd_discovery_run (urj_chain_t *chain, char *params[])
{
    int ret;

    if (urj_cmd_test_cable (chain) != URJ_STATUS_OK)
        return URJ_STATUS_FAIL;

    switch (urj_cmd_params (params))
    {
    case 1:
        ret = urj_tap_discovery (chain);
        break;

    case 2:
        if (strcmp (params[1], "current") == 0)
            ret = urj_tap_discovery_one_dr (chain, NULL);
        else
            ret = urj_tap_discovery_one_dr (chain, params[1]);
        break;

    default:
        urj_error_set (URJ_ERROR_SYNTAX,
                       "%s: #parameters should be 1 or 2, not %d",
                       params[0], urj_cmd_params (params));
        ret = URJ_STATUS_FAIL;
        break;
    }

    return ret;
}

static void
cmd_discovery_help (void)
{
    urj_log (URJ_LOG_LEVEL_NORMAL,
             _("Usage: %s [current|INSTRUCTION]\n"
               "When no arguments, discovery of unknown parts in the JTAG chain.\n"
               "\n"
               "'%s' attempts to detect these parameters of an unknown JTAG\n"
               "chain:\n"
               " 1. IR (instruction register) length\n"
               " 2. DR (data register) length for all possible instructions\n"
               "\n"
               "Warning: This may be dangerous for some parts (especially if the\n"
               "part doesn't have TRST signal).\n"
               "\n"
               "When there is an argument, discovery of the DR size of the current or\n"
               "specified instruction.\n"), "discovery",
            "discovery");
}

const urj_cmd_t urj_cmd_discovery = {
    "discovery",
    N_("discovery of unknown parts in the JTAG chain"),
    cmd_discovery_help,
    cmd_discovery_run
};
