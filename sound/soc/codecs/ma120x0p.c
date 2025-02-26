// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * ASoC Driver for Infineon Merus(TM) ma120x0p multi-level class-D amplifier
 *
 * Authors:	Ariel Muszkat <ariel.muszkat@gmail.com>
 * Jorgen Kragh Jakobsen <jorgen.kraghjakobsen@infineon.com>
 *
 * Copyright (C) 2019 Infineon Technologies AG
 *
 */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm_runtime.h>
#include <linux/i2c.h>
#include <linux/of_device.h>
#include <linux/spi/spi.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <linux/interrupt.h>

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#ifndef _MA120X0P_
#define _MA120X0P_
//------------------------------------------------------------------manualPM---
// Select Manual PowerMode control
#define ma_manualpm__a 0
#define ma_manualpm__len 1
#define ma_manualpm__mask 0x40
#define ma_manualpm__shift 0x06
#define ma_manualpm__reset 0x00
//--------------------------------------------------------------------pm_man---
// manual selected power mode
#define ma_pm_man__a 0
#define ma_pm_man__len 2
#define ma_pm_man__mask 0x30
#define ma_pm_man__shift 0x04
#define ma_pm_man__reset 0x03
//------------------------------------------ ----------------------mthr_1to2---
// mod. index threshold value for pm1=>pm2 change.
#define ma_mthr_1to2__a 1
#define ma_mthr_1to2__len 8
#define ma_mthr_1to2__mask 0xff
#define ma_mthr_1to2__shift 0x00
#define ma_mthr_1to2__reset 0x3c
//-----------------------------------------------------------------mthr_2to1---
// mod. index threshold value for pm2=>pm1 change.
#define ma_mthr_2to1__a 2
#define ma_mthr_2to1__len 8
#define ma_mthr_2to1__mask 0xff
#define ma_mthr_2to1__shift 0x00
#define ma_mthr_2to1__reset 0x32
//-----------------------------------------------------------------mthr_2to3---
// mod. index threshold value for pm2=>pm3 change.
#define ma_mthr_2to3__a 3
#define ma_mthr_2to3__len 8
#define ma_mthr_2to3__mask 0xff
#define ma_mthr_2to3__shift 0x00
#define ma_mthr_2to3__reset 0x5a
//-----------------------------------------------------------------mthr_3to2---
// mod. index threshold value for pm3=>pm2 change.
#define ma_mthr_3to2__a 4
#define ma_mthr_3to2__len 8
#define ma_mthr_3to2__mask 0xff
#define ma_mthr_3to2__shift 0x00
#define ma_mthr_3to2__reset 0x50
//-------------------------------------------------------------pwmclkdiv_nom---
// pwm default clock divider value
#define ma_pwmclkdiv_nom__a 8
#define ma_pwmclkdiv_nom__len 8
#define ma_pwmclkdiv_nom__mask 0xff
#define ma_pwmclkdiv_nom__shift 0x00
#define ma_pwmclkdiv_nom__reset 0x26
//--------- ----------------------------------------------------ocp_latch_en---
// high to use permanently latching level-2 ocp
#define ma_ocp_latch_en__a 10
#define ma_ocp_latch_en__len 1
#define ma_ocp_latch_en__mask 0x02
#define ma_ocp_latch_en__shift 0x01
#define ma_ocp_latch_en__reset 0x00
//---------------------------------------------------------------lf_clamp_en---
// high (default) to enable lf int2+3 clamping on clip
#define ma_lf_clamp_en__a 10
#define ma_lf_clamp_en__len 1
#define ma_lf_clamp_en__mask 0x80
#define ma_lf_clamp_en__shift 0x07
#define ma_lf_clamp_en__reset 0x00
//-------------------------------------------------------pmcfg_btl_b.modtype---
//
#define ma_pmcfg_btl_b__modtype__a 18
#define ma_pmcfg_btl_b__modtype__len 2
#define ma_pmcfg_btl_b__modtype__mask 0x18
#define ma_pmcfg_btl_b__modtype__shift 0x03
#define ma_pmcfg_btl_b__modtype__reset 0x02
//-------------------------------------------------------pmcfg_btl_b.freqdiv---
#define ma_pmcfg_btl_b__freqdiv__a 18
#define ma_pmcfg_btl_b__freqdiv__len 2
#define ma_pmcfg_btl_b__freqdiv__mask 0x06
#define ma_pmcfg_btl_b__freqdiv__shift 0x01
#define ma_pmcfg_btl_b__freqdiv__reset 0x01
//----------------------------------------------------pmcfg_btl_b.lf_gain_ol---
//
#define ma_pmcfg_btl_b__lf_gain_ol__a 18
#define ma_pmcfg_btl_b__lf_gain_ol__len 1
#define ma_pmcfg_btl_b__lf_gain_ol__mask 0x01
#define ma_pmcfg_btl_b__lf_gain_ol__shift 0x00
#define ma_pmcfg_btl_b__lf_gain_ol__reset 0x01
//-------------------------------------------------------pmcfg_btl_c.freqdiv---
//
#define ma_pmcfg_btl_c__freqdiv__a 19
#define ma_pmcfg_btl_c__freqdiv__len 2
#define ma_pmcfg_btl_c__freqdiv__mask 0x06
#define ma_pmcfg_btl_c__freqdiv__shift 0x01
#define ma_pmcfg_btl_c__freqdiv__reset 0x01
//-------------------------------------------------------pmcfg_btl_c.modtype---
//
#define ma_pmcfg_btl_c__modtype__a 19
#define ma_pmcfg_btl_c__modtype__len 2
#define ma_pmcfg_btl_c__modtype__mask 0x18
#define ma_pmcfg_btl_c__modtype__shift 0x03
#define ma_pmcfg_btl_c__modtype__reset 0x01
//----------------------------------------------------pmcfg_btl_c.lf_gain_ol---
//
#define ma_pmcfg_btl_c__lf_gain_ol__a 19
#define ma_pmcfg_btl_c__lf_gain_ol__len 1
#define ma_pmcfg_btl_c__lf_gain_ol__mask 0x01
#define ma_pmcfg_btl_c__lf_gain_ol__shift 0x00
#define ma_pmcfg_btl_c__lf_gain_ol__reset 0x00
//-------------------------------------------------------pmcfg_btl_d.modtype---
//
#define ma_pmcfg_btl_d__modtype__a 20
#define ma_pmcfg_btl_d__modtype__len 2
#define ma_pmcfg_btl_d__modtype__mask 0x18
#define ma_pmcfg_btl_d__modtype__shift 0x03
#define ma_pmcfg_btl_d__modtype__reset 0x02
//-------------------------------------------------------pmcfg_btl_d.freqdiv---
//
#define ma_pmcfg_btl_d__freqdiv__a 20
#define ma_pmcfg_btl_d__freqdiv__len 2
#define ma_pmcfg_btl_d__freqdiv__mask 0x06
#define ma_pmcfg_btl_d__freqdiv__shift 0x01
#define ma_pmcfg_btl_d__freqdiv__reset 0x02
//----------------------------------------------------pmcfg_btl_d.lf_gain_ol---
//
#define ma_pmcfg_btl_d__lf_gain_ol__a 20
#define ma_pmcfg_btl_d__lf_gain_ol__len 1
#define ma_pmcfg_btl_d__lf_gain_ol__mask 0x01
#define ma_pmcfg_btl_d__lf_gain_ol__shift 0x00
#define ma_pmcfg_btl_d__lf_gain_ol__reset 0x00
//------------ -------------------------------------------pmcfg_se_a.modtype---
//
#define ma_pmcfg_se_a__modtype__a 21
#define ma_pmcfg_se_a__modtype__len 2
#define ma_pmcfg_se_a__modtype__mask 0x18
#define ma_pmcfg_se_a__modtype__shift 0x03
#define ma_pmcfg_se_a__modtype__reset 0x01
//--------------------------------------------------------pmcfg_se_a.freqdiv---
//
#define ma_pmcfg_se_a__freqdiv__a 21
#define ma_pmcfg_se_a__freqdiv__len 2
#define ma_pmcfg_se_a__freqdiv__mask 0x06
#define ma_pmcfg_se_a__freqdiv__shift 0x01
#define ma_pmcfg_se_a__freqdiv__reset 0x00
//-----------------------------------------------------pmcfg_se_a.lf_gain_ol---
//
#define ma_pmcfg_se_a__lf_gain_ol__a 21
#define ma_pmcfg_se_a__lf_gain_ol__len 1
#define ma_pmcfg_se_a__lf_gain_ol__mask 0x01
#define ma_pmcfg_se_a__lf_gain_ol__shift 0x00
#define ma_pmcfg_se_a__lf_gain_ol__reset 0x01
//-----------------------------------------------------pmcfg_se_b.lf_gain_ol---
//
#define ma_pmcfg_se_b__lf_gain_ol__a 22
#define ma_pmcfg_se_b__lf_gain_ol__len 1
#define ma_pmcfg_se_b__lf_gain_ol__mask 0x01
#define ma_pmcfg_se_b__lf_gain_ol__shift 0x00
#define ma_pmcfg_se_b__lf_gain_ol__reset 0x00
//--------------------------------------------------------pmcfg_se_b.freqdiv---
//
#define ma_pmcfg_se_b__freqdiv__a 22
#define ma_pmcfg_se_b__freqdiv__len 2
#define ma_pmcfg_se_b__freqdiv__mask 0x06
#define ma_pmcfg_se_b__freqdiv__shift 0x01
#define ma_pmcfg_se_b__freqdiv__reset 0x01
//--------------------------------------------------------pmcfg_se_b.modtype---
//
#define ma_pmcfg_se_b__modtype__a 22
#define ma_pmcfg_se_b__modtype__len 2
#define ma_pmcfg_se_b__modtype__mask 0x18
#define ma_pmcfg_se_b__modtype__shift 0x03
#define ma_pmcfg_se_b__modtype__reset 0x01
//----------------------------------------------------------balwaitcount_pm1---
// pm1 balancing period.
#define ma_balwaitcount_pm1__a 23
#define ma_balwaitcount_pm1__len 8
#define ma_balwaitcount_pm1__mask 0xff
#define ma_balwaitcount_pm1__shift 0x00
#define ma_balwaitcount_pm1__reset 0x14
//----------------------------------------------------------balwaitcount_pm2---
// pm2 balancing period.
#define ma_balwaitcount_pm2__a 24
#define ma_balwaitcount_pm2__len 8
#define ma_balwaitcount_pm2__mask 0xff
#define ma_balwaitcount_pm2__shift 0x00
#define ma_balwaitcount_pm2__reset 0x14
//----------------------------------------------------------balwaitcount_pm3---
// pm3 balancing period.
#define ma_balwaitcount_pm3__a 25
#define ma_balwaitcount_pm3__len 8
#define ma_balwaitcount_pm3__mask 0xff
#define ma_balwaitcount_pm3__shift 0x00
#define ma_balwaitcount_pm3__reset 0x1a
//-------------------------------------------------------------usespread_pm1---
// pm1 pwm spread-spectrum mode on/off.
#define ma_usespread_pm1__a 26
#define ma_usespread_pm1__len 1
#define ma_usespread_pm1__mask 0x40
#define ma_usespread_pm1__shift 0x06
#define ma_usespread_pm1__reset 0x00
//---------------------------------------------------------------dtsteps_pm1---
// pm1 dead time setting [10ns steps].
#define ma_dtsteps_pm1__a 26
#define ma_dtsteps_pm1__len 3
#define ma_dtsteps_pm1__mask 0x38
#define ma_dtsteps_pm1__shift 0x03
#define ma_dtsteps_pm1__reset 0x04
//---------------------------------------------------------------baltype_pm1---
// pm1 balancing sensor scheme.
#define ma_baltype_pm1__a 26
#define ma_baltype_pm1__len 3
#define ma_baltype_pm1__mask 0x07
#define ma_baltype_pm1__shift 0x00
#define ma_baltype_pm1__reset 0x00
//-------------------------------------------------------------usespread_pm2---
// pm2 pwm spread-spectrum mode on/off.
#define ma_usespread_pm2__a 27
#define ma_usespread_pm2__len 1
#define ma_usespread_pm2__mask 0x40
#define ma_usespread_pm2__shift 0x06
#define ma_usespread_pm2__reset 0x00
//---------------------------------------------------------------dtsteps_pm2---
// pm2 dead time setting [10ns steps].
#define ma_dtsteps_pm2__a 27
#define ma_dtsteps_pm2__len 3
#define ma_dtsteps_pm2__mask 0x38
#define ma_dtsteps_pm2__shift 0x03
#define ma_dtsteps_pm2__reset 0x03
//---------------------------------------------------------------baltype_pm2---
// pm2 balancing sensor scheme.
#define ma_baltype_pm2__a 27
#define ma_baltype_pm2__len 3
#define ma_baltype_pm2__mask 0x07
#define ma_baltype_pm2__shift 0x00
#define ma_baltype_pm2__reset 0x01
//-------------------------------------------------------------usespread_pm3---
// pm3 pwm spread-spectrum mode on/off.
#define ma_usespread_pm3__a 28
#define ma_usespread_pm3__len 1
#define ma_usespread_pm3__mask 0x40
#define ma_usespread_pm3__shift 0x06
#define ma_usespread_pm3__reset 0x00
//---------------------------------------------------------------dtsteps_pm3---
// pm3 dead time setting [10ns steps].
#define ma_dtsteps_pm3__a 28
#define ma_dtsteps_pm3__len 3
#define ma_dtsteps_pm3__mask 0x38
#define ma_dtsteps_pm3__shift 0x03
#define ma_dtsteps_pm3__reset 0x01
//---------------------------------------------------------------baltype_pm3---
// pm3 balancing sensor scheme.
#define ma_baltype_pm3__a 28
#define ma_baltype_pm3__len 3
#define ma_baltype_pm3__mask 0x07
#define ma_baltype_pm3__shift 0x00
#define ma_baltype_pm3__reset 0x03
//-----------------------------------------------------------------pmprofile---
// pm profile select. valid presets: 0-1-2-3-4. 5=> custom profile.
#define ma_pmprofile__a 29
#define ma_pmprofile__len 3
#define ma_pmprofile__mask 0x07
#define ma_pmprofile__shift 0x00
#define ma_pmprofile__reset 0x00
//-------------------------------------------------------------------pm3_man---
// custom profile pm3 contents. 0=>a,  1=>b,  2=>c,  3=>d
#define ma_pm3_man__a 30
#define ma_pm3_man__len 2
#define ma_pm3_man__mask 0x30
#define ma_pm3_man__shift 0x04
#define ma_pm3_man__reset 0x02
//-------------------------------------------------------------------pm2_man---
// custom profile pm2 contents. 0=>a,  1=>b,  2=>c,  3=>d
#define ma_pm2_man__a 30
#define ma_pm2_man__len 2
#define ma_pm2_man__mask 0x0c
#define ma_pm2_man__shift 0x02
#define ma_pm2_man__reset 0x03
//-------------------------------------------------------------------pm1_man---
// custom profile pm1 contents. 0=>a,  1=>b,  2=>c,  3=>d
#define ma_pm1_man__a 30
#define ma_pm1_man__len 2
#define ma_pm1_man__mask 0x03
#define ma_pm1_man__shift 0x00
#define ma_pm1_man__reset 0x03
//-----------------------------------------------------------ocp_latch_clear---
// low-high clears current ocp latched condition.
#define ma_ocp_latch_clear__a 32
#define ma_ocp_latch_clear__len 1
#define ma_ocp_latch_clear__mask 0x80
#define ma_ocp_latch_clear__shift 0x07
#define ma_ocp_latch_clear__reset 0x00
//-------------------------------------------------------------audio_in_mode---
// audio input mode; 0-1-2-3-4-5
#define ma_audio_in_mode__a 37
#define ma_audio_in_mode__len 3
#define ma_audio_in_mode__mask 0xe0
#define ma_audio_in_mode__shift 0x05
#define ma_audio_in_mode__reset 0x00
//-----------------------------------------------------------------eh_dcshdn---
// high to enable dc protection
#define ma_eh_dcshdn__a 38
#define ma_eh_dcshdn__len 1
#define ma_eh_dcshdn__mask 0x04
#define ma_eh_dcshdn__shift 0x02
#define ma_eh_dcshdn__reset 0x01
//---------------------------------------------------------audio_in_mode_ext---
// if set,  audio_in_mode is controlled from audio_in_mode register. if not set
//audio_in_mode is set from fuse bank setting
#define ma_audio_in_mode_ext__a 39
#define ma_audio_in_mode_ext__len 1
#define ma_audio_in_mode_ext__mask 0x20
#define ma_audio_in_mode_ext__shift 0x05
#define ma_audio_in_mode_ext__reset 0x00
//------------------------------------------------------------------eh_clear---
// flip to clear error registers
#define ma_eh_clear__a 45
#define ma_eh_clear__len 1
#define ma_eh_clear__mask 0x04
#define ma_eh_clear__shift 0x02
#define ma_eh_clear__reset 0x00
//----------------------------------------------------------thermal_compr_en---
// enable otw-contr.  input compression?
#define ma_thermal_compr_en__a 45
#define ma_thermal_compr_en__len 1
#define ma_thermal_compr_en__mask 0x20
#define ma_thermal_compr_en__shift 0x05
#define ma_thermal_compr_en__reset 0x01
//---------------------------------------------------------------system_mute---
// 1 = mute system,  0 = normal operation
#define ma_system_mute__a 45
#define ma_system_mute__len 1
#define ma_system_mute__mask 0x40
#define ma_system_mute__shift 0x06
#define ma_system_mute__reset 0x00
//------------------------------------------------------thermal_compr_max_db---
// audio limiter max thermal reduction
#define ma_thermal_compr_max_db__a 46
#define ma_thermal_compr_max_db__len 3
#define ma_thermal_compr_max_db__mask 0x07
#define ma_thermal_compr_max_db__shift 0x00
#define ma_thermal_compr_max_db__reset 0x04
//---------------------------------------------------------audio_proc_enable---
// enable audio proc,  bypass if not enabled
#define ma_audio_proc_enable__a 53
#define ma_audio_proc_enable__len 1
#define ma_audio_proc_enable__mask 0x08
#define ma_audio_proc_enable__shift 0x03
#define ma_audio_proc_enable__reset 0x00
//--------------------------------------------------------audio_proc_release---
// 00:slow,  01:normal,  10:fast
#define ma_audio_proc_release__a 53
#define ma_audio_proc_release__len 2
#define ma_audio_proc_release__mask 0x30
#define ma_audio_proc_release__shift 0x04
#define ma_audio_proc_release__reset 0x00
//---------------------------------------------------------audio_proc_attack---
// 00:slow,  01:normal,  10:fast
#define ma_audio_proc_attack__a 53
#define ma_audio_proc_attack__len 2
#define ma_audio_proc_attack__mask 0xc0
#define ma_audio_proc_attack__shift 0x06
#define ma_audio_proc_attack__reset 0x00
//----------------------------------------------------------------i2s_format---
// i2s basic data format,  000 = std. i2s,  001 = left justified (default)
#define ma_i2s_format__a 53
#define ma_i2s_format__len 3
#define ma_i2s_format__mask 0x07
#define ma_i2s_format__shift 0x00
#define ma_i2s_format__reset 0x01
//--------------------------------------------------audio_proc_limiterenable---
// 1: enable limiter,  0: disable limiter
#define ma_audio_proc_limiterenable__a 54
#define ma_audio_proc_limiterenable__len 1
#define ma_audio_proc_limiterenable__mask 0x40
#define ma_audio_proc_limiterenable__shift 0x06
#define ma_audio_proc_limiterenable__reset 0x00
//-----------------------------------------------------------audio_proc_mute---
// 1: mute,  0: unmute
#define ma_audio_proc_mute__a 54
#define ma_audio_proc_mute__len 1
#define ma_audio_proc_mute__mask 0x80
#define ma_audio_proc_mute__shift 0x07
#define ma_audio_proc_mute__reset 0x00
//---------------------------------------------------------------i2s_sck_pol---
// i2s sck polarity cfg. 0 = rising edge data change
#define ma_i2s_sck_pol__a 54
#define ma_i2s_sck_pol__len 1
#define ma_i2s_sck_pol__mask 0x01
#define ma_i2s_sck_pol__shift 0x00
#define ma_i2s_sck_pol__reset 0x01
//-------------------------------------------------------------i2s_framesize---
// i2s word length. 00 = 32bit,  01 = 24bit
#define ma_i2s_framesize__a 54
#define ma_i2s_framesize__len 2
#define ma_i2s_framesize__mask 0x18
#define ma_i2s_framesize__shift 0x03
#define ma_i2s_framesize__reset 0x00
//----------------------------------------------------------------i2s_ws_pol---
// i2s ws polarity. 0 = low first
#define ma_i2s_ws_pol__a 54
#define ma_i2s_ws_pol__len 1
#define ma_i2s_ws_pol__mask 0x02
#define ma_i2s_ws_pol__shift 0x01
#define ma_i2s_ws_pol__reset 0x00
//-----------------------------------------------------------------i2s_order---
// i2s word bit order. 0 = msb first
#define ma_i2s_order__a 54
#define ma_i2s_order__len 1
#define ma_i2s_order__mask 0x04
#define ma_i2s_order__shift 0x02
#define ma_i2s_order__reset 0x00
//------------------------------------------------------------i2s_rightfirst---
// i2s l/r word order; 0 = left first
#define ma_i2s_rightfirst__a 54
#define ma_i2s_rightfirst__len 1
#define ma_i2s_rightfirst__mask 0x20
#define ma_i2s_rightfirst__shift 0x05
#define ma_i2s_rightfirst__reset 0x00
//-------------------------------------------------------------vol_db_master---
// master volume db
#define ma_vol_db_master__a 64
#define ma_vol_db_master__len 8
#define ma_vol_db_master__mask 0xff
#define ma_vol_db_master__shift 0x00
#define ma_vol_db_master__reset 0x18
//------------------------------------------------------------vol_lsb_master---
// master volume lsb 1/4 steps
#define ma_vol_lsb_master__a 65
#define ma_vol_lsb_master__len 2
#define ma_vol_lsb_master__mask 0x03
#define ma_vol_lsb_master__shift 0x00
#define ma_vol_lsb_master__reset 0x00
//----------------------------------------------------------------vol_db_ch0---
// volume channel 0
#define ma_vol_db_ch0__a 66
#define ma_vol_db_ch0__len 8
#define ma_vol_db_ch0__mask 0xff
#define ma_vol_db_ch0__shift 0x00
#define ma_vol_db_ch0__reset 0x18
//----------------------------------------------------------------vol_db_ch1---
// volume channel 1
#define ma_vol_db_ch1__a 67
#define ma_vol_db_ch1__len 8
#define ma_vol_db_ch1__mask 0xff
#define ma_vol_db_ch1__shift 0x00
#define ma_vol_db_ch1__reset 0x18
//----------------------------------------------------------------vol_db_ch2---
// volume channel 2
#define ma_vol_db_ch2__a 68
#define ma_vol_db_ch2__len 8
#define ma_vol_db_ch2__mask 0xff
#define ma_vol_db_ch2__shift 0x00
#define ma_vol_db_ch2__reset 0x18
//----------------------------------------------------------------vol_db_ch3---
// volume channel 3
#define ma_vol_db_ch3__a 69
#define ma_vol_db_ch3__len 8
#define ma_vol_db_ch3__mask 0xff
#define ma_vol_db_ch3__shift 0x00
#define ma_vol_db_ch3__reset 0x18
//---------------------------------------------------------------vol_lsb_ch0---
// volume channel 1 - 1/4 steps
#define ma_vol_lsb_ch0__a 70
#define ma_vol_lsb_ch0__len 2
#define ma_vol_lsb_ch0__mask 0x03
#define ma_vol_lsb_ch0__shift 0x00
#define ma_vol_lsb_ch0__reset 0x00
//---------------------------------------------------------------vol_lsb_ch1---
// volume channel 3 - 1/4 steps
#define ma_vol_lsb_ch1__a 70
#define ma_vol_lsb_ch1__len 2
#define ma_vol_lsb_ch1__mask 0x0c
#define ma_vol_lsb_ch1__shift 0x02
#define ma_vol_lsb_ch1__reset 0x00
//---------------------------------------------------------------vol_lsb_ch2---
// volume channel 2 - 1/4 steps
#define ma_vol_lsb_ch2__a 70
#define ma_vol_lsb_ch2__len 2
#define ma_vol_lsb_ch2__mask 0x30
#define ma_vol_lsb_ch2__shift 0x04
#define ma_vol_lsb_ch2__reset 0x00
//---------------------------------------------------------------vol_lsb_ch3---
// volume channel 3 - 1/4 steps
#define ma_vol_lsb_ch3__a 70
#define ma_vol_lsb_ch3__len 2
#define ma_vol_lsb_ch3__mask 0xc0
#define ma_vol_lsb_ch3__shift 0x06
#define ma_vol_lsb_ch3__reset 0x00
//----------------------------------------------------------------thr_db_ch0---
// thr_db channel 0
#define ma_thr_db_ch0__a 71
#define ma_thr_db_ch0__len 8
#define ma_thr_db_ch0__mask 0xff
#define ma_thr_db_ch0__shift 0x00
#define ma_thr_db_ch0__reset 0x18
//----------------------------------------------------------------thr_db_ch1---
// thr db ch1
#define ma_thr_db_ch1__a 72
#define ma_thr_db_ch1__len 8
#define ma_thr_db_ch1__mask 0xff
#define ma_thr_db_ch1__shift 0x00
#define ma_thr_db_ch1__reset 0x18
//----------------------------------------------------------------thr_db_ch2---
// thr db ch2
#define ma_thr_db_ch2__a 73
#define ma_thr_db_ch2__len 8
#define ma_thr_db_ch2__mask 0xff
#define ma_thr_db_ch2__shift 0x00
#define ma_thr_db_ch2__reset 0x18
//----------------------------------------------------------------thr_db_ch3---
// threshold db ch3
#define ma_thr_db_ch3__a 74
#define ma_thr_db_ch3__len 8
#define ma_thr_db_ch3__mask 0xff
#define ma_thr_db_ch3__shift 0x00
#define ma_thr_db_ch3__reset 0x18
//---------------------------------------------------------------thr_lsb_ch0---
// thr lsb ch0
#define ma_thr_lsb_ch0__a 75
#define ma_thr_lsb_ch0__len 2
#define ma_thr_lsb_ch0__mask 0x03
#define ma_thr_lsb_ch0__shift 0x00
#define ma_thr_lsb_ch0__reset 0x00
//---------------------------------------------------------------thr_lsb_ch1---
// thr lsb ch1
#define ma_thr_lsb_ch1__a 75
#define ma_thr_lsb_ch1__len 2
#define ma_thr_lsb_ch1__mask 0x0c
#define ma_thr_lsb_ch1__shift 0x02
#define ma_thr_lsb_ch1__reset 0x00
//---------------------------------------------------------------thr_lsb_ch2---
// thr lsb ch2 1/4 db step
#define ma_thr_lsb_ch2__a 75
#define ma_thr_lsb_ch2__len 2
#define ma_thr_lsb_ch2__mask 0x30
#define ma_thr_lsb_ch2__shift 0x04
#define ma_thr_lsb_ch2__reset 0x00
//---------------------------------------------------------------thr_lsb_ch3---
// threshold lsb ch3
#define ma_thr_lsb_ch3__a 75
#define ma_thr_lsb_ch3__len 2
#define ma_thr_lsb_ch3__mask 0xc0
#define ma_thr_lsb_ch3__shift 0x06
#define ma_thr_lsb_ch3__reset 0x00
//-----------------------------------------------------------dcu_mon0.pm_mon---
// power mode monitor channel 0
#define ma_dcu_mon0__pm_mon__a 96
#define ma_dcu_mon0__pm_mon__len 2
#define ma_dcu_mon0__pm_mon__mask 0x03
#define ma_dcu_mon0__pm_mon__shift 0x00
#define ma_dcu_mon0__pm_mon__reset 0x00
//-----------------------------------------------------dcu_mon0.freqmode_mon---
// frequence mode monitor channel 0
#define ma_dcu_mon0__freqmode_mon__a 96
#define ma_dcu_mon0__freqmode_mon__len 3
#define ma_dcu_mon0__freqmode_mon__mask 0x70
#define ma_dcu_mon0__freqmode_mon__shift 0x04
#define ma_dcu_mon0__freqmode_mon__reset 0x00
//-------------------------------------------------------dcu_mon0.pps_passed---
// dcu0 pps completion indicator
#define ma_dcu_mon0__pps_passed__a 96
#define ma_dcu_mon0__pps_passed__len 1
#define ma_dcu_mon0__pps_passed__mask 0x80
#define ma_dcu_mon0__pps_passed__shift 0x07
#define ma_dcu_mon0__pps_passed__reset 0x00
//----------------------------------------------------------dcu_mon0.ocp_mon---
// ocp monitor channel 0
#define ma_dcu_mon0__ocp_mon__a 97
#define ma_dcu_mon0__ocp_mon__len 1
#define ma_dcu_mon0__ocp_mon__mask 0x01
#define ma_dcu_mon0__ocp_mon__shift 0x00
#define ma_dcu_mon0__ocp_mon__reset 0x00
//--------------------------------------------------------dcu_mon0.vcfly1_ok---
// cfly1 protection monitor channel 0.
#define ma_dcu_mon0__vcfly1_ok__a 97
#define ma_dcu_mon0__vcfly1_ok__len 1
#define ma_dcu_mon0__vcfly1_ok__mask 0x02
#define ma_dcu_mon0__vcfly1_ok__shift 0x01
#define ma_dcu_mon0__vcfly1_ok__reset 0x00
//--------------------------------------------------------dcu_mon0.vcfly2_ok---
// cfly2 protection monitor channel 0.
#define ma_dcu_mon0__vcfly2_ok__a 97
#define ma_dcu_mon0__vcfly2_ok__len 1
#define ma_dcu_mon0__vcfly2_ok__mask 0x04
#define ma_dcu_mon0__vcfly2_ok__shift 0x02
#define ma_dcu_mon0__vcfly2_ok__reset 0x00
//----------------------------------------------------------dcu_mon0.pvdd_ok---
// dcu0 pvdd monitor
#define ma_dcu_mon0__pvdd_ok__a 97
#define ma_dcu_mon0__pvdd_ok__len 1
#define ma_dcu_mon0__pvdd_ok__mask 0x08
#define ma_dcu_mon0__pvdd_ok__shift 0x03
#define ma_dcu_mon0__pvdd_ok__reset 0x00
//-----------------------------------------------------------dcu_mon0.vdd_ok---
// dcu0 vdd monitor
#define ma_dcu_mon0__vdd_ok__a 97
#define ma_dcu_mon0__vdd_ok__len 1
#define ma_dcu_mon0__vdd_ok__mask 0x10
#define ma_dcu_mon0__vdd_ok__shift 0x04
#define ma_dcu_mon0__vdd_ok__reset 0x00
//-------------------------------------------------------------dcu_mon0.mute---
// dcu0 mute monitor
#define ma_dcu_mon0__mute__a 97
#define ma_dcu_mon0__mute__len 1
#define ma_dcu_mon0__mute__mask 0x20
#define ma_dcu_mon0__mute__shift 0x05
#define ma_dcu_mon0__mute__reset 0x00
//------------------------------------------------------------dcu_mon0.m_mon---
// m sense monitor channel 0
#define ma_dcu_mon0__m_mon__a 98
#define ma_dcu_mon0__m_mon__len 8
#define ma_dcu_mon0__m_mon__mask 0xff
#define ma_dcu_mon0__m_mon__shift 0x00
#define ma_dcu_mon0__m_mon__reset 0x00
//-----------------------------------------------------------dcu_mon1.pm_mon---
// power mode monitor channel 1
#define ma_dcu_mon1__pm_mon__a 100
#define ma_dcu_mon1__pm_mon__len 2
#define ma_dcu_mon1__pm_mon__mask 0x03
#define ma_dcu_mon1__pm_mon__shift 0x00
#define ma_dcu_mon1__pm_mon__reset 0x00
//-----------------------------------------------------dcu_mon1.freqmode_mon---
// frequence mode monitor channel 1
#define ma_dcu_mon1__freqmode_mon__a 100
#define ma_dcu_mon1__freqmode_mon__len 3
#define ma_dcu_mon1__freqmode_mon__mask 0x70
#define ma_dcu_mon1__freqmode_mon__shift 0x04
#define ma_dcu_mon1__freqmode_mon__reset 0x00
//-------------------------------------------------------dcu_mon1.pps_passed---
// dcu1 pps completion indicator
#define ma_dcu_mon1__pps_passed__a 100
#define ma_dcu_mon1__pps_passed__len 1
#define ma_dcu_mon1__pps_passed__mask 0x80
#define ma_dcu_mon1__pps_passed__shift 0x07
#define ma_dcu_mon1__pps_passed__reset 0x00
//----------------------------------------------------------dcu_mon1.ocp_mon---
// ocp monitor channel 1
#define ma_dcu_mon1__ocp_mon__a 101
#define ma_dcu_mon1__ocp_mon__len 1
#define ma_dcu_mon1__ocp_mon__mask 0x01
#define ma_dcu_mon1__ocp_mon__shift 0x00
#define ma_dcu_mon1__ocp_mon__reset 0x00
//--------------------------------------------------------dcu_mon1.vcfly1_ok---
// cfly1 protcetion monitor channel 1
#define ma_dcu_mon1__vcfly1_ok__a 101
#define ma_dcu_mon1__vcfly1_ok__len 1
#define ma_dcu_mon1__vcfly1_ok__mask 0x02
#define ma_dcu_mon1__vcfly1_ok__shift 0x01
#define ma_dcu_mon1__vcfly1_ok__reset 0x00
//--------------------------------------------------------dcu_mon1.vcfly2_ok---
// cfly2 protection monitor channel 1
#define ma_dcu_mon1__vcfly2_ok__a 101
#define ma_dcu_mon1__vcfly2_ok__len 1
#define ma_dcu_mon1__vcfly2_ok__mask 0x04
#define ma_dcu_mon1__vcfly2_ok__shift 0x02
#define ma_dcu_mon1__vcfly2_ok__reset 0x00
//----------------------------------------------------------dcu_mon1.pvdd_ok---
// dcu1 pvdd monitor
#define ma_dcu_mon1__pvdd_ok__a 101
#define ma_dcu_mon1__pvdd_ok__len 1
#define ma_dcu_mon1__pvdd_ok__mask 0x08
#define ma_dcu_mon1__pvdd_ok__shift 0x03
#define ma_dcu_mon1__pvdd_ok__reset 0x00
//-----------------------------------------------------------dcu_mon1.vdd_ok---
// dcu1 vdd monitor
#define ma_dcu_mon1__vdd_ok__a 101
#define ma_dcu_mon1__vdd_ok__len 1
#define ma_dcu_mon1__vdd_ok__mask 0x10
#define ma_dcu_mon1__vdd_ok__shift 0x04
#define ma_dcu_mon1__vdd_ok__reset 0x00
//-------------------------------------------------------------dcu_mon1.mute---
// dcu1 mute monitor
#define ma_dcu_mon1__mute__a 101
#define ma_dcu_mon1__mute__len 1
#define ma_dcu_mon1__mute__mask 0x20
#define ma_dcu_mon1__mute__shift 0x05
#define ma_dcu_mon1__mute__reset 0x00
//------------------------------------------------------------dcu_mon1.m_mon---
// m sense monitor channel 1
#define ma_dcu_mon1__m_mon__a 102
#define ma_dcu_mon1__m_mon__len 8
#define ma_dcu_mon1__m_mon__mask 0xff
#define ma_dcu_mon1__m_mon__shift 0x00
#define ma_dcu_mon1__m_mon__reset 0x00
//--------------------------------------------------------dcu_mon0.sw_enable---
// dcu0 switch enable monitor
#define ma_dcu_mon0__sw_enable__a 104
#define ma_dcu_mon0__sw_enable__len 1
#define ma_dcu_mon0__sw_enable__mask 0x40
#define ma_dcu_mon0__sw_enable__shift 0x06
#define ma_dcu_mon0__sw_enable__reset 0x00
//--------------------------------------------------------dcu_mon1.sw_enable---
// dcu1 switch enable monitor
#define ma_dcu_mon1__sw_enable__a 104
#define ma_dcu_mon1__sw_enable__len 1
#define ma_dcu_mon1__sw_enable__mask 0x80
#define ma_dcu_mon1__sw_enable__shift 0x07
#define ma_dcu_mon1__sw_enable__reset 0x00
//------------------------------------------------------------hvboot0_ok_mon---
// hvboot0_ok for test/debug
#define ma_hvboot0_ok_mon__a 105
#define ma_hvboot0_ok_mon__len 1
#define ma_hvboot0_ok_mon__mask 0x40
#define ma_hvboot0_ok_mon__shift 0x06
#define ma_hvboot0_ok_mon__reset 0x00
//------------------------------------------------------------hvboot1_ok_mon---
// hvboot1_ok for test/debug
#define ma_hvboot1_ok_mon__a 105
#define ma_hvboot1_ok_mon__len 1
#define ma_hvboot1_ok_mon__mask 0x80
#define ma_hvboot1_ok_mon__shift 0x07
#define ma_hvboot1_ok_mon__reset 0x00
//-----------------------------------------------------------------error_acc---
// accumulated errors,  at and after triggering
#define ma_error_acc__a 109
#define ma_error_acc__len 8
#define ma_error_acc__mask 0xff
#define ma_error_acc__shift 0x00
#define ma_error_acc__reset 0x00
//-------------------------------------------------------------i2s_data_rate---
// detected i2s data rate: 00/01/10 = x1/x2/x4
#define ma_i2s_data_rate__a 116
#define ma_i2s_data_rate__len 2
#define ma_i2s_data_rate__mask 0x03
#define ma_i2s_data_rate__shift 0x00
#define ma_i2s_data_rate__reset 0x00
//---------------------------------------------------------audio_in_mode_mon---
// audio input mode monitor
#define ma_audio_in_mode_mon__a 116
#define ma_audio_in_mode_mon__len 3
#define ma_audio_in_mode_mon__mask 0x1c
#define ma_audio_in_mode_mon__shift 0x02
#define ma_audio_in_mode_mon__reset 0x00
//------------------------------------------------------------------msel_mon---
// msel[2:0] monitor register
#define ma_msel_mon__a 117
#define ma_msel_mon__len 3
#define ma_msel_mon__mask 0x07
#define ma_msel_mon__shift 0x00
#define ma_msel_mon__reset 0x00
//---------------------------------------------------------------------error---
// current error flag monitor reg - for app. ctrl.
#define ma_error__a 124
#define ma_error__len 8
#define ma_error__mask 0xff
#define ma_error__shift 0x00
#define ma_error__reset 0x00
//----------------------------------------------------audio_proc_limiter_mon---
// b7-b4: channel 3-0 limiter active
#define ma_audio_proc_limiter_mon__a 126
#define ma_audio_proc_limiter_mon__len 4
#define ma_audio_proc_limiter_mon__mask 0xf0
#define ma_audio_proc_limiter_mon__shift 0x04
#define ma_audio_proc_limiter_mon__reset 0x00
//-------------------------------------------------------audio_proc_clip_mon---
// b3-b0: channel 3-0 clipping monitor
#define ma_audio_proc_clip_mon__a 126
#define ma_audio_proc_clip_mon__len 4
#define ma_audio_proc_clip_mon__mask 0x0f
#define ma_audio_proc_clip_mon__shift 0x00
#define ma_audio_proc_clip_mon__reset 0x00
#endif

