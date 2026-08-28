/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
 *
 * FORWARDPORT from msm-5.10. Only the extended error structure and the
 * transport key broadcast operation are included; the other HWKM Master
 * types have no users.
 */
#ifndef _TME_HWKM_MASTER_DEFS_H_
#define _TME_HWKM_MASTER_DEFS_H_

#include <linux/types.h>

/**
 * struct tme_ext_err_info - extended error information from TME.
 *
 * Layout must match struct tme_response_sts; the firmware returns the same
 * five words.
 */
struct tme_ext_err_info {
	uint32_t  tme_err_status;	/**< TME FW response status */
	uint32_t  seq_err_status;	/**< CSR_CMD_ERROR_STATUS */
	uint32_t  seq_kp_err_status0;	/**< KEY_POLICY_ERROR_STATUS0 */
	uint32_t  seq_kp_err_status1;	/**< KEY_POLICY_ERROR_STATUS1 */
	uint32_t  seq_rsp_status;	/**< CSR_CMD_RESPONSE_STATUS */
} __packed;

#if IS_ENABLED(CONFIG_MSM_TMECOM_QMP)
uint32_t tme_hwkm_master_broadcast_transportkey(struct tme_ext_err_info *err_info);
#else
static inline uint32_t
tme_hwkm_master_broadcast_transportkey(struct tme_ext_err_info *err_info)
{
	return -ENODEV;
}
#endif

#endif /* _TME_HWKM_MASTER_DEFS_H_ */
