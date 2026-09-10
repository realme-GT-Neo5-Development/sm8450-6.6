// SPDX-License-Identifier: BSD/GPL-2.0
/*
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * Copyright (c) 2024 Shanghai Awinic Technology Co., Ltd. All Rights Reserved
 *
 * GPL LICENSE SUMMARY
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * The full GNU General Public License is included in this distribution
 * in the file called LICENSE.GPL.
 *
 * BSD LICENSE
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/firmware.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/debugfs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/leds.h>

//#define AW210XX_DEBUG_FULL
//#define AW_DEBUG

#include "leds_aw210xx.h"
#include "leds_aw210xx_reg.h"

static int aw210xx_hw_enable(struct aw210xx *aw210xx, bool flag);
static int aw210xx_led_init(struct aw210xx *aw210xx);
static int aw210xx_read_chipid(struct aw210xx *aw210xx);

/******************************************************
 *
 * Marco
 *
 ******************************************************/
#define AW210XX_DRIVER_VERSION "V1.0.Q"
#define AW_I2C_RETRIES 5
#define AW_I2C_RETRY_DELAY 1
#define AW_READ_CHIPID_RETRIES 2
#define AW_READ_CHIPID_RETRY_DELAY 10
#define AW210XX_CFG_NAME_MAX	64

/******************************************************
 *
 * aw210xx led parameter
 *
 ******************************************************/

#ifdef AW210XX_DEBUG_FULL
struct aw210xx_cfg aw210xx_cfg_array[] = {
	{aw210xx_group_cfg_led_off, sizeof(aw210xx_group_cfg_led_off)},
	{aw21018_group_all_leds_on, sizeof(aw21018_group_all_leds_on)},
	{aw21018_group_red_leds_on, sizeof(aw21018_group_red_leds_on)},
	{aw21018_group_green_leds_on, sizeof(aw21018_group_green_leds_on)},
	{aw21018_group_blue_leds_on, sizeof(aw21018_group_blue_leds_on)},
	{aw21018_group_breath_leds_on, sizeof(aw21018_group_breath_leds_on)},
	{aw21012_group_all_leds_on, sizeof(aw21012_group_all_leds_on)},
	{aw21012_group_red_leds_on, sizeof(aw21012_group_red_leds_on)},
	{aw21012_group_green_leds_on, sizeof(aw21012_group_green_leds_on)},
	{aw21012_group_blue_leds_on, sizeof(aw21012_group_blue_leds_on)},
	{aw21012_group_breath_leds_on, sizeof(aw21012_group_breath_leds_on)},
	{aw21009_group_all_leds_on, sizeof(aw21009_group_all_leds_on)},
	{aw21009_group_red_leds_on, sizeof(aw21009_group_red_leds_on)},
	{aw21009_group_green_leds_on, sizeof(aw21009_group_green_leds_on)},
	{aw21009_group_blue_leds_on, sizeof(aw21009_group_blue_leds_on)},
	{aw21009_group_breath_leds_on, sizeof(aw21009_group_breath_leds_on)}
};
static char aw210xx_cfg_name[][AW210XX_CFG_NAME_MAX] = {
	{"aw210xx_group_cfg_led_off"},
	{"aw21018_group_all_leds_on"},
	{"aw21018_group_red_leds_on"},
	{"aw21018_group_green_leds_on"},
	{"aw21018_group_blue_leds_on"},
	{"aw21018_group_breath_leds_on"},
	{"aw21012_group_all_leds_on"},
	{"aw21012_group_red_leds_on"},
	{"aw21012_group_green_leds_on"},
	{"aw21012_group_blue_leds_on"},
	{"aw21012_group_breath_leds_on"},
	{"aw21009_group_all_leds_on"},
	{"aw21009_group_red_leds_on"},
	{"aw21009_group_green_leds_on"},
	{"aw21009_group_blue_leds_on"},
	{"aw21009_group_breath_leds_on"},
};
#endif

/******************************************************
 *
 * aw210xx i2c write/read
 *
 ******************************************************/
static int aw210xx_i2c_write(struct aw210xx *aw210xx,
		unsigned char reg_addr, unsigned char reg_data)
{
	int ret = -1;
	unsigned char cnt = 0;

	while (cnt < AW_I2C_RETRIES) {
		ret = i2c_smbus_write_byte_data(aw210xx->i2c,
				reg_addr, reg_data);
		if (ret < 0)
			AW_ERR("i2c_write cnt=%d ret=%d\n", cnt, ret);
		else
			break;
		cnt++;
		usleep_range(AW_I2C_RETRY_DELAY * 1000,
				AW_I2C_RETRY_DELAY * 1000 + 500);
	}

	return ret;
}

static int aw210xx_i2c_read(struct aw210xx *aw210xx,
		unsigned char reg_addr, unsigned char *reg_data)
{
	int ret = -1;
	unsigned char cnt = 0;

	while (cnt < AW_I2C_RETRIES) {
		ret = i2c_smbus_read_byte_data(aw210xx->i2c, reg_addr);
		if (ret < 0) {
			AW_ERR("i2c_read cnt=%d ret=%d\n", cnt, ret);
		} else {
			*reg_data = ret;
			break;
		}
		cnt++;
		usleep_range(AW_I2C_RETRY_DELAY * 1000,
				AW_I2C_RETRY_DELAY * 1000 + 500);
	}

	return ret;
}

static int aw210xx_i2c_write_bits(struct aw210xx *aw210xx,
		unsigned char reg_addr, unsigned int mask,
		unsigned char reg_data)
{
	unsigned char reg_val;

	aw210xx_i2c_read(aw210xx, reg_addr, &reg_val);
	reg_val &= mask;
	reg_val |= (reg_data & (~mask));
	aw210xx_i2c_write(aw210xx, reg_addr, reg_val);

	return 0;
}

/*****************************************************
 * led Interface: set effect
 *****************************************************/

