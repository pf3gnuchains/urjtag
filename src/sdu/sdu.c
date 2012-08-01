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

#include <sysdep.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <urjtag/chain.h>
#include <urjtag/part.h>
#include <urjtag/tap_state.h>
#include <urjtag/tap_register.h>
#include <urjtag/data_register.h>
#include <urjtag/part_instruction.h>

/* TODO generic code should be moved out of src/bfin so we don't
   need to include this file here.  */
#include <urjtag/bfin.h>

#include <urjtag/sdu.h>


static int
is_sdu_part (urj_part_t *part)
{
    if (part->params && part->params->data && PART_MAGIC (part) == PART_MAGIC_SDU)
        return 1;
    else
        return 0;
}

int
part_is_sdu (urj_chain_t *chain, int n)
{
    return is_sdu_part (chain->parts->parts[n]);
}

#define SDU_CTL_BIT_SET(name)                                           \
    static void                                                         \
    sdu_ctl_bit_set_##name (urj_chain_t *chain, int n)                  \
    {                                                                   \
        urj_part_t *part = chain->parts->parts[n];                      \
        urj_tap_register_t *r = part->active_instruction->data_register->in; \
        SDU_PART_CTL (part) |= SDU_PART_DATA (part)->ctl_##name;        \
        register_init_value (r, SDU_PART_CTL (part));                   \
    }

#define SDU_CTL_BIT_CLEAR(name)                                         \
    static void                                                         \
    sdu_ctl_bit_clear_##name (urj_chain_t *chain, int n)                \
    {                                                                   \
        urj_part_t *part = chain->parts->parts[n];                      \
        urj_tap_register_t *r = part->active_instruction->data_register->in; \
        SDU_PART_CTL (part) &= ~SDU_PART_DATA (part)->ctl_##name;       \
        register_init_value (r, SDU_PART_CTL (part));                   \
    }

