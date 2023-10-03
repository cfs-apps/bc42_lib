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
**    1. The BC42_ReadFromSocket() and BC42_WriteToSocket() functions
**       are derived from 42's AppReadFromSocket.c and AppWriteToSocket.c
**       files. These files are created by a code generation tool. For
**       each bc42_lib release these functions are copied from the 42 
**       source files and then hand modified to work with the OSAL
**       socket API. All modified code is annotated with a '~bc42~' comment. 
**
*/

/*
** Include Files:
*/

#include "cfe.h"
#include "bc42.h"

static void InitParameters(struct AcType *Ac);

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
   
   /* Dynamics */
   Bc42.AcVar.Nb = BC42_NB;
   Bc42.AcVar.B  = &Bc42.BcVar.B[0];
 
   Bc42.AcVar.Ng = BC42_NG;
   Bc42.AcVar.G  = &Bc42.BcVar.G[0];

   /* Sensors */
   Bc42.AcVar.Ngyro = BC42_NGYRO;
   Bc42.AcVar.Gyro  = &Bc42.BcVar.Gyro[0];

   Bc42.AcVar.Nmag  = BC42_NMAG;
   Bc42.AcVar.MAG   = &Bc42.BcVar.MAG[0];

   Bc42.AcVar.Ncss  = BC42_NCSS;
   Bc42.AcVar.CSS   = &Bc42.BcVar.CSS[0];

   Bc42.AcVar.Nfss  = BC42_NFSS;
   Bc42.AcVar.FSS   = &Bc42.BcVar.FSS[0];

   Bc42.AcVar.Nst   = BC42_NST;
   Bc42.AcVar.ST    = &Bc42.BcVar.ST[0];

   Bc42.AcVar.Ngps  = BC42_NGPS;
   Bc42.AcVar.GPS   = &Bc42.BcVar.GPS[0];

   /* Actuators */

   Bc42.AcVar.Nwhl = BC42_NWHL;
   Bc42.AcVar.Whl  = &Bc42.BcVar.Whl[0];

   Bc42.AcVar.Nmtb = BC42_NMTB;
   Bc42.AcVar.MTB  = &Bc42.BcVar.MTB[0];

   Bc42.AcVar.Nthr = BC42_NTHR;
   Bc42.AcVar.Thr  = &Bc42.BcVar.Thr[0];

   InitParameters(&(Bc42.AcVar));

   return CfeStatus;

} /* End BC42_Constructor() */


/******************************************************************************
** Function: BC42_GivePtr
**
*/
void BC42_GivePtr(BC42_Class_t *Bc)
{

   OS_MutSemGive(Bc->MutexId);

} /* End BC42_GivePtr() */