#ifdef AW210XX_DEBUG_FULL
static void aw210xx_update_cfg_array(struct aw210xx *aw210xx,
		uint8_t *p_cfg_data, uint32_t cfg_size)
{
	unsigned int i = 0;

	for (i = 0; i < cfg_size; i += 2)
		aw210xx_i2c_write(aw210xx, p_cfg_data[i], p_cfg_data[i + 1]);
}

void aw210xx_cfg_update(struct aw210xx *aw210xx)
{
	AW_LOG("aw210xx->effect = %d", aw210xx->effect);

	aw210xx_update_cfg_array(aw210xx,
			aw210xx_cfg_array[aw210xx->effect].p,
			aw210xx_cfg_array[aw210xx->effect].count);
}
#endif

void aw210xx_uvlo_set(struct aw210xx *aw210xx, bool flag)
{
	if (flag) {
		aw210xx_i2c_write_bits(aw210xx,
				AW210XX_REG_UVCR,
				AW210XX_BIT_UVPD_MASK,
				AW210XX_BIT_UVPD_DISENA);
		aw210xx_i2c_write_bits(aw210xx,
				AW210XX_REG_UVCR,
				AW210XX_BIT_UVDIS_MASK,
				AW210XX_BIT_UVDIS_DISENA);
	} else {
		aw210xx_i2c_write_bits(aw210xx,
				AW210XX_REG_UVCR,
				AW210XX_BIT_UVPD_MASK,
				AW210XX_BIT_UVPD_ENABLE);
		aw210xx_i2c_write_bits(aw210xx,
				AW210XX_REG_UVCR,
				AW210XX_BIT_UVDIS_MASK,
				AW210XX_BIT_UVDIS_ENABLE);
	}
}

void aw210xx_sbmd_set(struct aw210xx *aw210xx, bool flag)
{
	if (flag) {
		aw210xx_i2c_write_bits(aw210xx,
				AW210XX_REG_GCR2,
				AW210XX_BIT_SBMD_MASK,
				AW210XX_BIT_SBMD_ENABLE);
		aw210xx->sdmd_flag = 1;
	} else {
		aw210xx_i2c_write_bits(aw210xx,
				AW210XX_REG_GCR2,
				AW210XX_BIT_SBMD_MASK,
				AW210XX_BIT_SBMD_DISENA);
		aw210xx->sdmd_flag = 0;
	}
}

void aw210xx_rgbmd_set(struct aw210xx *aw210xx, bool flag)
{
	if (flag) {
		aw210xx_i2c_write_bits(aw210xx,
				AW210XX_REG_GCR2,
				AW210XX_BIT_RGBMD_MASK,
				AW210XX_BIT_RGBMD_ENABLE);
		aw210xx->rgbmd_flag = 1;
	} else {
		aw210xx_i2c_write_bits(aw210xx,
				AW210XX_REG_GCR2,
				AW210XX_BIT_RGBMD_MASK,
				AW210XX_BIT_RGBMD_DISENA);
		aw210xx->rgbmd_flag = 0;
	}
}

void aw210xx_apse_set(struct aw210xx *aw210xx, bool flag)
{
	if (flag) {
		aw210xx_i2c_write_bits(aw210xx,
				AW210XX_REG_GCR,
				AW210XX_BIT_APSE_MASK,
				AW210XX_BIT_APSE_ENABLE);
	} else {
		aw210xx_i2c_write_bits(aw210xx,
				AW210XX_REG_GCR,
				AW210XX_BIT_APSE_MASK,
				AW210XX_BIT_APSE_DISENA);
	}
}

/*****************************************************
 * aw210xx led function set
 *****************************************************/
int32_t aw210xx_osc_pwm_set(struct aw210xx *aw210xx)
{
	switch (aw210xx->osc_clk) {
	case CLK_FRQ_16M:
		AW_LOG("osc is 16MHz!\n");
		aw210xx_i2c_write_bits(aw210xx,
				AW210XX_REG_GCR,
				AW210XX_BIT_CLKFRQ_MASK,
				AW210XX_BIT_CLKFRQ_16MHz);
		break;
	case CLK_FRQ_8M:
		AW_LOG("osc is 8MHz!\n");
		aw210xx_i2c_write_bits(aw210xx,
				AW210XX_REG_GCR,
				AW210XX_BIT_CLKFRQ_MASK,
				AW210XX_BIT_CLKFRQ_8MHz);
		break;
	case CLK_FRQ_1M:
		AW_LOG("osc is 1MHz!\n");
		aw210xx_i2c_write_bits(aw210xx,
				AW210XX_REG_GCR,
				AW210XX_BIT_CLKFRQ_MASK,
				AW210XX_BIT_CLKFRQ_1MHz);
		break;
	case CLK_FRQ_512k:
		AW_LOG("osc is 512KHz!\n");
		aw210xx_i2c_write_bits(aw210xx,
				AW210XX_REG_GCR,
				AW210XX_BIT_CLKFRQ_MASK,
				AW210XX_BIT_CLKFRQ_512kHz);
		break;
	case CLK_FRQ_256k:
		AW_LOG("osc is 256KHz!\n");
		aw210xx_i2c_write_bits(aw210xx,
				AW210XX_REG_GCR,
				AW210XX_BIT_CLKFRQ_MASK,
				AW210XX_BIT_CLKFRQ_256kHz);
		break;
	case CLK_FRQ_125K:
		AW_LOG("osc is 125KHz!\n");
		aw210xx_i2c_write_bits(aw210xx,
				AW210XX_REG_GCR,
				AW210XX_BIT_CLKFRQ_MASK,
				AW210XX_BIT_CLKFRQ_125kHz);
		break;
	case CLK_FRQ_62_5K:
		AW_LOG("osc is 62.5KHz!\n");
		aw210xx_i2c_write_bits(aw210xx,
				AW210XX_REG_GCR,
				AW210XX_BIT_CLKFRQ_MASK,
				AW210XX_BIT_CLKFRQ_62_5kHz);
		break;
	case CLK_FRQ_31_25K:
		AW_LOG("osc is 31.25KHz!\n");
		aw210xx_i2c_write_bits(aw210xx,
				AW210XX_REG_GCR,
				AW210XX_BIT_CLKFRQ_MASK,
				AW210XX_BIT_CLKFRQ_31_25kHz);
		break;
	default:
		AW_LOG("this clk_pwm is unsupported!\n");
		return -AW210XX_CLK_MODE_UNSUPPORT;
	}

	return 0;
}

