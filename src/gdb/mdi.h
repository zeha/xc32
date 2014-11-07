/* MDI (Microprocessor Debug Interface) definitions

   Contributed by MIPS Technologies, Inc.

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

#ifndef MDI_Specification_Definitions
#define MDI_Specification_Definitions

/* To build MDILib:
   Define MDI_LIB before #include "mdi.h"
   Include mdi.def in the link on Windows hosts.
   When building an MDI application (debugger):
   In one source file only, define MDILOAD_DEFINE before
   #include "mdi.h" to define pointer variables for the API
   functions. */

typedef unsigned int MDIUint32;
typedef int MDIInt32;
#ifdef _MSC_VER
typedef unsigned __int64 MDIUint64;
typedef __int64 MDIInt64;
#else
typedef unsigned long long MDIUint64;
typedef long long MDIInt64;
#endif

#ifndef __stdcall
#define __stdcall
#endif

#ifndef __declspec
#define __declspec(e)
#endif

typedef MDIUint32 MDIVersionT;
typedef struct MDIVersionRange_struct
{
  MDIVersionT oldest;
  MDIVersionT newest;
} MDIVersionRangeT;

#define MDIMajor 0x0002
#define MDIMinor 0x0014
#define MDIOldMajor 0x0001
#define MDIOldMinor 0x0000
#define MDICurrentRevision ((MDIMajor << 16 ) | MDIMinor)
#define MDIOldestRevision ((MDIOldMajor << 16) | MDIOldMinor)

typedef MDIUint32 MDIHandleT;
#define MDINoHandle ((MDIHandleT)-1)

typedef MDIUint32 MDITGIdT;

typedef struct MDITGData_struct
{
  MDITGIdT TGId;		/* MDI ID to reference this Target Group */
  char TGName[81];		/* Descriptive string identifying this TG */
} MDITGDataT;

typedef MDIUint32 MDIDeviceIdT;

typedef struct MDIDData_Struct
{
  MDIDeviceIdT Id;		/* MDI ID to reference this device */
  char DName[81];		/* Descriptive string identifying device */
  char Family[15];		/* Device's Family (CPU, DSP) */
  char FClass[15];		/* Device's Class (MIPS, X86, PPC) */
  char FPart[15];		/* Generic Part Name */
  char FISA[15];		/* Instruction Set Architecture */
  char Vendor[15];		/* Vendor of Part */
  char VFamily[15];		/* Vendor Family name */
  char VPart[15];		/* Vendor Part Number */
  char VPartRev[15];		/* Vendor Part Revision Number */
  char VPartData[15];		/* Used for Part Specific Data */
  char Endian;			/* 0 Big Endian, 1 Little Endian */
} MDIDDataT;

/* Valid values for MDIDDataT.Family: */
#define MDIFamilyCPU "CPU"
#define MDIFamilyDSP "DSP"

/* Valid values for MDIDDataT.Endian: */
#define MDIEndianBig 0
#define MDIEndianLittle 1

/* MDI Resources */
typedef MDIUint32 MDIResourceT;

typedef MDIUint64 MDIOffsetT;

typedef struct MDIRange_struct
{
  MDIOffsetT Start;
  MDIOffsetT End;
} MDIRangeT;

typedef struct MDICRange_struct
{
  MDIOffsetT Offset;
  MDIResourceT Resource;
  MDIInt32 Count;
} MDICRangeT;

typedef struct MDIConfig_struct
{
  /* Provided By: Other Comments */
  char User[80];		/* Host: ID of caller of MDI */

  char Implementer[80];		/* MDI ID of who implemented MDI */
  MDIUint32 MDICapability;	/* MDI: Flags for optional capabilities */

  /* Host: CB fn for MDI output */
  MDIInt32 (__stdcall * MDICBOutput) (MDIHandleT Device,
				      MDIInt32 Type,
				      char *Buffer, MDIInt32 Count);

  /* Host: CB fn for MDI input */
  MDIInt32 (__stdcall * MDICBInput) (MDIHandleT Device,
				     MDIInt32 Type,
				     MDIInt32 Mode,
				     char **Buffer, MDIInt32 * Count);

  /* Host: CB fn for expression eval */
  MDIInt32 (__stdcall * MDICBEvaluate) (MDIHandleT Device,
					char *Buffer,
					MDIInt32 * ResultType,
					MDIResourceT * Resource,
					MDIOffsetT * Offset,
					MDIInt32 * Size, void **Value);

  /* Host: CB fn for sym/src lookup */
  MDIInt32 (__stdcall * MDICBLookup) (MDIHandleT Device,
				      MDIInt32 Type,
				      MDIResourceT Resource,
				      MDIOffsetT Offset, char **Buffer);

  /* Host: CB fn for Event processing */
  MDIInt32 (__stdcall * MDICBPeriodic) (MDIHandleT Device);

  /* Host: CB fn for Synchronizing */
  MDIInt32 (__stdcall * MDICBSync) (MDIHandleT Device,
				    MDIInt32 Type, MDIResourceT Resource);
} MDIConfigT;

