/*
 * $Id
 *
 * Analog Devices ADSP-BF609 EZ-BRD bus driver
 * Copyright (C) 2011 Analog Devices, Inc.
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
 * Written by Jie Zhang <jie.zhang@analog.com>, 2011.
 */

#include "blackfin.h"

typedef struct
{
    bfin_bus_params_t params; /* needs to be first */
} bus_params_t;

static urj_bus_t *
bf609_ezkit_bus_new (urj_chain_t *chain, const urj_bus_driver_t *driver,
                     const urj_param_t *cmd_params[])
{
    urj_bus_t *bus;
    urj_part_t *part;
    bfin_bus_params_t *params;
    int failed = 0;

    bus = urj_bus_generic_new (chain, driver, sizeof (bus_params_t));
    if (bus == NULL)
        return NULL;
    part = bus->part;

    params = bus->params;
    params->async_base = 0xb0000000;
    params->async_size = 32 * 1024 * 1024;
    params->ams_cnt = 1;
    params->addr_cnt = 24;
    params->data_cnt = 16;
    failed |= urj_bus_generic_attach_sig (part, &params->dcs0, "DDR_CS_b");

    failed |= bfin_bus_new (bus, cmd_params, NULL);

    if (failed)
    {
        urj_bus_generic_free (bus);
        return NULL;
    }

    return bus;
}

BFIN_BUS_DECLARE(bf609_ezkit, "BF609 EZ-KIT board");
