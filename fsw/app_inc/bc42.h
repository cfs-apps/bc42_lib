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
**    Provide a 42 AcApp wrapper object
**
**  Notes:
**    1. The make system must define _AC_STANDALONE_
**    2. 42 function names don't follow cFS/Basecamp naming conventions of
**       prefixing global names with object's name. but the chance of a name
**       clash are slim and a porting goal was to minimize any changes to
**       the 42 code so the names have not been changed.
**    3. bc42.c's prologue for more information.  
**
*/
#ifndef _bc42_
#define _bc42_

/*
** Includes
*/

#include "42defines.h"
#include "42types.h"


/***********************/
/** Macro Definitions **/
/***********************/

#define BC42_MUTEX_NAME "BC42"


/**********************/
/** Type Definitions **/
/**********************/

typedef struct AcType BC42_Ac_t;

typedef struct
{
   double Kr[3];
   double Kp[3];
   double Kunl;
   
} BC42_CtrlGains_t;


/******************************************************************************
** BC42 Class
*/
typedef struct
{

   struct AcType    Ac;
   struct AcIpcType AcIpc;
   
   BC42_CtrlGains_t DefaultCtrlGains;
   
   uint32  MutexId;
   
} BC42_Class_t;


/************************/
/** Exported Functions **/
/************************/

/******************************************************************************
** Function protoypes for 42's AcApp.c 
**
*/

void AllocateAC(struct AcType *AC);
void AllocateAcBufs(struct AcIpcType *I);
void InitAC(struct AcType *AC);
void AcFsw(struct AcType *AC);

void GyroProcessing(struct AcType *AC);
void MagnetometerProcessing(struct AcType *AC);
void CssProcessing(struct AcType *AC);
void FssProcessing(struct AcType *AC);
void StarTrackerProcessing(struct AcType *AC);
void GpsProcessing(struct AcType *AC);
     
void WheelProcessing(struct AcType *AC);
void MtbProcessing(struct AcType *AC);


/******************************************************************************
** Function protoypes for 42's AcIPC.c 
**
*/
void ReadAcArraySizesFromSocket(struct AcType *AC, struct AcIpcType *I);
void ReadAcBufLensFromSocket(struct AcIpcType *I);
void ReadAcInFromSocket(struct AcType *AC,struct AcIpcType *I);
void ReadAcTblFromSocket(struct AcType *AC,struct AcIpcType *I);
void WriteAcOutToSocket(struct AcType *AC,struct AcIpcType *I);


/******************************************************************************
** Function: BC42_Constructor
**
** Notes:
**   1. This must be called prior to any other function.
**
*/
int32 BC42_Constructor(void);


/******************************************************************************
** Function: BC42_GetCOntrolGains
**
*/
void BC42_GetControlGains(BC42_CtrlGains_t *CtrlGains);


/******************************************************************************
** Function: BC42_ReadSensorData
**
*/
bool BC42_ReadSensorData(const BC42_Ac_t **Ac);


/******************************************************************************
** Function: BC42_RestoreDefaultCtrlGains
**
*/
void BC42_RestoreDefaultCtrlGains(void);


/******************************************************************************
** Function: BC42_RunController
**
** AcApp's controller and return load Tcmd and Mcmd arrays.
*/
bool BC42_RunController(const BC42_Ac_t **Ac);


/******************************************************************************
** Function: BC42_SetControlGains
**
*/
void BC42_SetControlGains(const BC42_CtrlGains_t *CtrlGains);


/******************************************************************************
** Function: BC42_StartSim
**
** Notes:
**   1. Call AcApp's initialization functions to prepare for a sim with 42
**
*/
bool BC42_StartSim(uint16 Port);


/******************************************************************************
** Function: BC42_StopSim
**
** Notes:
**   1. Close 42 socket
**
*/
bool BC42_StopSim(void);


/******************************************************************************
** Function: BC42_WriteActuatorData
**
*/
bool BC42_WriteActuatorData(const double Tcmd[3], const double Mcmd[3], double SAcmd);


#endif /*_bc42_*/