/* MDIConfigT.MDICapability flag values, can be OR'ed together */
#define MDICAP_NoParser 0x00000001	/* No command parser */
#define MDICAP_NoDebugOutput 0x00000002	/* No Target I/O */
#define MDICAP_TraceOutput 0x00000004	/* Supports Trace Output */
#define MDICAP_TraceCtrl 0x00000008	/* Supports Trace Control */
#define MDICAP_TargetGroups 0x00000010	/* Supports Target Groups */
#define MDICAP_PDtrace 0x00000020	/* Supports PDtrace functions */
#define MDICAP_TraceFetchI 0x00000040	/* Supports Instr Fetch during Trace */
#define MDICAP_TC 0x00000080		/* Supports Thread Contexts */
#define MDICAP_Teams 0x00000100		/* Supports Teams */

typedef struct MDIRunState_struct
{
  MDIUint32 Status;
  union u_info
  {
    void *ptr;
    MDIUint32 value;
  } Info;
} MDIRunStateT;

/* Status values: */		/* Info interpretation: */
#define MDIStatusNotRunning 1	/* none */
#define MDIStatusRunning 2	/* none */
#define MDIStatusHalted 3	/* none */
#define MDIStatusStepsDone 4	/* none */
#define MDIStatusExited 5	/* Info.value = exit value */
#define MDIStatusBPHit 6	/* Info.value = BpID */
#define MDIStatusUsrBPHit 7	/* none */
#define MDIStatusException 8	/* Info.value = which exception */
#define MDIStatusTraceFull 9	/* none */
#define MDIStatusDisabled 10	/* none */
#define MDIStatusVPENoTCs 10	/* none */ /* old name */
#define MDIStatusVPEDisabled 11	/* none */
#define MDIStatusMask 0x00ff	/* Status values are in lowest byte */

/* These can be OR'ed in with MDIStatusRunning and MDIStatusNotRunning */
#define MDIStatusReset 0x0100		/* currently held reset */
#define MDIStatusWasReset 0x0200	/* reset asserted & released */
#define MDIStatusResetMask 0x0300	/* reset state mask */

/* This can also be OR'ed in with MDIStatusHalted */
#define MDIStatusDescription 0x0400	/* Info.ptr = Descriptive string */

typedef struct MDICacheInfo_struct
{
  MDIInt32 Type;
  MDIUint32 LineSize;		/* Bytes of data in a cache line */
  MDIUint32 LinesPerSet;	/* Number of lines in a set */
  MDIUint32 Sets;		/* Number of sets */
} MDICacheInfoT;

/* Values for MDICacheInfoT.Type (Cache types): */
#define MDICacheTypeNone 0
#define MDICacheTypeUnified 1
#define MDICacheTypeInstruction 2
#define MDICacheTypeData 3

typedef MDIUint32 MDIBpT;

#define MDIBPT_SWInstruction 1
#define MDIBPT_SWOneShot 2
#define MDIBPT_HWInstruction 3
#define MDIBPT_HWData 4
#define MDIBPT_HWBus 5
#define MDIBPT_TypeMax MDIBPT_HWBus
#define MDIBPT_TypeMask 0x0000ffff

#define MDIBPT_HWType_Exec 0x00000001
#define MDIBPT_HWType_Data 0x00000002
#define MDIBPT_HWType_Bus 0x00000004
#define MDIBPT_HWType_AlignMask 0x000000f0
#define MDIBPT_HWType_AlignShift 4
#define MDIBPT_HWType_MaxSMask 0x00003f00
#define MDIBPT_HWType_MaxSShift 9
#define MDIBPT_HWType_VirtAddr 0x00004000
#define MDIBPT_HWType_ASID 0x00008000

/* Hardware breakpoint types may have one or more of the following
   flag bits OR'ed in to specify additional qualifications. */
