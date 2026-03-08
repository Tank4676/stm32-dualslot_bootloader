/*
 * app_header_inst.c
 *
 *  Created on: Mar 8, 2026
 *      Author: tanmay
 */




#include "app_header.h"

const AppHeader_t __attribute__((section(".header"))) app_header =
{
    .magic       = APP_MAGIC_WORD,
    .hdr_version = APP_HEADER_VERSION,
    .fw_version  = 1,
    .fw_size     = 0,
    .fw_crc      = 0,
    .hdr_crc     = 0,
    ._reserved   = {0, 0}
};