#define SOC_ENUM_ERR(xname, xenum)\
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname),\
	.access = SNDRV_CTL_ELEM_ACCESS_READ,\
	.info = snd_soc_info_enum_double,\
	.get = snd_soc_get_enum_double, .put = snd_soc_put_enum_double,\
	.private_value = (unsigned long)&(xenum) }

static struct i2c_client *i2c;

struct ma120x0p_priv {
	struct regmap *regmap;
	int mclk_div;
	struct snd_soc_component *component;
	struct gpio_desc *enable_gpio;
	struct gpio_desc *mute_gpio;
	struct gpio_desc *booster_gpio;
	struct gpio_desc *error_gpio;
};

static struct ma120x0p_priv *priv_data;

//Used to share the IRQ number within this file
static unsigned int irqNumber;

// Function prototype for the custom IRQ handler function
static irqreturn_t ma120x0p_irq_handler(int irq, void *data);

//Alsa Controls
static const char * const limenable_text[] = {"Bypassed", "Enabled"};
static const char * const limatack_text[] = {"Slow", "Normal", "Fast"};
static const char * const limrelease_text[] = {"Slow", "Normal", "Fast"};

static const char * const err_flycap_text[] = {"Ok", "Error"};
static const char * const err_overcurr_text[] = {"Ok", "Error"};
static const char * const err_pllerr_text[] = {"Ok", "Error"};
static const char * const err_pvddunder_text[] = {"Ok", "Error"};
static const char * const err_overtempw_text[] = {"Ok", "Error"};
static const char * const err_overtempe_text[] = {"Ok", "Error"};
static const char * const err_pinlowimp_text[] = {"Ok", "Error"};
static const char * const err_dcprot_text[] = {"Ok", "Error"};

