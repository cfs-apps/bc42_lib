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
**    1. A primary goal is avoid any changes to 42 source code
**       which simplifies updating to future 42 releases but 
**       it makes the Basecamp implemenmtation a little less
**       realistic because BC42_LIB encapsulates 42's AC structure
**       and the BC42_INTF and BC42_CTRL apps access the same
**       data through BC42_LIB. In a more flight like environment
**       the required data would be partition locally to each
**       app. 
**    2. This is non-flight code and 42's dynamic memory usage is
**       left intact.
**    TODO: Think through AC data protection, semaphore, etc.
*/

/*
** Include Files:
*/

#include "cfe.h"
#include "bc42.h"

/*
** Macros
*/
#define  AC      (&(Bc42.Ac))
#define  AC_IPC  (&(Bc42.AcIpc))

static BC42_Class_t Bc42;

/******************************************************************************
** Function: BC42_Constructor
**
** Initialize a 42 AC data object.
**
** Notes:
**   1. This must be called prior to any other function.
**   2. AC structure initialization mimics 42's AcApp.c dynamic memory 
**      configuration so AcApp's functions can be used unmodified.
**
*/
int32 BC42_Constructor(void)
{
   int32 CfeStatus;
   
   CFE_PSP_MemSet((void*)&Bc42, 0, sizeof(BC42_Class_t));

   CfeStatus = OS_MutSemCreate(&Bc42.MutexId, BC42_MUTEX_NAME, 0);
   
   return CfeStatus;

} /* End BC42_Constructor() */


/******************************************************************************
** Function: BC42_GetControlGains
**
*/
void BC42_GetControlGains(BC42_CtrlGains_t *CtrlGains)
{
   int i;
   
   OS_MutSemTake(Bc42.MutexId);
   for (i=0; i < 3; i++)
   {
      CtrlGains->Kr[i] = AC->CfsCtrl.Kr[i];
      CtrlGains->Kp[i] = AC->CfsCtrl.Kp[i];
   }
   CtrlGains->Kunl = AC->CfsCtrl.Kunl;
   OS_MutSemGive(Bc42.MutexId);
   
} /* End BC42_GetControlGains() */


/******************************************************************************
** Function: BC42_ReadSensorData
**
*/
bool BC42_ReadSensorData(const BC42_Ac_t **Ac)
{
   bool RetStatus = true;
   

   OS_MutSemTake(Bc42.MutexId);
   ReadAcInFromSocket(AC, AC_IPC);
   GyroProcessing(AC);
   MagnetometerProcessing(AC);
   CssProcessing(AC);
   FssProcessing(AC);
   StarTrackerProcessing(AC);
   GpsProcessing(AC);
   OS_MutSemGive(Bc42.MutexId);
   
   *Ac = AC;
   
   return RetStatus;
   
} /* End BC42_ReadSensorData() */


/******************************************************************************
** Function: BC42_RestoreDefaultCtrlGains
**
*/
void BC42_RestoreDefaultCtrlGains(void)
{
   
   BC42_SetControlGains(&Bc42.DefaultCtrlGains);

} /* End BC42_RestoreDefaultCtrlGains() */


/******************************************************************************
** Function: BC42_RunController
**
*/
bool BC42_RunController(const BC42_Ac_t **Ac)
{
   bool RetStatus = true;
   
   OS_MutSemTake(Bc42.MutexId); 
   AcFsw(AC);
   OS_MutSemGive(Bc42.MutexId);

   *Ac = AC;
   
   return RetStatus;
   
} /* End BC42_RunController() */


/******************************************************************************
** Function: BC42_SetControlGains
**
*/
void BC42_SetControlGains(const BC42_CtrlGains_t *CtrlGains)
{
   int i;
   
   OS_MutSemTake(Bc42.MutexId);
   for (i=0; i < 3; i++)
   {
      AC->CfsCtrl.Kr[i] = CtrlGains->Kr[i];
      AC->CfsCtrl.Kp[i] = CtrlGains->Kp[i];
   }
   AC->CfsCtrl.Kunl = CtrlGains->Kunl;
   OS_MutSemGive(Bc42.MutexId);
   
} /* End BC42_SetControlGains() */


/******************************************************************************
** Function: BC42_StartSim
**
** Notes:
**   1. Performs the same functions as 42's AcApp.c main() function prior to
**      the inifinite control loop.
**   2. Save default control gains so they can be restored. 42 derives the 
**      gains during it's initialization.
**   3. TODO: What error checking should be done?
**
*/
bool BC42_StartSim(uint16 Port)
{

   bool RetStatus = true;


   Bc42.AcIpc.Init = 0;
   Bc42.AcIpc.Port = Port;
   Bc42.AcIpc.AllowBlocking = 1;
   
   Bc42.AcIpc.Socket = InitSocketClient("localhost",Bc42.AcIpc.Port,Bc42.AcIpc.AllowBlocking);
   
   ReadAcArraySizesFromSocket(AC, AC_IPC);
   AllocateAC(AC);
   
   ReadAcBufLensFromSocket(AC_IPC);
   AllocateAcBufs(AC_IPC);
   
   InitAC(AC);

   ReadAcTblFromSocket(AC, AC_IPC);      
   
   BC42_GetControlGains(&Bc42.DefaultCtrlGains);
OS_print("Default Kunl %f\n",Bc42.DefaultCtrlGains.Kunl);
   return RetStatus;
   
} /* End BC42_StartSim() */


/******************************************************************************
** Function: BC42_StopSim
**
** Notes:
**   TODO: Close socket
**
*/
bool BC42_StopSim(void)
{

   bool RetStatus = true;      
   
   return RetStatus;
   
} /* End BC42_BC42_StopSim() */


/******************************************************************************
** Function: BC42_WriteActuatorData
**
*/
bool BC42_WriteActuatorData(const double Tcmd[3], const double Mcmd[3], double SAcmd)
{
   int  i;
   bool RetStatus = true;


   OS_MutSemTake(Bc42.MutexId); 
   for (i=0; i < 3; i++)
   {
      Bc42.Ac.Tcmd[i] = Tcmd[i];
      Bc42.Ac.Mcmd[i] = Mcmd[i];
   }
   Bc42.Ac.G[0].GCmd.AngRate[0] = SAcmd;
   WriteAcOutToSocket(AC, AC_IPC);
   OS_MutSemGive(Bc42.MutexId);

   return RetStatus;

} /* End BC42_WriteActuatorData() */
