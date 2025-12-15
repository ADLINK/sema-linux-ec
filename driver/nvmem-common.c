/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause) */
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

#include "adl-ec.h"


int StatusCheck(void)
{
    unsigned char status, i;

    for(i=0; i<100; i++)
    {
        if(adl_bmc_ec_read_device(EC_RW_ADDR_IIC_BMC_STATUS, &status, 1, EC_REGION_2) == 0)
        {
            if((status & 0x1)==0)
                    return 0;
        }
    }
    return -1;
}

int ReadMem(unsigned char Region, unsigned int nAdr, u8* pData, unsigned int nSize)
{
  
    unsigned char pDataIn[] = { 0x2, 0x1, (unsigned char)nSize, (unsigned char)(Region + 1), (unsigned char)(nAdr >> 8), (unsigned char)(nAdr & 0xFF) };

    if(StatusCheck()!=0)
    {
        return -1;
    }

    if (adl_bmc_ec_write_device(EC_WO_ADDR_IIC_CMD_START,pDataIn, 6, EC_REGION_2) == 0)
    {
        pDataIn[0] = 4;

        if (adl_bmc_ec_write_device(EC_RW_ADDR_IIC_BMC_STATUS, pDataIn, 1,EC_REGION_2) == 0)
        {
    	    unsigned char  i;
            for (i = 0; i < 100; i++)
            {
                if (adl_bmc_ec_read_device(EC_RW_ADDR_IIC_BMC_STATUS, pDataIn, 1,EC_REGION_2) == 0)
                {
                    if (!!(pDataIn[0] & 0x4) == 0x0 && (pDataIn[0] & 0x1) == 0 && !!(pDataIn[0] & 0x8) == 0)
                    {
                        delay(40);
                        adl_bmc_ec_read_device(EC_RW_ADDR_IIC_BUFFER, pData, nSize,EC_REGION_2);
                        return 0;
                    }
                }
                delay(100);
            }
        }
    }

    return -1;
}


int WriteMem(unsigned char Region, unsigned int nAdr, u8* pData, unsigned int nSize)
{
   
    unsigned char pDataIn[] = { 0x2, 0x2, (unsigned char)nSize, (unsigned char)(Region + 1), (unsigned char)(nAdr >> 8), (unsigned char)(nAdr & 0xFF) };


    if(StatusCheck() != 0)
    {
        return -1;
    }

    if (adl_bmc_ec_write_device(EC_WO_ADDR_IIC_CMD_START, pDataIn, 6,EC_REGION_2) == 0)
    {
        if (adl_bmc_ec_write_device(EC_RW_ADDR_IIC_BUFFER, pData, nSize,EC_REGION_2) == 0)
        {
            pDataIn[0] = 4;
            if (adl_bmc_ec_write_device(EC_RW_ADDR_IIC_BMC_STATUS, pDataIn, 1,EC_REGION_2) == 0)
            {
    		unsigned char  i;
                for (i = 0; i < 100; i++)
                {
                    if (adl_bmc_ec_read_device(EC_RW_ADDR_IIC_BMC_STATUS, pDataIn, 1,EC_REGION_2) == 0)
		    {
			    if (!!(pDataIn[0] & 0x4) == 0x0 && (pDataIn[0] & 0x1) == 0 && !!(pDataIn[0] & 0x8) == 0)
			    {
				    return 0;
			    }
		    }
                    delay(100);
                }
            }
        }
    }


    return -1;
}
