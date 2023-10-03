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
**    Provide a 42 AcType wrapper object
**
**  Notes:
**    1. BC42_TakePtr() must be called prior to using either 
**       BC42_ReadFromSocket() or BC42_WriteToSocket() and BC42_GivePtr()
**       must be called after the Socket function calls. The Ptr functions
**       are not embedded within the Socket functions to allow the caller
**       the flexibility to work with the pointer while they're working
**       with the socket data.
**
*/
#ifndef _bc42_
#define _bc42_

/*
** Includes
*/

#include "Ac.h"


/***********************/
/** Macro Definitions **/
/***********************/

#define BC42_MUTEX_NAME "BC42"

/*
** 42 AcApp dimensions
*/

/* Dynamics */
#define BC42_NB 2
#define BC42_NG 1

/* Sensors */
#define BC42_NGYRO 3
#define BC42_NMAG  3
#define BC42_NCSS  8
#define BC42_NFSS  1
#define BC42_NST   1
#define BC42_NGPS  1

/* Actuators */
#define BC42_NWHL 4  // TODO: Use EDS for single source definitions
#define BC42_NMTB 3
#define BC42_NTHR 2  // Should be zero, but can't have a zero dimension


/**********************/
/** Type Definitions **/
/**********************/

/******************************************************************************
** BC42_Struct
**
** Define fixed sized structure for 42's actypes that are dynamically allocated.
** Keep naming consistent with 42. 
*/

typedef struct
{

   /* Dynamics */
   
   struct AcBodyType   B[BC42_NB];
   struct AcJointType  G[BC42_NG];

   /* Sensors */

   struct AcGyroType          Gyro[BC42_NGYRO];
   struct AcMagnetometerType  MAG[BC42_NMAG];
   struct AcCssType           CSS[BC42_NCSS];
   struct AcFssType           FSS[BC42_NFSS];
   struct AcStarTrackerType   ST[BC42_NST];
   struct AcGpsType           GPS[BC42_NGPS];


   /* Actuators */

   struct AcWhlType  Whl[BC42_NWHL];
   struct AcMtbType  MTB[BC42_NMTB];
   struct AcThrType  Thr[BC42_NTHR];


} BC42_VarStruct_t;


/******************************************************************************
** BC42 Class
*/
typedef struct
{

   BC42_VarStruct_t  BcVar;
   struct AcType     AcVar;

   uint32  MutexId;
   
} BC42_Class_t;


/************************/
/** Exported Functions **/
/************************/

/******************************************************************************
** Function protoypes for 42's AcApp.c which is used as Basecamp's example FSW
** app. The function names don't follow cFS/Basecamp naming conventions but the
** chance of a name clash are slim and a porting goal was to minimize any
** changes to the 42 code so the names have not been changed.
**
*/

void GyroProcessing(struct AcType *AC);
void MagnetometerProcessing(struct AcType *AC);
void CssProcessing(struct AcType *AC);
void FssProcessing(struct AcType *AC);
void StarTrackerProcessing(struct AcType *AC);
void GpsProcessing(struct AcType *AC);
     
void WheelProcessing(struct AcType *AC);
void MtbProcessing(struct AcType *AC);

void InitAC(struct AcType *AC);
void AcFsw(struct AcType *AC);


/******************************************************************************
** Function: BC42_Constructor
**
** Notes:
**   1. This must be called prior to any other function.
**
*/
int32 BC42_Constructor(void);


/******************************************************************************
** Function: BC42_GivePtr
**
*/
void BC42_GivePtr(BC42_Class_t *Bc42);


/******************************************************************************
** Function: BC42_ReadFromSocket
**
*/
int32 BC42_ReadFromSocket(osal_id_t SocketId, OS_SockAddr_t *RemoteAddr, struct AcType *AC);


/******************************************************************************
** Function: BC42_TakePtr
**
*/
BC42_Class_t *BC42_TakePtr(void);


/******************************************************************************
** Function: BC42_WriteToSocket
**
*/
void BC42_WriteToSocket(osal_id_t SocketId, OS_SockAddr_t *RemoteAddr, struct AcType *AC);


#endif /*_bc42_*/