int32_t aw210xx_br_res_set(struct aw210xx *aw210xx)
{
	switch (aw210xx->br_res) {
	case BR_RESOLUTION_8BIT:
		AW_LOG("br resolution select 8bit!\n");
		aw210xx_i2c_write_bits(aw210xx,
				AW210XX_REG_GCR,
				AW210XX_BIT_PWMRES_MASK,
				AW210XX_BIT_PWMRES_8BIT);
		break;
	case BR_RESOLUTION_9BIT:
		AW_LOG("br resolution select 9bit!\n");
		aw210xx_i2c_write_bits(aw210xx,
				AW210XX_REG_GCR,
				AW210XX_BIT_PWMRES_MASK,
				AW210XX_BIT_PWMRES_9BIT);
		break;
	case BR_RESOLUTION_12BIT:
		AW_LOG("br resolution select 12bit!\n");
		aw210xx_i2c_write_bits(aw210xx,
				AW210XX_REG_GCR,
				AW210XX_BIT_PWMRES_MASK,
				AW210XX_BIT_PWMRES_12BIT);
		break;
	case BR_RESOLUTION_9_AND_3_BIT:
		AW_LOG("br resolution select 9+3bit!\n");
		aw210xx_i2c_write_bits(aw210xx,
				AW210XX_REG_GCR,
				AW210XX_BIT_PWMRES_MASK,
				AW210XX_BIT_PWMRES_9_AND_3_BIT);
		break;
	default:
		AW_LOG("this br_res is unsupported!\n");
		return -AW210XX_CLK_MODE_UNSUPPORT;
	}

	return 0;
}

/*****************************************************
 * aw210xx debug interface set
 *****************************************************/
static void aw210xx_update(struct aw210xx *aw210xx)
{
	aw210xx_i2c_write(aw210xx, AW210XX_REG_UPDATE, AW210XX_UPDATE_BR_SL);
}

void aw210xx_global_set(struct aw210xx *aw210xx)
{
	aw210xx_i2c_write(aw210xx, AW210XX_REG_GCCR, aw210xx->glo_current);
}
void aw210xx_current_set(struct aw210xx *aw210xx)
{
	aw210xx_i2c_write(aw210xx, AW210XX_REG_GCCR, aw210xx->set_current);
}

/*****************************************************
 * aw210xx basic function set
 *****************************************************/
void aw210xx_chipen_set(struct aw210xx *aw210xx, bool flag)
{
	if (flag) {
		aw210xx_i2c_write_bits(aw210xx,
				AW210XX_REG_GCR,
				AW210XX_BIT_CHIPEN_MASK,
				AW210XX_BIT_CHIPEN_ENABLE);
	} else {
		aw210xx_i2c_write_bits(aw210xx,
				AW210XX_REG_GCR,
				AW210XX_BIT_CHIPEN_MASK,
				AW210XX_BIT_CHIPEN_DISENA);
	}
}

static int aw210xx_led_init(struct aw210xx *aw210xx)
{
	aw210xx->sdmd_flag = 0;
	aw210xx->rgbmd_flag = 0;
	/* chip enable */
	aw210xx_chipen_set(aw210xx, true);
	usleep_range(1000, 1500);
	/* sbmd enable */
	aw210xx_sbmd_set(aw210xx, true);
	/* rgbmd enable */
	aw210xx_rgbmd_set(aw210xx, false);
	/* clk_pwm selsect */
	aw210xx_osc_pwm_set(aw210xx);
	/* br_res select */
	aw210xx_br_res_set(aw210xx);
	/* global set */
	aw210xx_global_set(aw210xx);
	/* under voltage lock out */
	aw210xx_uvlo_set(aw210xx, false);
	/* apse enable */
	aw210xx_apse_set(aw210xx, false);
	/* set chip to oplus group mode*/
	aw210xx_i2c_write(aw210xx, AW210XX_REG_GCFG, 0x4f); // Fix channels: oplus custom code(?)

	return 0;
}

static int aw210xx_hw_enable(struct aw210xx *aw210xx, bool flag)
{
	AW_LOG("enter\n");

	if (aw210xx && gpio_is_valid(aw210xx->enable_gpio) && gpio_is_valid(aw210xx->vbled_enable_gpio)) {
		if (flag) {
			gpio_set_value_cansleep(aw210xx->enable_gpio, 1);
			gpio_set_value_cansleep(aw210xx->vbled_enable_gpio, 1);
			usleep_range(2000, 2500);
			aw210xx_led_init(aw210xx);
		} else {
			gpio_set_value_cansleep(aw210xx->enable_gpio, 0);
			gpio_set_value_cansleep(aw210xx->vbled_enable_gpio, 0);
		}
	} else {
		AW_ERR("failed\n");
	}

	return 0;
}

/*****************************************************
 * open short detect
 *****************************************************/

 #ifdef AW210XX_DEBUG_FULL

void aw210xx_open_detect_cfg(struct aw210xx *aw210xx)
{
	/*enable open detect*/
	aw210xx_i2c_write(aw210xx, AW210XX_REG_OSDCR, AW210XX_OPEN_DETECT_EN);
	/*set DCPWM = 1*/
	aw210xx_i2c_write_bits(aw210xx, AW210XX_REG_SSCR,
							AW210XX_DCPWM_SET_MASK,
							AW210XX_DCPWM_SET);
	/*set Open threshold = 0.2v*/
	aw210xx_i2c_write_bits(aw210xx,
							AW210XX_REG_OSDCR,
							AW210XX_OPEN_THRESHOLD_SET_MASK,
							AW210XX_OPEN_THRESHOLD_SET);
}

