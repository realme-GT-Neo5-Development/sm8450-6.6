/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
 *
 * FORWARDPORT from msm-5.10. Only the definitions needed for the transport
 * key broadcast are included (see tme_hwkm_master.c). Do not change the
 * field layout: these structures are passed as-is to the TME firmware.
 */
#ifndef _TME_HWKM_MASTER_INTERFACE_H_
#define _TME_HWKM_MASTER_INTERFACE_H_

#include <linux/types.h>

/* HWKM Master command IDs (5.10: tme_hwkm_master_intf.h:13-22) */
enum tme_hwkm_cmd {
	TME_HWKM_CMD_CLEAR_KEY        = 0,
	TME_HWKM_CMD_GENERATE_KEY     = 1,
	TME_HWKM_CMD_DERIVE_KEY       = 2,
	TME_HWKM_CMD_WRAP_KEY         = 3,
	TME_HWKM_CMD_UNWRAP_KEY       = 4,
	TME_HWKM_CMD_BROADCAST_TP_KEY = 6,
	TMW_HWKM_CMD_INVALID          = 7,
};

struct tme_hwkm_master_cmd {
	uint32_t  code;
} __packed;

struct tme_response_sts {
	uint32_t  tme_err_status;	/**< TME FW response status */
	uint32_t  seq_err_status;	/**< CSR_CMD_ERROR_STATUS */
	uint32_t  seq_kp_err_status0;	/**< KEY_POLICY_ERROR_STATUS0 */
	uint32_t  seq_kp_err_status1;	/**< KEY_POLICY_ERROR_STATUS1 */
	uint32_t  seq_rsp_status;	/**< CSR_CMD_RESPONSE_STATUS */
} __packed;

struct broadcast_tpkey_req {
	uint32_t cbor_header;		/**< CBOR-encoded tag */
	struct tme_hwkm_master_cmd cmd;	/**< TME_HWKM_CMD_BROADCAST_TP_KEY */
} __packed;

#endif /* _TME_HWKM_MASTER_INTERFACE_H_ */
