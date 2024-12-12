/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: power manage handle.
 */

#include "mrs_common_power_manage.h"
#include <soc_mdm.h>
#include <soc_lowpower.h>

td_void aes_lowpower_prepare(td_void);
td_void aes_lowpower_restore(td_void);
td_void hash_lowpower_prepare(td_void);
td_void hash_lowpower_restore(td_void);
td_void trng_lowpower_prepare(td_void);
td_void trng_lowpower_restore(td_void);
td_void rsa_lowpower_prepare(td_void);
td_void rsa_lowpower_restore(td_void);
static td_void lowpower_vote_register(td_void);

td_void aes_lowpower_prepare(td_void)
{
    uapi_lowpower_forbid(EXT_LOWPOWER_ID_SPACC, EXT_SLEEP_LIGHT);
}

td_void aes_lowpower_restore(td_void)
{
    uapi_lowpower_unforbid(EXT_LOWPOWER_ID_SPACC, EXT_SLEEP_LIGHT, 0);
}

td_void hash_lowpower_prepare(td_void)
{
    uapi_lowpower_forbid(EXT_LOWPOWER_ID_HASH, EXT_SLEEP_LIGHT);
}

td_void hash_lowpower_restore(td_void)
{
    uapi_lowpower_unforbid(EXT_LOWPOWER_ID_HASH, EXT_SLEEP_LIGHT, 0);
}

td_void trng_lowpower_prepare(td_void)
{
    uapi_lowpower_forbid(EXT_LOWPOWER_ID_TRNG, EXT_SLEEP_LIGHT);
}

td_void trng_lowpower_restore(td_void)
{
    uapi_lowpower_unforbid(EXT_LOWPOWER_ID_TRNG, EXT_SLEEP_LIGHT, 0);
}

td_void rsa_lowpower_prepare(td_void)
{
    uapi_lowpower_forbid(EXT_LOWPOWER_ID_RSA, EXT_SLEEP_LIGHT);
}

td_void rsa_lowpower_restore(td_void)
{
    uapi_lowpower_unforbid(EXT_LOWPOWER_ID_RSA, EXT_SLEEP_LIGHT, 0);
}

static td_void lowpower_vote_register(td_void)
{
    EXT_AES_REGISTER_S aes_cfg;
    EXT_HASH_REGISTER_S hash_cfg;
    EXT_TRNG_REGISTER_S trng_cfg;
    EXT_RSA_REGISTER_S rsa_cfg;

    aes_cfg.prepare_func = aes_lowpower_prepare;
    aes_cfg.restore_func = aes_lowpower_restore;
    uapi_aes_register(aes_cfg);

    hash_cfg.prepare_func = hash_lowpower_prepare;
    hash_cfg.restore_func = hash_lowpower_restore;
    uapi_hash_register(hash_cfg);

    trng_cfg.prepare_func = trng_lowpower_prepare;
    trng_cfg.restore_func = trng_lowpower_restore;
    uapi_trng_register(trng_cfg);

    rsa_cfg.prepare_func = rsa_lowpower_prepare;
    rsa_cfg.restore_func = rsa_lowpower_restore;
    uapi_rsa_register(rsa_cfg);
}

td_void mrs_common_power_manage_init(td_void)
{
    td_u32 ret;
    ext_nv_sys_startup cfg;

    (td_void) memset_s(&cfg, sizeof(cfg), 0, sizeof(cfg));
    ret = uapi_nv_read(EXT_NV_SYS_STARTUP_ID, (td_pvoid)&cfg, sizeof(ext_nv_sys_startup));
    if (ret != EXT_ERR_SUCCESS) {
        return;
    }

    lowpower_vote_register();

    /* set watchdog interruption with awakening sleep */
    uapi_lowpower_wakeup_config(EXT_LOWPOWER_ID_WDG, TD_TRUE);

    /* set PLC channel interruption with awakening sleep */
    uapi_lowpower_wakeup_config(EXT_LOWPOWER_ID_PHY, TD_TRUE);

    /* set UART0 communicate interruption with awakening sleep */
    uapi_lowpower_wakeup_config(EXT_LOWPOWER_ID_UART0, TD_TRUE);

    /* set UART1 communicate interruption with awakening sleep */
    uapi_lowpower_wakeup_config(EXT_LOWPOWER_ID_UART1, TD_TRUE);

    /* set high-precision timer interruption with awakening sleep */
    uapi_lowpower_wakeup_config(EXT_LOWPOWER_ID_TIMER0, TD_TRUE);

    /* set system timer interruption with awakening sleep */
    uapi_lowpower_wakeup_config(EXT_LOWPOWER_ID_TIMER5, TD_TRUE);

    /* set GPIO interruption with awakening sleep */
    uapi_lowpower_wakeup_config(EXT_LOWPOWER_ID_GPIO, TD_TRUE);
#if defined(PRODUCT_CFG_SUPPORT_DEEP_SLEEP)
    uapi_init_lowpower_deep_sleep_proc();
#endif
    (td_void) uapi_lowpower_init(cfg.lightsleep, cfg.deepsleep, EXT_LP_DELAY_DEFAULT_TIME);
}
