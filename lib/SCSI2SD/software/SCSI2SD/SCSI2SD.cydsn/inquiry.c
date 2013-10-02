//	Copyright (C) 2013 Michael McMaster <michael@codesrc.com>
//
//	This file is part of SCSI2SD.
//
//	SCSI2SD is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	SCSI2SD is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with SCSI2SD.  If not, see <http://www.gnu.org/licenses/>.

#include "device.h"
#include "scsi.h"
#include "inquiry.h"

#include <string.h>

static const uint8 StandardResponse[] =
{
0x00, // "Direct-access device". AKA standard hard disk
0x00, // device type qualifier
0x02, // Complies with ANSI SCSI-2.
0x02, // SCSI-2 Inquiry response
31, // standard length
0, 0, //Reserved
0, // We don't support anything at all
'c','o','d','e','s','r','c',' ',
'S','C','S','I','2','S','D',' ',' ',' ',' ',' ',' ',' ',' ',' ',
'2','.','0','a'
};

static const uint8 SupportedVitalPages[] =
{
0x00, // "Direct-access device". AKA standard hard disk
0x00, // Page Code
0x00, // Reserved
0x04, // Page length
0x00, // Support "Supported vital product data pages"
0x80, // Support "Unit serial number page"
0x81, // Support "Implemented operating definition page"
0x82 // Support "ASCII Implemented operating definition page"
};

static const uint8 UnitSerialNumber[] =
{
0x00, // "Direct-access device". AKA standard hard disk
0x80, // Page Code
0x00, // Reserved
0x10, // Page length
'c','o','d','e','s','r','c','-','1','2','3','4','5','6','7','8'
};

static const uint8 ImpOperatingDefinition[] =
{
0x00, // "Direct-access device". AKA standard hard disk
0x81, // Page Code
0x00, // Reserved
0x03, // Page length
0x03, // Current: SCSI-2 operating definition
0x03, // Default: SCSI-2 operating definition
0x03 // Supported (list): SCSI-2 operating definition.
};

static const uint8 AscImpOperatingDefinition[] =
{
0x00, // "Direct-access device". AKA standard hard disk
0x82, // Page Code
0x00, // Reserved
0x07, // Page length
0x06, // Ascii length
'S','C','S','I','-','2'
};

void scsiInquiry()
{
	uint8 evpd = scsiDev.cdb[1] & 1; // enable vital product data.
	uint8 pageCode = scsiDev.cdb[2];
	uint8 lun = scsiDev.cdb[1] >> 5;
	uint32 allocationLength = scsiDev.cdb[4];
	if (allocationLength == 0) allocationLength = 256;

	if (!evpd)
	{
		if (pageCode)
		{
			// error.
			scsiDev.status = CHECK_CONDITION;
			scsiDev.sense.code = ILLEGAL_REQUEST;
			scsiDev.sense.asc = INVALID_FIELD_IN_CDB;
			scsiDev.phase = STATUS;
		}
		else
		{
			memcpy(scsiDev.data, StandardResponse, sizeof(StandardResponse));
			scsiDev.dataLen = sizeof(StandardResponse);
			scsiDev.phase = DATA_IN;
			
			if (!lun) scsiDev.unitAttention = 0;
		}
	}
	else if (pageCode == 0x00)
	{
		memcpy(scsiDev.data, SupportedVitalPages, sizeof(SupportedVitalPages));
		scsiDev.dataLen = sizeof(SupportedVitalPages);
		scsiDev.phase = DATA_IN;
	}
	else if (pageCode == 0x80)
	{
		memcpy(scsiDev.data, UnitSerialNumber, sizeof(UnitSerialNumber));
		scsiDev.dataLen = sizeof(UnitSerialNumber);
		scsiDev.phase = DATA_IN;
	}
	else if (pageCode == 0x81)
	{
		memcpy(
			scsiDev.data,
			ImpOperatingDefinition,
			sizeof(ImpOperatingDefinition));
		scsiDev.dataLen = sizeof(ImpOperatingDefinition);
		scsiDev.phase = DATA_IN;
	}
	else if (pageCode == 0x82)
	{
		memcpy(
			scsiDev.data,
			AscImpOperatingDefinition,
			sizeof(AscImpOperatingDefinition));
		scsiDev.dataLen = sizeof(AscImpOperatingDefinition);
		scsiDev.phase = DATA_IN;
	}
	else
	{
		// error.
		scsiDev.status = CHECK_CONDITION;
		scsiDev.sense.code = ILLEGAL_REQUEST;
		scsiDev.sense.asc = INVALID_FIELD_IN_CDB;
		scsiDev.phase = STATUS;
	}


	if (scsiDev.phase == DATA_IN && scsiDev.dataLen > allocationLength)
	{
		// Spec 8.2.5 requires us to simply truncate the response.
		scsiDev.dataLen = allocationLength;
	}


	// Set the first byte to indicate LUN presence.
	if (lun) // We only support lun 0
	{
		scsiDev.data[0] = 0x7F;
	}
}

