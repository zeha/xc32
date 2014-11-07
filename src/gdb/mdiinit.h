/* MDI (Microprocessor Debug Interface) DLL loader definitions.

   Copyright (c) 2000 LSI Logic Corporation - All Rights Reserved

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifndef _MDIINIT_H_
#define _MDIINIT_H_

#ifndef GDB_MULTI_ARCH
int MDIInit (char *libname, HMODULE *handle);
#if defined(_WIN32) || defined(__CYGWIN32__)
int MDIDllName (char *libname, unsigned int size);
#endif
#endif

#if defined(_WIN32) || defined(__CYGWIN32__)

/* Have to look for symbols prefixed with _ and postfixed with @ sizeof args */
#define FMDIAbort "_MDIAbort@4"
#define FMDIAttachTM "_MDIAttachTM@12"
#define FMDIBpQuery "_MDIBpQuery@12"
#define FMDICacheFlush "_MDICacheFlush@12"
#define FMDICacheQuery "_MDICacheQuery@8"
#define FMDIClearBp "_MDIClearBp@8"
#define FMDIClearTeam "_MDIClearTeam@8"
#define FMDIClose "_MDIClose@8"
#define FMDIConnect "_MDIConnect@12"
#define FMDICreateTeam "_MDICreateTeam@8"
#define FMDIDestroyTeam "_MDIDestroyTeam@8"
#define FMDIDetachTM "_MDIDetachTM@12"
#define FMDIDQuery "_MDIDQuery@12"
#define FMDIDisableBp "_MDIDisableBp@8"
#define FMDIDisconnect "_MDIDisconnect@8"
#define FMDIDoCommand "_MDIDoCommand@8"
#define FMDIEnableBp "_MDIEnableBp@8"
#define FMDIExecute "_MDIExecute@4"
#define FMDIFill "_MDIFill@36"
#define FMDIFind "_MDIFind@48"
#define FMDIGetTC "_MDIGetTC@8"
#define FMDIHwBpQuery "_MDIHwBpQuery@12"
#define FMDIMove "_MDIMove@40"
#define FMDIOpen "_MDIOpen@16"
#define FMDIQueryTC "_MDIQueryTC@12"
#define FMDIQueryTeams "_MDIQueryTeams@12"
#define FMDIRead "_MDIRead@28"
#define FMDIReadList "_MDIReadList@20"
#define FMDIReset "_MDIReset@8"
#define FMDIRunState "_MDIRunState@12"
#define FMDISetBp "_MDISetBp@8"
#define FMDISetRunMode "_MDISetRunMode@16"
#define FMDISetSWBp "_MDISetSWBp@20"
#define FMDISetTC "_MDISetTC@8"
#define FMDISetTCRunMode "_MDISetTCRunMode@16"
#define FMDIStep "_MDIStep@12"
#define FMDIStop "_MDIStop@4"
#define FMDITCGet "_MDITCGet@8"
#define FMDITCQuery "_MDITCQuery@12"
#define FMDITCSet "_MDITCSet@8"
#define FMDITCSetRunMode "_MDITCSetRunMode@16"
#define FMDITGClose "_MDITGClose@8"
#define FMDITGExecute "_MDITGExecute@4"
#define FMDITGOpen "_MDITGOpen@16"
#define FMDITGQuery "_MDITGQuery@12"
#define FMDITGStop "_MDITGStop@4"
#define FMDITMQuery "_MDITMQuery@16"
#define FMDITeamExecute "_MDITeamExecute@8"
#define FMDITraceClear "_MDITraceClear@4"
#define FMDITraceCount "_MDITraceCount@8"
#define FMDITraceDisable "_MDITraceDisable@4"
#define FMDITraceEnable "_MDITraceEnable@4"
#define FMDITraceRead "_MDITraceRead@20"
#define FMDITraceStatus "_MDITraceStatus@8"
#define FMDIVersion "_MDIVersion@4"
#define FMDIWrite "_MDIWrite@28"
#define FMDIWriteList "_MDIWriteList@20"
#define FMDITraceRead "_MDITraceRead@20"

/* MIPSsim special interfaces */
#define FMIPSsim_SetConfigFile "_MIPSsim_SetConfigFile@8"
#define FMIPSsim_CreateProfile "_MIPSsim_CreateProfile@24"
#define FMIPSsim_DestroyProfile "_MIPSsim_DestroyProfile@8"
#define FMIPSsim_StartProfile "_MIPSsim_StartProfile@8"
#define FMIPSsim_StopProfile "_MIPSsim_StopProfile@8"
#define FMIPSsim_ClearProfile "_MIPSsim_ClearProfile@8"
#define FMIPSsim_FetchProfile "_MIPSsim_FetchProfile@16"
#define FMIPSsim_FreeProfileData "_MIPSsim_FreeProfileData@8"
#define FMIPSsim_GetTraceLevel "_MIPSsim_GetTraceLevel@8"
#define FMIPSsim_SetTraceLevel "_MIPSsim_SetTraceLevel@8"
#define FMIPSsim_GetStats "_MIPSsim_GetStats@12"
#define FMIPSsim_ClearStats "_MIPSsim_ClearStats@8"
#define FMIPSsim_GetPerfCounter "_MIPSsim_GetPerfCounter@20"
#define FMIPSsim_ZeroPerfCounter "_MIPSsim_ZeroPerfCounter@16"
#define FMIPSsim_ZeroPerfCounters "_MIPSsim_ZeroPerfCounters@4"
#define FMIPSsim_GetVersion "_MIPSsim_GetVersion@16"

