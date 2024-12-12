/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS diag cmd handle.
 */

#include <dfx_app.h>
#include "mrs_common_tools.h"
#include "mrs_common_diag.h"

#ifdef __cplusplus
extern "C" {
#endif

EXT_PRV td_u32 mrs_diag_cmd_handle(td_u16 id, const td_pvoid cmd, td_u16 size, td_u8 option);

EXT_CONST ext_diag_cmd_reg_obj g_mrs_diag_objs[] = {
    { ID_DIAG_CMD_APP_BEGIN, ID_DIAG_CMD_APP_END, mrs_diag_cmd_handle },
};

td_void mrs_diag_init(td_void)
{
    uapi_diag_register_cmd(g_mrs_diag_objs, ext_array_count(g_mrs_diag_objs));
}

EXT_PRV td_u32 mrs_diag_cmd_handle(td_u16 id, const td_pvoid cmd, td_u16 size, td_u8 option)
{
    td_pbyte payload = TD_NULL;
    td_u32 ret;
    td_u16 paylaod_size = size;

    if ((cmd != TD_NULL) && (size != 0)) {
        payload = (td_pbyte)mrs_malloc(paylaod_size);
        if (payload == TD_NULL) {
            return EXT_ERR_MALLOC_FAILUE;
        }

        (td_void) memcpy_s(payload, paylaod_size, cmd, size);
    }

    ret = mrs_msg_queue_send(MRS_MSG_ID_DIAG, id, (uintptr_t)payload,
                             (uintptr_t)(size | (option << 16))); /* high 16b */
    if (ret != EXT_ERR_SUCCESS) {
        mrs_free(payload);
    }

    return ret;
}

td_void mrs_msg_on_diag(EXT_CONST mrs_queue_msg *msg)
{
    ext_diag_cmd_reg_obj tbl[] = MRS_DIAG_CMD_OBJS_TBL;
    td_u16 i;
    td_u16 id = msg->param0;
    td_pvoid cmd = (td_pvoid)msg->param1;
    td_u16 tbl_num = sizeof(tbl) / sizeof(ext_diag_cmd_reg_obj);
    for (i = 0; i < tbl_num; i++) {
        if ((tbl[i].min_id <= id) && (tbl[i].max_id >= id)) {
            if (tbl[i].fn_input_cmd != TD_NULL) {
                td_u16 size = (td_u16)(msg->param2 & 0xffff);
                td_u8 option = (td_u8)((msg->param2 >> 16) & 0xff); /* high 16bits shifts 16bits to the right */

                tbl[i].fn_input_cmd(id, cmd, size, option);
            }
            break;
        }
    }

    mrs_free(cmd);
}

#ifdef __cplusplus
}
#endif

