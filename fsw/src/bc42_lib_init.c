/*
**  Copyright 2022 bitValence, Inc.
**  All Rights Reserved.
**
**  This program is free software; you can modify and/or redistribute it
**  under the terms of the GNU Affero General Public License
**  as published by the Free Software Foundation; version 3 with
**  attribution addendums as found in the LICENSE.txt
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU Affero General Public License for more details.
**
**  Purpose:
**    Entry point function for 42 library
**
**  Notes:
**    Nones
**
*/

/*
** Includes
*/
#include "bc42_lib.h"
#include "bc42_lib_ver.h"

/*
** Exported Functions
*/

/******************************************************************************
** Entry function
**
*/
int32 BC42_LIB_LibInit(void)
{

   OS_printf("Basecamp 42 Library Initialized. Version %d.%d.%d\n",
             BC42_LIB_MAJOR_VER, BC42_LIB_MINOR_VER, BC42_LIB_LOCAL_REV);
   
   return OS_SUCCESS;

} /* End BC42_LIB_LibInit() */

