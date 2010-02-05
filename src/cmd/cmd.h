/*
 * $Id: cmd.h 1746 2010-01-29 09:09:11Z vapier $
 *
 * Copyright (C) 2003 ETC s.r.o.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the ETC s.r.o. nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Written by Marcel Telka <marcel@telka.sk>, 2003.
 *
 */

#ifndef URJ_SRC_CMD_H
#define URJ_SRC_CMD_H

#include <stdlib.h> /* qsort */
#include <sysdep.h>

#include <urjtag/types.h>

typedef struct
{
    char *name;
    char *description;
    void (*help) (void);
    /** @return URJ_STATUS_OK on success; URJ_STATUS_FAIL on error, both
     * syntax and library errors */
    int (*run) (urj_chain_t *chain, char *params[]);
} urj_cmd_t;

#define _URJ_CMD(cmd) extern const urj_cmd_t urj_cmd_##cmd;
#include "cmd_list.h"

extern const urj_cmd_t * const urj_cmds[];

/**
 * Tests if chain has a cable pointer
 *
 * @return URJ_STATUS_OK if success; URJ_STATUS_FAIL on error or failure
 */
int urj_cmd_test_cable (urj_chain_t *chain);

/**
 * Count the number of parameters in this NULL-terminated list
 */
int urj_cmd_params (char *params[]);
/**
 * Parse parameter as a long unsigned
 * @return URJ_STATUS_OK on success, URJ_STATUS_FAIL on error
 */
int urj_cmd_get_number (const char *s, long unsigned *i);

/**
 * Show a list of structures with name/desc
 */
#define urj_cmd_show_list(arr) \
do { \
    int i, max_len = 0; \
    for (i = 0; arr[i]; ++i) { \
        int this_len = strlen(arr[i]->name); \
        if (max_len < this_len) \
            max_len = this_len; \
    } \
    for (i = 0; arr[i]; ++i) \
        urj_log (URJ_LOG_LEVEL_NORMAL, "%-*s %s\n", max_len + 1, \
                 arr[i]->name, _(arr[i]->description)); \
} while (0)

#endif /* URJ_CMD_H */
