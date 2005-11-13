//----------------------------------------------------------------------------
//  ZIP structures, raw on-disk layout
//----------------------------------------------------------------------------
//
//  Copyright (c) 2004-2005  The EDGE Team.
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//----------------------------------------------------------------------------

#ifndef __EPI_RAWDEF_ZIP__
#define __EPI_RAWDEF_ZIP__

#include "types.h"

namespace epi
{

#define CENTRAL_HDR_SIG	'\001','\002'	/* the infamous "PK" signature bytes, */
#define LOCAL_HDR_SIG	'\003','\004'	/*  sans "PK" (so unzip executable not */
#define END_CENTRAL_SIG	'\005','\006'	/*  mistaken for zipfile itself) */
#define EXTD_LOCAL_SIG	'\007','\010'	/* [ASCII "\113" == EBCDIC "\080" ??] */

#define DEF_WBITS	15		/* Default LZ77 window size */
#define ZIP_STORE	0		/* 'STORED' method id */
#define ZIP_DEFLATE	8		/* 'DEFLATE' method id */

#define CRCVAL_INITIAL  0L

typedef struct
{
	u8_t  version_needed_to_extract[2];
	u16_t general_purpose_bit_flag;
	u16_t compression_method;
	u16_t last_mod_file_time;
	u16_t last_mod_file_date;
	u32_t crc32;
	u32_t csize;
	u32_t ucsize;
	u16_t filename_length;
	u16_t extra_field_length;
}
ZIP_local_file_header;

typedef struct
{
	u8_t  version_made_by[2];
	u8_t  version_needed_to_extract[2];
	u16_t general_purpose_bit_flag;
	u16_t compression_method;
	u16_t last_mod_file_time;
	u16_t last_mod_file_date;
	u32_t crc32;
	u32_t csize;
	u32_t ucsize;
	u16_t filename_length;
	u16_t extra_field_length;
	u16_t file_comment_length;
	u16_t disk_number_start;
	u16_t internal_file_attributes;
	u32_t external_file_attributes;
	u32_t relative_offset_local_header;
}
ZIP_central_directory_file_header;

typedef struct
{
	u16_t number_this_disk;
	u16_t num_disk_start_cdir;
	u16_t num_entries_centrl_dir_ths_disk;
	u16_t total_entries_central_dir;
	u32_t size_central_directory;
	u32_t offset_start_central_directory;
	u16_t zipfile_comment_length;
}
ZIP_end_central_dir_record;

//--- ZIP_local_file_header layout ---------------------------------------------
#define ZIP_LOCAL_FILE_HEADER_SIZE              26
#      define L_VERSION_NEEDED_TO_EXTRACT_0     0
#      define L_VERSION_NEEDED_TO_EXTRACT_1     1
#      define L_GENERAL_PURPOSE_BIT_FLAG        2
#      define L_COMPRESSION_METHOD              4
#      define L_LAST_MOD_FILE_TIME              6
#      define L_LAST_MOD_FILE_DATE              8
#      define L_CRC32                           10
#      define L_COMPRESSED_SIZE                 14
#      define L_UNCOMPRESSED_SIZE               18
#      define L_FILENAME_LENGTH                 22
#      define L_EXTRA_FIELD_LENGTH              24

//--- ZIP_central_directory_file_header layout ---------------------------------
#define ZIP_CENTRAL_DIRECTORY_FILE_HEADER_SIZE  42
#      define C_VERSION_MADE_BY_0               0
#      define C_VERSION_MADE_BY_1               1
#      define C_VERSION_NEEDED_TO_EXTRACT_0     2
#      define C_VERSION_NEEDED_TO_EXTRACT_1     3
#      define C_GENERAL_PURPOSE_BIT_FLAG        4
#      define C_COMPRESSION_METHOD              6
#      define C_LAST_MOD_FILE_TIME              8
#      define C_LAST_MOD_FILE_DATE              10
#      define C_CRC32                           12
#      define C_COMPRESSED_SIZE                 16
#      define C_UNCOMPRESSED_SIZE               20
#      define C_FILENAME_LENGTH                 24
#      define C_EXTRA_FIELD_LENGTH              26
#      define C_FILE_COMMENT_LENGTH             28
#      define C_DISK_NUMBER_START               30
#      define C_INTERNAL_FILE_ATTRIBUTES        32
#      define C_EXTERNAL_FILE_ATTRIBUTES        34
#      define C_RELATIVE_OFFSET_LOCAL_HEADER    38

//--- ZIP_end_central_dir_record layout ----------------------------------------
#define ZIP_END_CENTRAL_DIR_RECORD_SIZE         18
#      define E_NUMBER_THIS_DISK                0
#      define E_NUM_DISK_WITH_START_CENTRAL_DIR 2
#      define E_NUM_ENTRIES_CENTRL_DIR_THS_DISK 4
#      define E_TOTAL_ENTRIES_CENTRAL_DIR       6
#      define E_SIZE_CENTRAL_DIRECTORY          8
#      define E_OFFSET_START_CENTRAL_DIRECTORY  12
#      define E_ZIPFILE_COMMENT_LENGTH          16

}  // namespace epi

#endif  // __EPI_RAWDEF_ZIP__
