// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright (c) 2022, ADLINK Technology, Inc
// All rights reserved.
//
// Redistribution and use of this software in source and binary forms,
// with or without modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Neither the name of ADLINK Technology nor the names of its contributors may be used
//   to endorse or promote products derived from this software without specific
//   prior written permission of ADLINK Technology, Inc.

#ifndef __COMMON_H__
#define __COMMON_H__

int get_hwmon_num(void);

int read_sysfs_file(char *sysfile, char *value, unsigned short size);
int write_sysfs_file(char *sysfile, char *value, unsigned short size);

#endif