static const char * const pwr_mode_prof_text[] = {"PMF0", "PMF1", "PMF2",
"PMF3", "PMF4"};

static const struct soc_enum lim_enable_ctrl =
	SOC_ENUM_SINGLE(ma_audio_proc_limiterenable__a,
		ma_audio_proc_limiterenable__shift,
		ma_audio_proc_limiterenable__len + 1,
		limenable_text);
static const struct soc_enum limatack_ctrl =
	SOC_ENUM_SINGLE(ma_audio_proc_attack__a,
		ma_audio_proc_attack__shift,
		ma_audio_proc_attack__len + 1,
		limatack_text);
static const struct soc_enum limrelease_ctrl =
	SOC_ENUM_SINGLE(ma_audio_proc_release__a,
		ma_audio_proc_release__shift,
		ma_audio_proc_release__len + 1,
		limrelease_text);
static const struct soc_enum err_flycap_ctrl =
	SOC_ENUM_SINGLE(ma_error__a, 0, 3, err_flycap_text);
static const struct soc_enum err_overcurr_ctrl =
	SOC_ENUM_SINGLE(ma_error__a, 1, 3, err_overcurr_text);
static const struct soc_enum err_pllerr_ctrl =
	SOC_ENUM_SINGLE(ma_error__a, 2, 3, err_pllerr_text);