#define SDU_CTL_BIT_IS(name)                                            \
    int                                                                 \
    sdu_ctl_is_##name (urj_chain_t *chain, int n)                       \
    {                                                                   \
        urj_part_t *part = chain->parts->parts[n];                      \
        if (SDU_PART_CTL (part) & SDU_PART_DATA (part)->ctl_##name)     \
            return 1;                                                   \
        else                                                            \
            return 0;                                                   \
    }


SDU_CTL_BIT_SET (sysrst)
SDU_CTL_BIT_SET (cspen)
SDU_CTL_BIT_SET (emeen)

SDU_CTL_BIT_CLEAR (cspen)
SDU_CTL_BIT_CLEAR (sysrst)
SDU_CTL_BIT_CLEAR (emeen)

SDU_CTL_BIT_IS (cspen)

#define SDU_STAT_BIT_IS(name)                                           \
    int                                                                 \
    sdu_stat_is_##name (urj_chain_t *chain, int n)                      \
    {                                                                   \
        urj_part_t *part = chain->parts->parts[n];                      \
        if (SDU_PART_STAT (part) & SDU_PART_DATA (part)->stat_##name)   \
            return 1;                                                   \
        else                                                            \
            return 0;                                                   \
    }

#define SDU_STAT_BIT_CLEAR(name)                                        \
    static void                                                         \
    sdu_stat_bit_clear_##name (urj_chain_t *chain, int n)               \
    {                                                                   \
        urj_part_t *part = chain->parts->parts[n];                      \
        urj_tap_register_t *r = part->active_instruction->data_register->in; \
        SDU_PART_STAT (part) &= ~SDU_PART_DATA (part)->stat_##name;     \
        register_init_value (r, SDU_PART_DATA (part)->stat_##name);     \
    }


SDU_STAT_BIT_IS (sysrst)
SDU_STAT_BIT_IS (err)
SDU_STAT_BIT_IS (deepsleep)
SDU_STAT_BIT_IS (secure)
SDU_STAT_BIT_IS (macrdy)
SDU_STAT_BIT_IS (dmardrdy)
SDU_STAT_BIT_IS (dmawdrdy)
SDU_STAT_BIT_IS (addrerr)
SDU_STAT_BIT_IS (ghlt)
SDU_STAT_BIT_IS (eme)
SDU_STAT_BIT_IS (chlt)
SDU_STAT_BIT_IS (crst)

SDU_STAT_BIT_CLEAR (eme)
SDU_STAT_BIT_CLEAR (sysrst)

#define SDU_STAT_CAUSE(name)                                            \
    uint32_t                                                            \
    sdu_stat_##name (urj_chain_t *chain, int n)                         \
    {                                                                   \
        urj_part_t *part = chain->parts->parts[n];                      \
        uint32_t cause;                                                 \
        uint32_t mask;                                                  \
                                                                        \
        mask = SDU_PART_DATA (part)->stat_##name##_mask;                \
        cause = SDU_PART_STAT (part) & mask;                            \
                                                                        \
        while (!(mask & 0x1))                                           \
        {                                                               \
            mask >>= 1;                                                 \
            cause >>= 1;                                                \
        }                                                               \
                                                                        \
        return cause;                                                   \
    }


SDU_STAT_CAUSE (errc)
SDU_STAT_CAUSE (dmafifo)
SDU_STAT_CAUSE (ghltc)


static void
sdu_ctl_set_ehlt (urj_chain_t * chain, int n, int what)
{
    urj_part_t *part = chain->parts->parts[n];
    urj_tap_register_t *r = part->active_instruction->data_register->in;
    SDU_PART_CTL (part) &= ~SDU_PART_DATA (part)->ctl_ehlt_mask;
    SDU_PART_CTL (part) |= (what << 8);
    register_init_value (r, SDU_PART_CTL (part));
}


static int
sdu_core_scan_path_1 (urj_chain_t *chain, int n, bool enable)
{
    /* Make sure that there is an SDU on it.  */
    if (!part_is_sdu (chain, n))
    {
        urj_log (URJ_LOG_LEVEL_ERROR, "Part %d isn't an SDU", n);
        return URJ_STATUS_FAIL;
    }

    part_scan_select (chain, n, SDU_CTL_SCAN);
    if (enable)
        sdu_ctl_bit_set_cspen (chain, n);
    else
        sdu_ctl_bit_clear_cspen (chain, n);

    urj_tap_chain_shift_data_registers_mode (chain, 0, 1, URJ_CHAIN_EXITMODE_IDLE);

    /* FIXME  According the SDU spec v0.9, we should not need
       this TAP reset.  But without it, we cannot halt the cores.

       Previously we use this.  But it does not save us anymore
       with gnICE+ @10MHz or higher.

       urj_tap_chain_defer_clock (chain, 0, 0, 1);
    */

    urj_tap_reset (chain);

    return URJ_STATUS_OK;
}

int
sdu_enable_core_scan_path (urj_chain_t *chain, int n)
{
    return sdu_core_scan_path_1 (chain, n, true);
}

int
sdu_disable_core_scan_path (urj_chain_t *chain, int n)
{
    return sdu_core_scan_path_1 (chain, n, false);
}

void
sdu_halt_trigger (urj_chain_t *chain, int n, int what)
{
    part_scan_select (chain, n, SDU_CTL_SCAN);
    sdu_ctl_set_ehlt (chain, n, what);
    sdu_ctl_bit_set_emeen (chain, n);
    urj_tap_chain_shift_data_registers_mode (chain, 0, 1, URJ_CHAIN_EXITMODE_UPDATE);
}

void
sdu_halt_wait (urj_chain_t *chain, int n)
{
    /* Check if there are some cores halted.  */
    /* I don't know why, but the we at least need read SDU_STAT twice
       to halt the cores.  */

    sdu_stat_get (chain, n);
    /*
    if (sdu_stat_is_chlt (chain, n))
      urj_log (URJ_LOG_LEVEL_NORMAL,
	       _("%s: cores halted <1>\n"), "sdu");
    else
      urj_log (URJ_LOG_LEVEL_NORMAL,
	       _("%s: cores not halted <1>\n"), "sdu");
    */
    sdu_stat_get (chain, n);
    /*
    if (sdu_stat_is_chlt (chain, n))
      urj_log (URJ_LOG_LEVEL_NORMAL,
	       _("%s: cores halted <2>\n"), "sdu");
    else
      urj_log (URJ_LOG_LEVEL_NORMAL,
	       _("%s: cores not halted <2>\n"), "sdu");
    */

    if (!sdu_stat_is_chlt (chain, n))
        urj_log (URJ_LOG_LEVEL_ERROR,
                 _("%s: unable to halt core(s)\n"),
                 "sdu");

    /* Whether halt is successfull or not, we have to clear EMEEN bit
       in SDU_CTL and EME bit in SDU_STAT for next core halt.  */
    part_scan_select (chain, n, SDU_CTL_SCAN);
    sdu_ctl_bit_clear_emeen (chain, n);
    urj_tap_chain_shift_data_registers_mode (chain, 0, 1, URJ_CHAIN_EXITMODE_UPDATE);
    part_scan_select (chain, n, SDU_STAT_SCAN);
    sdu_stat_bit_clear_eme (chain, n);
    urj_tap_chain_shift_data_registers_mode (chain, 0, 1, URJ_CHAIN_EXITMODE_UPDATE);
}

void
sdu_halt (urj_chain_t *chain, int n, int what)
{
  sdu_halt_trigger (chain, n, what);
  sdu_halt_wait (chain, n);
}

void
sdu_reset_assert (urj_chain_t *chain, int n)
{
    part_scan_select (chain, n, SDU_CTL_SCAN);
    sdu_ctl_bit_set_sysrst (chain, n);
    urj_tap_chain_shift_data_registers_mode (chain, 0, 1, URJ_CHAIN_EXITMODE_UPDATE);

    /* Check if system has been reset.  */
    sdu_stat_get (chain, n);
    if (!sdu_stat_is_sysrst (chain, n))
        urj_log (URJ_LOG_LEVEL_ERROR,
                 _("%s: unable to reset system\n"),
                 "sdu");
}

void
sdu_reset_deassert (urj_chain_t *chain, int n)
{
    /* Check if system has been reset.  */
    sdu_stat_get (chain, n);
    if (sdu_stat_is_sysrst (chain, n))
    {
        /* Clear SYSRST bit in SDU_CTL. */
        part_scan_select (chain, n, SDU_CTL_SCAN);
        sdu_ctl_bit_clear_sysrst (chain, n);
        urj_tap_chain_shift_data_registers_mode (chain, 0, 1, URJ_CHAIN_EXITMODE_UPDATE);

        /* Clear SYSRST bit in SDU_STAT.  */
        part_scan_select (chain, n, SDU_STAT_SCAN);
        sdu_stat_bit_clear_sysrst (chain, n);
        urj_tap_chain_shift_data_registers_mode (chain, 0, 1, URJ_CHAIN_EXITMODE_UPDATE);
    }
    else
        urj_log (URJ_LOG_LEVEL_ERROR,
                 _("%s: system is not in reset\n"),
                 "sdu");
}
 
void
sdu_reset (urj_chain_t *chain, int n)
{
    sdu_reset_assert (chain, n);
    sdu_reset_deassert (chain, n);
}

void
sdu_stat_get (urj_chain_t *chain, int n)
{
    urj_part_t *part;

    part_scan_select (chain, n, SDU_STAT_SCAN);
    urj_tap_chain_shift_data_registers_mode (chain, 1, 1, URJ_CHAIN_EXITMODE_UPDATE);
    part = chain->parts->parts[n];
    SDU_PART_STAT (part) = register_value (part->active_instruction->data_register->out);
}

void
sdu_ctl_get (urj_chain_t *chain, int n)
{
    urj_part_t *part;
    urj_tap_register_t *r;

    part_scan_select (chain, n, SDU_CTL_SCAN);
    urj_tap_chain_shift_data_registers_mode (chain, 1, 1, URJ_CHAIN_EXITMODE_EXIT1);
    part = chain->parts->parts[n];
    r = part->active_instruction->data_register->out;
    SDU_PART_CTL (part) = register_value (r);

    urj_tap_chain_defer_clock (chain, 0, 0, 1);     /* Pause-DR */
    urj_tap_chain_defer_clock (chain, 1, 0, 1);     /* Exit2 */
    urj_tap_chain_defer_clock (chain, 0, 0, 1);     /* Shift-DR */
 
    r = part->active_instruction->data_register->in;
    register_init_value (r, SDU_PART_CTL (part));
    urj_tap_chain_shift_data_registers_mode (chain, 0, 0, URJ_CHAIN_EXITMODE_UPDATE);
}

#define IDCODE                      0xFFC1F020
#define SDU0_MACCTL                 0xFFC1F058
#define SDU0_MACADDR                0xFFC1F05C
#define SDU0_MACDATA                0xFFC1F060

#define SDU_MACCTL_BIT_SET(name)                                        \
    void                                                                \
    sdu_macctl_bit_set_##name (urj_chain_t *chain, int n)               \
    {                                                                   \
        urj_part_t *part = chain->parts->parts[n];                      \
        urj_tap_register_t *r = part->active_instruction->data_register->in; \
        SDU_PART_MACCTL (part) |= SDU_PART_DATA (part)->macctl_##name;  \
        register_init_value (r, SDU_PART_MACCTL (part));                \
    }

#define SDU_MACCTL_BIT_CLEAR(name)                                      \
    void                                                                \
    sdu_macctl_bit_clear_##name (urj_chain_t *chain, int n)             \
    {                                                                   \
        urj_part_t *part = chain->parts->parts[n];                      \
        urj_tap_register_t *r = part->active_instruction->data_register->in; \
        SDU_PART_MACCTL (part) &= ~SDU_PART_DATA (part)->macctl_##name; \
        register_init_value (r, SDU_PART_MACCTL (part));                \
    }

SDU_MACCTL_BIT_SET (rnw)
SDU_MACCTL_BIT_SET (autoinc)

SDU_MACCTL_BIT_CLEAR (rnw)
SDU_MACCTL_BIT_CLEAR (autoinc)

void
sdu_macctl_set_size (urj_chain_t *chain, int n, enum sdu_macctl_size size)
{
    urj_part_t *part = chain->parts->parts[n];
    urj_tap_register_t *r = part->active_instruction->data_register->in;
    uint8_t s;
    uint8_t mask;

    mask = SDU_PART_DATA (part)->macctl_size_mask;
    s = size;

    while (!(mask & 0x1))
    {
        mask >>= 1;
        s <<= 1;
    }
    mask = SDU_PART_DATA (part)->macctl_size_mask;
    s &= mask;

    SDU_PART_MACCTL (part) &= ~mask;
    SDU_PART_MACCTL (part) |= s;
    register_init_value (r, SDU_PART_MACCTL (part));
}

static void
sdu_macaddr_set (urj_chain_t *chain, int n, uint32_t addr)
{
    urj_part_t *part = chain->parts->parts[n];

    if (SDU_PART_MACADDR (part) == addr)
        return;

    SDU_PART_MACADDR (part) = addr;

    part_scan_select (chain, n, SDU_MACADDR_SCAN);
    register_init_value (part->active_instruction->data_register->in, addr);
    urj_tap_chain_shift_data_registers_mode (chain, 0, 1, URJ_CHAIN_EXITMODE_UPDATE);
}

static uint32_t
sdu_macdata_get (urj_chain_t *chain, int n)
{
    urj_part_t *part = chain->parts->parts[n];
    uint32_t data;

    part_scan_select (chain, n, SDU_MACDATA_SCAN);
    urj_tap_chain_shift_data_registers_mode (chain, 1, 1, URJ_CHAIN_EXITMODE_UPDATE);
    data = register_value (part->active_instruction->data_register->out);

    return data;
}

/* TODO optimize if DATA is already in SDU_MACDATA */
static void
sdu_macdata_set (urj_chain_t *chain, int n, uint32_t data)
{
    urj_part_t *part = chain->parts->parts[n];

    part_scan_select (chain, n, SDU_MACDATA_SCAN);
    register_init_value (part->active_instruction->data_register->in, data);
    urj_tap_chain_shift_data_registers_mode (chain, 0, 1, URJ_CHAIN_EXITMODE_UPDATE);
}

/* AUTOINC is always set in SDU_MACCTL.  */
static void
sdu_mac_memory_read_1 (urj_chain_t *chain, int n,
                       uint32_t addr, uint8_t *buf, int s,
                       int check_macrdy)
{
    urj_part_t *part = chain->parts->parts[n];
    enum sdu_macctl_size size;
    uint32_t data;

    switch (s)
    {
        case 1: size = SDU_MACCTL_SIZE_BYTE;  break;
        case 2: size = SDU_MACCTL_SIZE_SHORT; break;
        case 4: size = SDU_MACCTL_SIZE_WORD;  break;
        default: abort ();
    }

    sdu_macaddr_set (chain, n, addr);

    part_scan_select (chain, n, SDU_MACCTL_SCAN);
    sdu_macctl_bit_set_autoinc (chain, n);
    sdu_macctl_bit_set_rnw (chain, n);
    sdu_macctl_set_size (chain, n, size);
    urj_tap_chain_shift_data_registers_mode (chain, 0, 1, URJ_CHAIN_EXITMODE_UPDATE);

    if (check_macrdy)
    {
        do
        {
            sdu_stat_get (chain, n);
        }
        while (!sdu_stat_is_macrdy (chain, n));
    }

    SDU_PART_MACADDR (part) += s;
    data = sdu_macdata_get (chain, n);

    *buf++ = data & 0xff;
    if (s >= 2)
        *buf++ = (data >> 8) & 0xff;
    if (s == 4)
    {
        *buf++ = (data >> 16) & 0xff;
        *buf++ = (data >> 24) & 0xff;
    }
}

/* TODO Can we optimize this like memory_read_1 in target_bfin_new.c?  */
int
sdu_mac_memory_read (urj_chain_t *chain, int n,
                     uint32_t addr, uint8_t *buf, int size,
                     int check_macrdy)
{
    /* Make sure that there is an SDU on it.  */
    if (!part_is_sdu (chain, n))
    {
        urj_log (URJ_LOG_LEVEL_ERROR, "Part %d isn't an SDU", n);
        return URJ_STATUS_FAIL;
    }

    if (size == 0)
        return URJ_STATUS_OK;

    sdu_stat_get (chain, n);

    if (!sdu_stat_is_macrdy (chain, n))
    {
        urj_log (URJ_LOG_LEVEL_ERROR,
                 "%s: [%d] MAC is not ready", "sdu", n);
        return URJ_STATUS_FAIL;
    }

    if ((addr & 0x1) != 0)
    {
        sdu_mac_memory_read_1 (chain, n, addr, buf, 1, check_macrdy);
        buf++;
        addr++;
        size--;
    }

    if ((addr & 0x2) != 0)
    {
        sdu_mac_memory_read_1 (chain, n, addr, buf, 2, check_macrdy);
        buf += 2;
        addr += 2;
        size -= 2;
    }

    while (size >= 4)
    {
        sdu_mac_memory_read_1 (chain, n, addr, buf, 4, check_macrdy);
        buf += 4;
        addr += 4;
        size -= 4;
    }

    if (size >= 2)
    {
        sdu_mac_memory_read_1 (chain, n, addr, buf, 2, check_macrdy);
        buf += 2;
        addr += 2;
        size -= 2;
    }

    if (size == 1)
        sdu_mac_memory_read_1 (chain, n, addr, buf, 1, check_macrdy);

    /* If we do not check MACRDY for each read, we have to check it
       and ERR here.  */
    if (!check_macrdy)
    {
        sdu_stat_get (chain, n);
        if (!sdu_stat_is_macrdy (chain, n) || sdu_stat_is_err (chain, n))
        {
            urj_log (URJ_LOG_LEVEL_ERROR,
                     _("%s: MAC read memory error\n"),
                     "sdu");
            return URJ_STATUS_FAIL;
        }
    }

    return URJ_STATUS_OK;
}

/* AUTOINC is always set in SDU_MACCTL.  */
static void
sdu_mac_memory_write_1 (urj_chain_t *chain, int n,
                        uint32_t addr, uint8_t *buf, int s,
                        int check_macrdy)
{
    urj_part_t *part = chain->parts->parts[n];
    enum sdu_macctl_size size;
    uint32_t data;

    assert (s == 1 || s == 2 || s == 4);

    switch (s)
    {
        case 1: size = SDU_MACCTL_SIZE_BYTE;  break;
        case 2: size = SDU_MACCTL_SIZE_SHORT; break;
        case 4: size = SDU_MACCTL_SIZE_WORD;  break;
        default: abort ();
    }

    data = *buf++;
    if (s >= 2)
        data |= (*buf++) << 8;
    if (s == 4)
    {
        data |= (*buf++) << 16;
        data |= (*buf++) << 24;
    }

    sdu_macaddr_set (chain, n, addr);

    sdu_macdata_set (chain, n, data);

    part_scan_select (chain, n, SDU_MACCTL_SCAN);
    sdu_macctl_bit_set_autoinc (chain, n);
    sdu_macctl_bit_clear_rnw (chain, n);
    sdu_macctl_set_size (chain, n, size);
    urj_tap_chain_shift_data_registers_mode (chain, 0, 1, URJ_CHAIN_EXITMODE_UPDATE);

    if (check_macrdy)
    {
        do
        {
            sdu_stat_get (chain, n);
        }
        while (!sdu_stat_is_macrdy (chain, n));
    }

    SDU_PART_MACADDR (part) += s;
}

int
sdu_mac_memory_write (urj_chain_t *chain, int n,
                      uint32_t addr, uint8_t *buf, int size,
                      int check_macrdy)
{
    /* Make sure that there is an SDU on it.  */
    if (!part_is_sdu (chain, n))
    {
        urj_log (URJ_LOG_LEVEL_ERROR, "Part %d isn't an SDU", n);
        return URJ_STATUS_FAIL;
    }

    if (size == 0)
        return URJ_STATUS_OK;

    sdu_stat_get (chain, n);

    if (!sdu_stat_is_macrdy (chain, n))
    {
        urj_log (URJ_LOG_LEVEL_ERROR,
                 "%s: [%d] MAC is not ready", "sdu", n);
        return URJ_STATUS_FAIL;
    }

    if ((addr & 0x1) != 0)
    {
        sdu_mac_memory_write_1 (chain, n, addr, buf, 1, check_macrdy);
        buf++;
        addr++;
        size--;
    }

    if ((addr & 0x2) != 0)
    {
        sdu_mac_memory_write_1 (chain, n, addr, buf, 2, check_macrdy);
        buf += 2;
        addr += 2;
        size -= 2;
    }

    while (size >= 4)
    {
        sdu_mac_memory_write_1 (chain, n, addr, buf, 4, check_macrdy);
        buf += 4;
        addr += 4;
        size -= 4;
    }

    if (size >= 2)
    {
        sdu_mac_memory_write_1 (chain, n, addr, buf, 2, check_macrdy);
        buf += 2;
        addr += 2;
        size -= 2;
    }

    if (size == 1)
        sdu_mac_memory_write_1 (chain, n, addr, buf, 1, check_macrdy);
 
    /* If we do not check MACRDY for each write, we have to check it
       and ERR here.  */
    if (!check_macrdy)
    {
        sdu_stat_get (chain, n);
        if (!sdu_stat_is_macrdy (chain, n) || sdu_stat_is_err (chain, n))
        {
            urj_log (URJ_LOG_LEVEL_ERROR,
                     _("%s: MAC write memory error\n"),
                     "sdu");
            return URJ_STATUS_FAIL;
        }
    }

    return URJ_STATUS_OK;
}


struct sdu_part_data sdu_part_data_initializer =
{
    PART_MAGIC_SDU, /* magic */
    0, /* bypass */
    0, /* scan */

    0, /* ctl */
    0, /* stat */

    0x0001, /* ctl_sysrst */
    0x0002, /* ctl_cspen */
    0x0004, /* ctl_dmaen */
    0x0010, /* ctl_emeen */
    0xff00, /* ctl_ehlt_mask */

    0x00000001, /* stat_sysrst */
    0x00000002, /* stat_err */
    0x00000004, /* stat_deepsleep */
    0x00000008, /* stat_secure */
    0x000000f0, /* stat_errc_mask */
    0x00000100, /* stat_macrdy */
    0x00000200, /* stat_dmardrdy */
    0x00000400, /* stat_dmawdrdy */
    0x00000800, /* stat_addrerr */
    0x00007000, /* stat_dmafifo_mask */
    0x00010000, /* stat_ghlt */
    0x000e0000, /* stat_ghltc_mask */
    0x00100000, /* stat_eme */
    0x00200000, /* stat_chlt */
    0x00400000, /* stat_crst */

    0, /* macctl */
    0, /* macaddr */
    0, /* macdata */

    0x07, /* macctl_size_mask */
    0x08, /* macctl_rnw */
    0x10, /* macctl_autoinc */
};

static void
sdu_part_init (urj_part_t *part)
{
    int i;

    if (!part || !part->params)
        goto error;

    part->params->free = free;
    part->params->wait_ready = NULL;
    part->params->data = malloc (sizeof (struct sdu_part_data));

    *SDU_PART_DATA (part) = sdu_part_data_initializer;

    if (!part->active_instruction)
        goto error;
    for (i = 0; i < NUM_SCANS; i++)
        if (strcmp (part->active_instruction->name, scans[i]) == 0)
            break;

    if (i == NUM_SCANS)
        goto error;

    SDU_PART_SCAN (part) = i;

    return;

 error:
    urj_warning (_("SDU part is missing instructions\n"));
}

void sdu_init (void) __attribute__((constructor));

void
sdu_init (void)
{
    urj_part_init_register ("SDU", sdu_part_init);
}