void aw210xx_short_detect_cfg(struct aw210xx *aw210xx)
{
	/*enable short detect*/
	aw210xx_i2c_write(aw210xx, AW210XX_REG_OSDCR, AW210XX_SHORT_DETECT_EN);
	/*set DCPWM = 1*/
	aw210xx_i2c_write_bits(aw210xx, AW210XX_REG_SSCR,
							AW210XX_DCPWM_SET_MASK,
							AW210XX_DCPWM_SET);
	/*set Short threshold = 1v*/
	aw210xx_i2c_write_bits(aw210xx,
							AW210XX_REG_OSDCR,
							AW210XX_SHORT_THRESHOLD_SET_MASK,
							AW210XX_SHORT_THRESHOLD_SET);
}

void aw210xx_open_short_dis(struct aw210xx *aw210xx)
{
	aw210xx_i2c_write(aw210xx, AW210XX_REG_OSDCR, AW210XX_OPEN_SHORT_DIS);
	/*SET DCPWM = 0*/
	aw210xx_i2c_write_bits(aw210xx, AW210XX_REG_SSCR, AW210XX_DCPWM_SET_MASK,
							AW210XX_DCPWM_CLEAN);
}
void aw210xx_open_short_detect(struct aw210xx *aw210xx, int32_t detect_flg, u8 *reg_val)
{
	/*config for open shor detect*/
	if (detect_flg == AW210XX_OPEN_DETECT)
		aw210xx_open_detect_cfg(aw210xx);
	else if (detect_flg == AW210XX_SHORT_DETECT)
		aw210xx_short_detect_cfg(aw210xx);
	/*read detect result*/
	aw210xx_i2c_read(aw210xx, AW210XX_REG_OSST0, &reg_val[0]);
	aw210xx_i2c_read(aw210xx, AW210XX_REG_OSST1, &reg_val[1]);
	aw210xx_i2c_read(aw210xx, AW210XX_REG_OSST2, &reg_val[2]);
	/*close for open short detect*/
	aw210xx_open_short_dis(aw210xx);
}
#endif

/******************************************************
 *
 * sys group attribute: reg
 *
 ******************************************************/
static ssize_t aw210xx_reg_store(struct device *dev,
		struct device_attribute *attr, const char *buf,
		size_t len)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct aw210xx *aw210xx = container_of(led_cdev, struct aw210xx, cdev);
	uint32_t databuf[2] = { 0, 0 };

	if (sscanf(buf, "%x %x", &databuf[0], &databuf[1]) == 2) {
		if (aw210xx_reg_access[(uint8_t)databuf[0]] & REG_WR_ACCESS)
			aw210xx_i2c_write(aw210xx, (uint8_t)databuf[0],
					(uint8_t)databuf[1]);
	}

	return len;
}

static ssize_t aw210xx_reg_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct aw210xx *aw210xx = container_of(led_cdev, struct aw210xx, cdev);
	ssize_t len = 0;
	unsigned int i = 0;
	unsigned char reg_val = 0;
	uint8_t br_max = 0;
	uint8_t sl_val = 0;

	aw210xx_read_chipid(aw210xx);
	aw210xx_i2c_read(aw210xx, AW210XX_REG_GCR, &reg_val);
	len += snprintf(buf + len, PAGE_SIZE - len,
			"reg:0x%02x=0x%02x\n", AW210XX_REG_GCR, reg_val);
	switch (aw210xx->chipid) {
	case AW21018_CHIPID:
		br_max = AW210XX_REG_BR17H;
		sl_val = AW210XX_REG_SL17;
		break;
	case AW21012_CHIPID:
		br_max = AW210XX_REG_BR11H;
		sl_val = AW210XX_REG_SL11;
		break;
	case AW21009_CHIPID:
		br_max = AW210XX_REG_BR08H;
		sl_val = AW210XX_REG_SL08;
		break;
	default:
		AW_LOG("chip is unsupported device!\n");
		return len;
	}

	for (i = AW210XX_REG_BR00L; i <= br_max; i++) {
		if (!(aw210xx_reg_access[i] & REG_RD_ACCESS))
			continue;
		aw210xx_i2c_read(aw210xx, i, &reg_val);
		len += snprintf(buf + len, PAGE_SIZE - len,
				"reg:0x%02x=0x%02x\n", i, reg_val);
	}
	for (i = AW210XX_REG_SL00; i <= sl_val; i++) {
		if (!(aw210xx_reg_access[i] & REG_RD_ACCESS))
			continue;
		aw210xx_i2c_read(aw210xx, i, &reg_val);
		len += snprintf(buf + len, PAGE_SIZE - len,
				"reg:0x%02x=0x%02x\n", i, reg_val);
	}
	for (i = AW210XX_REG_GCCR; i <= AW210XX_REG_GCFG; i++) {
		if (!(aw210xx_reg_access[i] & REG_RD_ACCESS))
			continue;
		aw210xx_i2c_read(aw210xx, i, &reg_val);
		len += snprintf(buf + len, PAGE_SIZE - len,
				"reg:0x%02x=0x%02x\n", i, reg_val);
	}

	return len;
}

static ssize_t aw210xx_hwen_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t len)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct aw210xx *aw210xx = container_of(led_cdev, struct aw210xx, cdev);
	int rc;
	unsigned int val = 0;

	rc = kstrtouint(buf, 0, &val);
	if (rc < 0)
		return rc;

	if (val > 0)
		aw210xx_hw_enable(aw210xx, true);
	else
		aw210xx_hw_enable(aw210xx, false);

	return len;
}