#define MDIBPT_HWFlg_AddrMask 0x00010000
#define MDIBPT_HWFlg_AddrRange 0x00020000
#define MDIBPT_HWFlg_DataValue 0x00040000
#define MDIBPT_HWFlg_DataMask 0x00080000
#define MDIBPT_HWFlg_DataRead 0x00100000
#define MDIBPT_HWFlg_DataWrite 0x00200000
#define MDIBPT_HWFlg_Trigger 0x00400000
#define MDIBPT_HWFlg_TriggerOnly 0x00800000
#define MDIBPT_HWFlg_TCMatch 0x01000000
#define MDIBPT_TypeQualMask 0xffff0000

typedef MDIUint32 MDIBpIdT;

#define MDIAllBpID (~(MDIBpIdT)0)

typedef struct MDIBpData_struct
{
  MDIBpIdT Id;
  MDIBpT Type;
  MDIUint32 Enabled;		/* 0 if currently disabled, else 1 */
  MDIResourceT Resource;
  MDIRangeT Range;		/* Range.End may be an end addr or mask */
  MDIUint64 Data;		/* valid only for data write breaks */
  MDIUint64 DataMask;		/* valid only for data write breaks */
  MDIUint32 PassCount;		/* Pass count reloaded when hit */
  MDIUint32 PassesToGo;		/* Passes to go until next hit */
} MDIBpDataT;

typedef struct MDIBpInfo_struct
{
  MDIInt32 Num;			/* the number of breakpoints of this type */
  MDIUint32 Type;		/* the exact type of hardware breakpoint */
} MDIBpInfoT;

/* MDI Trace data type */
typedef struct MDITrcFrame_Struct
{
  MDIUint32 Type;
  MDIResourceT Resource;
  MDIOffsetT Offset;
  MDIUint64 Value;
} MDITrcFrameT;

#define MDITTypePC 1		/* Instruction address only */
#define MDITTypeInst 2		/* Instruction address and value */
#define MDITTypeRead 3		/* Data Load address only */
#define MDITTypeWrite 4		/* Data Store address only */
#define MDITTypeAccess 5	/* Data Access (Load/Store) address only */
#define MDITTypeRData_8 6	/* Data Load address and 8-bit value */
#define MDITTypeWData_8 7	/* Data Store address and 8-bit value */
#define MDITTypeRData_16 8	/* Data Load address and 16-bit value */
#define MDITTypeWData_16 9	/* Data Store address and 16-bit value */
#define MDITTypeRData_32 10	/* Data Load address and 32-bit value */
#define MDITTypeWData_32 11	/* Data Store address and 32-bit value */
#define MDITTypeRData_64 12	/* Data Load address and 64-bit value */
#define MDITTypeWData_64 13	/* Data Store address and 64-bit value */

/* Values for Flags parameter to MDITGOpen() and MDIOpen(): */
#define MDISharedAccess 0
#define MDIExclusiveAccess 1

/* Values for Flags parameter to MDITGClose() and MDIClose(): */
#define MDICurrentState 0
#define MDIResetState 1

/* Values for SyncType parameter to MDICBSync(): */
#define MDISyncBP 0
#define MDISyncState 1
#define MDISyncWrite 2

/* Values for Direction parameter to MDIMove(): */
#define MDIMoveForward 0
#define MDIMoveBackward 1

/* Values for Mode parameter to MDIFind(): */
#define MDIMatchForward 0
#define MDIMismatchForward 1
#define MDIMatchBackward 2
#define MDIMismatchBackward 3

/* Values for Mode parameter to MDIStep(): */
#define MDIStepInto 0
#define MDIStepForward 1
#define MDIStepOver 2
#define MDINoStep ~0

/* "Wait Forever" value for WaitTime parameter to MDIRunState(): */
#define MDIWaitForever -1

/* Values for Mode parameter to MDIReset(): */
#define MDIFullReset 0
#define MDIDeviceReset 1
#define MDICPUReset 2
#define MDIPeripheralReset 3

/* Values for Flags parameter to MDICacheFlush(): */
#define MDICacheWriteBack 0x01
#define MDICacheInvalidate 0x02

/* Values for Status parameter from MDITraceStatus(): */
#define MDITraceStatusNone 1
#define MDITraceStatusTracing 2
#define MDITraceStatusWaiting 3
#define MDITraceStatusFilling 4
#define MDITraceStatusStopped 5

/* Values for Type parameter to MDICBOutput() and MDICBInput(): */
#define MDIIOTypeMDIIn 1
#define MDIIOTypeMDIOut 2
#define MDIIOTypeMDIErr 3
#define MDIIOTypeTgtIn 4
#define MDIIOTypeTgtOut 5
#define MDIIOTypeTgtErr 6

