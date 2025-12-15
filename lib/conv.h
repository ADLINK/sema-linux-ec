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

/**
********************************************************************************
*	\FILE		Conv.h
*
*	\DESCRIPTION
*		Misc. conversion functions
*
*	\AUTHOR		Martin Hein
*				(c) LiPPERT Embedded Computers GmbH Germany
*
********************************************************************************
**/

#ifndef CONV_H
#define CONV_H


#define false 0
#define true 1


//#include "Globals.h"

/* ------------------------------ Definitions ------------------------------- */
/* ------------------------------- Typedefs  -------------------------------- */
/* -------------------------------- Macros ---------------------------------- */
/* ------------------------------- Variables -------------------------------- */
/* ------------------------------- Functions -------------------------------- */

/**
********************************************************************************
*	\FUNCTION		unsigned char Conv_IsHexDigit(char c)
*
*	\DESCRIPTION
*		tests if c is valid hex character (0..9, a..f, A..F)
*
*	\PARAMETERS
*		c		character to test
*
*	\RETURNS
*		1 if c is valid hex character, otherwise 0
********************************************************************************
**/
unsigned char Conv_IsHexDigit(char c);


/**
********************************************************************************
*	\FUNCTION		unsigned char Conv_IsHexString(char *String)
*
*	\DESCRIPTION
*		tests if String contains only valid hex characters (0..9, a..f, A..F)
*
*	\PARAMETERS
*		String	null terminated string to test
*
*	\RETURNS
*		1 if String is valid hex string, otherwise 0
********************************************************************************
**/
unsigned char Conv_IsHexString(const char *String);


/**
********************************************************************************
*	\FUNCTION		unsigned char Conv_Hex2Dez(char c1, char c2)
*
*	\DESCRIPTION
*		Converts two hex digits to one unsigned char
*
*	\PARAMETERS
*		c1		Hex digit containing UPPER nibble							\np
*		c2		Hex digit containing LOWER nibble
*
*	\RETURNS
*		unsigned char value
********************************************************************************
**/
unsigned char Conv_Hex2Dez(char c1, char c2);


/**
********************************************************************************
*	\FUNCTION		unsigned char Conv_GetDigit(char c)
*
*	\DESCRIPTION
*		Transforms a 1 unsigned char hex value in a decimal value.
*
*	\PARAMETERS
*		c			Hex value 
*
*	\RETURNS
*		Decimal value of the input
********************************************************************************
**/
unsigned char Conv_GetDigit(char c);


/**
********************************************************************************
*	\FUNCTION unsigned char Conv_GetHex(char* buf)
*
*	\DESCRIPTION
*		Transforms a 2 unsigned char hex value in a decimal value.
*
*	\PARAMETERS
*		buf			Hex string value 
*
*	\RETURNS
*		Decimal value of the input
********************************************************************************
**/
unsigned char Conv_GetHex(const char* buf);


/**
********************************************************************************
*	\FUNCTION		char Conv_HexString2Byte(char *String, unsigned char *Hex)
*
*	\DESCRIPTION
*		Converts up to 2 hex digits to a 8bit unsigned value
*
*		Note: An optional leading "0x" and leading zeroes will be ignored
*
*	\PARAMETERS
*		String		Pointer to string to convert							\np
*		Hex			Pointer to BYTE where the result will be stored
*
*	\RETURNS
*		\c true if conversion was successful, otherwise \c false
********************************************************************************
**/
char Conv_HexString2Byte(char *String, unsigned char *Hex);


/**
********************************************************************************
*	\FUNCTION		char Conv_HexString2Word(char *String, unsigned short *Hex)
*
*	\DESCRIPTION
*		Converts up to 4 hex digits to a 16bit unsigned value
*
*		Note: An optional leading "0x" and leading zeroes will be ignored
*
*	\PARAMETERS
*		String		Pointer to string to convert							\np
*		Hex			Pointer to WORD where the result will be stored
*
*	\RETURNS
*		\c true if conversion was successful, otherwise \c false
********************************************************************************
**/
char Conv_HexString2Word(char *String, unsigned short *Hex);


/**
********************************************************************************
*	\FUNCTION		char Conv_HexString2DWord(char *String, dunsigned short *Hex)
*
*	\DESCRIPTION
*		Converts up to 8 hex digits to a 32bit unsigned value
*
*		Note: An optional leading "0x" and leading zeroes will be ignored
*
*	\PARAMETERS
*		String		Pointer to string to convert							\np
*		Hex			Pointer to DWORD where the result will be stored
*
*	\RETURNS
*		\c true if conversion was successful, otherwise \c false
********************************************************************************
**/
char Conv_HexString2DWord(char *String, uint32_t *Hex);
int Conv_String2HexByte(char *string, char *result);


#endif /* CONV_H */