static const struct soc_enum err_pvddunder_ctrl =
	SOC_ENUM_SINGLE(ma_error__a, 3, 3, err_pvddunder_text);
static const struct soc_enum err_overtempw_ctrl =
	SOC_ENUM_SINGLE(ma_error__a, 4, 3, err_overtempw_text);
static const struct soc_enum err_overtempe_ctrl =
	SOC_ENUM_SINGLE(ma_error__a, 5, 3, err_overtempe_text);
static const struct soc_enum err_pinlowimp_ctrl =
	SOC_ENUM_SINGLE(ma_error__a, 6, 3, err_pinlowimp_text);
static const struct soc_enum err_dcprot_ctrl =
	SOC_ENUM_SINGLE(ma_error__a, 7, 3, err_dcprot_text);
static const struct soc_enum pwr_mode_prof_ctrl =
	SOC_ENUM_SINGLE(ma_pmprofile__a, ma_pmprofile__shift, 5,
		pwr_mode_prof_text);

static const char * const pwr_mode_texts[] = {
		"Dynamic power mode",
		"Power mode 1",
		"Power mode 2",
		"Power mode 3",
	};

static const int pwr_mode_values[] = {
		0x10,
		0x50,
		0x60,
		0x70,
	};

static SOC_VALUE_ENUM_SINGLE_DECL(pwr_mode_ctrl,
	ma_pm_man__a, 0, 0x70,
	pwr_mode_texts,
	pwr_mode_values);