/* Values for Mode parameter to MDICBInput(): */
#define MDIIOModeNormal 1
#define MDIIORawBlock 2
#define MDIIORawNoBlock 3

/* Values for Type parameter to MDICBEvaluate(): */
#define MDIEvalTypeResource 1
#define MDIEvalTypeChar 2
#define MDIEvalTypeInt 3
#define MDIEvalTypeUInt 4
#define MDIEvalTypeFloat 5
#define MDIEvalTypeNone 6

/* Values for Type parameter to MDICBLookup(): */
#define MDILookupNearest 1
#define MDILookupExact 2
#define MDILookupSource 3

/* MDI function return values: */
#define MDISuccess 0		/* Success. */
#define MDINotFound 1		/* MDIFind() did not find a match. */
#define MDIErrFailure -1	/* Unable to perform operation. */
#define MDIErrDevice -2		/* Invalid Device handle. */
#define MDIErrSrcResource -3	/* Invalid Resource type. */
#define MDIErrDstResource -4	/* 2nd Resource has invalid type. */
#define MDIErrInvalidSrcOffset -5	/* Offset is invalid for the specified
					   resource. */
#define MDIErrInvalidDstOffset -6	/* 2nd Offset is invalid for the 2nd
					   resource. */
#define MDIErrSrcOffsetAlignment -7	/* Offset is not correctly aligned. */
#define MDIErrDstOffsetAlignment -8	/* 2nd Offset is not correctly aligned
					   for the specified ObjectSize. */
#define MDIErrSrcCount -9	/* Count causes reference outside of the
				   resources space. */
#define MDIErrDstCount -10	/* Count causes reference outside of 2nd
				   resources space. */
#define MDIErrBPType -13	/* Invalid breakpoint type. */
#define MDIErrRange -14		/* Specified range is outside of the scope for
				   the resource. */
#define MDIErrNoResource -15	/* Hardware resources are unavailable. */
#define MDIErrBPId -16		/* Invalid Breakpoint ID. */
#define MDIErrMore -17		/* More data available than was requested. */
#define MDIErrParam -18		/* A parameter is in error (See specific
				   instructions). */
#define MDIErrTGHandle -19	/* Invalid Target Group Handle. */
#define MDIErrMDIHandle -20	/* Invalid MDI Environment Handle. */
#define MDIErrVersion -21	/* Version not supported. */
#define MDIErrLoadLib -22	/* Error loading library. */
#define MDIErrModule -23	/* Unable to link required MDI functions. */
#define MDIErrConfig -24	/* Required callback functions not present. */
#define MDIErrDeviceId -25	/* Invalid device ID. */
#define MDIErrAbort -26		/* Command has been aborted. */
#define MDIErrUnsupported -27	/* Unsupported feature. */
#define MDIErrLookupNone -28	/* Address did not match a symbol or source
				   line. */
#define MDIErrLookupError -29	/* Invalid address for look up. */
#define MDIErrTracing -30	/* Can't clear trace buffer while capturing
				   is in progress. */
#define MDIErrInvalidFunction -31	/* Function pointer is invalid. */
#define MDIErrAlreadyConnected -32	/* MDI Connection has already been
					   made for this thread. */
#define MDIErrTGId -33		/* Invalid Target Group ID. */
#define MDIErrDeviceHandle -34
#define MDIErrDevicesOpen -35
#define MDIErrInvalidData -36
#define MDIErrDuplicateBP -37
#define MDIErrInvalidFrames -38	/* Range of requested trace frames is
				   invalid. */
#define MDIErrWrongThread -39
#define MDIErrTargetRunning -40
#define MDIErrRecursive -41	/* Recursive call from MDICDPeriodic. */
#define MDIErrObjectSize -42	/* Invalid Object Size for Resource. */

#define MDIErrTCId -43		/* The specified TC id value is not a valid TC
				   number for the VPE being debugged.  */
#define MDIErrTooManyTeams -44	/* Too many teams for MDILib.  */
#define MDIErrTeamId -45	/* Invalid team id.  */
#define MDIErrDisabled -46	/* Device is disabled.  */
#define MDIErrAlreadyMember -47	/* Device is already a team member.  */
#define MDIErrNotMember -48	/* Device is not a team member.  */

typedef MDIInt32 MDITCIdT;