#else

/* Unix, just look for the symbol name */
#define FMDIAbort "MDIAbort"
#define FMDIAttachTM "MDIAttachTM"
#define FMDIBpQuery "MDIBpQuery"
#define FMDICacheFlush "MDICacheFlush"
#define FMDICacheQuery "MDICacheQuery"
#define FMDIClearBp "MDIClearBp"
#define FMDIClearTeam "MDIClearTeam"
#define FMDIClose "MDIClose"
#define FMDIConnect "MDIConnect"
#define FMDICreateTeam "MDICreateTeam"
#define FMDIDQuery "MDIDQuery"
#define FMDIDestroyTeam "MDIDestroyTeam"
#define FMDIDetachTM "MDIDetachTM"
#define FMDIDisableBp "MDIDisableBp"
#define FMDIDisconnect "MDIDisconnect"
#define FMDIDoCommand "MDIDoCommand"
#define FMDIEnableBp "MDIEnableBp"
#define FMDIExecute "MDIExecute"
#define FMDIFill "MDIFill"
#define FMDIFind "MDIFind"
#define FMDIGetTC "MDIGetTC"
#define FMDIHwBpQuery "MDIHwBpQuery"
#define FMDIMove "MDIMove"
#define FMDIOpen "MDIOpen"
#define FMDIQueryTC "MDIQueryTC"
#define FMDIQueryTeams "MDIQueryTeams"
#define FMDIRead "MDIRead"
#define FMDIReadList "MDIReadList"
#define FMDIReset "MDIReset"
#define FMDIRunState "MDIRunState"
#define FMDISetBp "MDISetBp"
#define FMDISetRunMode "MDISetRunMode"
#define FMDISetSWBp "MDISetSWBp"
#define FMDISetTC "MDISetTC"
#define FMDISetTCRunMode "MDISetTCRunMode"
#define FMDIStep "MDIStep"
#define FMDIStop "MDIStop"
#define FMDITCGet "MDITCGet"
#define FMDITCQuery "MDITCQuery"
#define FMDITCSet "MDITCSet"
#define FMDITCSetRunMode "MDITCSetRunMode"
#define FMDITGClose "MDITGClose"
#define FMDITGExecute "MDITGExecute"
#define FMDITGOpen "MDITGOpen"
#define FMDITGQuery "MDITGQuery"
#define FMDITGStop "MDITGStop"
#define FMDITMQuery "MDITMQuery"
#define FMDITeamExecute "MDITeamExecute"
#define FMDITraceClear "MDITraceClear"
#define FMDITraceCount "MDITraceCount"
#define FMDITraceDisable "MDITraceDisable"
#define FMDITraceEnable "MDITraceEnable"
#define FMDITraceRead "MDITraceRead"
#define FMDITraceStatus "MDITraceStatus"
#define FMDIVersion "MDIVersion"
#define FMDIWrite "MDIWrite"
#define FMDIWriteList "MDIWriteList"

/* MIPSsim special interfaces */
#define FMIPSsim_SetConfigFile "MIPSsim_SetConfigFile"
#define FMIPSsim_CreateProfile "MIPSsim_CreateProfile"
#define FMIPSsim_DestroyProfile "MIPSsim_DestroyProfile"
#define FMIPSsim_StartProfile "MIPSsim_StartProfile"
#define FMIPSsim_StopProfile "MIPSsim_StopProfile"
#define FMIPSsim_ClearProfile "MIPSsim_ClearProfile"
#define FMIPSsim_FetchProfile "MIPSsim_FetchProfile"
#define FMIPSsim_FreeProfileData "MIPSsim_FreeProfileData"
#define FMIPSsim_GetTraceLevel "MIPSsim_GetTraceLevel"
#define FMIPSsim_SetTraceLevel "MIPSsim_SetTraceLevel"
#define FMIPSsim_GetStats "MIPSsim_GetStats"
#define FMIPSsim_ClearStats "MIPSsim_ClearStats"
#define FMIPSsim_GetPerfCounter "MIPSsim_GetPerfCounter"
#define FMIPSsim_ZeroPerfCounter "MIPSsim_ZeroPerfCounter"
#define FMIPSsim_ZeroPerfCounters "MIPSsim_ZeroPerfCounters"
#define FMIPSsim_GetVersion "MIPSsim_GetVersion"

#endif

#endif /* _MDIINIT_H_ */
