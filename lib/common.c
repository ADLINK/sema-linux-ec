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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <common.h>
#include <eapi.h>

int read_sysfs_file(const char *sysfile, char *value, unsigned short size)
{	
	int fd, ret;

	fd = open(sysfile, O_RDONLY);
	if(fd < 0)
		return -1;

	ret = read(fd, value, size);	
	if (ret)
		close(fd);
	else {
		close(fd);
		return -1;
	}

	return 0;
}

int write_sysfs_file(const char *sysfile, const char *value, unsigned short size)
{	
	int fd, ret;

	fd = open(sysfile, O_RDWR);
	if(fd < 0){
		return -1;
	}
	ret = write(fd, value, size);	
	if (ret)
		close(fd);
	else {
		close(fd);
		return -1;
	}

	return 0;
}


