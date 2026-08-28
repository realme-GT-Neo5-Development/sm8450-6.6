// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
 *
 * FORWARDPORT: HWKM transport key broadcast, from msm-5.10.
 *
 * The 6.6 tree has the tmecom QMP channel but lacks the HWKM Master request
 * layer. Without it ICE never receives the transport key (TPKEY) and every
 * key programming fails with qcom_scm_config_set_ice_key() = -EINVAL.
 *
 * Only tme_hwkm_master_broadcast_transportkey() is ported, as it is the
 * only operation used by the ICE path; the other HWKM operations
 * (generate/derive/wrap/unwrap/import/clear) have no users.
 */

#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/tme_hwkm_master_defs.h>

#include "tme_hwkm_master_intf.h"
#include "tmecom.h"

/* _be32 0xD9012F44, value from msm-5.10 */
#define TME_BORADCAST_KEY_CBOR_TAG 0x442F01D9

static uint32_t update_ext_err(struct tme_ext_err_info *err_info,
			       struct tme_response_sts *result)
{
	err_info->tme_err_status     = result->tme_err_status;
	err_info->seq_err_status     = result->seq_err_status;
	err_info->seq_kp_err_status0 = result->seq_kp_err_status0;
	err_info->seq_kp_err_status1 = result->seq_kp_err_status1;
	err_info->seq_rsp_status     = result->seq_rsp_status;

	return (err_info->tme_err_status ||
		err_info->seq_err_status ||
		err_info->seq_kp_err_status0 ||
		err_info->seq_kp_err_status1) ? 1 : 0;
}

/**
 * tme_hwkm_master_broadcast_transportkey() - broadcast the transport key.
 *
 * TME generates the TPKEY and broadcasts it to all HWKM slaves (including
 * ICE) that are in receive mode. The caller must put ICE into that mode
 * before the call and take it out afterwards; see ice.c.
 */
uint32_t tme_hwkm_master_broadcast_transportkey(struct tme_ext_err_info *err_info)
{
	struct broadcast_tpkey_req *request = NULL;
	struct tme_response_sts *response = NULL;
	size_t response_len = sizeof(*response);
	uint32_t ret = 0;

	if (!err_info)
		return -EINVAL;

	request = kzalloc(sizeof(*request), GFP_KERNEL);
	response = kzalloc(response_len, GFP_KERNEL);
	if (!request || !response) {
		ret = -ENOMEM;
		goto err_exit;
	}

	request->cbor_header = TME_BORADCAST_KEY_CBOR_TAG;
	request->cmd.code    = TME_HWKM_CMD_BROADCAST_TP_KEY;

	ret = tmecom_process_request(request, sizeof(*request), response,
				     &response_len);
	if (ret != 0) {
		pr_err("%s: TP key broadcast request failed, ret=%u\n",
		       __func__, ret);
		goto err_exit;
	}

	if (response_len != sizeof(*response)) {
		pr_err("%s: bad response length: %zu, expected %zu\n",
		       __func__, response_len, sizeof(*response));
		ret = -EBADMSG;
		goto err_exit;
	}

	ret = update_ext_err(err_info, response);

err_exit:
	kfree(request);
	kfree(response);
	return ret;
}
EXPORT_SYMBOL_GPL(tme_hwkm_master_broadcast_transportkey);