static const DECLARE_TLV_DB_SCALE(ma120x0p_vol_tlv, -5000, 100,  0);
static const DECLARE_TLV_DB_SCALE(ma120x0p_lim_tlv, -5000, 100,  0);
static const DECLARE_TLV_DB_SCALE(ma120x0p_lr_tlv, -5000, 100,  0);

static const struct snd_kcontrol_new ma120x0p_snd_controls[] = {
	//Master Volume
	SOC_SINGLE_RANGE_TLV("A.Mstr Vol Volume",
		ma_vol_db_master__a, 0, 0x18, 0x4a, 1, ma120x0p_vol_tlv),

	//L-R Volume ch0
	SOC_SINGLE_RANGE_TLV("B.L Vol Volume",
		ma_vol_db_ch0__a, 0, 0x18, 0x4a, 1, ma120x0p_lr_tlv),
	SOC_SINGLE_RANGE_TLV("C.R Vol Volume",
		ma_vol_db_ch1__a, 0, 0x18, 0x4a, 1, ma120x0p_lr_tlv),

	//L-R Limiter Threshold ch0-ch1
	SOC_DOUBLE_R_RANGE_TLV("D.Lim thresh Volume",
		ma_thr_db_ch0__a, ma_thr_db_ch1__a, 0, 0x0e, 0x4a, 1,
		ma120x0p_lim_tlv),

	//Enum Switches/Selectors
	//SOC_ENUM("E.AudioProc Mute", audioproc_mute_ctrl),
	SOC_ENUM("F.Limiter Enable", lim_enable_ctrl),
	SOC_ENUM("G.Limiter Attck", limatack_ctrl),
	SOC_ENUM("H.Limiter Rls", limrelease_ctrl),

	//Enum Error Monitor (read-only)
	SOC_ENUM_ERR("I.Err flycap", err_flycap_ctrl),
	SOC_ENUM_ERR("J.Err overcurr", err_overcurr_ctrl),
	SOC_ENUM_ERR("K.Err pllerr", err_pllerr_ctrl),
	SOC_ENUM_ERR("L.Err pvddunder", err_pvddunder_ctrl),
	SOC_ENUM_ERR("M.Err overtempw", err_overtempw_ctrl),
	SOC_ENUM_ERR("N.Err overtempe", err_overtempe_ctrl),
	SOC_ENUM_ERR("O.Err pinlowimp", err_pinlowimp_ctrl),
	SOC_ENUM_ERR("P.Err dcprot", err_dcprot_ctrl),

	//Power modes profiles
	SOC_ENUM("Q.PM Prof", pwr_mode_prof_ctrl),

	// Power mode selection (Dynamic,1,2,3)
	SOC_ENUM("R.Power Mode", pwr_mode_ctrl),
};

