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
**    Define the 42 Library interface
**
**  Notes:
**   1. This is part of prototype effort to port a 42 simulator FSW controller
**      component into a cFS-based application. The source files in this 
**      library have been copied from 42. For convenience they are contained
**      in a single directory although they come from the following 42 
**      source directories:
**         42/Include
**         42/Kit/
**         42/Source
**   2. This library is used by the BC42_INTF and BC42_CTRL apps. Effort has
**      been made to minimize changes to the 42 source files. All changes are
**      marked with a "~bc~" tag in a comment. 
**   3. For more 42 information please see
**         42/Docs/
**         42/Database/Readme.txt
**         42/Standalone/Readme.txt
**
*/
#ifndef _bc42_lib_h_
#define _bc42_lib_h_

/*
** Includes
*/

#include "cfe.h"
#include "bc42.h"

/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: BC42_LIB_LibInit
**
** Library Initialization Function
**
** Notes:
**   1. This function is required by CFE to initialize the library. It should
**      be specified in the cfe_es_startup.scr file as part of loading this 
**      library.  It is not directly invoked by applications.
**
*/
int32 BC42_LIB_LibInit(void);


#endif /* _bc42_lib_h_ */

