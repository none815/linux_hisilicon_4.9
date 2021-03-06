/*
 * Copyright (c) 2018 HiSilicon Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _DRV_HASH_V1_H_
#define _DRV_HASH_V1_H_

#include "drv_osal_lib.h"

/*************************** Internal Structure Definition ****************************/
/** \addtogroup      hash drivers*/
/** @{*/  /** <!-- [hash]*/

/*! \Define the offset of reg */
#define SEC_CHN_CFG                      (0x0304)
#define CALC_ERR                         (0x0320)
#define CIPHER_INT_STATUS                (0x0404)
#define NORM_SMMU_START_ADDR             (0x0440)
#define SEC_SMMU_START_ADDR              (0x0444)
#define HASH_INT_STATUS                  (0x0804)
#define HASH_INT_EN                      (0x0808)
#define HASH_INT_RAW                     (0x080C)
#define HASH_IN_SMMU_EN                  (0x0810)
#define CHAN0_HASH_DAT_IN                (0x0818)
#define CHAN0_HASH_TOTAL_DAT_LEN         (0x081C)
#define CHANn_HASH_CTRL(id)              (0x0800 + (id)*0x80)
#define CHANn_HASH_IN_NODE_CFG(id)       (0x0804 + (id)*0x80)
#define CHANn_HASH_IN_NODE_START_ADDR(id)(0x0808 + (id)*0x80)
#define CHANn_HASH_IN_BUF_RPTR(id)       (0x080C + (id)*0x80)
#define CHANn_HASH_STATE_VAL(id)         (0x0340 + (id)*0x08)
#define CHANn_HASH_STATE_VAL_ADDR(id)    (0x0344 + (id)*0x08)
#define CHANN_HASH_IN_NODE_START_HIGH(id)(0x0820 + (id)*0x80)

/* Define the union sec_chn_cfg */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    cipher_sec_chn_cfg    : 8   ; /* [7..0]  */
        unsigned int    cipher_sec_chn_cfg_lock : 1   ; /* [8]  */
        unsigned int    reserved_0            : 7   ; /* [15..9]  */
        unsigned int    hash_sec_chn_cfg      : 8   ; /* [23..16]  */
        unsigned int    hash_sec_chn_cfg_lock : 1   ; /* [24]  */
        unsigned int    reserved_1            : 7   ; /* [31..25]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} sec_chn_cfg;

/* Define the union chan0_hash_ctrl */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    hash_ch0_start        : 1   ; /* [0]  */
        unsigned int    hash_ch0_agl_sel      : 3   ; /* [3..1]  */
        unsigned int    hash_ch0_hmac_calc_step : 1   ; /* [4]  */
        unsigned int    hash_ch0_mode         : 1   ; /* [5]  */
        unsigned int    hash_ch0_key_sel      : 1   ; /* [6]  */
        unsigned int    reserved_0            : 2   ; /* [8..7]  */
        unsigned int    hash_ch0_auto_padding_en : 1   ; /* [9]  */
        unsigned int    hash_ch0_hmac_key_addr : 3   ; /* [12..10]  */
        unsigned int    hash_ch0_used          : 1   ; /* [13]  */
        unsigned int    hash_ch0_sec_alarm     : 1   ; /* [13]  */
        unsigned int    reserved_1            : 17  ; /* [31..15]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} chan0_hash_ctrl;

/* Define the union hash_int_status */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    reserved_0            : 18  ; /* [17..0]  */
        unsigned int    hash_chn_oram_int     : 8   ; /* [25..18]  */
        unsigned int    reserved_1            : 6   ; /* [31..26]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} hash_int_status;

/* Define the union hash_int_en */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    reserved_0            : 18  ; /* [17..0]  */
        unsigned int    hash_chn_oram_en      : 8   ; /* [25..18]  */
        unsigned int    reserved_1            : 4   ; /* [29..26]  */
        unsigned int    hash_sec_int_en       : 1   ; /* [30]  */
        unsigned int    hash_int_en           : 1   ; /* [31]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} hash_int_en;

/* Define the union hash_int_raw */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    reserved_0            : 18  ; /* [17..0]  */
        unsigned int    hash_chn_oram_raw     : 8   ; /* [25..18]  */
        unsigned int    reserved_1            : 6   ; /* [31..26]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} hash_int_raw;

/* Define the union cipher_int_status */
typedef union {
    /* Define the struct bits */
    struct {
        hi_u32    reserved_0            : 1   ; /* [0]  */
        hi_u32    cipher_chn_ibuf_int   : 7   ; /* [7..1]  */
        hi_u32    cipher_chn_obuf_int   : 8   ; /* [15..8]  */
        hi_u32    reserved_1            : 16  ; /* [31..16]  */
    } bits;

    /* Define an unsigned member */
    hi_u32    u32;

} cipher_int_status;

/* Define the union hash_in_smmu_en */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    hash_in_chan_rd_dat_smmu_en : 7   ; /* [6..0]  */
        unsigned int    reserved_0            : 9   ; /* [15..7]  */
        unsigned int    hash_in_chan_rd_node_smmu_en : 7   ; /* [22..16]  */
        unsigned int    reserved_1            : 9   ; /* [31..23]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} hash_in_smmu_en;

/* Define the union chann_hash_ctrl */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    reserved_0            : 1   ; /* [0]  */
        unsigned int    hash_chn_agl_sel      : 3   ; /* [3..1]  */
        unsigned int    reserved_1            : 1   ; /* [4]  */
        unsigned int    hash_chn_mode         : 1   ; /* [5]  */
        unsigned int    hash_chn_key_sel      : 1   ; /* [6]  */
        unsigned int    hash_chn_dat_in_byte_swap_en : 1   ; /* [7]  */
        unsigned int    hash_chn_dat_in_bit_swap_en : 1   ; /* [8]  */
        unsigned int    hash_chn_auto_padding_en : 1   ; /* [9]  */
        unsigned int    hash_chn_hmac_key_addr : 3   ; /* [12..10]  */
        unsigned int    reserved_2            : 19  ; /* [31..13]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} chann_hash_ctrl;

/* Define the union chann_hash_int_node_cfg */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    hash_in_node_mpackage_int_level : 8   ; /* [7..0]  */
        unsigned int    hash_in_node_rptr     : 8   ; /* [15..8]  */
        unsigned int    hash_in_node_wptr     : 8   ; /* [23..16]  */
        unsigned int    hash_in_node_total_num : 8   ; /* [31..24]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} chann_hash_int_node_cfg;

/** @}*/  /** <!-- ==== Structure Definition end ====*/
#endif