//Machine Driver
static int ma120x0p_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	u16 blen = 0x00;

	struct snd_soc_component *component = dai->component;

	priv_data->component = component;

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		blen = 0x10;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		blen = 0x00;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		blen = 0x00;
		break;
	default:
		dev_err(dai->dev, "Unsupported word length: %u\n",
		params_format(params));
		return -EINVAL;
	}

	// set word length
	snd_soc_component_update_bits(component, ma_i2s_framesize__a,
		ma_i2s_framesize__mask, blen);

	return 0;
}

static int ma120x0p_mute_stream(struct snd_soc_dai *dai, int mute, int stream)
{
	int val = 0;

	struct ma120x0p_priv *ma120x0p;

	struct snd_soc_component *component = dai->component;

	ma120x0p = snd_soc_component_get_drvdata(component);

	if (mute)
		val = 0;
	else
		val = 1;

	gpiod_set_value_cansleep(priv_data->mute_gpio, val);

	return 0;
}

static const struct snd_soc_dai_ops ma120x0p_dai_ops = {
	.hw_params		=	ma120x0p_hw_params,
	.mute_stream	=	ma120x0p_mute_stream,
};

static struct snd_soc_dai_driver ma120x0p_dai = {
	.name		= "ma120x0p-amp",
	.playback	=	{
		.stream_name	= "Playback",
		.channels_min	= 2,
		.channels_max	= 2,
		.rates = SNDRV_PCM_RATE_CONTINUOUS,
		.rate_min = 44100,
		.rate_max = 192000,
		.formats = SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE
	},
	.ops        = &ma120x0p_dai_ops,
};

