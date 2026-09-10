// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/pinctrl.h>

#include "pinctrl-msm.h"
#include "pinctrl-cape.h"

/*
 * FORWARDPORT: `.egpio_func`, a field that did not exist in msm-5.10.
 *
 * msm-5.10 unconditionally claimed the pad for APPS on every mux change:
 *
 *     if (val & BIT(g->egpio_present))
 *             val |= BIT(g->egpio_enable);
 *
 * 6.6 gates this on `egpio_func` (msm_pinmux_set_mux()). Without it an
 * eGPIO-capable pad is never taken over by HLOS: the TLMM register accepts
 * writes, but the pad is still driven by the remote master.
 *
 * 9 is the function index within the group, not in `functions`
 * (msm_pinmux_set_mux() matches g->funcs[i] == function). PINGROUP has
 * 10 slots (gpio + f1..f9), so egpio sits in slot 9, as in mainline
 * pinctrl-sm8450.c. No cape pin has a real function in slot 9, so the
 * branch that hands a pad back to the remote master cannot trigger.
 *
 * This also makes msm_gpio_dbg_show_one() report `egpio` for pads owned by
 * the remote master, which it only does when egpio_func is non-zero.
 */
#define CAPE_EGPIO_FUNC 9

static const struct msm_pinctrl_soc_data cape_pinctrl = {
	.pins = cape_pins,
	.npins = ARRAY_SIZE(cape_pins),
	.functions = cape_functions,
	.nfunctions = ARRAY_SIZE(cape_functions),
	.groups = cape_groups,
	.ngroups = ARRAY_SIZE(cape_groups),
	.ngpios = 211,
	.qup_regs = cape_qup_regs,
	.nqup_regs = ARRAY_SIZE(cape_qup_regs),
	.wakeirq_map = cape_pdc_map,
	.nwakeirq_map = ARRAY_SIZE(cape_pdc_map),
	.egpio_func = CAPE_EGPIO_FUNC,
};

static const struct msm_pinctrl_soc_data cape_vm_pinctrl = {
	.pins = cape_pins,
	.npins = ARRAY_SIZE(cape_pins),
	.functions = cape_functions,
	.nfunctions = ARRAY_SIZE(cape_functions),
	.groups = cape_groups,
	.ngroups = ARRAY_SIZE(cape_groups),
	.ngpios = 211,
	.egpio_func = CAPE_EGPIO_FUNC,
};

/*
 * TODO: msm-5.10 registered android_vh_gpio_block_read here for
 * "qcom,cape-vm-pinctrl" to block GPIO reads inside the Gunyah VM. The hook
 * does not exist in 6.6 and is not in the GKI KMI. Only affects the
 * cape-tuivm target; restore if a VM variant is ever built.
 */
static int cape_pinctrl_probe(struct platform_device *pdev)
{
	const struct msm_pinctrl_soc_data *pinctrl_data;

	pinctrl_data = of_device_get_match_data(&pdev->dev);
	if (!pinctrl_data)
		return -EINVAL;

	return msm_pinctrl_probe(pdev, pinctrl_data);
}

static const struct of_device_id cape_pinctrl_of_match[] = {
	{ .compatible = "qcom,cape-pinctrl", .data = &cape_pinctrl},
	{ .compatible = "qcom,cape-vm-pinctrl", .data = &cape_vm_pinctrl},
	{ },
};

static struct platform_driver cape_pinctrl_driver = {
	.driver = {
		.name = "cape-pinctrl",
		.of_match_table = cape_pinctrl_of_match,
	},
	.probe = cape_pinctrl_probe,
	.remove = msm_pinctrl_remove,
};

static int __init cape_pinctrl_init(void)
{
	return platform_driver_register(&cape_pinctrl_driver);
}
arch_initcall(cape_pinctrl_init);

static void __exit cape_pinctrl_exit(void)
{
	platform_driver_unregister(&cape_pinctrl_driver);
}
module_exit(cape_pinctrl_exit);

MODULE_DESCRIPTION("QTI cape pinctrl driver");
MODULE_LICENSE("GPL v2");
MODULE_DEVICE_TABLE(of, cape_pinctrl_of_match);
MODULE_SOFTDEP("pre: qcom_tlmm_vm_irqchip");