static ssize_t aw210xx_hwen_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct aw210xx *aw210xx = container_of(led_cdev, struct aw210xx, cdev);
	ssize_t len = 0;

	len += snprintf(buf + len, PAGE_SIZE - len, "hwen=%d\n",
			gpio_get_value(aw210xx->enable_gpio));
	return len;
}

#ifdef AW210XX_DEBUG_FULL
static ssize_t
aw210xx_effect_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct aw210xx *aw210xx = container_of(led_cdev, struct aw210xx, cdev);
	ssize_t len = 0;
	unsigned int i;

	for (i = 0; i < (sizeof(aw210xx_cfg_array) / sizeof(struct aw210xx_cfg)); i++) {
		len += snprintf(buf + len, PAGE_SIZE - len, "effect[%x]: %s\n",
				i, aw210xx_cfg_name[i]);
	}

	len += snprintf(buf + len, PAGE_SIZE - len, "current effect[%d]: %s\n",
			aw210xx->effect, aw210xx_cfg_name[aw210xx->effect]);
	return len;
}

static ssize_t
aw210xx_effect_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct aw210xx *aw210xx = container_of(led_cdev, struct aw210xx, cdev);
	int rc;
	unsigned int val = 0;

	rc = kstrtouint(buf, 10, &val);
	if (rc < 0)
		return rc;
	if ((val >= (sizeof(aw210xx_cfg_array) / sizeof(struct aw210xx_cfg))) || (val < 0)) {
		AW_ERR("store effect num error.\n");
		return -EINVAL;
	}

	aw210xx->effect = val;

	aw210xx_cfg_update(aw210xx);

	return len;
}
#endif

void aw210xx_timings_update(struct aw210xx *aw210xx) {

    AW_LOG("applying timings...\n");
    if (global_mode == AW210XX_LED_BLINKMODE || global_mode == AW210XX_LED_BREATHMODE) {
        // If blink mode, avoid T0, T2 != 0
    	if (global_mode == AW210XX_LED_BLINKMODE) {
    		if (T0 != 0) T0 = 0;
    		if (T1 == 0) T1 = 4;
    		if (T2 != 0) T2 = 0;
    		if (T3 == 0) T3 = 4;
        	AW_LOG("T0 and T2 set to 0 due to blink_mode\n");
        	AW_LOG("T1 and T3 set to default 4 due to blink_mode\n");
    	} else if (global_mode == AW210XX_LED_BREATHMODE) {
    		if (T0 == 0) T0 = 4;
    		if (T2 == 0) T2 = 4;
    		AW_LOG("T0 and T2 set to default 4 due to breath_mode\n");
    	}

    // ABMT0 = T0 << 4 | T1; ABMT1 = T2 << 4 | T3
    aw210xx_i2c_write(aw210xx, AW210XX_REG_ABMT0, (T0 << 4) | T1);
    aw210xx_i2c_write(aw210xx, AW210XX_REG_ABMT1, (T2 << 4) | T3);

    AW_LOG("Sucsess TIMING write: T0=%02x, T1=%02x, T2=%02x, T3=%02x!\n", T0, T1, T2, T3);
    } else {
    	AW_LOG("Timing change write ignored due to unconfigurable mode\n");
    }
}

void aw210xx_mode_update(struct aw210xx *aw210xx) {
    u8 buf = gpio_get_value(aw210xx->enable_gpio);

    AW_LOG("applying mode %d\n", global_mode);
    if (buf == 1) { 
        aw210xx_i2c_write(aw210xx, AW210XX_REG_ABMCFG, 0x00);  // Disable pattern
        aw210xx_i2c_write(aw210xx, AW210XX_REG_ABMGO, 0x00);  // Stop breath
        aw210xx_i2c_write(aw210xx, AW210XX_REG_GCCR, 0x00); // Write 0 to dim all
        if (global_mode == AW210XX_LED_NONE) {
        	aw210xx_hw_enable(aw210xx, false);
        	AW_LOG("Send chip to sleep!!\n");
        }
    } else {
    	if (global_mode != AW210XX_LED_NONE) {
        	aw210xx_hw_enable(aw210xx, true);
        	AW_LOG("Wake-up chip from sleep state!!\n");
        }
    }

    if (global_mode == AW210XX_LED_NONE) {
        AW_LOG("ignore none: already done\n");
    } else {
    	if (global_mode == AW210XX_LED_CCMODE) {
            aw210xx_i2c_write(aw210xx, AW210XX_REG_GBRH, 0x00); // Invert L/H
            aw210xx_i2c_write(aw210xx, AW210XX_REG_GBRL, 0xff); // This works by looping aw into L state with max brightness
            aw210xx_i2c_write(aw210xx, AW210XX_REG_ABMCFG, 0x00); // = 0000, PAT=0, loop in L state
            AW_LOG("cc_mode scheduled succesfully\n");
    	} else {
    		aw210xx_i2c_write(aw210xx, AW210XX_REG_GBRH, 0xff); // High (max, dim trough channels)
            aw210xx_i2c_write(aw210xx, AW210XX_REG_GBRL, 0x00); // Low (0)
            aw210xx_i2c_write(aw210xx, AW210XX_REG_ABMCFG, 0x03); // = 1100, PAT=1, PAT_MANUAL=1

            aw210xx_timings_update(aw210xx); // Blinkmode or breathmode, default would be set if don't configured

            aw210xx_i2c_write(aw210xx, AW210XX_REG_ABMGO, 0x01);  // Start breath
            AW_LOG("blink or breath mode scheduled succesfully\n");
    	}
    	aw210xx_i2c_write(aw210xx, AW210XX_REG_GCCR, (aw210xx->cdev.brightness * 0xCC) / 0xFF ); // Scale to 0xCC since oplus make led color trough dim mask and uses 0xCC as max
    }

    aw210xx_update(aw210xx);
}

