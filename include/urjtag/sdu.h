#ifndef URJ_SDU_H
#define URJ_SDU_H


struct sdu_part_data
{
    /* The first three fields should be all same for all parts. */
    uint32_t magic;
    int bypass;
    int scan;

    uint16_t ctl;
    uint32_t stat;

    uint16_t ctl_sysrst;
    uint16_t ctl_cspen;
    uint16_t ctl_dmaen;
    uint16_t ctl_emeen;
    uint16_t ctl_ehlt_mask;

    uint32_t stat_sysrst;
    uint32_t stat_err;
    uint32_t stat_deepsleep;
    uint32_t stat_secure;
    uint32_t stat_errc_mask;
    uint32_t stat_macrdy;
    uint32_t stat_dmardrdy;
    uint32_t stat_dmawdrdy;
    uint32_t stat_addrerr;
    uint32_t stat_dmafifo_mask;
    uint32_t stat_ghlt;
    uint32_t stat_ghltc_mask;
    uint32_t stat_eme;
    uint32_t stat_chlt;
    uint32_t stat_crst;

    uint8_t macctl;
    uint32_t macaddr;
    uint32_t macdata;

    uint8_t macctl_size_mask;
    uint8_t macctl_rnw;
    uint8_t macctl_autoinc;
};

#define SDU_PART_BYPASS(part)   (PART_BYPASS (part))
#define SDU_PART_SCAN(part)     (PART_SCAN (part))

#define SDU_PART_DATA(part)     ((struct sdu_part_data *)((part)->params->data))
#define SDU_PART_CTL(part)      (((struct sdu_part_data *)((part)->params->data))->ctl)
#define SDU_PART_STAT(part)     (((struct sdu_part_data *)((part)->params->data))->stat)
#define SDU_PART_MACCTL(part)   (((struct sdu_part_data *)((part)->params->data))->macctl)
#define SDU_PART_MACADDR(part)  (((struct sdu_part_data *)((part)->params->data))->macaddr)
#define SDU_PART_MACDATA(part)  (((struct sdu_part_data *)((part)->params->data))->macdata)

#define SDU_CTL_EHLT_SYSTEM     0x1
#define SDU_CTL_EHLT_CORE(I)    (1 << ((I) + 1))
#define SDU_CTL_EHLT_CORE_0     SDU_CTL_EHLT_CORE(0)
#define SDU_CTL_EHLT_CORE_1     SDU_CTL_EHLT_CORE(1)
#define SDU_CTL_EHLT_CORE_ALL   (SDU_CTL_EHLT_CORE_0 | SDU_CTL_EHLT_CORE_1)
#define SDU_CTL_EHLT_ALL        (SDU_CTL_EHLT_CORE_ALL | SDU_CTL_EHLT_SYSTEM)

#define DECLARE_SDU_STAT_BIT_IS(name) \
    extern int sdu_stat_is_##name (urj_chain_t *, int);
DECLARE_SDU_STAT_BIT_IS (sysrst)
DECLARE_SDU_STAT_BIT_IS (err)
DECLARE_SDU_STAT_BIT_IS (deepsleep)
DECLARE_SDU_STAT_BIT_IS (secure)
DECLARE_SDU_STAT_BIT_IS (macrdy)
DECLARE_SDU_STAT_BIT_IS (dmardrdy)
DECLARE_SDU_STAT_BIT_IS (dmawdrdy)
DECLARE_SDU_STAT_BIT_IS (addrerr)
DECLARE_SDU_STAT_BIT_IS (ghlt)
DECLARE_SDU_STAT_BIT_IS (eme)
DECLARE_SDU_STAT_BIT_IS (chlt)
DECLARE_SDU_STAT_BIT_IS (crst)

#define DECLARE_SDU_STAT_CAUSE(name) \
    extern uint32_t sdu_stat_##name (urj_chain_t *, int);

DECLARE_SDU_STAT_CAUSE (errc)
DECLARE_SDU_STAT_CAUSE (dmafifo)
DECLARE_SDU_STAT_CAUSE (ghltc)

#define DECLARE_SDU_CTL_BIT_IS(name) \
    extern int sdu_ctl_is_##name (urj_chain_t *, int);

DECLARE_SDU_CTL_BIT_IS (cspen)

#define DECLARE_SDU_MACCTL_BIT_SET(name) \
    extern void sdu_macctl_bit_set_##name (urj_chain_t *, int);

#define DECLARE_SDU_MACCTL_BIT_CLEAR(name) \
    extern void sdu_macctl_bit_clear_##name (urj_chain_t *, int);

DECLARE_SDU_MACCTL_BIT_SET (rnw)
DECLARE_SDU_MACCTL_BIT_SET (autoinc)

DECLARE_SDU_MACCTL_BIT_CLEAR (rnw)
DECLARE_SDU_MACCTL_BIT_CLEAR (autoinc)

enum sdu_macctl_size
{
    SDU_MACCTL_SIZE_BYTE,     /* 000 =  8 b */
    SDU_MACCTL_SIZE_SHORT,    /* 001 = 16 b */
    SDU_MACCTL_SIZE_WORD,     /* 010 = 32 b */
                              /* 011 - 111 reserved */
};

extern void sdu_macctl_set_size (urj_chain_t *, int, enum sdu_macctl_size);

extern int part_is_sdu (urj_chain_t *, int);
extern int sdu_enable_core_scan_path (urj_chain_t *, int);
extern int sdu_disable_core_scan_path (urj_chain_t *, int);
extern void sdu_halt_trigger (urj_chain_t *, int, int);
extern void sdu_halt_wait (urj_chain_t *, int);
extern void sdu_halt (urj_chain_t *, int, int);
extern void sdu_reset_assert (urj_chain_t *, int);
extern void sdu_reset_deassert (urj_chain_t *, int);
extern void sdu_reset (urj_chain_t *, int);
extern void sdu_stat_get (urj_chain_t *, int);
extern void sdu_ctl_get (urj_chain_t *, int);

extern int sdu_mac_memory_read (urj_chain_t *, int, uint32_t, uint8_t *, int, int);
extern int sdu_mac_memory_write (urj_chain_t *, int, uint32_t, uint8_t *, int, int);

#endif /* URJ_SDU_H */