/******************************************************************************
** Function: BC42_ReadFromSocket
**
** Note:
**    1. Code copy from AppReadFromSocket.c
*/
int32 BC42_ReadFromSocket(osal_id_t SocketId, OS_SockAddr_t *RemoteAddr, struct AcType *AC)
{

      //~bc~ Unused: struct SCType *S;
      //~bc~ Unused: struct OrbitType *O;
      //~bc~ Unused: struct DynType *D;
      long Isc,i;  //~bc~ Unused: Iorb,Iw
      char line[512] = "Blank";
      long RequestTimeRefresh = 0;
      long Done;
      char Msg[16384];
      char AckMsg[5] = "Ack\n";
      long Imsg,Iline;
      int NumBytes;
      double DbleVal[30];
      long LongVal[30];

      long Year,doy,Hour,Minute;
      double Second;
      long Month,Day;
      
      memset(Msg,'\0',16384);
      NumBytes = OS_read(SocketId, Msg, 16384);
      //~bc~ NumBytes = OS_SocketRecvFrom(SocketId, Msg, 16384, RemoteAddr, OS_PEND); 

      CFE_EVS_SendEvent(999, CFE_EVS_EventType_INFORMATION, "**** BC42_ReadFromSocket() read %d bytes", NumBytes);
  
      //~bc~ if (NumBytes <= 0) return; /* Bail out if no message */
      if (NumBytes <= 0) return -1; //~bc~

      Done = 0;
      Imsg = 0;
      while(!Done) {
         /* Parse lines from Msg, newline-delimited */
         Iline = 0;
         memset(line,'\0',512);
         while((Msg[Imsg] != '\n') && (Iline < 511) && (Imsg < 16383)) {
            line[Iline++] = Msg[Imsg++];
         }
         line[Iline++] = Msg[Imsg++];
         if (AC->EchoEnabled) printf("%s",line);

         if (sscanf(line,"TIME %ld-%ld-%ld:%ld:%lf\n",
            &Year,&doy,&Hour,&Minute,&Second) == 5) {
            RequestTimeRefresh = 1;
         }

         if (sscanf(line,"SC[%ld].AC.ParmLoadEnabled = %ld",
            &Isc,
            &LongVal[0]) == 2) {
            if (Isc == AC->ID) {
               AC->ParmLoadEnabled = LongVal[0];
            }
         }

         if (sscanf(line,"SC[%ld].AC.ParmDumpEnabled = %ld",
            &Isc,
            &LongVal[0]) == 2) {
            if (Isc == AC->ID) {
               AC->ParmDumpEnabled = LongVal[0];
            }
         }

         if (sscanf(line,"SC[%ld].AC.G[%ld].Ang = %le %le %le",
            &Isc,&i,
            &DbleVal[0],
            &DbleVal[1],
            &DbleVal[2]) == 5) {
            if (Isc == AC->ID) {
               AC->G[i].Ang[0] = DbleVal[0];
               AC->G[i].Ang[1] = DbleVal[1];
               AC->G[i].Ang[2] = DbleVal[2];
            }
         }

         if (sscanf(line,"SC[%ld].AC.Gyro[%ld].Rate = %le",
            &Isc,&i,
            &DbleVal[0]) == 3) {
            if (Isc == AC->ID) {
               AC->Gyro[i].Rate = DbleVal[0];
            }
         }

         if (sscanf(line,"SC[%ld].AC.MAG[%ld].Field = %le",
            &Isc,&i,
            &DbleVal[0]) == 3) {
            if (Isc == AC->ID) {
               AC->MAG[i].Field = DbleVal[0];
            }
         }

         if (sscanf(line,"SC[%ld].AC.CSS[%ld].Valid = %ld",
            &Isc,&i,
            &LongVal[0]) == 3) {
            if (Isc == AC->ID) {
               AC->CSS[i].Valid = LongVal[0];
            }
         }

         if (sscanf(line,"SC[%ld].AC.CSS[%ld].Illum = %le",
            &Isc,&i,
            &DbleVal[0]) == 3) {
            if (Isc == AC->ID) {
               AC->CSS[i].Illum = DbleVal[0];
            }
         }

         if (sscanf(line,"SC[%ld].AC.FSS[%ld].Valid = %ld",
            &Isc,&i,
            &LongVal[0]) == 3) {
            if (Isc == AC->ID) {
               AC->FSS[i].Valid = LongVal[0];
            }
         }

         if (sscanf(line,"SC[%ld].AC.FSS[%ld].SunAng = %le %le",
            &Isc,&i,
            &DbleVal[0],
            &DbleVal[1]) == 4) {
            if (Isc == AC->ID) {
               AC->FSS[i].SunAng[0] = DbleVal[0];
               AC->FSS[i].SunAng[1] = DbleVal[1];
            }
         }

         if (sscanf(line,"SC[%ld].AC.ST[%ld].Valid = %ld",
            &Isc,&i,
            &LongVal[0]) == 3) {
            if (Isc == AC->ID) {
               AC->ST[i].Valid = LongVal[0];
            }
         }

         if (sscanf(line,"SC[%ld].AC.ST[%ld].qn = %le %le %le %le",
            &Isc,&i,
            &DbleVal[0],
            &DbleVal[1],
            &DbleVal[2],
            &DbleVal[3]) == 6) {
            if (Isc == AC->ID) {
               AC->ST[i].qn[0] = DbleVal[0];
               AC->ST[i].qn[1] = DbleVal[1];
               AC->ST[i].qn[2] = DbleVal[2];
               AC->ST[i].qn[3] = DbleVal[3];
            }
         }

         if (sscanf(line,"SC[%ld].AC.GPS[%ld].Valid = %ld",
            &Isc,&i,
            &LongVal[0]) == 3) {
            if (Isc == AC->ID) {
               AC->GPS[i].Valid = LongVal[0];
            }
         }

         if (sscanf(line,"SC[%ld].AC.GPS[%ld].Rollover = %ld",
            &Isc,&i,
            &LongVal[0]) == 3) {
            if (Isc == AC->ID) {
               AC->GPS[i].Rollover = LongVal[0];
            }
         }

         if (sscanf(line,"SC[%ld].AC.GPS[%ld].Week = %ld",
            &Isc,&i,
            &LongVal[0]) == 3) {
            if (Isc == AC->ID) {
               AC->GPS[i].Week = LongVal[0];
            }
         }

         if (sscanf(line,"SC[%ld].AC.GPS[%ld].Sec = %le",
            &Isc,&i,
            &DbleVal[0]) == 3) {
            if (Isc == AC->ID) {
               AC->GPS[i].Sec = DbleVal[0];
            }
         }

         if (sscanf(line,"SC[%ld].AC.GPS[%ld].PosN = %le %le %le",
            &Isc,&i,
            &DbleVal[0],
            &DbleVal[1],
            &DbleVal[2]) == 5) {
            if (Isc == AC->ID) {
               AC->GPS[i].PosN[0] = DbleVal[0];
               AC->GPS[i].PosN[1] = DbleVal[1];
               AC->GPS[i].PosN[2] = DbleVal[2];
            }
         }

         if (sscanf(line,"SC[%ld].AC.GPS[%ld].VelN = %le %le %le",
            &Isc,&i,
            &DbleVal[0],
            &DbleVal[1],
            &DbleVal[2]) == 5) {
            if (Isc == AC->ID) {
               AC->GPS[i].VelN[0] = DbleVal[0];
               AC->GPS[i].VelN[1] = DbleVal[1];
               AC->GPS[i].VelN[2] = DbleVal[2];
            }
         }

         if (sscanf(line,"SC[%ld].AC.GPS[%ld].PosW = %le %le %le",
            &Isc,&i,
            &DbleVal[0],
            &DbleVal[1],
            &DbleVal[2]) == 5) {
            if (Isc == AC->ID) {
               AC->GPS[i].PosW[0] = DbleVal[0];
               AC->GPS[i].PosW[1] = DbleVal[1];
               AC->GPS[i].PosW[2] = DbleVal[2];
            }
         }

         if (sscanf(line,"SC[%ld].AC.GPS[%ld].VelW = %le %le %le",
            &Isc,&i,
            &DbleVal[0],
            &DbleVal[1],
            &DbleVal[2]) == 5) {
            if (Isc == AC->ID) {
               AC->GPS[i].VelW[0] = DbleVal[0];
               AC->GPS[i].VelW[1] = DbleVal[1];
               AC->GPS[i].VelW[2] = DbleVal[2];
            }
         }

         if (sscanf(line,"SC[%ld].AC.GPS[%ld].Lng = %le",
            &Isc,&i,
            &DbleVal[0]) == 3) {
            if (Isc == AC->ID) {
               AC->GPS[i].Lng = DbleVal[0];
            }
         }

         if (sscanf(line,"SC[%ld].AC.GPS[%ld].Lat = %le",
            &Isc,&i,
            &DbleVal[0]) == 3) {
            if (Isc == AC->ID) {
               AC->GPS[i].Lat = DbleVal[0];
            }
         }

         if (sscanf(line,"SC[%ld].AC.GPS[%ld].Alt = %le",
            &Isc,&i,
            &DbleVal[0]) == 3) {
            if (Isc == AC->ID) {
               AC->GPS[i].Alt = DbleVal[0];
            }
         }

         if (sscanf(line,"SC[%ld].AC.GPS[%ld].WgsLng = %le",
            &Isc,&i,
            &DbleVal[0]) == 3) {
            if (Isc == AC->ID) {
               AC->GPS[i].WgsLng = DbleVal[0];
            }
         }

         if (sscanf(line,"SC[%ld].AC.GPS[%ld].WgsLat = %le",
            &Isc,&i,
            &DbleVal[0]) == 3) {
            if (Isc == AC->ID) {
               AC->GPS[i].WgsLat = DbleVal[0];
            }
         }

         if (sscanf(line,"SC[%ld].AC.GPS[%ld].WgsAlt = %le",
            &Isc,&i,
            &DbleVal[0]) == 3) {
            if (Isc == AC->ID) {
               AC->GPS[i].WgsAlt = DbleVal[0];
            }
         }

         if (sscanf(line,"SC[%ld].AC.Accel[%ld].Acc = %le",
            &Isc,&i,
            &DbleVal[0]) == 3) {
            if (Isc == AC->ID) {
               AC->Accel[i].Acc = DbleVal[0];
            }
         }

         if (sscanf(line,"SC[%ld].AC.Whl[%ld].H = %le",
            &Isc,&i,
            &DbleVal[0]) == 3) {
            if (Isc == AC->ID) {
               AC->Whl[i].H = DbleVal[0];
            }
         }

         if (AC->ParmLoadEnabled) {
            if (sscanf(line,"SC[%ld].AC.ID = %ld",
               &Isc,
               &LongVal[0]) == 2) {
               if (Isc == AC->ID) {
                  AC->ID = LongVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.EchoEnabled = %ld",
               &Isc,
               &LongVal[0]) == 2) {
               if (Isc == AC->ID) {
                  AC->EchoEnabled = LongVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.Nb = %ld",
               &Isc,
               &LongVal[0]) == 2) {
               if (Isc == AC->ID) {
                  AC->Nb = LongVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.Ng = %ld",
               &Isc,
               &LongVal[0]) == 2) {
               if (Isc == AC->ID) {
                  AC->Ng = LongVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.Nwhl = %ld",
               &Isc,
               &LongVal[0]) == 2) {
               if (Isc == AC->ID) {
                  AC->Nwhl = LongVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.Nmtb = %ld",
               &Isc,
               &LongVal[0]) == 2) {
               if (Isc == AC->ID) {
                  AC->Nmtb = LongVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.Nthr = %ld",
               &Isc,
               &LongVal[0]) == 2) {
               if (Isc == AC->ID) {
                  AC->Nthr = LongVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.Ncmg = %ld",
               &Isc,
               &LongVal[0]) == 2) {
               if (Isc == AC->ID) {
                  AC->Ncmg = LongVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.Ngyro = %ld",
               &Isc,
               &LongVal[0]) == 2) {
               if (Isc == AC->ID) {
                  AC->Ngyro = LongVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.Nmag = %ld",
               &Isc,
               &LongVal[0]) == 2) {
               if (Isc == AC->ID) {
                  AC->Nmag = LongVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.Ncss = %ld",
               &Isc,
               &LongVal[0]) == 2) {
               if (Isc == AC->ID) {
                  AC->Ncss = LongVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.Nfss = %ld",
               &Isc,
               &LongVal[0]) == 2) {
               if (Isc == AC->ID) {
                  AC->Nfss = LongVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.Nst = %ld",
               &Isc,
               &LongVal[0]) == 2) {
               if (Isc == AC->ID) {
                  AC->Nst = LongVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.Ngps = %ld",
               &Isc,
               &LongVal[0]) == 2) {
               if (Isc == AC->ID) {
                  AC->Ngps = LongVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.Nacc = %ld",
               &Isc,
               &LongVal[0]) == 2) {
               if (Isc == AC->ID) {
                  AC->Nacc = LongVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.Pi = %le",
               &Isc,
               &DbleVal[0]) == 2) {
               if (Isc == AC->ID) {
                  AC->Pi = DbleVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.TwoPi = %le",
               &Isc,
               &DbleVal[0]) == 2) {
               if (Isc == AC->ID) {
                  AC->TwoPi = DbleVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.DT = %le",
               &Isc,
               &DbleVal[0]) == 2) {
               if (Isc == AC->ID) {
                  AC->DT = DbleVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.mass = %le",
               &Isc,
               &DbleVal[0]) == 2) {
               if (Isc == AC->ID) {
                  AC->mass = DbleVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.cm = %le %le %le",
               &Isc,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 4) {
               if (Isc == AC->ID) {
                  AC->cm[0] = DbleVal[0];
                  AC->cm[1] = DbleVal[1];
                  AC->cm[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.MOI = %le %le %le %le %le %le %le %le %le",
               &Isc,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2],
               &DbleVal[3],
               &DbleVal[4],
               &DbleVal[5],
               &DbleVal[6],
               &DbleVal[7],
               &DbleVal[8]) == 10) {
               if (Isc == AC->ID) {
                  AC->MOI[0][0] = DbleVal[0];
                  AC->MOI[0][1] = DbleVal[1];
                  AC->MOI[0][2] = DbleVal[2];
                  AC->MOI[1][0] = DbleVal[3];
                  AC->MOI[1][1] = DbleVal[4];
                  AC->MOI[1][2] = DbleVal[5];
                  AC->MOI[2][0] = DbleVal[6];
                  AC->MOI[2][1] = DbleVal[7];
                  AC->MOI[2][2] = DbleVal[8];
               }
            }

            if (sscanf(line,"SC[%ld].AC.B[%ld].mass = %le",
               &Isc,&i,
               &DbleVal[0]) == 3) {
               if (Isc == AC->ID) {
                  AC->B[i].mass = DbleVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.B[%ld].cm = %le %le %le",
               &Isc,&i,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 5) {
               if (Isc == AC->ID) {
                  AC->B[i].cm[0] = DbleVal[0];
                  AC->B[i].cm[1] = DbleVal[1];
                  AC->B[i].cm[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.B[%ld].MOI = %le %le %le %le %le %le %le %le %le",
               &Isc,&i,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2],
               &DbleVal[3],
               &DbleVal[4],
               &DbleVal[5],
               &DbleVal[6],
               &DbleVal[7],
               &DbleVal[8]) == 11) {
               if (Isc == AC->ID) {
                  AC->B[i].MOI[0][0] = DbleVal[0];
                  AC->B[i].MOI[0][1] = DbleVal[1];
                  AC->B[i].MOI[0][2] = DbleVal[2];
                  AC->B[i].MOI[1][0] = DbleVal[3];
                  AC->B[i].MOI[1][1] = DbleVal[4];
                  AC->B[i].MOI[1][2] = DbleVal[5];
                  AC->B[i].MOI[2][0] = DbleVal[6];
                  AC->B[i].MOI[2][1] = DbleVal[7];
                  AC->B[i].MOI[2][2] = DbleVal[8];
               }
            }

            if (sscanf(line,"SC[%ld].AC.G[%ld].IsSpherical = %ld",
               &Isc,&i,
               &LongVal[0]) == 3) {
               if (Isc == AC->ID) {
                  AC->G[i].IsSpherical = LongVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.G[%ld].RotDOF = %ld",
               &Isc,&i,
               &LongVal[0]) == 3) {
               if (Isc == AC->ID) {
                  AC->G[i].RotDOF = LongVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.G[%ld].TrnDOF = %ld",
               &Isc,&i,
               &LongVal[0]) == 3) {
               if (Isc == AC->ID) {
                  AC->G[i].TrnDOF = LongVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.G[%ld].RotSeq = %ld",
               &Isc,&i,
               &LongVal[0]) == 3) {
               if (Isc == AC->ID) {
                  AC->G[i].RotSeq = LongVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.G[%ld].TrnSeq = %ld",
               &Isc,&i,
               &LongVal[0]) == 3) {
               if (Isc == AC->ID) {
                  AC->G[i].TrnSeq = LongVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.G[%ld].CGiBi = %le %le %le %le %le %le %le %le %le",
               &Isc,&i,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2],
               &DbleVal[3],
               &DbleVal[4],
               &DbleVal[5],
               &DbleVal[6],
               &DbleVal[7],
               &DbleVal[8]) == 11) {
               if (Isc == AC->ID) {
                  AC->G[i].CGiBi[0][0] = DbleVal[0];
                  AC->G[i].CGiBi[0][1] = DbleVal[1];
                  AC->G[i].CGiBi[0][2] = DbleVal[2];
                  AC->G[i].CGiBi[1][0] = DbleVal[3];
                  AC->G[i].CGiBi[1][1] = DbleVal[4];
                  AC->G[i].CGiBi[1][2] = DbleVal[5];
                  AC->G[i].CGiBi[2][0] = DbleVal[6];
                  AC->G[i].CGiBi[2][1] = DbleVal[7];
                  AC->G[i].CGiBi[2][2] = DbleVal[8];
               }
            }

            if (sscanf(line,"SC[%ld].AC.G[%ld].CBoGo = %le %le %le %le %le %le %le %le %le",
               &Isc,&i,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2],
               &DbleVal[3],
               &DbleVal[4],
               &DbleVal[5],
               &DbleVal[6],
               &DbleVal[7],
               &DbleVal[8]) == 11) {
               if (Isc == AC->ID) {
                  AC->G[i].CBoGo[0][0] = DbleVal[0];
                  AC->G[i].CBoGo[0][1] = DbleVal[1];
                  AC->G[i].CBoGo[0][2] = DbleVal[2];
                  AC->G[i].CBoGo[1][0] = DbleVal[3];
                  AC->G[i].CBoGo[1][1] = DbleVal[4];
                  AC->G[i].CBoGo[1][2] = DbleVal[5];
                  AC->G[i].CBoGo[2][0] = DbleVal[6];
                  AC->G[i].CBoGo[2][1] = DbleVal[7];
                  AC->G[i].CBoGo[2][2] = DbleVal[8];
               }
            }

            if (sscanf(line,"SC[%ld].AC.G[%ld].AngGain = %le %le %le",
               &Isc,&i,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 5) {
               if (Isc == AC->ID) {
                  AC->G[i].AngGain[0] = DbleVal[0];
                  AC->G[i].AngGain[1] = DbleVal[1];
                  AC->G[i].AngGain[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.G[%ld].AngRateGain = %le %le %le",
               &Isc,&i,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 5) {
               if (Isc == AC->ID) {
                  AC->G[i].AngRateGain[0] = DbleVal[0];
                  AC->G[i].AngRateGain[1] = DbleVal[1];
                  AC->G[i].AngRateGain[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.G[%ld].PosGain = %le %le %le",
               &Isc,&i,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 5) {
               if (Isc == AC->ID) {
                  AC->G[i].PosGain[0] = DbleVal[0];
                  AC->G[i].PosGain[1] = DbleVal[1];
                  AC->G[i].PosGain[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.G[%ld].PosRateGain = %le %le %le",
               &Isc,&i,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 5) {
               if (Isc == AC->ID) {
                  AC->G[i].PosRateGain[0] = DbleVal[0];
                  AC->G[i].PosRateGain[1] = DbleVal[1];
                  AC->G[i].PosRateGain[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.G[%ld].MaxAngRate = %le %le %le",
               &Isc,&i,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 5) {
               if (Isc == AC->ID) {
                  AC->G[i].MaxAngRate[0] = DbleVal[0];
                  AC->G[i].MaxAngRate[1] = DbleVal[1];
                  AC->G[i].MaxAngRate[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.G[%ld].MaxPosRate = %le %le %le",
               &Isc,&i,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 5) {
               if (Isc == AC->ID) {
                  AC->G[i].MaxPosRate[0] = DbleVal[0];
                  AC->G[i].MaxPosRate[1] = DbleVal[1];
                  AC->G[i].MaxPosRate[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.G[%ld].MaxTrq = %le %le %le",
               &Isc,&i,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 5) {
               if (Isc == AC->ID) {
                  AC->G[i].MaxTrq[0] = DbleVal[0];
                  AC->G[i].MaxTrq[1] = DbleVal[1];
                  AC->G[i].MaxTrq[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.G[%ld].MaxFrc = %le %le %le",
               &Isc,&i,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 5) {
               if (Isc == AC->ID) {
                  AC->G[i].MaxFrc[0] = DbleVal[0];
                  AC->G[i].MaxFrc[1] = DbleVal[1];
                  AC->G[i].MaxFrc[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.Gyro[%ld].Axis = %le %le %le",
               &Isc,&i,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 5) {
               if (Isc == AC->ID) {
                  AC->Gyro[i].Axis[0] = DbleVal[0];
                  AC->Gyro[i].Axis[1] = DbleVal[1];
                  AC->Gyro[i].Axis[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.MAG[%ld].Axis = %le %le %le",
               &Isc,&i,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 5) {
               if (Isc == AC->ID) {
                  AC->MAG[i].Axis[0] = DbleVal[0];
                  AC->MAG[i].Axis[1] = DbleVal[1];
                  AC->MAG[i].Axis[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.CSS[%ld].Body = %ld",
               &Isc,&i,
               &LongVal[0]) == 3) {
               if (Isc == AC->ID) {
                  AC->CSS[i].Body = LongVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.CSS[%ld].Axis = %le %le %le",
               &Isc,&i,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 5) {
               if (Isc == AC->ID) {
                  AC->CSS[i].Axis[0] = DbleVal[0];
                  AC->CSS[i].Axis[1] = DbleVal[1];
                  AC->CSS[i].Axis[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.CSS[%ld].Scale = %le",
               &Isc,&i,
               &DbleVal[0]) == 3) {
               if (Isc == AC->ID) {
                  AC->CSS[i].Scale = DbleVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.FSS[%ld].qb = %le %le %le %le",
               &Isc,&i,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2],
               &DbleVal[3]) == 6) {
               if (Isc == AC->ID) {
                  AC->FSS[i].qb[0] = DbleVal[0];
                  AC->FSS[i].qb[1] = DbleVal[1];
                  AC->FSS[i].qb[2] = DbleVal[2];
                  AC->FSS[i].qb[3] = DbleVal[3];
               }
            }

            if (sscanf(line,"SC[%ld].AC.FSS[%ld].CB = %le %le %le %le %le %le %le %le %le",
               &Isc,&i,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2],
               &DbleVal[3],
               &DbleVal[4],
               &DbleVal[5],
               &DbleVal[6],
               &DbleVal[7],
               &DbleVal[8]) == 11) {
               if (Isc == AC->ID) {
                  AC->FSS[i].CB[0][0] = DbleVal[0];
                  AC->FSS[i].CB[0][1] = DbleVal[1];
                  AC->FSS[i].CB[0][2] = DbleVal[2];
                  AC->FSS[i].CB[1][0] = DbleVal[3];
                  AC->FSS[i].CB[1][1] = DbleVal[4];
                  AC->FSS[i].CB[1][2] = DbleVal[5];
                  AC->FSS[i].CB[2][0] = DbleVal[6];
                  AC->FSS[i].CB[2][1] = DbleVal[7];
                  AC->FSS[i].CB[2][2] = DbleVal[8];
               }
            }

            if (sscanf(line,"SC[%ld].AC.ST[%ld].qb = %le %le %le %le",
               &Isc,&i,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2],
               &DbleVal[3]) == 6) {
               if (Isc == AC->ID) {
                  AC->ST[i].qb[0] = DbleVal[0];
                  AC->ST[i].qb[1] = DbleVal[1];
                  AC->ST[i].qb[2] = DbleVal[2];
                  AC->ST[i].qb[3] = DbleVal[3];
               }
            }

            if (sscanf(line,"SC[%ld].AC.ST[%ld].CB = %le %le %le %le %le %le %le %le %le",
               &Isc,&i,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2],
               &DbleVal[3],
               &DbleVal[4],
               &DbleVal[5],
               &DbleVal[6],
               &DbleVal[7],
               &DbleVal[8]) == 11) {
               if (Isc == AC->ID) {
                  AC->ST[i].CB[0][0] = DbleVal[0];
                  AC->ST[i].CB[0][1] = DbleVal[1];
                  AC->ST[i].CB[0][2] = DbleVal[2];
                  AC->ST[i].CB[1][0] = DbleVal[3];
                  AC->ST[i].CB[1][1] = DbleVal[4];
                  AC->ST[i].CB[1][2] = DbleVal[5];
                  AC->ST[i].CB[2][0] = DbleVal[6];
                  AC->ST[i].CB[2][1] = DbleVal[7];
                  AC->ST[i].CB[2][2] = DbleVal[8];
               }
            }

            if (sscanf(line,"SC[%ld].AC.Accel[%ld].PosB = %le %le %le",
               &Isc,&i,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 5) {
               if (Isc == AC->ID) {
                  AC->Accel[i].PosB[0] = DbleVal[0];
                  AC->Accel[i].PosB[1] = DbleVal[1];
                  AC->Accel[i].PosB[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.Accel[%ld].Axis = %le %le %le",
               &Isc,&i,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 5) {
               if (Isc == AC->ID) {
                  AC->Accel[i].Axis[0] = DbleVal[0];
                  AC->Accel[i].Axis[1] = DbleVal[1];
                  AC->Accel[i].Axis[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.Whl[%ld].Body = %ld",
               &Isc,&i,
               &LongVal[0]) == 3) {
               if (Isc == AC->ID) {
                  AC->Whl[i].Body = LongVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.Whl[%ld].Axis = %le %le %le",
               &Isc,&i,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 5) {
               if (Isc == AC->ID) {
                  AC->Whl[i].Axis[0] = DbleVal[0];
                  AC->Whl[i].Axis[1] = DbleVal[1];
                  AC->Whl[i].Axis[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.Whl[%ld].DistVec = %le %le %le",
               &Isc,&i,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 5) {
               if (Isc == AC->ID) {
                  AC->Whl[i].DistVec[0] = DbleVal[0];
                  AC->Whl[i].DistVec[1] = DbleVal[1];
                  AC->Whl[i].DistVec[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.Whl[%ld].J = %le",
               &Isc,&i,
               &DbleVal[0]) == 3) {
               if (Isc == AC->ID) {
                  AC->Whl[i].J = DbleVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.Whl[%ld].Tmax = %le",
               &Isc,&i,
               &DbleVal[0]) == 3) {
               if (Isc == AC->ID) {
                  AC->Whl[i].Tmax = DbleVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.Whl[%ld].Hmax = %le",
               &Isc,&i,
               &DbleVal[0]) == 3) {
               if (Isc == AC->ID) {
                  AC->Whl[i].Hmax = DbleVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.MTB[%ld].Axis = %le %le %le",
               &Isc,&i,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 5) {
               if (Isc == AC->ID) {
                  AC->MTB[i].Axis[0] = DbleVal[0];
                  AC->MTB[i].Axis[1] = DbleVal[1];
                  AC->MTB[i].Axis[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.MTB[%ld].DistVec = %le %le %le",
               &Isc,&i,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 5) {
               if (Isc == AC->ID) {
                  AC->MTB[i].DistVec[0] = DbleVal[0];
                  AC->MTB[i].DistVec[1] = DbleVal[1];
                  AC->MTB[i].DistVec[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.MTB[%ld].Mmax = %le",
               &Isc,&i,
               &DbleVal[0]) == 3) {
               if (Isc == AC->ID) {
                  AC->MTB[i].Mmax = DbleVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.Thr[%ld].Body = %ld",
               &Isc,&i,
               &LongVal[0]) == 3) {
               if (Isc == AC->ID) {
                  AC->Thr[i].Body = LongVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.Thr[%ld].PosB = %le %le %le",
               &Isc,&i,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 5) {
               if (Isc == AC->ID) {
                  AC->Thr[i].PosB[0] = DbleVal[0];
                  AC->Thr[i].PosB[1] = DbleVal[1];
                  AC->Thr[i].PosB[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.Thr[%ld].Axis = %le %le %le",
               &Isc,&i,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 5) {
               if (Isc == AC->ID) {
                  AC->Thr[i].Axis[0] = DbleVal[0];
                  AC->Thr[i].Axis[1] = DbleVal[1];
                  AC->Thr[i].Axis[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.Thr[%ld].rxA = %le %le %le",
               &Isc,&i,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 5) {
               if (Isc == AC->ID) {
                  AC->Thr[i].rxA[0] = DbleVal[0];
                  AC->Thr[i].rxA[1] = DbleVal[1];
                  AC->Thr[i].rxA[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.Thr[%ld].Fmax = %le",
               &Isc,&i,
               &DbleVal[0]) == 3) {
               if (Isc == AC->ID) {
                  AC->Thr[i].Fmax = DbleVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.PrototypeCtrl.wc = %le",
               &Isc,
               &DbleVal[0]) == 2) {
               if (Isc == AC->ID) {
                  AC->PrototypeCtrl.wc = DbleVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.PrototypeCtrl.amax = %le",
               &Isc,
               &DbleVal[0]) == 2) {
               if (Isc == AC->ID) {
                  AC->PrototypeCtrl.amax = DbleVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.PrototypeCtrl.vmax = %le",
               &Isc,
               &DbleVal[0]) == 2) {
               if (Isc == AC->ID) {
                  AC->PrototypeCtrl.vmax = DbleVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.PrototypeCtrl.Kprec = %le",
               &Isc,
               &DbleVal[0]) == 2) {
               if (Isc == AC->ID) {
                  AC->PrototypeCtrl.Kprec = DbleVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.PrototypeCtrl.Knute = %le",
               &Isc,
               &DbleVal[0]) == 2) {
               if (Isc == AC->ID) {
                  AC->PrototypeCtrl.Knute = DbleVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.AdHocCtrl.Kr = %le %le %le",
               &Isc,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 4) {
               if (Isc == AC->ID) {
                  AC->AdHocCtrl.Kr[0] = DbleVal[0];
                  AC->AdHocCtrl.Kr[1] = DbleVal[1];
                  AC->AdHocCtrl.Kr[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.AdHocCtrl.Kp = %le %le %le",
               &Isc,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 4) {
               if (Isc == AC->ID) {
                  AC->AdHocCtrl.Kp[0] = DbleVal[0];
                  AC->AdHocCtrl.Kp[1] = DbleVal[1];
                  AC->AdHocCtrl.Kp[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.SpinnerCtrl.Ispin = %le",
               &Isc,
               &DbleVal[0]) == 2) {
               if (Isc == AC->ID) {
                  AC->SpinnerCtrl.Ispin = DbleVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.SpinnerCtrl.Itrans = %le",
               &Isc,
               &DbleVal[0]) == 2) {
               if (Isc == AC->ID) {
                  AC->SpinnerCtrl.Itrans = DbleVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.SpinnerCtrl.SpinRate = %le",
               &Isc,
               &DbleVal[0]) == 2) {
               if (Isc == AC->ID) {
                  AC->SpinnerCtrl.SpinRate = DbleVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.SpinnerCtrl.Knute = %le",
               &Isc,
               &DbleVal[0]) == 2) {
               if (Isc == AC->ID) {
                  AC->SpinnerCtrl.Knute = DbleVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.SpinnerCtrl.Kprec = %le",
               &Isc,
               &DbleVal[0]) == 2) {
               if (Isc == AC->ID) {
                  AC->SpinnerCtrl.Kprec = DbleVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.ThreeAxisCtrl.Kr = %le %le %le",
               &Isc,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 4) {
               if (Isc == AC->ID) {
                  AC->ThreeAxisCtrl.Kr[0] = DbleVal[0];
                  AC->ThreeAxisCtrl.Kr[1] = DbleVal[1];
                  AC->ThreeAxisCtrl.Kr[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.ThreeAxisCtrl.Kp = %le %le %le",
               &Isc,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 4) {
               if (Isc == AC->ID) {
                  AC->ThreeAxisCtrl.Kp[0] = DbleVal[0];
                  AC->ThreeAxisCtrl.Kp[1] = DbleVal[1];
                  AC->ThreeAxisCtrl.Kp[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.ThreeAxisCtrl.Kunl = %le",
               &Isc,
               &DbleVal[0]) == 2) {
               if (Isc == AC->ID) {
                  AC->ThreeAxisCtrl.Kunl = DbleVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.IssCtrl.Kr = %le %le %le",
               &Isc,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 4) {
               if (Isc == AC->ID) {
                  AC->IssCtrl.Kr[0] = DbleVal[0];
                  AC->IssCtrl.Kr[1] = DbleVal[1];
                  AC->IssCtrl.Kr[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.IssCtrl.Kp = %le %le %le",
               &Isc,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 4) {
               if (Isc == AC->ID) {
                  AC->IssCtrl.Kp[0] = DbleVal[0];
                  AC->IssCtrl.Kp[1] = DbleVal[1];
                  AC->IssCtrl.Kp[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.IssCtrl.Tmax = %le",
               &Isc,
               &DbleVal[0]) == 2) {
               if (Isc == AC->ID) {
                  AC->IssCtrl.Tmax = DbleVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.CmgCtrl.Kr = %le %le %le",
               &Isc,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 4) {
               if (Isc == AC->ID) {
                  AC->CmgCtrl.Kr[0] = DbleVal[0];
                  AC->CmgCtrl.Kr[1] = DbleVal[1];
                  AC->CmgCtrl.Kr[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.CmgCtrl.Kp = %le %le %le",
               &Isc,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 4) {
               if (Isc == AC->ID) {
                  AC->CmgCtrl.Kp[0] = DbleVal[0];
                  AC->CmgCtrl.Kp[1] = DbleVal[1];
                  AC->CmgCtrl.Kp[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.ThrCtrl.Kw = %le %le %le",
               &Isc,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 4) {
               if (Isc == AC->ID) {
                  AC->ThrCtrl.Kw[0] = DbleVal[0];
                  AC->ThrCtrl.Kw[1] = DbleVal[1];
                  AC->ThrCtrl.Kw[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.ThrCtrl.Kth = %le %le %le",
               &Isc,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 4) {
               if (Isc == AC->ID) {
                  AC->ThrCtrl.Kth[0] = DbleVal[0];
                  AC->ThrCtrl.Kth[1] = DbleVal[1];
                  AC->ThrCtrl.Kth[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.ThrCtrl.Kv = %le",
               &Isc,
               &DbleVal[0]) == 2) {
               if (Isc == AC->ID) {
                  AC->ThrCtrl.Kv = DbleVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.ThrCtrl.Kp = %le",
               &Isc,
               &DbleVal[0]) == 2) {
               if (Isc == AC->ID) {
                  AC->ThrCtrl.Kp = DbleVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.CfsCtrl.Kr = %le %le %le",
               &Isc,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 4) {
               if (Isc == AC->ID) {
                  AC->CfsCtrl.Kr[0] = DbleVal[0];
                  AC->CfsCtrl.Kr[1] = DbleVal[1];
                  AC->CfsCtrl.Kr[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.CfsCtrl.Kp = %le %le %le",
               &Isc,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 4) {
               if (Isc == AC->ID) {
                  AC->CfsCtrl.Kp[0] = DbleVal[0];
                  AC->CfsCtrl.Kp[1] = DbleVal[1];
                  AC->CfsCtrl.Kp[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.CfsCtrl.Kunl = %le",
               &Isc,
               &DbleVal[0]) == 2) {
               if (Isc == AC->ID) {
                  AC->CfsCtrl.Kunl = DbleVal[0];
               }
            }

            if (sscanf(line,"SC[%ld].AC.ThrSteerCtrl.Kr = %le %le %le",
               &Isc,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 4) {
               if (Isc == AC->ID) {
                  AC->ThrSteerCtrl.Kr[0] = DbleVal[0];
                  AC->ThrSteerCtrl.Kr[1] = DbleVal[1];
                  AC->ThrSteerCtrl.Kr[2] = DbleVal[2];
               }
            }

            if (sscanf(line,"SC[%ld].AC.ThrSteerCtrl.Kp = %le %le %le",
               &Isc,
               &DbleVal[0],
               &DbleVal[1],
               &DbleVal[2]) == 4) {
               if (Isc == AC->ID) {
                  AC->ThrSteerCtrl.Kp[0] = DbleVal[0];
                  AC->ThrSteerCtrl.Kp[1] = DbleVal[1];
                  AC->ThrSteerCtrl.Kp[2] = DbleVal[2];
               }
            }

         }


         if (!strncmp(line,"[EOF]",5)) {
            Done = 1;
            sprintf(line,"[EOF] reached\n");
         }
         if (Imsg >= 16383) {
            Done = 1;
            printf("Imsg limit exceeded\n");
         }
      }

      /* Acknowledge receipt */
      //~bc~ send(Socket,AckMsg,strlen(AckMsg),0);
      OS_SocketSendTo(SocketId, AckMsg, strlen(AckMsg), RemoteAddr);//~bc~

      if (RequestTimeRefresh) {
         /* Update AC->Time */
         DOY2MD(Year,doy,&Month,&Day);
         AC->Time = DateToTime(Year,Month,Day,Hour,Minute,Second);
      }

   return NumBytes;

} /* End BC42_ReadFromSocket() */


/******************************************************************************
** Function: BC42_TakePtr
**
*/
BC42_Class_t *BC42_TakePtr(void)
{
   
   OS_MutSemTake(Bc42.MutexId); 
   
   return  &Bc42;
 
} /* End BC42_TakePtr() */


/******************************************************************************
** Function: BC42_WriteToSocket
**
** Initialize a 42 AC data object.
**
** Note:
**    1. Code copy from AppWriteToSocket.c
**
*/
void BC42_WriteToSocket(osal_id_t SocketId, OS_SockAddr_t *RemoteAddr, struct AcType *AC)
{

      long Isc,i; //~bc~ Unused: Iorb,Iw,Ipfx
      char AckMsg[5] = "Ack\n";
      char Msg[16384];
      long MsgLen = 0;
      long LineLen;
      //~bc~ Unused: long PfxLen;
      char line[512];

      Isc = AC->ID;

      sprintf(line,"SC[%ld].AC.svb = %18.12le %18.12le %18.12le\n",
         Isc,
         AC->svb[0],
         AC->svb[1],
         AC->svb[2]);
      LineLen = strlen(line);
      memcpy(&Msg[MsgLen],line,LineLen);
      MsgLen += LineLen;
      if (AC->EchoEnabled) printf("%s",line);

      sprintf(line,"SC[%ld].AC.bvb = %18.12le %18.12le %18.12le\n",
         Isc,
         AC->bvb[0],
         AC->bvb[1],
         AC->bvb[2]);
      LineLen = strlen(line);
      memcpy(&Msg[MsgLen],line,LineLen);
      MsgLen += LineLen;
      if (AC->EchoEnabled) printf("%s",line);

      sprintf(line,"SC[%ld].AC.Hvb = %18.12le %18.12le %18.12le\n",
         Isc,
         AC->Hvb[0],
         AC->Hvb[1],
         AC->Hvb[2]);
      LineLen = strlen(line);
      memcpy(&Msg[MsgLen],line,LineLen);
      MsgLen += LineLen;
      if (AC->EchoEnabled) printf("%s",line);

      for(i=0;i<AC->Ng;i++) {
         sprintf(line,"SC[%ld].AC.G[%ld].Cmd.AngRate = %18.12le %18.12le %18.12le\n",
            Isc,i,
            AC->G[i].Cmd.AngRate[0],
            AC->G[i].Cmd.AngRate[1],
            AC->G[i].Cmd.AngRate[2]);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.G[%ld].Cmd.Ang = %18.12le %18.12le %18.12le\n",
            Isc,i,
            AC->G[i].Cmd.Ang[0],
            AC->G[i].Cmd.Ang[1],
            AC->G[i].Cmd.Ang[2]);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

      }

      for(i=0;i<AC->Nwhl;i++) {
         sprintf(line,"SC[%ld].AC.Whl[%ld].Tcmd = %18.12le\n",
            Isc,i,
            AC->Whl[i].Tcmd);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

      }

      for(i=0;i<AC->Nmtb;i++) {
         sprintf(line,"SC[%ld].AC.MTB[%ld].Mcmd = %18.12le\n",
            Isc,i,
            AC->MTB[i].Mcmd);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

      }

      for(i=0;i<AC->Nthr;i++) {
         sprintf(line,"SC[%ld].AC.Thr[%ld].PulseWidthCmd = %18.12le\n",
            Isc,i,
            AC->Thr[i].PulseWidthCmd);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

      }

      sprintf(line,"SC[%ld].AC.Cmd.AngRate = %18.12le %18.12le %18.12le\n",
         Isc,
         AC->Cmd.AngRate[0],
         AC->Cmd.AngRate[1],
         AC->Cmd.AngRate[2]);
      LineLen = strlen(line);
      memcpy(&Msg[MsgLen],line,LineLen);
      MsgLen += LineLen;
      if (AC->EchoEnabled) printf("%s",line);

      sprintf(line,"SC[%ld].AC.Cmd.Ang = %18.12le %18.12le %18.12le\n",
         Isc,
         AC->Cmd.Ang[0],
         AC->Cmd.Ang[1],
         AC->Cmd.Ang[2]);
      LineLen = strlen(line);
      memcpy(&Msg[MsgLen],line,LineLen);
      MsgLen += LineLen;
      if (AC->EchoEnabled) printf("%s",line);

      if (AC->ParmDumpEnabled) {
         sprintf(line,"SC[%ld].AC.ID = %ld\n",
            Isc,
            AC->ID);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.EchoEnabled = %ld\n",
            Isc,
            AC->EchoEnabled);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.Nb = %ld\n",
            Isc,
            AC->Nb);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.Ng = %ld\n",
            Isc,
            AC->Ng);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.Nwhl = %ld\n",
            Isc,
            AC->Nwhl);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.Nmtb = %ld\n",
            Isc,
            AC->Nmtb);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.Nthr = %ld\n",
            Isc,
            AC->Nthr);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.Ncmg = %ld\n",
            Isc,
            AC->Ncmg);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.Ngyro = %ld\n",
            Isc,
            AC->Ngyro);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.Nmag = %ld\n",
            Isc,
            AC->Nmag);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.Ncss = %ld\n",
            Isc,
            AC->Ncss);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.Nfss = %ld\n",
            Isc,
            AC->Nfss);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.Nst = %ld\n",
            Isc,
            AC->Nst);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.Ngps = %ld\n",
            Isc,
            AC->Ngps);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.Nacc = %ld\n",
            Isc,
            AC->Nacc);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.Pi = %18.12le\n",
            Isc,
            AC->Pi);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.TwoPi = %18.12le\n",
            Isc,
            AC->TwoPi);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.DT = %18.12le\n",
            Isc,
            AC->DT);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.mass = %18.12le\n",
            Isc,
            AC->mass);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.cm = %18.12le %18.12le %18.12le\n",
            Isc,
            AC->cm[0],
            AC->cm[1],
            AC->cm[2]);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.MOI = %18.12le %18.12le %18.12le %18.12le %18.12le %18.12le %18.12le %18.12le %18.12le\n",
            Isc,
            AC->MOI[0][0],
            AC->MOI[0][1],
            AC->MOI[0][2],
            AC->MOI[1][0],
            AC->MOI[1][1],
            AC->MOI[1][2],
            AC->MOI[2][0],
            AC->MOI[2][1],
            AC->MOI[2][2]);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         for(i=0;i<AC->Nb;i++) {
            sprintf(line,"SC[%ld].AC.B[%ld].mass = %18.12le\n",
               Isc,i,
               AC->B[i].mass);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

            sprintf(line,"SC[%ld].AC.B[%ld].cm = %18.12le %18.12le %18.12le\n",
               Isc,i,
               AC->B[i].cm[0],
               AC->B[i].cm[1],
               AC->B[i].cm[2]);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

            sprintf(line,"SC[%ld].AC.B[%ld].MOI = %18.12le %18.12le %18.12le %18.12le %18.12le %18.12le %18.12le %18.12le %18.12le\n",
               Isc,i,
               AC->B[i].MOI[0][0],
               AC->B[i].MOI[0][1],
               AC->B[i].MOI[0][2],
               AC->B[i].MOI[1][0],
               AC->B[i].MOI[1][1],
               AC->B[i].MOI[1][2],
               AC->B[i].MOI[2][0],
               AC->B[i].MOI[2][1],
               AC->B[i].MOI[2][2]);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

         }

         for(i=0;i<AC->Ng;i++) {
            sprintf(line,"SC[%ld].AC.G[%ld].IsSpherical = %ld\n",
               Isc,i,
               AC->G[i].IsSpherical);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

            sprintf(line,"SC[%ld].AC.G[%ld].RotDOF = %ld\n",
               Isc,i,
               AC->G[i].RotDOF);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

            sprintf(line,"SC[%ld].AC.G[%ld].TrnDOF = %ld\n",
               Isc,i,
               AC->G[i].TrnDOF);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

            sprintf(line,"SC[%ld].AC.G[%ld].RotSeq = %ld\n",
               Isc,i,
               AC->G[i].RotSeq);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

            sprintf(line,"SC[%ld].AC.G[%ld].TrnSeq = %ld\n",
               Isc,i,
               AC->G[i].TrnSeq);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

            sprintf(line,"SC[%ld].AC.G[%ld].CGiBi = %18.12le %18.12le %18.12le %18.12le %18.12le %18.12le %18.12le %18.12le %18.12le\n",
               Isc,i,
               AC->G[i].CGiBi[0][0],
               AC->G[i].CGiBi[0][1],
               AC->G[i].CGiBi[0][2],
               AC->G[i].CGiBi[1][0],
               AC->G[i].CGiBi[1][1],
               AC->G[i].CGiBi[1][2],
               AC->G[i].CGiBi[2][0],
               AC->G[i].CGiBi[2][1],
               AC->G[i].CGiBi[2][2]);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

            sprintf(line,"SC[%ld].AC.G[%ld].CBoGo = %18.12le %18.12le %18.12le %18.12le %18.12le %18.12le %18.12le %18.12le %18.12le\n",
               Isc,i,
               AC->G[i].CBoGo[0][0],
               AC->G[i].CBoGo[0][1],
               AC->G[i].CBoGo[0][2],
               AC->G[i].CBoGo[1][0],
               AC->G[i].CBoGo[1][1],
               AC->G[i].CBoGo[1][2],
               AC->G[i].CBoGo[2][0],
               AC->G[i].CBoGo[2][1],
               AC->G[i].CBoGo[2][2]);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

            sprintf(line,"SC[%ld].AC.G[%ld].AngGain = %18.12le %18.12le %18.12le\n",
               Isc,i,
               AC->G[i].AngGain[0],
               AC->G[i].AngGain[1],
               AC->G[i].AngGain[2]);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

            sprintf(line,"SC[%ld].AC.G[%ld].AngRateGain = %18.12le %18.12le %18.12le\n",
               Isc,i,
               AC->G[i].AngRateGain[0],
               AC->G[i].AngRateGain[1],
               AC->G[i].AngRateGain[2]);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

            sprintf(line,"SC[%ld].AC.G[%ld].PosGain = %18.12le %18.12le %18.12le\n",
               Isc,i,
               AC->G[i].PosGain[0],
               AC->G[i].PosGain[1],
               AC->G[i].PosGain[2]);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

            sprintf(line,"SC[%ld].AC.G[%ld].PosRateGain = %18.12le %18.12le %18.12le\n",
               Isc,i,
               AC->G[i].PosRateGain[0],
               AC->G[i].PosRateGain[1],
               AC->G[i].PosRateGain[2]);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

            sprintf(line,"SC[%ld].AC.G[%ld].MaxAngRate = %18.12le %18.12le %18.12le\n",
               Isc,i,
               AC->G[i].MaxAngRate[0],
               AC->G[i].MaxAngRate[1],
               AC->G[i].MaxAngRate[2]);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

            sprintf(line,"SC[%ld].AC.G[%ld].MaxPosRate = %18.12le %18.12le %18.12le\n",
               Isc,i,
               AC->G[i].MaxPosRate[0],
               AC->G[i].MaxPosRate[1],
               AC->G[i].MaxPosRate[2]);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

            sprintf(line,"SC[%ld].AC.G[%ld].MaxTrq = %18.12le %18.12le %18.12le\n",
               Isc,i,
               AC->G[i].MaxTrq[0],
               AC->G[i].MaxTrq[1],
               AC->G[i].MaxTrq[2]);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

            sprintf(line,"SC[%ld].AC.G[%ld].MaxFrc = %18.12le %18.12le %18.12le\n",
               Isc,i,
               AC->G[i].MaxFrc[0],
               AC->G[i].MaxFrc[1],
               AC->G[i].MaxFrc[2]);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

         }

         for(i=0;i<AC->Ngyro;i++) {
            sprintf(line,"SC[%ld].AC.Gyro[%ld].Axis = %18.12le %18.12le %18.12le\n",
               Isc,i,
               AC->Gyro[i].Axis[0],
               AC->Gyro[i].Axis[1],
               AC->Gyro[i].Axis[2]);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

         }

         for(i=0;i<AC->Nmag;i++) {
            sprintf(line,"SC[%ld].AC.MAG[%ld].Axis = %18.12le %18.12le %18.12le\n",
               Isc,i,
               AC->MAG[i].Axis[0],
               AC->MAG[i].Axis[1],
               AC->MAG[i].Axis[2]);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

         }

         for(i=0;i<AC->Ncss;i++) {
            sprintf(line,"SC[%ld].AC.CSS[%ld].Body = %ld\n",
               Isc,i,
               AC->CSS[i].Body);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

            sprintf(line,"SC[%ld].AC.CSS[%ld].Axis = %18.12le %18.12le %18.12le\n",
               Isc,i,
               AC->CSS[i].Axis[0],
               AC->CSS[i].Axis[1],
               AC->CSS[i].Axis[2]);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

            sprintf(line,"SC[%ld].AC.CSS[%ld].Scale = %18.12le\n",
               Isc,i,
               AC->CSS[i].Scale);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

         }

         for(i=0;i<AC->Nfss;i++) {
            sprintf(line,"SC[%ld].AC.FSS[%ld].qb = %18.12le %18.12le %18.12le %18.12le\n",
               Isc,i,
               AC->FSS[i].qb[0],
               AC->FSS[i].qb[1],
               AC->FSS[i].qb[2],
               AC->FSS[i].qb[3]);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

            sprintf(line,"SC[%ld].AC.FSS[%ld].CB = %18.12le %18.12le %18.12le %18.12le %18.12le %18.12le %18.12le %18.12le %18.12le\n",
               Isc,i,
               AC->FSS[i].CB[0][0],
               AC->FSS[i].CB[0][1],
               AC->FSS[i].CB[0][2],
               AC->FSS[i].CB[1][0],
               AC->FSS[i].CB[1][1],
               AC->FSS[i].CB[1][2],
               AC->FSS[i].CB[2][0],
               AC->FSS[i].CB[2][1],
               AC->FSS[i].CB[2][2]);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

         }

         for(i=0;i<AC->Nst;i++) {
            sprintf(line,"SC[%ld].AC.ST[%ld].qb = %18.12le %18.12le %18.12le %18.12le\n",
               Isc,i,
               AC->ST[i].qb[0],
               AC->ST[i].qb[1],
               AC->ST[i].qb[2],
               AC->ST[i].qb[3]);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

            sprintf(line,"SC[%ld].AC.ST[%ld].CB = %18.12le %18.12le %18.12le %18.12le %18.12le %18.12le %18.12le %18.12le %18.12le\n",
               Isc,i,
               AC->ST[i].CB[0][0],
               AC->ST[i].CB[0][1],
               AC->ST[i].CB[0][2],
               AC->ST[i].CB[1][0],
               AC->ST[i].CB[1][1],
               AC->ST[i].CB[1][2],
               AC->ST[i].CB[2][0],
               AC->ST[i].CB[2][1],
               AC->ST[i].CB[2][2]);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

         }

         for(i=0;i<AC->Nacc;i++) {
            sprintf(line,"SC[%ld].AC.Accel[%ld].PosB = %18.12le %18.12le %18.12le\n",
               Isc,i,
               AC->Accel[i].PosB[0],
               AC->Accel[i].PosB[1],
               AC->Accel[i].PosB[2]);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

            sprintf(line,"SC[%ld].AC.Accel[%ld].Axis = %18.12le %18.12le %18.12le\n",
               Isc,i,
               AC->Accel[i].Axis[0],
               AC->Accel[i].Axis[1],
               AC->Accel[i].Axis[2]);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

         }

         for(i=0;i<AC->Nwhl;i++) {
            sprintf(line,"SC[%ld].AC.Whl[%ld].Body = %ld\n",
               Isc,i,
               AC->Whl[i].Body);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

            sprintf(line,"SC[%ld].AC.Whl[%ld].Axis = %18.12le %18.12le %18.12le\n",
               Isc,i,
               AC->Whl[i].Axis[0],
               AC->Whl[i].Axis[1],
               AC->Whl[i].Axis[2]);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

            sprintf(line,"SC[%ld].AC.Whl[%ld].DistVec = %18.12le %18.12le %18.12le\n",
               Isc,i,
               AC->Whl[i].DistVec[0],
               AC->Whl[i].DistVec[1],
               AC->Whl[i].DistVec[2]);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

            sprintf(line,"SC[%ld].AC.Whl[%ld].J = %18.12le\n",
               Isc,i,
               AC->Whl[i].J);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

            sprintf(line,"SC[%ld].AC.Whl[%ld].Tmax = %18.12le\n",
               Isc,i,
               AC->Whl[i].Tmax);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

            sprintf(line,"SC[%ld].AC.Whl[%ld].Hmax = %18.12le\n",
               Isc,i,
               AC->Whl[i].Hmax);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

         }

         for(i=0;i<AC->Nmtb;i++) {
            sprintf(line,"SC[%ld].AC.MTB[%ld].Axis = %18.12le %18.12le %18.12le\n",
               Isc,i,
               AC->MTB[i].Axis[0],
               AC->MTB[i].Axis[1],
               AC->MTB[i].Axis[2]);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

            sprintf(line,"SC[%ld].AC.MTB[%ld].DistVec = %18.12le %18.12le %18.12le\n",
               Isc,i,
               AC->MTB[i].DistVec[0],
               AC->MTB[i].DistVec[1],
               AC->MTB[i].DistVec[2]);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

            sprintf(line,"SC[%ld].AC.MTB[%ld].Mmax = %18.12le\n",
               Isc,i,
               AC->MTB[i].Mmax);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

         }

         for(i=0;i<AC->Nthr;i++) {
            sprintf(line,"SC[%ld].AC.Thr[%ld].Body = %ld\n",
               Isc,i,
               AC->Thr[i].Body);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

            sprintf(line,"SC[%ld].AC.Thr[%ld].PosB = %18.12le %18.12le %18.12le\n",
               Isc,i,
               AC->Thr[i].PosB[0],
               AC->Thr[i].PosB[1],
               AC->Thr[i].PosB[2]);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

            sprintf(line,"SC[%ld].AC.Thr[%ld].Axis = %18.12le %18.12le %18.12le\n",
               Isc,i,
               AC->Thr[i].Axis[0],
               AC->Thr[i].Axis[1],
               AC->Thr[i].Axis[2]);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

            sprintf(line,"SC[%ld].AC.Thr[%ld].rxA = %18.12le %18.12le %18.12le\n",
               Isc,i,
               AC->Thr[i].rxA[0],
               AC->Thr[i].rxA[1],
               AC->Thr[i].rxA[2]);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

            sprintf(line,"SC[%ld].AC.Thr[%ld].Fmax = %18.12le\n",
               Isc,i,
               AC->Thr[i].Fmax);
            LineLen = strlen(line);
            memcpy(&Msg[MsgLen],line,LineLen);
            MsgLen += LineLen;
            if (AC->EchoEnabled) printf("%s",line);

         }

         sprintf(line,"SC[%ld].AC.PrototypeCtrl.wc = %18.12le\n",
            Isc,
            AC->PrototypeCtrl.wc);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.PrototypeCtrl.amax = %18.12le\n",
            Isc,
            AC->PrototypeCtrl.amax);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.PrototypeCtrl.vmax = %18.12le\n",
            Isc,
            AC->PrototypeCtrl.vmax);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.PrototypeCtrl.Kprec = %18.12le\n",
            Isc,
            AC->PrototypeCtrl.Kprec);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.PrototypeCtrl.Knute = %18.12le\n",
            Isc,
            AC->PrototypeCtrl.Knute);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.AdHocCtrl.Kr = %18.12le %18.12le %18.12le\n",
            Isc,
            AC->AdHocCtrl.Kr[0],
            AC->AdHocCtrl.Kr[1],
            AC->AdHocCtrl.Kr[2]);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.AdHocCtrl.Kp = %18.12le %18.12le %18.12le\n",
            Isc,
            AC->AdHocCtrl.Kp[0],
            AC->AdHocCtrl.Kp[1],
            AC->AdHocCtrl.Kp[2]);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.SpinnerCtrl.Ispin = %18.12le\n",
            Isc,
            AC->SpinnerCtrl.Ispin);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.SpinnerCtrl.Itrans = %18.12le\n",
            Isc,
            AC->SpinnerCtrl.Itrans);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.SpinnerCtrl.SpinRate = %18.12le\n",
            Isc,
            AC->SpinnerCtrl.SpinRate);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.SpinnerCtrl.Knute = %18.12le\n",
            Isc,
            AC->SpinnerCtrl.Knute);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.SpinnerCtrl.Kprec = %18.12le\n",
            Isc,
            AC->SpinnerCtrl.Kprec);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.ThreeAxisCtrl.Kr = %18.12le %18.12le %18.12le\n",
            Isc,
            AC->ThreeAxisCtrl.Kr[0],
            AC->ThreeAxisCtrl.Kr[1],
            AC->ThreeAxisCtrl.Kr[2]);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.ThreeAxisCtrl.Kp = %18.12le %18.12le %18.12le\n",
            Isc,
            AC->ThreeAxisCtrl.Kp[0],
            AC->ThreeAxisCtrl.Kp[1],
            AC->ThreeAxisCtrl.Kp[2]);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.ThreeAxisCtrl.Kunl = %18.12le\n",
            Isc,
            AC->ThreeAxisCtrl.Kunl);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.IssCtrl.Kr = %18.12le %18.12le %18.12le\n",
            Isc,
            AC->IssCtrl.Kr[0],
            AC->IssCtrl.Kr[1],
            AC->IssCtrl.Kr[2]);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.IssCtrl.Kp = %18.12le %18.12le %18.12le\n",
            Isc,
            AC->IssCtrl.Kp[0],
            AC->IssCtrl.Kp[1],
            AC->IssCtrl.Kp[2]);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.IssCtrl.Tmax = %18.12le\n",
            Isc,
            AC->IssCtrl.Tmax);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.CmgCtrl.Kr = %18.12le %18.12le %18.12le\n",
            Isc,
            AC->CmgCtrl.Kr[0],
            AC->CmgCtrl.Kr[1],
            AC->CmgCtrl.Kr[2]);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.CmgCtrl.Kp = %18.12le %18.12le %18.12le\n",
            Isc,
            AC->CmgCtrl.Kp[0],
            AC->CmgCtrl.Kp[1],
            AC->CmgCtrl.Kp[2]);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.ThrCtrl.Kw = %18.12le %18.12le %18.12le\n",
            Isc,
            AC->ThrCtrl.Kw[0],
            AC->ThrCtrl.Kw[1],
            AC->ThrCtrl.Kw[2]);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.ThrCtrl.Kth = %18.12le %18.12le %18.12le\n",
            Isc,
            AC->ThrCtrl.Kth[0],
            AC->ThrCtrl.Kth[1],
            AC->ThrCtrl.Kth[2]);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.ThrCtrl.Kv = %18.12le\n",
            Isc,
            AC->ThrCtrl.Kv);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.ThrCtrl.Kp = %18.12le\n",
            Isc,
            AC->ThrCtrl.Kp);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.CfsCtrl.Kr = %18.12le %18.12le %18.12le\n",
            Isc,
            AC->CfsCtrl.Kr[0],
            AC->CfsCtrl.Kr[1],
            AC->CfsCtrl.Kr[2]);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.CfsCtrl.Kp = %18.12le %18.12le %18.12le\n",
            Isc,
            AC->CfsCtrl.Kp[0],
            AC->CfsCtrl.Kp[1],
            AC->CfsCtrl.Kp[2]);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.CfsCtrl.Kunl = %18.12le\n",
            Isc,
            AC->CfsCtrl.Kunl);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.ThrSteerCtrl.Kr = %18.12le %18.12le %18.12le\n",
            Isc,
            AC->ThrSteerCtrl.Kr[0],
            AC->ThrSteerCtrl.Kr[1],
            AC->ThrSteerCtrl.Kr[2]);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

         sprintf(line,"SC[%ld].AC.ThrSteerCtrl.Kp = %18.12le %18.12le %18.12le\n",
            Isc,
            AC->ThrSteerCtrl.Kp[0],
            AC->ThrSteerCtrl.Kp[1],
            AC->ThrSteerCtrl.Kp[2]);
         LineLen = strlen(line);
         memcpy(&Msg[MsgLen],line,LineLen);
         MsgLen += LineLen;
         if (AC->EchoEnabled) printf("%s",line);

      }

      sprintf(line,"[EOF]\n\n");
      if (AC->EchoEnabled) printf("%s",line);

      LineLen = strlen(line);
      memcpy(&Msg[MsgLen],line,LineLen);
      MsgLen += LineLen;
      //~bc~ send(Socket,Msg,MsgLen,0);
      OS_write(SocketId, Msg, MsgLen);
     
      /* Wait for Ack */
      //~bc~ recv(Socket,AckMsg,5,0);
      OS_read(SocketId, AckMsg, 5);
      
} /* End BC42_WriteToSocket() */


/******************************************************************************
** Function:  InitParameters
**
** This initial version was created by cutting and pasting the default
** parameters in AcParmDump00.txt created by running "./42 Basecamp" in
** standalone with "./AcApp 0". AcParmDump00.txt was copied to the bc42_lib/fsw
** directory to preserve the initial value file.
**
** The goal is to use a tool to automate the parameter-to-FSW app 
** definition.
*/
static void InitParameters(struct AcType *Ac) 
{

   Ac->svb[0] = 1.000000000000e+00;
   Ac->svb[1] = 0.000000000000e+00;
   Ac->svb[2] = 0.000000000000e+00;
   Ac->bvb[0] = -2.078000000000e-05;
   Ac->bvb[1] = -1.726000000000e-05;
   Ac->bvb[2] = -2.770000000000e-06;
   Ac->Hvb[0] =  1.338004006967e-03;
   Ac->Hvb[1] = -2.356030865574e+00;
   Ac->Hvb[2] = -1.859392602549e-03; 
   Ac->G[0].Cmd.Ang[0] = 1.570796326795e+00;
   Ac->G[0].Cmd.Ang[1] = 0.000000000000e+00;
   Ac->G[0].Cmd.Ang[2] = 0.000000000000e+00;
   Ac->Whl[0].Tcmd = 4.317034137377e-04;
   Ac->Whl[1].Tcmd = 3.908564277547e-04;
   Ac->Whl[2].Tcmd = 1.846120782472e-03;
   Ac->Whl[3].Tcmd = 1.886967768455e-03;
   Ac->MTB[0].Mcmd = 6.494112381320e+00;
   Ac->MTB[1].Mcmd = 4.234444938026e-02;
   Ac->MTB[2].Mcmd = -4.898141533579e+01;
   Ac->Cmd.Ang[0] = 6.916920265054e-310;
   Ac->Cmd.Ang[1] = 6.916920374303e-310;
   Ac->Cmd.Ang[2] = 6.916920368368e-310;
   Ac->ID = 0;
   Ac->EchoEnabled = 1;
   Ac->Nb = 2;
   Ac->Ng = 1;
   Ac->Nwhl = 4;
   Ac->Nmtb = 3;
   Ac->Nthr = 0;
   Ac->Ncmg = 0;
   Ac->Ngyro = 3;
   Ac->Nmag = 3;
   Ac->Ncss = 8;
   Ac->Nfss = 1;
   Ac->Nst = 1;
   Ac->Ngps = 1;
   Ac->Nacc = 0;
   Ac->DT = 2.000000000000e-01;
   Ac->mass = 1.020000000000e+03;
   Ac->cm[0] = 2.041176470588e+00; 
   Ac->cm[1] = -1.798039215686e-01;
   Ac->cm[2] = -1.470588235294e-02;
   Ac->MOI[0][0] = 2.759831372549e+03;
   Ac->MOI[0][1] = 3.775882352941e+02;
   Ac->MOI[0][2] = 3.088235294118e+01;
   Ac->MOI[1][0] = 3.775882352941e+02;
   Ac->MOI[1][1] = 2.117500000000e+03;
   Ac->MOI[1][2] = -1.348529411765e+02;
   Ac->MOI[2][0] = 3.088235294118e+01;
   Ac->MOI[2][1] = -1.348529411765e+02;
   Ac->MOI[2][2] = 3.835272549020e+03;
   Ac->B[0].mass = 1.000000000000e+03;
   Ac->B[0].cm[0] = 2.000000000000e+00;
   Ac->B[0].cm[1] = 0.000000000000e+00;
   Ac->B[0].cm[2] = 0.000000000000e+00;
   Ac->B[0].MOI[0][0] = 1.000000000000e+03;
   Ac->B[0].MOI[0][1] = -0.000000000000e+00;
   Ac->B[0].MOI[0][2] = -0.000000000000e+00;
   Ac->B[0].MOI[1][0] = -0.000000000000e+00;
   Ac->B[0].MOI[1][1] = 2.000000000000e+03;
   Ac->B[0].MOI[1][2] = -0.000000000000e+00;
   Ac->B[0].MOI[2][0] = -0.000000000000e+00;
   Ac->B[0].MOI[2][1] = -0.000000000000e+00;
   Ac->B[0].MOI[2][2] = 2.000000000000e+03;
   Ac->B[1].mass = 2.000000000000e+01;
   Ac->B[1].cm[0] = 0.000000000000e+00;
   Ac->B[1].cm[1] = -8.000000000000e+00;
   Ac->B[1].cm[2] = 0.000000000000e+00;
   Ac->B[1].MOI[0][0] = 1.000000000000e+02;
   Ac->B[1].MOI[0][1] = -0.000000000000e+00;
   Ac->B[1].MOI[0][2] = -0.000000000000e+00;
   Ac->B[1].MOI[1][0] = -0.000000000000e+00;
   Ac->B[1].MOI[1][1] = 2.000000000000e+01;
   Ac->B[1].MOI[1][2] = -0.000000000000e+00;
   Ac->B[1].MOI[2][0] = -0.000000000000e+00;
   Ac->B[1].MOI[2][1] = -0.000000000000e+00;
   Ac->B[1].MOI[2][2] = 1.000000000000e+02;
   Ac->G[0].IsSpherical = 0;
   Ac->G[0].RotDOF = 1;
   Ac->G[0].TrnDOF = 0;
   Ac->G[0].RotSeq = 231;
   Ac->G[0].TrnSeq = 123;
   Ac->G[0].CGiBi[0][0] = 1.000000000000e+00;
   Ac->G[0].CGiBi[0][1] = 0.000000000000e+00;
   Ac->G[0].CGiBi[0][2] = -0.000000000000e+00;
   Ac->G[0].CGiBi[1][0] = -0.000000000000e+00;
   Ac->G[0].CGiBi[1][1] = 1.000000000000e+00;
   Ac->G[0].CGiBi[1][2] = 0.000000000000e+00;
   Ac->G[0].CGiBi[2][0] = 0.000000000000e+00;
   Ac->G[0].CGiBi[2][1] = 0.000000000000e+00;
   Ac->G[0].CGiBi[2][2] = 1.000000000000e+00;
   Ac->G[0].CBoGo[0][0] = 1.000000000000e+00;
   Ac->G[0].CBoGo[0][1] = 0.000000000000e+00;
   Ac->G[0].CBoGo[0][2] = -0.000000000000e+00;
   Ac->G[0].CBoGo[1][0] = -0.000000000000e+00;
   Ac->G[0].CBoGo[1][1] = 1.000000000000e+00;
   Ac->G[0].CBoGo[1][2] = 0.000000000000e+00;
   Ac->G[0].CBoGo[2][0] = 0.000000000000e+00;
   Ac->G[0].CBoGo[2][1] = 0.000000000000e+00;
   Ac->G[0].CBoGo[2][2] = 1.000000000000e+00;
   Ac->G[0].AngGain[0] = 4.000000000000e+00;
   Ac->G[0].AngGain[1] = 0.000000000000e+00;
   Ac->G[0].AngGain[2] =  0.000000000000e+00;
   Ac->G[0].AngRateGain[0] = 4.000000000000e+01;
   Ac->G[0].AngRateGain[1] = 0.000000000000e+00;
   Ac->G[0].AngRateGain[2] = 0.000000000000e+00;
   Ac->G[0].PosGain[0] = 0.000000000000e+00;
   Ac->G[0].PosGain[1] = 0.000000000000e+00;
   Ac->G[0].PosGain[2] = 0.000000000000e+00;
   Ac->G[0].PosRateGain[0] = 0.000000000000e+00;
   Ac->G[0].PosRateGain[1] = 0.000000000000e+00;
   Ac->G[0].PosRateGain[2] = 0.000000000000e+00;
   Ac->G[0].MaxAngRate[0] = 1.745329251994e-02;
   Ac->G[0].MaxAngRate[1] = 0.000000000000e+00;
   Ac->G[0].MaxAngRate[2] = 0.000000000000e+00;
   Ac->G[0].MaxPosRate[0] = 0.000000000000e+00;
   Ac->G[0].MaxPosRate[1] = 0.000000000000e+00;
   Ac->G[0].MaxPosRate[2] = 0.000000000000e+00;
   Ac->G[0].MaxTrq[0] = 1.000000000000e+01;
   Ac->G[0].MaxTrq[1] = 0.000000000000e+00;
   Ac->G[0].MaxTrq[2] = 0.000000000000e+00;
   Ac->G[0].MaxFrc[0] = 0.000000000000e+00;
   Ac->G[0].MaxFrc[1] = 0.000000000000e+00;
   Ac->G[0].MaxFrc[2] = 0.000000000000e+00;
   Ac->Gyro[0].Axis[0] = 1.000000000000e+00;
   Ac->Gyro[0].Axis[1] = 0.000000000000e+00;
   Ac->Gyro[0].Axis[2] = 0.000000000000e+00;
   Ac->Gyro[1].Axis[0] = 0.000000000000e+00;
   Ac->Gyro[1].Axis[1] = 1.000000000000e+00;
   Ac->Gyro[1].Axis[2] = 0.000000000000e+00;
   Ac->Gyro[2].Axis[0] = 0.000000000000e+00;
   Ac->Gyro[2].Axis[1] = 0.000000000000e+00;
   Ac->Gyro[2].Axis[2] = 1.000000000000e+00;
   Ac->MAG[0].Axis[0] = 1.000000000000e+00;
   Ac->MAG[0].Axis[1] = 0.000000000000e+00;
   Ac->MAG[0].Axis[2] = 0.000000000000e+00;
   Ac->MAG[1].Axis[0] = 0.000000000000e+00;
   Ac->MAG[1].Axis[1] = 1.000000000000e+00;
   Ac->MAG[1].Axis[2] = 0.000000000000e+00;
   Ac->MAG[2].Axis[0] = 0.000000000000e+00;
   Ac->MAG[2].Axis[1] = 0.000000000000e+00;
   Ac->MAG[2].Axis[2] = 1.000000000000e+00;
   Ac->CSS[0].Body = 0;
   Ac->CSS[0].Axis[0] = 5.773502691896e-01;
   Ac->CSS[0].Axis[1] = 5.773502691896e-01;
   Ac->CSS[0].Axis[2] = 5.773502691896e-01;
   Ac->CSS[0].Scale = 1.000000000000e+00;
   Ac->CSS[1].Body = 0;
   Ac->CSS[1].Axis[0] = 5.773502691896e-01;
   Ac->CSS[1].Axis[1] = 5.773502691896e-01;
   Ac->CSS[1].Axis[2] = -5.773502691896e-01;
   Ac->CSS[1].Scale = 1.000000000000e+00;
   Ac->CSS[2].Body = 0;
   Ac->CSS[2].Axis[0] = 5.773502691896e-01;
   Ac->CSS[2].Axis[1] = -5.773502691896e-01;
   Ac->CSS[2].Axis[2] = 5.773502691896e-01;
   Ac->CSS[2].Scale = 1.000000000000e+00;
   Ac->CSS[3].Body = 0;
   Ac->CSS[3].Axis[0] = 5.773502691896e-01;
   Ac->CSS[3].Axis[1] = -5.773502691896e-01;
   Ac->CSS[3].Axis[2] = -5.773502691896e-01;
   Ac->CSS[3].Scale = 1.000000000000e+00;
   Ac->CSS[4].Body = 0;
   Ac->CSS[4].Axis[0] = -5.773502691896e-01;
   Ac->CSS[4].Axis[1] = 5.773502691896e-01;
   Ac->CSS[4].Axis[2] = 5.773502691896e-01;
   Ac->CSS[4].Scale = 1.000000000000e+00;
   Ac->CSS[5].Body = 0;
   Ac->CSS[5].Axis[0] = -5.773502691896e-01;
   Ac->CSS[5].Axis[1] = 5.773502691896e-01;
   Ac->CSS[5].Axis[2] = -5.773502691896e-01;
   Ac->CSS[5].Scale = 1.000000000000e+00;
   Ac->CSS[6].Body = 0;
   Ac->CSS[6].Axis[0] = -5.773502691896e-01;
   Ac->CSS[6].Axis[1] = -5.773502691896e-01;
   Ac->CSS[6].Axis[2] =  5.773502691896e-01;
   Ac->CSS[6].Scale = 1.000000000000e+00;
   Ac->CSS[7].Body = 0;
   Ac->CSS[7].Axis[0] = -5.773502691896e-01;
   Ac->CSS[7].Axis[1] = -5.773502691896e-01;
   Ac->CSS[7].Axis[2] = -5.773502691896e-01;
   Ac->CSS[7].Scale = 1.000000000000e+00;
   Ac->FSS[0].qb[0] = 0.000000000000e+00;
   Ac->FSS[0].qb[1] = 7.071067811865e-01;
   Ac->FSS[0].qb[2] = 0.000000000000e+00;
   Ac->FSS[0].qb[3] = 7.071067811865e-01;
   Ac->FSS[0].CB[0][0] = 6.123233995737e-17;
   Ac->FSS[0].CB[0][1] = 0.000000000000e+00;
   Ac->FSS[0].CB[0][2] = -1.000000000000e+00;
   Ac->FSS[0].CB[1][0] = 0.000000000000e+00;
   Ac->FSS[0].CB[1][1] = 1.000000000000e+00;
   Ac->FSS[0].CB[1][2] = 0.000000000000e+00;
   Ac->FSS[0].CB[2][0] = 1.000000000000e+00;
   Ac->FSS[0].CB[2][1] = -0.000000000000e+00;
   Ac->FSS[0].CB[2][2] = 6.123233995737e-17;
   Ac->ST[0].qb[0] = 0.000000000000e+00;
   Ac->ST[0].qb[1] = 1.000000000000e+00;
   Ac->ST[0].qb[2] = 0.000000000000e+00;
   Ac->ST[0].qb[3] = 6.123233995737e-17;
   Ac->ST[0].CB[0][0] = -1.000000000000e+00;
   Ac->ST[0].CB[0][1] =  0.000000000000e+00;
   Ac->ST[0].CB[0][2] =  -1.224646799147e-16;
   Ac->ST[0].CB[1][0] =  0.000000000000e+00;
   Ac->ST[0].CB[1][1] =  1.000000000000e+00;
   Ac->ST[0].CB[1][2] =  0.000000000000e+00;
   Ac->ST[0].CB[2][0] =  1.224646799147e-16;
   Ac->ST[0].CB[2][1] =  -0.000000000000e+00;
   Ac->ST[0].CB[2][2] =  -1.000000000000e+00;
   Ac->Whl[0].Axis[0] = 5.773502691896e-01;
   Ac->Whl[0].Axis[1] =  5.773502691896e-01;
   Ac->Whl[0].Axis[2] =  5.773502691896e-01;
   Ac->Whl[0].DistVec[0] = 4.330127018922e-01;
   Ac->Whl[0].DistVec[1] = 4.330127018922e-01;
   Ac->Whl[0].DistVec[2] = 4.330127018922e-01;
   Ac->Whl[0].J = 1.200000000000e-02;
   Ac->Whl[0].Tmax = 1.400000000000e-01;
   Ac->Whl[0].Hmax = 4.000000000000e+01;
   Ac->Whl[1].Axis[0] = -5.773502691896e-01;
   Ac->Whl[1].Axis[1] =  5.773502691896e-01;
   Ac->Whl[1].Axis[2] =  5.773502691896e-01;
   Ac->Whl[1].DistVec[0] = -4.330127018922e-01;
   Ac->Whl[1].DistVec[1] =  4.330127018922e-01;
   Ac->Whl[1].DistVec[2] =  4.330127018922e-01;
   Ac->Whl[1].J = 1.200000000000e-02;
   Ac->Whl[1].Tmax = 1.400000000000e-01;
   Ac->Whl[1].Hmax = 4.000000000000e+01;
   Ac->Whl[2].Axis[0] = -5.773502691896e-01;
   Ac->Whl[2].Axis[1] =  -5.773502691896e-01;
   Ac->Whl[2].Axis[2] =  5.773502691896e-01;
   Ac->Whl[2].DistVec[0] = -4.330127018922e-01;
   Ac->Whl[2].DistVec[1] =  -4.330127018922e-01;
   Ac->Whl[2].DistVec[2] =  4.330127018922e-01;
   Ac->Whl[2].J = 1.200000000000e-02;
   Ac->Whl[2].Tmax = 1.400000000000e-01;
   Ac->Whl[2].Hmax = 4.000000000000e+01;
   Ac->Whl[3].Axis[0] = 5.773502691896e-01;
   Ac->Whl[3].Axis[1] = -5.773502691896e-01;
   Ac->Whl[3].Axis[2] = 5.773502691896e-01;
   Ac->Whl[3].DistVec[0] = 4.330127018922e-01;
   Ac->Whl[3].DistVec[1] = -4.330127018922e-01;
   Ac->Whl[3].DistVec[2] =  4.330127018922e-01;
   Ac->Whl[3].J = 1.200000000000e-02;
   Ac->Whl[3].Tmax = 1.400000000000e-01;
   Ac->Whl[3].Hmax = 4.000000000000e+01;
   Ac->MTB[0].Axis[0] = 1.000000000000e+00;
   Ac->MTB[0].Axis[1] =  0.000000000000e+00;
   Ac->MTB[0].Axis[2] =  0.000000000000e+00;
   Ac->MTB[0].DistVec[0] = 1.000000000000e+00;
   Ac->MTB[0].DistVec[1] =  0.000000000000e+00;
   Ac->MTB[0].DistVec[2] =  0.000000000000e+00;
   Ac->MTB[0].Mmax = 1.800000000000e+02;
   Ac->MTB[1].Axis[0] = 0.000000000000e+00;
   Ac->MTB[1].Axis[1] =  1.000000000000e+00;
   Ac->MTB[1].Axis[2] =  0.000000000000e+00;
   Ac->MTB[1].DistVec[0] = 0.000000000000e+00;
   Ac->MTB[1].DistVec[1] = 1.000000000000e+00;
   Ac->MTB[1].DistVec[2] = 0.000000000000e+00;
   Ac->MTB[1].Mmax = 1.800000000000e+02;
   Ac->MTB[2].Axis[0] = 0.000000000000e+00;
   Ac->MTB[2].Axis[1] =  0.000000000000e+00;
   Ac->MTB[2].Axis[2] =  1.000000000000e+00;
   Ac->MTB[2].DistVec[0] = 0.000000000000e+00;
   Ac->MTB[2].DistVec[1] =  0.000000000000e+00;
   Ac->MTB[2].DistVec[2] =  1.000000000000e+00;
   Ac->MTB[2].Mmax = 1.800000000000e+02;
   Ac->PrototypeCtrl.wc = 3.141592653590e-01;
   Ac->PrototypeCtrl.amax = 1.000000000000e-02;
   Ac->PrototypeCtrl.vmax = 8.726646259972e-03;
   Ac->PrototypeCtrl.Kprec = 0.000000000000e+00;
   Ac->PrototypeCtrl.Knute = 0.000000000000e+00;
   Ac->AdHocCtrl.Kr[0] = 0.000000000000e+00;
   Ac->AdHocCtrl.Kr[1] =  0.000000000000e+00;
   Ac->AdHocCtrl.Kr[2] =  0.000000000000e+00;
   Ac->AdHocCtrl.Kp[0] = 0.000000000000e+00;
   Ac->AdHocCtrl.Kp[1] =  0.000000000000e+00;
   Ac->AdHocCtrl.Kp[2] =  0.000000000000e+00;
   Ac->SpinnerCtrl.Ispin = 0.000000000000e+00;
   Ac->SpinnerCtrl.Itrans = 0.000000000000e+00;
   Ac->SpinnerCtrl.SpinRate = 0.000000000000e+00;
   Ac->SpinnerCtrl.Knute = 0.000000000000e+00;
   Ac->SpinnerCtrl.Kprec = 0.000000000000e+00;
   Ac->ThreeAxisCtrl.Kr[0] = 0.000000000000e+00;
   Ac->ThreeAxisCtrl.Kr[1] =  0.000000000000e+00;
   Ac->ThreeAxisCtrl.Kr[2] =  0.000000000000e+00;
   Ac->ThreeAxisCtrl.Kp[0] = 0.000000000000e+00;
   Ac->ThreeAxisCtrl.Kp[1] =  0.000000000000e+00;
   Ac->ThreeAxisCtrl.Kp[2] =  0.000000000000e+00;
   Ac->ThreeAxisCtrl.Kunl = 0.000000000000e+00;
   Ac->IssCtrl.Kr[0] = 0.000000000000e+00;
   Ac->IssCtrl.Kr[1] =  0.000000000000e+00;
   Ac->IssCtrl.Kr[2] =  0.000000000000e+00;
   Ac->IssCtrl.Kp[0] = 0.000000000000e+00;
   Ac->IssCtrl.Kp[1] =  0.000000000000e+00;
   Ac->IssCtrl.Kp[2] =  0.000000000000e+00;
   Ac->IssCtrl.Tmax = 0.000000000000e+00;
   Ac->CmgCtrl.Kr[0] = 0.000000000000e+00;
   Ac->CmgCtrl.Kr[1] =  0.000000000000e+00;
   Ac->CmgCtrl.Kr[2] =  0.000000000000e+00;
   Ac->CmgCtrl.Kp[0] = 0.000000000000e+00;
   Ac->CmgCtrl.Kp[1] =  0.000000000000e+00;
   Ac->CmgCtrl.Kp[2] =  0.000000000000e+00;
   Ac->ThrCtrl.Kw[0] = 0.000000000000e+00;
   Ac->ThrCtrl.Kw[1] =  0.000000000000e+00;
   Ac->ThrCtrl.Kw[2] =  0.000000000000e+00;
   Ac->ThrCtrl.Kth[0] = 0.000000000000e+00;
   Ac->ThrCtrl.Kth[1] =  0.000000000000e+00;
   Ac->ThrCtrl.Kth[2] =  0.000000000000e+00;
   Ac->ThrCtrl.Kv = 0.000000000000e+00;
   Ac->ThrCtrl.Kp = 0.000000000000e+00;    
   Ac->CfsCtrl.Kr[0] = 3.863763921569e+02;
   Ac->CfsCtrl.Kr[1] = 2.964500000000e+02;
   Ac->CfsCtrl.Kr[2] = 5.369381568628e+02;
   Ac->CfsCtrl.Kp[0] = 2.759831372549e+01;
   Ac->CfsCtrl.Kp[1] = 2.117500000000e+01;
   Ac->CfsCtrl.Kp[2] = 3.835272549020e+01;
   Ac->CfsCtrl.Kunl = 1.000000000000e+06;
   Ac->ThrSteerCtrl.Kr[0] = 0.000000000000e+00;
   Ac->ThrSteerCtrl.Kr[1] = 0.000000000000e+00;
   Ac->ThrSteerCtrl.Kr[2] = 0.000000000000e+00;
   Ac->ThrSteerCtrl.Kp[0] = 0.000000000000e+00;
   Ac->ThrSteerCtrl.Kp[1] = 0.000000000000e+00;
   Ac->ThrSteerCtrl.Kp[2] = 0.000000000000e+00;

} /* End InitParameters() */