static ssize_t aw210xx_mode_show(struct device *dev,
                                 struct device_attribute *attr, char *buf) {
    ssize_t len = 0;
    unsigned int i;

    for (i = 0; i < AW210XX_NUM_MODES; i++) {
        len += snprintf(buf + len, PAGE_SIZE - len, "mode[%u]: %s\n",
                        i, aw210xx_mode_names[i]);
    }

    len += snprintf(buf + len, PAGE_SIZE - len, "current mode[%d]: %s\n",
                    global_mode, aw210xx_mode_names[global_mode]);

    return len;
}

static ssize_t aw210xx_mode_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t len)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct aw210xx *aw210xx = container_of(led_cdev, struct aw210xx, cdev);
	int rc;
	unsigned int val = 0;

	rc = kstrtouint(buf, 10, &val);
    if (rc < 0) {
        return rc;
    }
    if (val >= AW210XX_NUM_MODES) {
        AW_ERR("invalid mode %u\n", val);
        return -EINVAL;
    }

	global_mode = val;
    AW_LOG("mode change to %d (%s) scheduled!\n", val, aw210xx_mode_names[val]);

    aw210xx_mode_update(aw210xx);

    return len;
}

static ssize_t aw210xx_rgbcolor_show(struct device *dev,
                                     struct device_attribute *attr, char *buf) {
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    struct aw210xx *aw210xx = container_of(led_cdev, struct aw210xx, cdev);
    u8 r, g, b, w;

    aw210xx_i2c_read(aw210xx, AW210XX_REG_SL08, &w);  // White id=0
    aw210xx_i2c_read(aw210xx, AW210XX_REG_SL09, &r);  // Red   id=1
    aw210xx_i2c_read(aw210xx, AW210XX_REG_SL10, &g);  // Green id=2
    aw210xx_i2c_read(aw210xx, AW210XX_REG_SL11, &b);  // Blue  id=3

	return snprintf(buf, PAGE_SIZE, "W:%02x \nR:%02x \nG:%02x \nB:%02x\n", w, r, g, b);
}

static ssize_t aw210xx_rgbcolor_store(struct device *dev,
                                      struct device_attribute *attr,
                                      const char *buf, size_t len) {
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
    struct aw210xx *aw210xx = container_of(led_cdev, struct aw210xx, cdev);
    u8 r = 0, g = 0, b = 0, w = 0;  // u8 for 0-255
    int parsed, ret = 0;

    // First try decimal (0-255) (awoid hex 11 != dec 11)
    parsed = sscanf(buf, "%hhu %hhu %hhu %hhu", &w, &r, &g, &b);
    if (parsed != 4) {
        // if fail try HEX 0xXX
        /*
         * FORWARDPORT: this used "%x" with u8 pointers, making sscanf write
         * 4 bytes into each 1-byte variable and clobber the neighbouring colour
         * components on the stack. clang flags it with -Wformat. "%hhx" is the
         * same hex conversion at the correct width.
         */
        parsed = sscanf(buf, "%hhx %hhx %hhx %hhx", &w, &r, &g, &b);
        if (parsed != 4) {
            // if fail, try one HEX 0xXXXXXXXX
            unsigned int color;
            parsed = sscanf(buf, "%11x", &color);
            if (parsed != 1 || color > 0xFFFFFFFF) {
                AW_ERR("invalid input, expected 'AA RR GG BB' (0xHEX or decimal 0-255)\n");
                ret = -EINVAL;
                goto error;
            } else {
                w = (color >> 24) & 0xFF;
                r = (color >> 16) & 0xFF;
                g = (color >> 8) & 0xFF;
                b = color & 0xFF;
            }
        }
    }
    
    aw210xx_i2c_write(aw210xx, AW210XX_REG_SL08, w);  // White id=0
    aw210xx_i2c_write(aw210xx, AW210XX_REG_SL09, r);  // Red   id=1
    aw210xx_i2c_write(aw210xx, AW210XX_REG_SL10, g);  // Green id=2
    aw210xx_i2c_write(aw210xx, AW210XX_REG_SL11, b);  // Blue  id=3

    AW_LOG("Sucsess WRGB write: W=0x%02x R=0x%02x G=0x%02x B=0x%02x!\n", w, r, g, b);

	aw210xx_update(aw210xx);
		
error:
    return ret ? ret : len;  // Return error or len
}

static void aw210xx_set_brightness(struct led_classdev *cdev,
		enum led_brightness brightness)
{
	struct aw210xx *aw210xx = container_of(cdev, struct aw210xx, cdev);

    // Finally store brightness
    aw210xx->cdev.brightness = brightness;

    // Apply it
    aw210xx_i2c_write(aw210xx, AW210XX_REG_GCCR, (brightness * 0xCC) / 0xFF ); // Scale to 0xCC since oplus make led color trough dim mask and uses 0xCC as max

    AW_LOG("Sucsess brightness write: 0x%02x!\n", brightness);

	aw210xx_update(aw210xx);
}

static ssize_t aw210xx_led_timings_attr_show (struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "T0:%01x \nT1:%01x \nT2:%01x \nT3:%01x\n", T0, T1, T2, T3);
}

static ssize_t aw210xx_led_timings_attr_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t len)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
    struct aw210xx *aw210xx = container_of(led_cdev, struct aw210xx, cdev);
    u8 t0r = 0, t1h = 0, t2f = 0, t3h = 0;  // u8 for 0-255 range
    int parsed, ret = 0;

	// First try decimal (0-255) (awoid hex 11 != dec 11)
    parsed = sscanf(buf, "%hhu %hhu %hhu %hhu", &t0r, &t1h, &t2f, &t3h);
    if (parsed != 4) {
        // if fail, try HEX 0xXX
        /* "%hhx" rather than "%x"; see aw210xx_rgbcolor_store() */
        parsed = sscanf(buf, "%hhx %hhx %hhx %hhx", &t0r, &t1h, &t2f, &t3h);
        if (parsed != 4) {
            AW_ERR("invalid input, expected 'T0 T1 T2 T3' (0xHEX or decimal 0-15)\n");
            ret = -EINVAL;
            goto error;
        }
    }

    // Limit to 0-15 -> 0-F
    T0 = min(t0r, (u8)15);
    T1 = min(t1h, (u8)15);
    T2 = min(t2f, (u8)15);
    T3 = min(t3h, (u8)15);

    aw210xx_timings_update(aw210xx);

    aw210xx_update(aw210xx);