//Codec Driver
static int ma120x0p_clear_err(struct snd_soc_component *component)
{
	int ret = 0;

	struct ma120x0p_priv *ma120x0p;

	ma120x0p = snd_soc_component_get_drvdata(component);

	ret = snd_soc_component_update_bits(component,
		ma_eh_clear__a, ma_eh_clear__mask, 0x00);
	if (ret < 0)
		return ret;

	ret = snd_soc_component_update_bits(component,
		ma_eh_clear__a, ma_eh_clear__mask, 0x04);
	if (ret < 0)
		return ret;

	ret = snd_soc_component_update_bits(component,
		ma_eh_clear__a, ma_eh_clear__mask, 0x00);
	if (ret < 0)
		return ret;

	return 0;
}

static void ma120x0p_remove(struct snd_soc_component *component)
{
	struct ma120x0p_priv *ma120x0p;

	ma120x0p = snd_soc_component_get_drvdata(component);
}

static int ma120x0p_probe(struct snd_soc_component *component)
{
	struct ma120x0p_priv *ma120x0p;

	int ret = 0;

	i2c = container_of(component->dev, struct i2c_client, dev);

	ma120x0p = snd_soc_component_get_drvdata(component);

	//Reset error
	ma120x0p_clear_err(component);
	if (ret < 0)
		return ret;

	// set serial audio format I2S and enable audio processor
	ret = snd_soc_component_write(component, ma_i2s_format__a, 0x08);
	if (ret < 0)
		return ret;

	// Enable audio limiter
	ret = snd_soc_component_update_bits(component,
		ma_audio_proc_limiterenable__a,
		ma_audio_proc_limiterenable__mask, 0x40);
	if (ret < 0)
		return ret;

	// Set lim attack to fast
	ret = snd_soc_component_update_bits(component,
		ma_audio_proc_attack__a, ma_audio_proc_attack__mask, 0x80);
	if (ret < 0)
		return ret;

	// Set lim attack to low
	ret = snd_soc_component_update_bits(component,
		ma_audio_proc_release__a, ma_audio_proc_release__mask, 0x00);
	if (ret < 0)
		return ret;

	// set volume to 0dB
	ret = snd_soc_component_write(component, ma_vol_db_master__a, 0x18);
	if (ret < 0)
		return ret;

	// set ch0 lim thresh to -15dB
	ret = snd_soc_component_write(component, ma_thr_db_ch0__a, 0x27);
	if (ret < 0)
		return ret;

	// set ch1 lim thresh to -15dB
	ret = snd_soc_component_write(component, ma_thr_db_ch1__a, 0x27);
	if (ret < 0)
		return ret;

	//Check for errors
	ret = snd_soc_component_test_bits(component, ma_error_acc__a, 0x00, 0);
	if (ret < 0)
		return ret;
	ret = snd_soc_component_test_bits(component, ma_error_acc__a, 0x01, 0);
	if (ret < 0)
		return ret;
	ret = snd_soc_component_test_bits(component, ma_error_acc__a, 0x02, 0);
	if (ret < 0)
		return ret;
	ret = snd_soc_component_test_bits(component, ma_error_acc__a, 0x08, 0);
	if (ret < 0)
		return ret;
	ret = snd_soc_component_test_bits(component, ma_error_acc__a, 0x10, 0);
	if (ret < 0)
		return ret;
	ret = snd_soc_component_test_bits(component, ma_error_acc__a, 0x20, 0);
	if (ret < 0)
		return ret;
	ret = snd_soc_component_test_bits(component, ma_error_acc__a, 0x40, 0);
	if (ret < 0)
		return ret;
	ret = snd_soc_component_test_bits(component, ma_error_acc__a, 0x80, 0);
	if (ret < 0)
		return ret;

	return 0;
}

static int ma120x0p_set_bias_level(struct snd_soc_component *component,
	enum snd_soc_bias_level level)
{
	int ret = 0;

	struct ma120x0p_priv *ma120x0p;

	ma120x0p = snd_soc_component_get_drvdata(component);

	switch (level) {
	case SND_SOC_BIAS_ON:
		break;

	case SND_SOC_BIAS_PREPARE:
		break;

	case SND_SOC_BIAS_STANDBY:
		ret = gpiod_get_value_cansleep(priv_data->enable_gpio);
		if (ret != 0) {
			dev_err(component->dev, "Device ma120x0p disabled in STANDBY BIAS: %d\n",
			ret);
			return ret;
		}
		break;

	case SND_SOC_BIAS_OFF:
		break;
	}

	return 0;
}