typedef struct MDITCData_struct
{
  MDITCIdT TCId;
  MDIUint32 Status;
} MDITCDataT;

#define MDITCStatusHalted 0
#define MDITCStatusFree 1
#define MDITCStatusRunning 2
#define MDITCStatusBlockedOnWait 3
#define MDITCStatusBlockedOnYield 4
#define MDITCStatusBlockedOnGS 5

typedef MDIInt32 MDITeamIdT;

typedef struct
{
  MDIHandleT MDIHandle;
  MDITGIdT TGId;
  MDIDeviceIdT DevId;
} MDITMDataT;

/* Function Prototypes */

#if defined (MDI_LIB)

/* MDILib, do extern function declarations */
#define yf(str) extern int __declspec(dllexport) __stdcall str

#elif defined (MDILOAD_DEFINE)

/* debugger, do function pointer definitions */
#define yf(str) int (__stdcall *str)

#else

/* debugger, do extern function pointer declarations */
#define yf(str) extern int (__stdcall *str)

#endif

yf (MDIVersion) (MDIVersionRangeT *);
yf (MDIConnect) (MDIVersionT, MDIHandleT *, MDIConfigT *);
yf (MDIDisconnect) (MDIHandleT, MDIUint32);
yf (MDITGQuery) (MDIHandleT, MDIInt32 * HowMany, MDITGDataT *);
yf (MDITGOpen) (MDIHandleT, MDITGIdT, MDIUint32, MDIHandleT *);
yf (MDITGClose) (MDIHandleT, MDIUint32);
yf (MDITGExecute) (MDIHandleT);
yf (MDITGStop) (MDIHandleT);
yf (MDIDQuery) (MDIHandleT, MDIInt32 * HowMany, MDIDDataT *);
yf (MDIOpen) (MDIHandleT, MDIDeviceIdT, MDIUint32, MDIHandleT *);
yf (MDIClose) (MDIHandleT, MDIUint32);
yf (MDIRead) (MDIHandleT, MDIResourceT SrcR, MDIOffsetT SrcO,
	      void *Buffer, MDIUint32 ObjectSize, MDIUint32 Count);
yf (MDIWrite) (MDIHandleT, MDIResourceT DstR, MDIOffsetT DstO,
	       void *Buffer, MDIUint32 ObjectSize, MDIUint32 Count);
yf (MDIReadList) (MDIHandleT, MDIUint32 ObjectSize, MDICRangeT * SrcList,
		  MDIUint32 ListCount, void *Buffer);
yf (MDIWriteList) (MDIHandleT, MDIUint32 ObjectSize, MDICRangeT * DstList,
		   MDIUint32 ListCount, void *Buffer);
yf (MDIMove) (MDIHandleT, MDIResourceT SrcR, MDIOffsetT SrcO,
	      MDIResourceT DstR, MDIOffsetT DstO, MDIUint32 ObjectSize,
	      MDIUint32 Count, MDIUint32 Flag);
yf (MDIFill) (MDIHandleT, MDIResourceT DstR, MDIRangeT DstRng,
	      void *Buffer, MDIUint32 ObjectSize, MDIUint32 Count);
yf (MDIFind) (MDIHandleT, MDIResourceT SrcR, MDIRangeT SrcRng,
	      void *Buffer, void *MaskBuffer, MDIUint32 ObjectSize,
	      MDIUint32 Count, MDIOffsetT * FoundOffset, MDIUint32 Mode);
yf (MDIExecute) (MDIHandleT);
yf (MDIStep) (MDIHandleT, MDIUint32 Steps, MDIUint32 Mode);
yf (MDIStop) (MDIHandleT);
yf (MDIReset) (MDIHandleT, MDIUint32 Flag);
yf (MDICacheQuery) (MDIHandleT, MDICacheInfoT * CacheInfo);
yf (MDICacheFlush) (MDIHandleT, MDIUint32 Type, MDIUint32 Flag);
yf (MDIRunState) (MDIHandleT, MDIInt32 WaitTime, MDIRunStateT * runstate);
yf (MDISetBp) (MDIHandleT, MDIBpDataT *);
yf (MDISetSWBp) (MDIHandleT, MDIResourceT, MDIOffsetT, MDIBpIdT *);
yf (MDIClearBp) (MDIHandleT, MDIBpIdT);
yf (MDIEnableBp) (MDIHandleT, MDIBpIdT);
yf (MDIDisableBp) (MDIHandleT, MDIBpIdT);
yf (MDIBpQuery) (MDIHandleT, MDIInt32 * HowMany, MDIBpDataT *);
yf (MDIHwBpQuery) (MDIHandleT, MDIInt32 * HowMany, MDIBpInfoT *);
yf (MDIDoCommand) (MDIHandleT, char *Buffer);
yf (MDIAbort) (MDIHandleT);
yf (MDITraceEnable) (MDIHandleT);
yf (MDITraceDisable) (MDIHandleT);
yf (MDITraceClear) (MDIHandleT);
yf (MDITraceStatus) (MDIHandleT, MDIUint32 *);
yf (MDITraceCount) (MDIHandleT, MDIUint32 *);
yf (MDITraceRead) (MDIHandleT, MDIUint32, MDIUint32, MDIUint32,
		   MDITrcFrameT *);