error:
    return ret ? ret : len;  // Return error or len
}

#ifdef AW210XX_DEBUG_FULL

static ssize_t
aw210xx_opdetect_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct aw210xx *aw210xx = container_of(led_cdev, struct aw210xx, cdev);
	ssize_t len = 0;
	int i = 0;
	uint8_t reg_val[3] = {0};

	aw210xx_open_short_detect(aw210xx, AW210XX_OPEN_DETECT, reg_val);
	for (i = 0; i < sizeof(reg_val); i++)
		len += snprintf(buf + len, PAGE_SIZE - len, "OSST%d:%#x\n", i, reg_val[i]);

	return len;
}

static ssize_t
aw210xx_stdetect_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct aw210xx *aw210xx = container_of(led_cdev, struct aw210xx, cdev);
	ssize_t len = 0;
	int i = 0;
	uint8_t reg_val[3] = {0};

	aw210xx_open_short_detect(aw210xx, AW210XX_SHORT_DETECT, reg_val);
	for (i = 0; i < sizeof(reg_val); i++)
		len += snprintf(buf + len, PAGE_SIZE - len, "OSST%d:%#x\n", i, reg_val[i]);
	return len;
}

static DEVICE_ATTR(effect, 0664, aw210xx_effect_show, aw210xx_effect_store);
static DEVICE_ATTR(opdetect, 0664, aw210xx_opdetect_show, NULL);
static DEVICE_ATTR(stdetect, 0664, aw210xx_stdetect_show, NULL);
#endif
static DEVICE_ATTR(reg, 0664, aw210xx_reg_show, aw210xx_reg_store);
static DEVICE_ATTR(hwen, 0664, aw210xx_hwen_show, aw210xx_hwen_store);
static DEVICE_ATTR(mode, 0664, aw210xx_mode_show, aw210xx_mode_store);
static DEVICE_ATTR(timings, 0664, aw210xx_led_timings_attr_show, aw210xx_led_timings_attr_store);
static DEVICE_ATTR(rgbcolor, 0664, aw210xx_rgbcolor_show, aw210xx_rgbcolor_store);

static struct attribute *aw210xx_attributes[] = {
#ifdef AW210XX_DEBUG_FULL
	&dev_attr_effect.attr,
	&dev_attr_opdetect.attr,
	&dev_attr_stdetect.attr,
#endif
	&dev_attr_reg.attr,
	&dev_attr_hwen.attr,
	&dev_attr_mode.attr,
	&dev_attr_timings.attr,
	&dev_attr_rgbcolor.attr,
	NULL,
};

static struct attribute_group aw210xx_attribute_group = {
	.attrs = aw210xx_attributes
};
/******************************************************
 *
 * led class dev
 ******************************************************/

static int aw210xx_parse_led_cdev(struct aw210xx *aw210xx, struct device_node *np)
{
	int ret = -1;

	// We are NOT parsing cdev (LED's all wired to chip anyway, we just write to it's reg)
	aw210xx->imax = 0xFF;
	aw210xx->cdev.name = "backpanel"; // we want more understandable name than 'white'
	aw210xx->cdev.brightness = 0;
	aw210xx->cdev.max_brightness = 255;

	aw210xx->cdev.brightness_set = aw210xx_set_brightness;

	ret = led_classdev_register(aw210xx->dev, &aw210xx->cdev);
	if (ret) {
		AW_ERR("unable to register led ret=%d\n", ret);
		goto free_pdata;
	}

	ret = sysfs_create_group(&aw210xx->cdev.dev->kobj, &aw210xx_attribute_group);
	if (ret) {
		AW_ERR("led sysfs ret: %d\n", ret);
		goto free_class;
	}

	return 0;

free_class:
	led_classdev_unregister(&aw210xx->cdev);
free_pdata:
	return ret;
}

/*****************************************************
 *
 * check chip id and version
 *
 *****************************************************/
static int aw210xx_read_chipid(struct aw210xx *aw210xx)
{
	int ret = -1;
	unsigned char cnt = 0;
	unsigned char chipid = 0;

	while (cnt < AW_READ_CHIPID_RETRIES) {
		ret = aw210xx_i2c_read(aw210xx, AW210XX_REG_RESET, &chipid);
		if (ret < 0) {
			AW_ERR("failed to read chipid: %d\n", ret);
		} else {
			aw210xx->chipid = chipid;
			switch (aw210xx->chipid) {
			case AW21018_CHIPID:
				AW_LOG("AW21018, read chipid = 0x%02x!!\n", chipid);
				return 0;
			case AW21012_CHIPID:
				AW_LOG("AW21012, read chipid = 0x%02x!!\n", chipid);
				return 0;
			case AW21009_CHIPID:
				AW_LOG("AW21009, read chipid = 0x%02x!!\n", chipid);
				return 0;
			default:
				AW_LOG("chip is unsupported device id = %x\n", chipid);
				break;
			}
		}
		cnt++;
		usleep_range(AW_READ_CHIPID_RETRY_DELAY * 1000,
				AW_READ_CHIPID_RETRY_DELAY * 1000 + 500);
	}

	return -EINVAL;
}

/*****************************************************
 *
 * device tree
 *
 *****************************************************/