static const struct snd_soc_dapm_widget ma120x0p_dapm_widgets[] = {
	SND_SOC_DAPM_OUTPUT("OUT_A"),
	SND_SOC_DAPM_OUTPUT("OUT_B"),
};

static const struct snd_soc_dapm_route ma120x0p_dapm_routes[] = {
	{ "OUT_B",  NULL, "Playback" },
	{ "OUT_A",  NULL, "Playback" },
};

static const struct snd_soc_component_driver ma120x0p_component_driver = {
	.probe = ma120x0p_probe,
	.remove = ma120x0p_remove,
	.set_bias_level = ma120x0p_set_bias_level,
	.dapm_widgets		= ma120x0p_dapm_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(ma120x0p_dapm_widgets),
	.dapm_routes		= ma120x0p_dapm_routes,
	.num_dapm_routes	= ARRAY_SIZE(ma120x0p_dapm_routes),
	.controls = ma120x0p_snd_controls,
	.num_controls = ARRAY_SIZE(ma120x0p_snd_controls),
	.use_pmdown_time	= 1,
	.endianness		= 1,
};

//I2C Driver
static const struct reg_default ma120x0p_reg_defaults[] = {
	{	0x01,	0x3c	},
};

static bool ma120x0p_reg_volatile(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case ma_error__a:
			return true;
	default:
			return false;
	}
}

static const struct of_device_id ma120x0p_of_match[] = {
	{ .compatible = "ma,ma120x0p", },
	{ }
};

MODULE_DEVICE_TABLE(of, ma120x0p_of_match);

static struct regmap_config ma120x0p_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,

	.max_register = 255,
	.volatile_reg = ma120x0p_reg_volatile,

	.cache_type = REGCACHE_RBTREE,
	.reg_defaults = ma120x0p_reg_defaults,
	.num_reg_defaults = ARRAY_SIZE(ma120x0p_reg_defaults),
};

static int ma120x0p_i2c_probe(struct i2c_client *i2c,
	const struct i2c_device_id *id)
{
	int ret;

	priv_data = devm_kzalloc(&i2c->dev, sizeof(*priv_data), GFP_KERNEL);
	if (!priv_data)
		return -ENOMEM;
	i2c_set_clientdata(i2c, priv_data);

	priv_data->regmap = devm_regmap_init_i2c(i2c, &ma120x0p_regmap_config);
	if (IS_ERR(priv_data->regmap)) {
		ret = PTR_ERR(priv_data->regmap);
		return ret;
	}

	//Startup sequence

	//Make sure the device is muted
	priv_data->mute_gpio = devm_gpiod_get_optional(&i2c->dev, "mute_gp",
		GPIOD_OUT_LOW);
	if (IS_ERR(priv_data->mute_gpio)) {
		ret = PTR_ERR(priv_data->mute_gpio);
		dev_err(&i2c->dev, "Failed to get mute gpio line: %d\n", ret);
		return ret;
	}
	msleep(50);

// MA120xx0P devices are usually powered by an integrated boost converter.
// An option GPIO control line is provided to enable the booster properly and
// in sync with the enable and mute GPIO lines.
	priv_data->booster_gpio = devm_gpiod_get_optional(&i2c->dev,
		"booster_gp", GPIOD_OUT_LOW);
	if (IS_ERR(priv_data->booster_gpio)) {
		ret = PTR_ERR(priv_data->booster_gpio);
		dev_err(&i2c->dev,
		"Failed to get booster enable gpio line: %d\n", ret);
		return ret;
	}
	msleep(50);

	//Enable booster and wait 200ms until stable PVDD
	gpiod_set_value_cansleep(priv_data->booster_gpio, 1);
	msleep(200);

	//Enable ma120x0pp
	priv_data->enable_gpio = devm_gpiod_get_optional(&i2c->dev,
		"enable_gp", GPIOD_OUT_LOW);
	if (IS_ERR(priv_data->enable_gpio)) {
		ret = PTR_ERR(priv_data->enable_gpio);
		dev_err(&i2c->dev,
		"Failed to get ma120x0p enable gpio line: %d\n", ret);
		return ret;
	}
	msleep(50);

	//Optional use of ma120x0pp error line as an interrupt trigger to
	//platform GPIO.
	//Get error input gpio ma120x0p
	priv_data->error_gpio = devm_gpiod_get_optional(&i2c->dev,
		 "error_gp", GPIOD_IN);
	if (IS_ERR(priv_data->error_gpio)) {
		ret = PTR_ERR(priv_data->error_gpio);
		dev_err(&i2c->dev,
			"Failed to get ma120x0p error gpio line: %d\n", ret);
		return ret;
	}

	if (priv_data->error_gpio != NULL) {
		irqNumber = gpiod_to_irq(priv_data->error_gpio);

		ret = devm_request_threaded_irq(&i2c->dev,
			 irqNumber, ma120x0p_irq_handler,
			 NULL, IRQF_TRIGGER_FALLING,
			 "ma120x0p", priv_data);
		if (ret != 0)
			dev_warn(&i2c->dev, "Failed to request IRQ: %d\n",
				ret);
	}

	ret = devm_snd_soc_register_component(&i2c->dev,
		&ma120x0p_component_driver, &ma120x0p_dai, 1);

	return ret;
}

static irqreturn_t ma120x0p_irq_handler(int irq, void *data)
{
	gpiod_set_value_cansleep(priv_data->mute_gpio, 0);
	gpiod_set_value_cansleep(priv_data->enable_gpio, 1);
	return IRQ_HANDLED;
}

static int ma120x0p_i2c_remove(struct i2c_client *i2c)
{
	snd_soc_unregister_component(&i2c->dev);
	i2c_set_clientdata(i2c, NULL);

	gpiod_set_value_cansleep(priv_data->mute_gpio, 0);
	msleep(30);
	gpiod_set_value_cansleep(priv_data->enable_gpio, 1);
	msleep(200);
	gpiod_set_value_cansleep(priv_data->booster_gpio, 0);
	msleep(200);

	kfree(priv_data);

	return 0;
}

static void ma120x0p_i2c_shutdown(struct i2c_client *i2c)
{
	snd_soc_unregister_component(&i2c->dev);
	i2c_set_clientdata(i2c, NULL);

	gpiod_set_value_cansleep(priv_data->mute_gpio, 0);
	msleep(30);
	gpiod_set_value_cansleep(priv_data->enable_gpio, 1);
	msleep(200);
	gpiod_set_value_cansleep(priv_data->booster_gpio, 0);
	msleep(200);

	kfree(priv_data);
}

static const struct i2c_device_id ma120x0p_i2c_id[] = {
	{ "ma120x0p", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, ma120x0p_i2c_id);

static struct i2c_driver ma120x0p_i2c_driver = {
	.driver = {
		.name = "ma120x0p",
		.owner = THIS_MODULE,
		.of_match_table = ma120x0p_of_match,
	},
	.probe = ma120x0p_i2c_probe,
	.remove = ma120x0p_i2c_remove,
	.shutdown = ma120x0p_i2c_shutdown,
	.id_table = ma120x0p_i2c_id
};

static int __init ma120x0p_modinit(void)
{
	int ret = 0;

	ret = i2c_add_driver(&ma120x0p_i2c_driver);
	if (ret != 0) {
		pr_err("Failed to register MA120X0P I2C driver: %d\n", ret);
		return ret;
	}
	return ret;
}
module_init(ma120x0p_modinit);

static void __exit ma120x0p_exit(void)
{
	i2c_del_driver(&ma120x0p_i2c_driver);
}
module_exit(ma120x0p_exit);

MODULE_AUTHOR("Ariel Muszkat ariel.muszkat@gmail.com>");
MODULE_DESCRIPTION("ASoC driver for ma120x0p");
MODULE_LICENSE("GPL v2");
