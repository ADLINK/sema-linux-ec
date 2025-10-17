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

#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include "conv.h"


unsigned char Conv_IsHexDigit(char c)
{
	if ((c >= '0') && (c <= '9'))
		return 1;

	c = tolower(c);

	if ((c >= 'a') && (c <= 'f'))
		return 1;

	return 0;
}


unsigned char Conv_IsHexString(char *String)
{
	unsigned int i;

	for (i=0; i<strlen(String); i++)
		if (!Conv_IsHexDigit(String[i]))
			return false;					// invalid character (no hex digit)

	return true;
}


unsigned char Conv_Hex2Dez(char c1, char c2)
{
	unsigned char temp = 0;

	c1 = tolower((Conv_IsHexDigit(c1)) ? c1 : 0);
	c2 = tolower((Conv_IsHexDigit(c2)) ? c2 : 0);

	temp += (c1&0x10) ? (c1&0x0f) : ((c1&0x0f)+9);
	temp <<= 4;

	temp += (c2&0x10) ? (c2&0x0f) : ((c2&0x0f)+9);

	return temp;
}


unsigned char Conv_GetDigit(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';

	c = tolower(c);

	if (c >= 'a' || c <= 'f')
		return c - 'a' + 10;

	return 0;
}


unsigned char Conv_GetHex(char* buf)
{
	return Conv_GetDigit(buf[0]) * 16 + Conv_GetDigit(buf[1]);
}


char Conv_HexString2Byte(char *String, unsigned char *Hex)
{
	unsigned int i;

	*Hex = 0;

	if ((strlen(String) >= 3) && (String[0] == '0') && (tolower(String[1]) == 'x'))
		String += 2;						// Skip "0x" prefix if given

	while((strlen(String) > 0) && (String[0] == '0'))
		String++;							// Skip leading zeroes

	if (strlen(String) > 2)
		return false;						// String too long

	for (i=0; i<strlen(String); i++)
	{

		if (!Conv_IsHexDigit(String[i]))
			return false;					// invalid character (no hex digit)
		*Hex <<= 4;
		*Hex += ((unsigned char)Conv_GetDigit(String[i]));
	}

	return true;
}


char Conv_HexString2Word(char *String, unsigned short *Hex)
{
	unsigned int i;

	*Hex = 0;

	if ((strlen(String) >= 3) && (String[0] == '0') && (tolower(String[1]) == 'x'))
		String += 2;						// Skip "0x" prefix if given

	while((strlen(String) > 0) && (String[0] == '0'))
		String++;							// Skip leading zeroes

	if (strlen(String) > 4)
		return false;						// String too long

	for (i=0; i<strlen(String); i++)
	{

		if (!Conv_IsHexDigit(String[i]))
			return false;					// invalid character (no hex digit)
		*Hex <<= 4;
		*Hex += ((unsigned short)Conv_GetDigit(String[i]));// << ((strlen(String)-i-1)*8);
	}

	return true;
}


char Conv_HexString2DWord(char *String, uint32_t *Hex)
{
	unsigned int i;

	*Hex = 0;

	if ((strlen(String) >= 3) && (String[0] == '0') && (tolower(String[1]) == 'x'))
		String += 2;						// Skip "0x" prefix if given

	while((strlen(String) > 0) && (String[0] == '0'))
		String++;							// Skip leading zeroes

	if (strlen(String) > 8)
		return false;						// String too long

	for (i=0; i<strlen(String); i++)
	{

		if (!Conv_IsHexDigit(String[i]))
			return false;					// invalid character (no hex digit)
		*Hex <<= 4;
		*Hex += ((unsigned short)Conv_GetDigit(String[i]));// << ((strlen(String)-i-1)*8);
	}

	return true;
}


int Conv_String2HexByte(char *string, char *result)
{
        int k=0;
        int Len = strlen(string);

        for(int i = 0; i < Len; i = i + 2)
        {
                result[k] = Conv_GetHex( &string[i] );
                k++;
        }

        return k; // number of bytes in the result array
}

