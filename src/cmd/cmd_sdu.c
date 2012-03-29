/* Copyright (C) 2011 Analog Devices, Inc.
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
 */


#include "sysdep.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <unistd.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include <urjtag/part.h>
#include <urjtag/chain.h>
#include <urjtag/tap.h>
#include <urjtag/cmd.h>
#include <urjtag/sdu.h>

#include "cmd.h"

static int
cmd_sdu_run (urj_chain_t *chain, char *params[])
{
    int num_params;

    num_params = urj_cmd_params (params);

    if (num_params < 2)
    {
        urj_error_set (URJ_ERROR_SYNTAX,
                       "#parameters should be >= 2, not %d",
                       num_params);
        return URJ_STATUS_FAIL;
    }

    /* The remaining commands require cable or parts.  */

    if (urj_cmd_test_cable (chain) != URJ_STATUS_OK)
        return URJ_STATUS_FAIL;

    if (!chain->parts)
    {
        urj_error_set (URJ_ERROR_ILLEGAL_STATE, "no parts, Run '%s' first",
                       "detect");
        return URJ_STATUS_FAIL;
    }

    if (chain->active_part >= chain->parts->len)
    {
        urj_error_set (URJ_ERROR_ILLEGAL_STATE, "no active part");
        return URJ_STATUS_FAIL;
    }

    if (!part_is_sdu (chain, chain->active_part))
    {
        urj_error_set (URJ_ERROR_ILLEGAL_STATE, "not an SDU part");
        return URJ_STATUS_FAIL;
    }

    assert (chain->active_part >= 0 && chain->active_part < chain->parts->len);

    if (strcmp (params[1], "enable") == 0)
    {
        if (num_params == 3)
        {
            if (!strcmp (params[2], "core-scan-path"))
            {
                int retval;
                int i = chain->active_part;

                sdu_ctl_get (chain, i);

                if (sdu_ctl_is_cspen (chain, i))
                {
                    urj_log (URJ_LOG_LEVEL_DEBUG,
                             "CSPEN has been already set in SDU %d", i);
                    return URJ_STATUS_OK;
                }

                retval = sdu_enable_core_scan_path (chain, i);
                if (retval != URJ_STATUS_OK)
                {
                    urj_error_set (URJ_ERROR_SDU,
                                   "failed to enable core scan path");
                    return retval;
                }

                /* Since the cores enabled by SDU don't have IDCODE register,
                   we need to set up configurations for them.

                   We assume SDU controls two cores and they come before it
                   in the scan chain.  */

                urj_tap_manual_add_at (chain, i, "BF609", 5);
                urj_tap_manual_init (chain, "analog/bf609/bf609");

                urj_tap_manual_add_at (chain, i + 1, "BF609", 5);
                urj_tap_manual_init (chain, "analog/bf609/bf609");

                chain->active_part = i + 2;

                return URJ_STATUS_OK;
            }
            else
            {
                urj_error_set (URJ_ERROR_SDU, "bad parameter '%s'", params[2]);
                return URJ_STATUS_FAIL;
            }
        }
        else
        {
            urj_error_set (URJ_ERROR_SDU,
                           "'sdu enable' requires 1 parameter, not %d",
                           num_params - 2);
            return URJ_STATUS_FAIL;
        }
    }
    else if (strcmp (params[1], "disable") == 0)
    {
        if (num_params == 3)
        {
            if (!strcmp (params[2], "core-scan-path"))
            {
                int retval;
                int i = chain->active_part;

                sdu_ctl_get (chain, i);

                if (!sdu_ctl_is_cspen (chain, i))
                {
                    urj_log (URJ_LOG_LEVEL_DEBUG,
                             "CSPEN is not set in SDU %d", i);
                    return URJ_STATUS_OK;
                }

                retval = sdu_disable_core_scan_path (chain, i);
                if (retval != URJ_STATUS_OK)
                {
                    urj_error_set (URJ_ERROR_SDU,
                                   "failed to disable core scan path");
                    return retval;
                }

                /* We assume SDU controls two cores and they come before it
                   in the scan chain.  */

                urj_tap_manual_remove (chain, i - 1);
                urj_tap_manual_remove (chain, i - 2);

                chain->active_part = i - 2;

                return URJ_STATUS_OK;
            }
            else
            {
                urj_error_set (URJ_ERROR_SDU, "bad parameter '%s'", params[2]);
                return URJ_STATUS_FAIL;
            }
        }
        else
        {
            urj_error_set (URJ_ERROR_SDU,
                           "'sdu disable' requires 1 parameter, not %d",
                           num_params - 2);
            return URJ_STATUS_FAIL;
        }
    }
    else if (strcmp (params[1], "reset") == 0)
    {
        if (num_params != 2)
        {
            urj_error_set (URJ_ERROR_SDU,
                           "'sdu reset' requires 0 parameter, not %d",
                           num_params - 2);
            return URJ_STATUS_FAIL;
        }

        urj_log (URJ_LOG_LEVEL_NORMAL,
                 _("%s: reseting processor ... "), "sdu");
        sdu_reset (chain, chain->active_part);
        urj_log (URJ_LOG_LEVEL_NORMAL, _("OK\n"));

        return URJ_STATUS_OK;
    }
    else
    {
        urj_error_set (URJ_ERROR_SDU,
                       "unknown command '%s'", params[1]);
        return URJ_STATUS_FAIL;
    }

    return URJ_STATUS_OK;
}


static void
cmd_sdu_help (void)
{
    urj_log (URJ_LOG_LEVEL_NORMAL,
             _("Usage: %s enable|disable core-scan-path\n"
               "Usage: %s reset\n"
               "SDU specific commands\n"),
             "sdu", "sdu");
}

static void
cmd_sdu_complete (urj_chain_t *chain, char ***matches, size_t *match_cnt,
                  char * const *tokens, const char *text, size_t text_len,
                  size_t token_point)
{
    static const char * const main_cmds[] = {
        "disable",
        "enable",
        "reset",
    };
    static const char * const enable_cmds[] = {
        "core-scan-path",
    };

    switch (token_point)
    {
    case 1:
        urj_completion_mayben_add_matches (matches, match_cnt, text, text_len,
                                           main_cmds);
        break;

    case 2:
        if (!strcmp (tokens[1], "enable") || !strcmp (tokens[1], "disable"))
            urj_completion_mayben_add_matches (matches, match_cnt, text,
                                               text_len, enable_cmds);
        break;
    }
}

const urj_cmd_t urj_cmd_sdu = {
    "sdu",
    N_("SDU specific commands"),
    cmd_sdu_help,
    cmd_sdu_run,
    cmd_sdu_complete,
};