static int aw210xx_parse_dt(struct device *dev, struct aw210xx *aw210xx, struct device_node *np)
{
	int ret = -EINVAL;

	aw210xx->enable_gpio = of_get_named_gpio(np, "enable-gpio", 0);
	if (aw210xx->enable_gpio < 0) {
		aw210xx->enable_gpio = -1;
		AW_ERR("no enable gpio provided, HW enable unsupported\n");
		return ret;
	}

	aw210xx->vbled_enable_gpio = of_get_named_gpio(np, "vbled-enable-gpio", 0);
	if (aw210xx->vbled_enable_gpio < 0) {
		aw210xx->vbled_enable_gpio = -1;
		AW_ERR("no vbled enable gpio provided, HW enable unsupported\n");
		return ret;
	}

	ret = of_property_read_u32(np, "osc_clk", &aw210xx->osc_clk);
	if (ret < 0) {
		AW_ERR("no osc_clk provided, osc clk unsupported\n");
		return ret;
	}

	ret = of_property_read_u32(np, "br_res", &aw210xx->br_res);
	if (ret < 0) {
		AW_ERR("brightness resolution unsupported\n");
		return ret;
	}

	ret = of_property_read_u32(np, "global_current", &aw210xx->glo_current);
	if (ret < 0) {
		AW_ERR("global current resolution unsupported\n");
		return ret;
	}

	return 0;
}

/******************************************************
 *
 * i2c driver
 *
 ******************************************************/
/*
 * FORWARDPORT: in 6.6 i2c_driver::probe only takes the i2c_client; the
 * variant with the i2c_device_id argument was removed. It was unused here,
 * matching is done via of_match_table.
 */
static int aw210xx_i2c_probe(struct i2c_client *i2c)
{
	struct aw210xx *aw210xx;
	struct device_node *np = i2c->dev.of_node;
	int ret;

	if (!i2c_check_functionality(i2c->adapter, I2C_FUNC_I2C)) {
		AW_ERR("check_functionality failed\n");
		return -EIO;
	}

	aw210xx = devm_kzalloc(&i2c->dev, sizeof(struct aw210xx), GFP_KERNEL);
	if (aw210xx == NULL) {
		AW_ERR("Error allocating memory\n");
		return -ENOMEM;
	}

	aw210xx->dev = &i2c->dev;
	aw210xx->i2c = i2c;
	i2c_set_clientdata(i2c, aw210xx);

	/* aw210xx parse device tree */
	if (np) {
		ret = aw210xx_parse_dt(&i2c->dev, aw210xx, np);
		if (ret) {
			AW_ERR("failed to parse device tree node\n");
			goto err_parse_dt;
		}
	}

	if (gpio_is_valid(aw210xx->enable_gpio)) {
		ret = devm_gpio_request_one(&i2c->dev, aw210xx->enable_gpio,
				GPIOF_OUT_INIT_LOW, "aw210xx_en");
		if (ret) {
			AW_ERR("enable gpio request failed\n");
			goto err_gpio_request;
		}
	}

	if (gpio_is_valid(aw210xx->vbled_enable_gpio)) {
		ret = devm_gpio_request_one(&i2c->dev, aw210xx->vbled_enable_gpio,
				GPIOF_OUT_INIT_LOW, "aw210xx_vbled_en");
		if (ret) {
			AW_ERR("vbled enable gpio request failed\n");
			goto err_gpio_request;
		}
	}

	/* hardware enable */
	aw210xx_hw_enable(aw210xx, true);

	/* aw210xx identify */
	ret = aw210xx_read_chipid(aw210xx);
	if (ret < 0) {
		AW_ERR("aw210xx_read_chipid failed ret=%d\n", ret);
		goto err_id;
	}

	dev_set_drvdata(&i2c->dev, aw210xx);
	aw210xx_parse_led_cdev(aw210xx, np);
	if (ret < 0) {
		AW_ERR("error creating led class dev\n");
		goto err_sysfs;
	}

	return 0;

/*
 * FORWARDPORT: devm_gpio_free() no longer exists in 6.6. The GPIO requested
 * with devm_gpio_request_one() is released by the driver core on unbind.
 */
err_sysfs:
err_id:
err_gpio_request:
err_parse_dt:
	devm_kfree(&i2c->dev, aw210xx);
	aw210xx = NULL;
	return ret;
}

/*
 * FORWARDPORT: two changes required by 6.6:
 *
 * 1. i2c_driver::remove returns void since 6.1; the core ignored the old
 *    return value anyway.
 * 2. The explicit devm_gpio_free() on enable_gpio is gone; the driver core
 *    releases the devm-requested GPIO on unbind.
 */
static void aw210xx_i2c_remove(struct i2c_client *i2c)
{
	struct aw210xx *aw210xx = i2c_get_clientdata(i2c);

	sysfs_remove_group(&aw210xx->cdev.dev->kobj, &aw210xx_attribute_group);
	led_classdev_unregister(&aw210xx->cdev);
	devm_kfree(&i2c->dev, aw210xx);
	aw210xx = NULL;
}

static const struct i2c_device_id aw210xx_i2c_id[] = {
	{AW210XX_I2C_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, aw210xx_i2c_id);

static const struct of_device_id aw210xx_dt_match[] = {
	{.compatible = "awinic,aw210xx_led"},
	{}
};

static struct i2c_driver aw210xx_i2c_driver = {
	.driver = {
		.name = AW210XX_I2C_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(aw210xx_dt_match),
		},
	.probe = aw210xx_i2c_probe,
	.remove = aw210xx_i2c_remove,
	.id_table = aw210xx_i2c_id,
};

static int __init aw210xx_i2c_init(void)
{
	int ret = 0;

	AW_LOG("enter, aw210xx driver version %s\n", AW210XX_DRIVER_VERSION);

	ret = i2c_add_driver(&aw210xx_i2c_driver);
	if (ret) {
		AW_ERR("failed to register aw210xx driver!\n");
		return ret;
	}

	return 0;
}
module_init(aw210xx_i2c_init);

static void __exit aw210xx_i2c_exit(void)
{
	i2c_del_driver(&aw210xx_i2c_driver);
}
module_exit(aw210xx_i2c_exit);

MODULE_DESCRIPTION("AW210XX LED Driver");
MODULE_LICENSE("GPL v2");