yf (MDISetTC) (MDIHandleT, MDITCIdT);
yf (MDIGetTC) (MDIHandleT, MDITCIdT *);
yf (MDITCQuery) (MDIHandleT, MDIInt32 *, MDITCDataT *);
yf (MDISetRunMode) (MDIHandleT, MDITCIdT, MDIUint32, MDIUint32);
yf (MDISetTCRunMode) (MDIHandleT, MDITCIdT, MDIUint32, MDIUint32);
yf (MDIQueryTC) (MDIHandleT, MDIInt32 *, MDITCDataT *);
yf (MDITCSet) (MDIHandleT, MDITCIdT);
yf (MDITCGet) (MDIHandleT, MDITCIdT *);
yf (MDITCSetRunMode) (MDIHandleT, MDIInt32, MDIUint32, MDIUint32);
yf (MDITeamCreate) (MDIHandleT, MDITeamIdT *);
yf (MDIQueryTeams) (MDIHandleT, MDIInt32 *, MDITeamIdT *);
yf (MDITeamClear) (MDIHandleT, MDITeamIdT);
yf (MDITeamDestroy) (MDIHandleT, MDITeamIdT);
yf (MDITMAttach) (MDIHandleT, MDITeamIdT, MDITMDataT *);
yf (MDITMDetach) (MDIHandleT, MDITeamIdT, MDITMDataT *);
yf (MDIQueryTM) (MDIHandleT, MDITeamIdT, MDIInt32 *, MDITMDataT *);
yf (MDICreateTeam) (MDIHandleT, MDITeamIdT *);
yf (MDIClearTeam) (MDIHandleT, MDITeamIdT);
yf (MDIDestroyTeam) (MDIHandleT, MDITeamIdT);
yf (MDIAttachTM) (MDIHandleT, MDITeamIdT, MDITMDataT *);
yf (MDIDetachTM) (MDIHandleT, MDITeamIdT, MDITMDataT *);
yf (MDITMQuery) (MDIHandleT, MDITeamIdT, MDIInt32 *, MDITMDataT *);
yf (MDITeamExecute) (MDIHandleT, MDITeamIdT);

/* MIPSsim special interfaces */
yf (MIPSsim_SetConfigFile) (MDIHandleT, char *);
yf (MIPSsim_CreateProfile) (MDIHandleT, MDIInt32 *, MDIUint64, MDIUint64);
yf (MIPSsim_DestroyProfile) (MDIHandleT, MDIInt32);
yf (MIPSsim_StartProfile) (MDIHandleT, MDIInt32);
yf (MIPSsim_StopProfile) (MDIHandleT, MDIInt32);
yf (MIPSsim_ClearProfile) (MDIHandleT, MDIInt32);
yf (MIPSsim_FetchProfile) (MDIHandleT, MDIInt32, MDIUint32 **, MDIUint64 *);
yf (MIPSsim_FreeProfileData) (MDIHandleT, MDIUint32 **);
yf (MIPSsim_GetTraceLevel) (MDIHandleT, int *);
yf (MIPSsim_SetTraceLevel) (MDIHandleT, int);
yf (MIPSsim_GetStats) (MDIHandleT, int, MDIUint64 *);
yf (MIPSsim_ClearStats) (MDIHandleT, int);
yf (MIPSsim_GetPerfCounter) (MDIHandleT, int, int, int, MDIUint64 *);
yf (MIPSsim_ZeroPerfCounter) (MDIHandleT, int, int, int);
yf (MIPSsim_ZeroPerfCounters) (MDIHandleT);
yf (MIPSsim_GetVersion) (MDIHandleT, MDIInt32 *, MDIInt32 *, MDIInt32 *);

#undef yf

#endif /* MDI_Specification_Definitions */
