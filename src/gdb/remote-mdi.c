/* MDI (MIPS Debug Interface) interface for GDB

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

#include "defs.h"
#include "version.h"
#include "inferior.h"
#include "gdb_wait.h"
#include "gdb_string.h"
#include "arch-utils.h"
#include <sys/types.h>
#include "gdb_regex.h"
#include "../gprof/gmon.h"
#include "../gprof/gmon_out.h"
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>

#if defined(_WIN32) || defined(__CYGWIN32__)
#include <windows.h>
#else
typedef void * HMODULE;
#endif

#ifdef __CYGWIN32__
#include <sys/cygwin.h>
#include <cygwin/version.h>
#endif

#ifndef MAX_PATH
#define MAX_PATH	1024
#endif

#include "gdbthread.h"
#include "terminal.h"
#include "gdbcore.h"
#include "gdbcmd.h"
#include "remote-utils.h"
#include "block.h"

#define MDILOAD_DEFINE
#include "mdi.h"
#include "mdimips.h"
#include "mips-tdep.h"

#ifdef GDBTK
#include "tcl.h"
extern Tcl_Interp *gdbtk_interp;
#endif

/* readline include files */
#include <readline/readline.h>
#include <readline/history.h>

#define GET_PID(ptid) ptid_get_pid (ptid)

/* These are pointers to hook functions that may be set in order to
   modify resume/wait behavior for a particular architecture.  */
void (*target_resume_hook) (void);
void (*target_wait_loop_hook) (void);

static const char * mdierrcode (MDIInt32);

/* The list of available "mdi", "set mdi" and "show mdi" commands. */
static struct cmd_list_element *mdicmdlist = NULL;
static struct cmd_list_element *setmdicmdlist = NULL;
static struct cmd_list_element *showmdicmdlist = NULL;

static HMODULE mdiModule;

static char *mdi_logfile;
static struct ui_file *mdi_logfp;
static int mdi_logging;

static int mdi_target;
static int mdi_device;
static int mdi_team;
static int mdi_waittime = 10;	/* XXXX 1/100th second seems very small */
static int mdi_stepinto = 0;
static int mdi_threadstepall = 0;
static int mdi_continueonclose = -1;
static int mdi_rununcached = 1;
static int mdi_threadviewinactive = 0;
static unsigned int mdi_connecttimeout = 1;
static char *mdi_library;
static char *mdi_configfile;
static char *mdi_devcfgfile;
static char *mdi_ftext_syms;
static char *mdi_etext_syms;
static char *mdi_mcount_syms;
static int mdi_connectreset = -1;
static int mdi_releaseonclose = 0;
static char *mdi_logdocommand;
static int mdi_quiet;
static int mdi_mem_written;
static char **mdi_argv;

static enum auto_boolean mdi_profiling = AUTO_BOOLEAN_AUTO;
static char *mdi_gmonfile;
static int mdi_profile_cycles;
static int mdi_profile_mcount;

static int mdi_wait_active;
static int mdi_step_active;
static int mdi_starting;

static MDIRunStateT mdi_runstate;
static MDIBpDataT mdi_curbpt;

static char *mdi_target_prompt;	/* the input prompt string */
static char *mdi_implementer;

static int mdi_found_relf;
static int mdi_syscall_inserted;
static CORE_ADDR mdi_syscall = 0;
static CORE_ADDR mdi_syscall_rom = 0;
static CORE_ADDR mdi_entrypoint = 0;
static int mdi_loaded = 0;
static int mdi_exited = 0;

static enum target_signal mdi_lastsig;
static int mdi_signalled = 0;

/* Bitmap of missing cp registers: 4 coprocessors x 8 banks x 32 registers. */
static unsigned int mdi_cpdreg_miss[4][8];
/* Bitmap of missing cp control registers: 4 coprocessors x 32 registers. */
static unsigned int mdi_cpcreg_miss[4];
/* Bitmap of write protected cp registers: 4 coprocessors x 8 banks x 32 registers. */
static unsigned int mdi_cpdreg_wp[4][8];
/* Bitmap of write protected cp control registers: 4 coprocessors x 32 registers. */
static unsigned int mdi_cpcreg_wp[4];
static unsigned int mdi_hilo_miss;
static unsigned int mdi_dsp_miss;

/* Set if the new MDIMIPFPR resource available. */
static int mdi_fpr_avail;

/* Set if the new MDIHwBpQuery call available. */
static int mdi_hwbpq_avail;

/* Specify the address type for virtual memory accesses and
   breakpoints. */

enum mdi_asidtype
{
    ASIDNone,			/* Use global address space. */
    ASIDReg,			/* Use current ASID in CP0 EntryHi */
    ASIDFixed,			/* Use fixed ASID in mdi_asid */
    ASIDAuto			/* Use global for unmapped segments,
				   or current ASID for mapped. */
};

static enum mdi_asidtype mdi_asidtype = ASIDAuto;

/* ASID for for ASIDFixed type. */
static int mdi_asid;

/*extern int MDIInit (char *dllpath, HMODULE * handle);*/
/*extern int MDIRelease (HMODULE handle);*/

static ULONGEST mdi_readreg (int regno);
static void mdi_writereg (int regno, ULONGEST val);

static ULONGEST mdi_readpc (void);
static void mdi_writepc (ULONGEST val);

static void mdi_reset_breakpoints (void);
static MDIBpDataT * mdi_find_breakpoint (MDIHandleT devhandle, MDIBpIdT id);
static MDIBpDataT * mdi_find_usr_breakpoint (CORE_ADDR addr);
static int mdi_insert_breakpoint1 (CORE_ADDR addr, MDIInt32);
static int mdi_insert_breakpoint (CORE_ADDR addr, char *save);
static int mdi_remove_breakpoint1 (CORE_ADDR addr, MDIInt32);
static int mdi_remove_breakpoint (CORE_ADDR addr, char *save);

static void mdi_team_status (MDIInt32 mdistat);
static void mdi_tm_status (MDIInt32 mdistat, MDITGIdT target,
			   MDIDeviceIdT device);

static int mdi_cycles_enable (PTR);
static void mdi_cycles_update (void);

static void dump_mem (void *, int, int);
static MDIResourceT mdi_memresource (CORE_ADDR);
static MDIOffsetT mdi_memoffset (CORE_ADDR);

static void mdi_insert_syscallbreakpoint (void);
static void mdi_remove_syscallbreakpoint (void);

static void mdi_fdinit (void);
static void mdi_fdcloseall (void);

static void mdi_profiling_start (void);
static void mdi_profiling_close (int);

static void sleep_ms (long);

static void mdi_switch_to_thread (void);
static void mdi_find_new_threads (void);

static ptid_t mdi_ptid;

enum mdiTlb { NO_TLB, R3000_TLB, R4000_32TLB, R4000_64TLB };

/* List of MDI team members.  */
struct mdi_tm_data_list
{
  MDITMDataT tmdata;		/* MDI handle, TG id, Dev id */
  struct mdi_tm_data_list *next;
};

/* List of MDI group members.  */
struct mdi_gm_data
{
  MDITMDataT tmdata;		/* MDI handle, TG ID, Dev ID */
  MDIHandleT TGHandle;		/* Target handle */
  MDIHandleT DevHandle;		/* Device handle */
  MDIDDataT DeviceData;		/* Device data */
  MDICacheInfoT CacheInfo[4];	/* Cache information */
  int disabled;			/* The device is disabled. */
  int probed;			/* The device has been probed. */
  int ntlb;			/* Number of TLB entries */
};

/* MDI connection state. */
struct mdi_state
{
  MDIHandleT MDIHandle;		/* MDI handle */
  MDITeamIdT gid;		/* Team ID for the group */
  struct mdi_gm_data gmdata[256];
				/* Devices opened */
  int gmcount;			/* Number of devices opened */
  int gmindex;			/* Index of the selected device */

  MDIConfigT Config;		/* Configuration data */

  int mipssim_p;		/* Connected to MIPSsim */
  char *cfgfile;		/* MIPSsim temp config file */

  enum mdiTlb tlb;		/* TLB type */
  int regsize;			/* Device register size */
};


static struct mdi_state *mdi_desc = NULL;

static void mdi_kill (void);

/* Forward data declarations. */
struct target_ops mdi_ops;

#define ec(str) {str, #str}

struct errorcodes_struct
{
  int errorcode;
  const char *str;
};

static const struct errorcodes_struct errorcodes[] =
{
  ec (MDINotFound),
  ec (MDIErrFailure),
  ec (MDIErrDevice),
  ec (MDIErrSrcResource),
  ec (MDIErrDstResource),
  ec (MDIErrInvalidSrcOffset),
  ec (MDIErrInvalidDstOffset),
  ec (MDIErrSrcOffsetAlignment),
  ec (MDIErrDstOffsetAlignment),
  ec (MDIErrSrcCount),
  ec (MDIErrDstCount),
  ec (MDIErrBPType),
  ec (MDIErrRange),
  ec (MDIErrNoResource),
  ec (MDIErrBPId),
  ec (MDIErrMore),
  ec (MDIErrParam),
  ec (MDIErrTGHandle),
  ec (MDIErrMDIHandle),
  ec (MDIErrVersion),
  ec (MDIErrLoadLib),
  ec (MDIErrModule),
  ec (MDIErrConfig),
  ec (MDIErrDeviceId),
  ec (MDIErrAbort),
  ec (MDIErrUnsupported),
  ec (MDIErrLookupNone),
  ec (MDIErrLookupError),
  ec (MDIErrTracing),
  ec (MDIErrInvalidFunction),
  ec (MDIErrAlreadyConnected),
  ec (MDIErrTGId),
  ec (MDIErrDeviceHandle),
  ec (MDIErrDevicesOpen),
  ec (MDIErrInvalidData),
  ec (MDIErrDuplicateBP),
  ec (MDIErrInvalidFrames),
  ec (MDIErrWrongThread),
  ec (MDIErrTargetRunning),
  ec (MDIErrRecursive),
  ec (MDIErrObjectSize),
  ec (MDIErrTCId),
  ec (MDIErrTooManyTeams),
  ec (MDIErrTeamId),
  ec (MDIErrDisabled),
  ec (MDIErrAlreadyMember),
  ec (MDIErrNotMember),
  {0, 0}
};

struct mdiresourcename_struct
{
  int mdiid;
  const char *mdistr;
};

static const struct mdiresourcename_struct resrc_Names[] =
{
  ec (MDIMIPCPU),
  ec (MDIMIPPC),
  ec (MDIMIPHILO),
  ec (MDIMIPTLB),
  ec (MDIMIPPICACHET),
  ec (MDIMIPPUCACHET),
  ec (MDIMIPPDCACHET),
  ec (MDIMIPSICACHET),
  ec (MDIMIPSUCACHET),
  ec (MDIMIPSDCACHET),
  ec (MDIMIP192ACC),
  ec (MDIMIPCP0),
  ec (MDIMIPCP0C),
  ec (MDIMIPCP1),
  ec (MDIMIPCP1C),
  ec (MDIMIPCP2),
  ec (MDIMIPCP2C),
  ec (MDIMIPCP3),
  ec (MDIMIPCP3C),
  ec (MDIMIPFP),
  ec (MDIMIPDFP),
  ec (MDIMIPPICACHE),
  ec (MDIMIPPUCACHE),
  ec (MDIMIPPDCACHE),
  ec (MDIMIPSICACHE),
  ec (MDIMIPSUCACHE),
  ec (MDIMIPSDCACHE),
  ec (MDIMIPPHYSICAL),
  ec (MDIMIPGVIRTUAL),
  ec (MDIMIPEJTAG),
  ec (MDIMIPSRS),
  ec (MDIMIPFPR),
  ec (MDIMIPDSP),
  {0, "Invalid Resource"}
};


static const struct mdiresourcename_struct status_Names[] =
{
 ec(MDIStatusNotRunning),
 ec(MDIStatusRunning),
 ec(MDIStatusHalted),
 ec(MDIStatusStepsDone),
 ec(MDIStatusExited),
 ec(MDIStatusBPHit),
 ec(MDIStatusUsrBPHit),
 ec(MDIStatusException),
 ec(MDIStatusTraceFull),
 ec(MDIStatusDisabled),
 ec(MDIStatusVPEDisabled),
 {0, "Invalid status"}
};

static const struct mdiresourcename_struct bptype_Names[] =
{
  ec (MDIBPT_SWInstruction),
  ec (MDIBPT_SWOneShot),
  ec (MDIBPT_HWInstruction),
  ec (MDIBPT_HWData),
  ec (MDIBPT_HWBus),
  {0, "Invalid bptype"}
};

static const struct mdiresourcename_struct cbtype_Names[] =
{
 ec(MDIIOTypeMDIIn),
 ec(MDIIOTypeMDIOut),
 ec(MDIIOTypeMDIErr),
 ec(MDIIOTypeTgtIn),
 ec(MDIIOTypeTgtOut),
 ec(MDIIOTypeTgtErr),
 {0, "Invalid type"}
};

struct isa_xlate_struct
{
  const char *isa;
  int num;
};

static const struct isa_xlate_struct isa_xlate[] =
{
  {MDIMIP_FISA_M1, 1} ,
  {MDIMIP_FISA_M2, 2} ,
  {MDIMIP_FISA_M3, 3} ,
  {MDIMIP_FISA_M4, 4} ,
  {MDIMIP_FISA_M5, 5} ,
  {MDIMIP_FISA_M32, 32} ,
  {MDIMIP_FISA_M64, 64} ,
  {0, -1}
};



static const char *
lookupName (const struct mdiresourcename_struct *ptr, int id)
{
  static char buf[64];
  for ( ; ptr->mdiid && ptr->mdiid != id; ptr++)
    continue;
  if (ptr->mdiid != 0)
    return (ptr->mdistr);
  sprintf (buf, "%s (0x%x)", ptr->mdistr, id);
  return buf;
}

static const char *
resrcName (int resrc)
{
  if ((MDIMIPVIRTUAL <= resrc) && (resrc < (MDIMIPVIRTUAL + 0x100))) {
    static char buf[64];
    sprintf (buf, "MDIMIPVIRTUAL/ASID=%02x", resrc & 0xff);
    return buf;
  }
  return lookupName (resrc_Names, resrc);
}

static const char *
statusName (int status)
{
  return lookupName (status_Names, status & MDIStatusMask);
}

static char *
bptypeName (int bptype)
{
  static char buf[64];
  snprintf (buf, sizeof buf, "%s%s%s%s%s%s%s",
	    lookupName (bptype_Names, bptype & MDIBPT_TypeMask),
	    (bptype & MDIBPT_HWFlg_AddrMask) ? "|AddrMask" : "",
	    (bptype & MDIBPT_HWFlg_AddrRange) ? "|AddrRange" : "",
	    (bptype & MDIBPT_HWFlg_DataValue) ? "|DataValue" : "",
	    (bptype & MDIBPT_HWFlg_DataMask) ? "|DataMask" : "",
	    (bptype & MDIBPT_HWFlg_Trigger) ? "|Trigger" : "",
	    (bptype & MDIBPT_HWFlg_TriggerOnly) ? "|TriggerOnly" : "");

  return buf;
}

static void
mdiDisplayRunState (MDIRunStateT * runstate)
{
  MDIUint32 status = runstate->Status;
  fprintf_unfiltered (mdi_logfp, "runstate=%s%s%s", 
		      statusName (status),
		      (status & MDIStatusReset) ? ", in reset" : "",
		      (status & MDIStatusWasReset) ? ", was reset" : "");
  switch (status & MDIStatusMask)
    {
    case MDIStatusExited:
      fprintf_unfiltered (mdi_logfp, ", ec=0x%x", 
			  runstate->Info.value);
      break;
    case MDIStatusBPHit:
      fprintf_unfiltered (mdi_logfp, ", BpID=%u", 
			  runstate->Info.value);
      break;
    case MDIStatusException:
      fprintf_unfiltered (mdi_logfp, ", exc=%u", 
			  runstate->Info.value);
      break;
    }
  if (status & MDIStatusDescription)
    fprintf_unfiltered (mdi_logfp, ", desc=\"%s\"",
			(char *) runstate->Info.ptr);
  fputc_unfiltered ('\n', mdi_logfp);
}



#include "mdiinit.h"

#if defined(_WIN32) || defined(__CYGWIN32__)
#include <windows.h>
#define DEFAULT_LIB_NAME "mdi.dll"
#else
#define DEFAULT_LIB_NAME "libmdi.so"
#endif

#if defined(_WIN32) && !defined(__CYGWIN32__)
#define OpenLib(libname)  \
  LoadLibrary (libname)
#define CloseLib(libhandle) \
  FreeLibrary ((HMODULE)(libhandle))
#define GetLibSymbolAddress(libhandle, symbolname) \
  ((void*)GetProcAddress ((HMODULE)(libhandle), (symbolname)))
#else
#include <dlfcn.h>
#define OpenLib(libname) \
  dlopen (libname, RTLD_LAZY)
#define CloseLib(libhandle) \
  dlclose (libhandle)
#define GetLibSymbolAddress(libhandle, symbolname) \
  dlsym (libhandle, symbolname)
#endif

struct libsymbol
{
  const char *name;
  void **funcp;
  int required;
};

#if __STDC__
#define sym(x, req) {F ## x, (void **) &x, req}
#else
#define sym(x, req) {F/**/x, (void **) &x, req}
#endif

/* List of symbols defined in MDI DLL */
static const struct libsymbol symbols[] = {
  sym (MDIVersion, 1),
  sym (MDIConnect, 1),
  sym (MDIDisconnect, 1),
  sym (MDITGQuery, 1),
  sym (MDITGOpen, 1),
  sym (MDITGClose, 1),
  sym (MDITGExecute, 1),
  sym (MDITGStop, 1),
  sym (MDIDQuery, 1),
  sym (MDIOpen, 1),
  sym (MDIClose, 1),
  sym (MDIRead, 1),
  sym (MDIWrite, 1),
  sym (MDIReadList, 1),
  sym (MDIWriteList, 1),
  sym (MDIMove, 1),
  sym (MDIFill, 1),
  sym (MDIFind, 1),
  sym (MDIExecute, 1),
  sym (MDIStep, 1),
  sym (MDIStop, 1),
  sym (MDIReset, 1),
  sym (MDICacheQuery, 1),
  sym (MDICacheFlush, 1),
  sym (MDIRunState, 1),
  sym (MDISetBp, 1),
  sym (MDISetSWBp, 1),
  sym (MDIClearBp, 1),
  sym (MDIEnableBp, 1),
  sym (MDIDisableBp, 1),
  sym (MDIBpQuery, 1),
  sym (MDIHwBpQuery, 0),
  sym (MDIDoCommand, 1),
  sym (MDIAbort, 1),
  sym (MDITraceEnable, 1),
  sym (MDITraceDisable, 1),
  sym (MDITraceClear, 1),
  sym (MDITraceStatus, 1),
  sym (MDITraceCount, 1),
  sym (MDITraceRead, 1),
  /* Optional multi-threading extensions */
  sym (MDIGetTC, 0),
  sym (MDISetTC, 0),
  sym (MDITCQuery, 0),
  sym (MDISetRunMode, 0),
  /* Early MT MDI spec revision names */
  sym (MDIQueryTC, 0),
  sym (MDITCGet, 0),
  sym (MDITCSet, 0),
  sym (MDITCSetRunMode, 0),
  sym (MDISetTCRunMode, 0),
  /* Optional team support extensions */
  sym (MDITeamCreate, 0),
  sym (MDIQueryTeams, 0),
  sym (MDITeamClear, 0),
  sym (MDITeamDestroy, 0),
  sym (MDITMAttach, 0),
  sym (MDITMDetach, 0),
  sym (MDIQueryTM, 0),
  sym (MDITeamExecute, 0),
  /* Early MT MDI spec revision names */
  sym (MDICreateTeam, 0),
  sym (MDIClearTeam, 0),
  sym (MDIDestroyTeam, 0),
  sym (MDIAttachTM, 0),
  sym (MDIDetachTM, 0),
  sym (MDITMQuery, 0),
  /* MIPSsim special interfaces */
  sym (MIPSsim_SetConfigFile, 0),
  sym (MIPSsim_CreateProfile, 0),
  sym (MIPSsim_DestroyProfile, 0),
  sym (MIPSsim_StartProfile, 0),
  sym (MIPSsim_StopProfile, 0),
  sym (MIPSsim_ClearProfile, 0),
  sym (MIPSsim_FetchProfile, 0),
  sym (MIPSsim_FreeProfileData, 0),
  sym (MIPSsim_GetTraceLevel, 0),
  sym (MIPSsim_SetTraceLevel, 0),
  sym (MIPSsim_GetStats, 0),
  sym (MIPSsim_ClearStats, 0),
  sym (MIPSsim_GetPerfCounter, 0),
  sym (MIPSsim_ZeroPerfCounter, 0),
  sym (MIPSsim_ZeroPerfCounters, 0),
  sym (MIPSsim_GetVersion, 0),
  {0, 0, 0}
};


#ifdef __CYGWIN32__
/* Copy Cygwin environment variables to Windows. */
static void
cygwin_sync_winenv (void)
{
#if CYGWIN_VERSION_API_MAJOR > 0 || CYGWIN_VERSION_API_MINOR >= 154
  /* Only available in Cygwin 1.15.20+ */
  cygwin_internal (CW_SYNC_WINENV);
#else
  extern char **environ;
  char **envp = environ;

  if (!envp)
    return;

  while (*envp)
    {
      char *var = xmalloc (strlen (*envp) + 1);
      char *eq;

      strcpy (var, *envp);
      eq = strchr (var, '=');
      if (eq)
	{
	  /* environment variables that need special handling. */
	  static const char * const skipenv[] = {
	    "PATH", "HOME", "LD_LIBRARY_PATH", "TMPDIR", "TMP", "TEMP", NULL
	  };
	  const char **skip;

	  /* Split into name/value pair */
	  *eq = '\0';
	  
	  /* Check for special variables. */
	  for (skip = skipenv; *skip; skip++)
	    if (strcmp (var, *skip) == 0)
	      break;

	  /* Add non-special variables to Windows environment */
	  if (!*skip)
	    SetEnvironmentVariable (var, eq + 1);
	}
      xfree (var);
      ++envp;
    }
#endif /* CYGWIN_VERSION_API_MAJOR == 0 && CYGWIN_VERSION_API_MINOR < 154 */
}
#endif /* __CYGWIN__ */


/* Debuggers and other MDILib using applications must call this
   function prior to calling any of the MDI API functions.  MDIInit()
   loads the MDILib whose pathname is supplied as an argument, or the
   registered "mdi.dll" if NULL is passed, and initializes the global
   function pointers with the addresses of the API functions exported
   from the library. */

static MDIInt32
MDIInit (char *libname, HMODULE *handle)
{
  char *mdilib;			/* the name of the library to open  */
  HMODULE lib;			/* the handle to the opened library */
  char buffer[MAX_PATH];
  int retvalue;
  const struct libsymbol *sym;

  mdilib = libname;

#if defined(_WIN32) || defined(__CYGWIN32__)
  if (!mdilib)
    {
      static int MDIDllName (char *, size_t);
      retvalue = MDIDllName (buffer, MAX_PATH);
      if (!retvalue)
	mdilib = buffer;
    }
#endif

  if (!mdilib)
    mdilib = DEFAULT_LIB_NAME;

#ifdef __CYGWIN32__
  /* Sync Cygwin & Windows environments now, in case we're loading a
     native Windows DLL which might needs a env variable set up within
     the Cygwin environment. Cygwin does this automatically only when
     spawning a new process, not when loading a DLL. */
  cygwin_sync_winenv ();
#endif

  lib = OpenLib (mdilib);
  if (!lib)
    {
#if ! (defined(_WIN32) && !defined(__CYGWIN32__))
      char *error  = dlerror ();
      if (error)
	fprintf_unfiltered (mdi_logfp, "DLL open error: %s\n", error);
#endif
      return MDIErrLoadLib;
    }

  /*
   * The library has been opened, Take all the function pointers
   * defined in mdi.h and point them into the library
   */
  for (sym = symbols; sym->name; sym++)
    {
      *sym->funcp = GetLibSymbolAddress (lib, sym->name);
      if (!*sym->funcp && sym->required)
	{
	  CloseLib (lib);
	  return MDIErrLoadLib;
	}
    }

  *handle = (HMODULE) lib;
  return (MDISuccess);
}

#if defined(_WIN32) || defined(__CYGWIN32__)

/* MDIDllName returns the fully qualified path of the active mdi.dll
   If there is no registered mdi.dll or mdi.dll is incorrectly
   registered MDIDllName will return the fully qualified path of
   mdi.dll as it would be loaded by the OS. */

static int
MDIDllName (char *dllname, size_t size)
{
  int status;
  HKEY keyHandle;
  DWORD type;
  DWORD bufsize;
  unsigned char buffer[MAX_PATH];

  status =
    RegOpenKeyEx (HKEY_LOCAL_MACHINE, "SOFTWARE\\MDI", 0, KEY_READ,
		  &keyHandle);
  if (status == ERROR_SUCCESS)
    {
      bufsize = MAX_PATH;
      status =
	RegQueryValueEx (keyHandle, "Active DLL", 0, &type, buffer, &bufsize);
      RegCloseKey (keyHandle);
      if (status == ERROR_SUCCESS && bufsize <= MAX_PATH)
	{
	  status =
	    RegOpenKeyEx (HKEY_LOCAL_MACHINE, "SOFTWARE\\MDI\\MDIDLL", 0,
			  KEY_READ, &keyHandle);
	  if (status == ERROR_SUCCESS)
	    {
	      bufsize = MAX_PATH;
	      status =
		RegQueryValueEx (keyHandle, (char *) buffer, 0, &type, buffer,
				 &bufsize);
	      RegCloseKey (keyHandle);
	      if (status == ERROR_SUCCESS && bufsize <= MAX_PATH)
		{
		  switch (type)
		    {
		    case REG_EXPAND_SZ:
		      bufsize =
			ExpandEnvironmentStrings ((const char *) buffer,
						  dllname, size);
		      if (bufsize > size)
			{
			  return -1;
			}
		      return 0;

		    case REG_SZ:
		    case REG_MULTI_SZ:
		      strncpy (dllname, (const char *) buffer, size);
		      if (bufsize > size)
			{
			  return -1;
			}
		      return 0;
		    }
		}
	    }
	}
    }

  bufsize = SearchPath (0, DEFAULT_LIB_NAME, 0, size, dllname, 0);
  if (bufsize && bufsize <= MAX_PATH)
    return 0;

  return -1;
}
#endif /* _WIN32 || __CYGWIN32__ */

static int
MDIRelease (HMODULE lib)
{
  const struct libsymbol *sym;

  /* Clear all the function pointers, just in case... */
  for (sym = symbols; sym->name; sym++)
    *sym->funcp = NULL;

  CloseLib (lib);
  return MDISuccess;
}



static MDIInt32
mdiVersion (MDIVersionRangeT * version)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiVersion (-)\n");
  stat = MDIVersion (version);
  if (remote_debug > 1)
    {
      if (stat != MDISuccess)
	fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
      else
	fprintf_unfiltered (mdi_logfp, "returned: version = 0x%x-0x%x\n", 
			    version->oldest, version->newest);
    }
  return stat;
}

static MDIInt32
mdiConnect (MDIVersionT version, MDIHandleT * handle, MDIConfigT * config)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiConnect (0x%08x, -, -)\n", version);
  stat = MDIConnect (version, handle, config);
  if (remote_debug > 1)
    {
      if (stat != MDISuccess)
	fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
      else
	fprintf_unfiltered (mdi_logfp, "returned: handle = 0x%x\n", *handle);
    }
  return stat;
}

static MDIInt32
mdiDisconnect (MDIHandleT MDIHandle, MDIUint32 flags)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiDisconnect (-, flags 0x%x)\n", flags);
  stat = MDIDisconnect (MDIHandle, flags);
  if (remote_debug > 1 && stat != MDISuccess)
    fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
  return stat;
}

static MDIInt32
mdiTGQuery (MDIHandleT MDIHandle, MDIInt32 * HowMany, MDITGDataT * TGData)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiTGQuery (-, &%d, %p)\n", *HowMany,
			TGData);
  stat = MDITGQuery (MDIHandle, HowMany, TGData);
  if (remote_debug > 1)
    {
      if (stat != MDISuccess)
	fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
      else
	fprintf_unfiltered (mdi_logfp, "returned: *HowMany = %d\n", *HowMany);
    }
  return stat;
}

static MDIInt32
mdiTGOpen (MDIHandleT MDIHandle, MDITGIdT TGId, MDIUint32 flags,
	   MDIHandleT * TGHandle)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp,
			"mdiTGOpen (-, TGId 0x%x, flags 0x%x, -)\n", 
			TGId, flags);
  stat = MDITGOpen (MDIHandle, TGId, flags, TGHandle);
  if (remote_debug > 1)
    {
      if (stat != MDISuccess)
	fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
      else
	fprintf_unfiltered (mdi_logfp, "returned: *TGHandle = 0x%x\n", *TGHandle);
    }
  return stat;
}

static MDIInt32
mdiTGClose (MDIHandleT TGHandle, MDIUint32 flags)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiTGClose (0x%x, flags 0x%x)\n", 
			TGHandle, flags);
  stat = MDITGClose (TGHandle, flags);
  if (remote_debug > 1 && stat != MDISuccess)
    fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
  return stat;
}

#if unused
static MDIInt32
mdiTGExecute (MDIHandleT TGHandle)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiTGExecute (0x%x)\n", TGHandle);
  stat = MDITGExecute (TGHandle);
  if (remote_debug > 1 && stat != MDISuccess)
    fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
  return stat;
}

static MDIInt32
mdiTGStop (MDIHandleT TGHandle)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiTGStop (0x%x)\n", TGHandle);
  stat = MDITGStop (TGHandle);
  if (remote_debug > 1 && stat != MDISuccess)
    fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
  return stat;
}
#endif

static MDIInt32
mdiDQuery (MDIHandleT TGHandle, MDIInt32 * HowMany, MDIDDataT * DData)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiDQuery (0x%x, &%d, %p)\n",
			TGHandle, *HowMany, DData);
  stat = MDIDQuery (TGHandle, HowMany, DData);
  if (remote_debug > 1)
    {
      if (stat != MDISuccess)
	fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
      else
	fprintf_unfiltered (mdi_logfp, "returned: *HowMany = %d\n", *HowMany);
    }
  return stat;
}

static MDIInt32
mdiOpen (MDIHandleT TGHandle, MDIDeviceIdT DeviceID, MDIUint32 flags,
	 MDIHandleT * DeviceHandle)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiOpen (0x%x, 0x%x, flags 0x%0x, -)\n",
			TGHandle, DeviceID, flags);
  stat = MDIOpen (TGHandle, DeviceID, flags, DeviceHandle);
  if (remote_debug > 1)
    {
      if (stat != MDISuccess)
	fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
      else
	fprintf_unfiltered (mdi_logfp, "returned: *DevHandle = 0x%x\n", *DeviceHandle);
    }
  return stat;
}

static MDIInt32
mdiClose (MDIHandleT DeviceHandle, MDIUint32 flags)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiClose (0x%x, flags 0x%x)\n", 
			DeviceHandle, flags);
  stat = MDIClose (DeviceHandle, flags);
  if (remote_debug > 1 && stat != MDISuccess)
    fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
  return stat;
}

static MDIInt32
mdiRead (MDIHandleT Device, MDIResourceT SrcR, MDIOffsetT SrcO, void *Buffer,
	 MDIUint32 ObjectSize, MDIUint32 Count)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiRead (0x%x, %s, 0x%s, -, %d, %d)\n", 
			Device, resrcName (SrcR),
			phex_nz (SrcO, sizeof SrcO), ObjectSize, Count);
  stat = MDIRead (Device, SrcR, SrcO, Buffer, ObjectSize, Count);
  if (remote_debug > 1)
    {
      if (stat != MDISuccess)
	fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
      else 
	{
	  fprintf_unfiltered (mdi_logfp, "read:");
	  dump_mem (Buffer, ObjectSize, Count);
	}
    }
  return stat;
}

static MDIInt32
mdiWrite (MDIHandleT Device, MDIResourceT DstR, MDIOffsetT DstO, void *Buffer,
	  MDIUint32 ObjectSize, MDIUint32 Count)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    {
      fprintf_unfiltered (mdi_logfp, "mdiWrite (0x%x, %s, 0x%s, -, %d, %d)\n",
			  Device, resrcName (DstR),
			  phex_nz (DstO, sizeof DstO),
			  ObjectSize, Count);
      fprintf_unfiltered (mdi_logfp, "write:");
      dump_mem (Buffer, ObjectSize, Count);
    }
  stat = MDIWrite (Device, DstR, DstO, Buffer, ObjectSize, Count);
  if (remote_debug > 1 && stat != MDISuccess)
    fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
  return stat;
}

#if unused
static MDIInt32
mdiReadList (MDIHandleT Device, MDIUint32 ObjectSize, MDICRangeT * SrcList,
	     MDIUint32 ListCount, void *Buffer)
{
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiReadList (-, %d, -, %d, -)\n", 
			ObjectSize, ListCount);
  return MDIReadList (Device, ObjectSize, SrcList, ListCount, Buffer);
}

static MDIInt32
mdiWriteList (MDIHandleT Device, MDIUint32 ObjectSize, MDICRangeT * DstList,
	      MDIUint32 ListCount, void *Buffer)
{
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiWriteList (-, %d, -, %d, -)\n", 
			ObjectSize, ListCount);
  return MDIWriteList (Device, ObjectSize, DstList, ListCount, Buffer);
}

static MDIInt32
mdiMove (MDIHandleT Device, MDIResourceT SrcR, MDIOffsetT SrcO,
	 MDIResourceT DstR, MDIOffsetT DstO, MDIUint32 ObjectSize,
	 MDIUint32 Count, MDIUint32 Flag)
{
  if (remote_debug > 1)
    fprintf_unfiltered 
      (mdi_logfp,
       "mdiMove (-, %s, 0x%s, %s, 0x%s, Objsz %d, Cnt %d, Flg 0x%x)\n",
       resrcName (SrcR), phex_nz (SrcO, sizeof SrcO), 
       resrcName (DstR), phex_nz (DstO, sizeof DstO), 
       ObjectSize, Count, Flag);
  return MDIMove (Device, SrcR, SrcO, DstR, DstO, ObjectSize, Count, Flag);
}

static MDIInt32
mdiFill (MDIHandleT Device, MDIResourceT DstR, MDIRangeT DstRng, void *Buffer,
	 MDIUint32 ObjectSize, MDIUint32 Count)
{
  if (remote_debug > 1)
    fprintf_unfiltered
      (mdi_logfp,
       "mdiFill (-, %s, Rng 0x%s-0x%s, -, Objsz %d, Cnt %d)\n",
       resrcName (DstR), phex_nz (DstRng.Start, sizeof DstRng.Start), 
       phex_nz (DstRng.End, sizeof DstRng.End), ObjectSize, Count);
  return MDIFill (Device, DstR, DstRng, Buffer, ObjectSize, Count);
}

static MDIInt32
mdiFind (MDIHandleT Device, MDIResourceT SrcR, MDIRangeT SrcRng, void *Buffer,
	 void *MaskBuffer, MDIUint32 ObjectSize, MDIUint32 Count,
	 MDIOffsetT * FoundOffset, MDIUint32 Mode)
{
  if (remote_debug > 1)
    fprintf_unfiltered
      (
       mdi_logfp,
       "mdiFind (-, %s, Rng 0x%s-0x%s, -, -, Objsz %d, Cnt %d, -, Mode %d)\n",
       resrcName (SrcR), phex_nz (SrcRng.Start, sizeof SrcRng.Start), 
       phex_nz (SrcRng.End, sizeof SrcRng.End), ObjectSize, Count, Mode);
  return MDIFind (Device, SrcR, SrcRng, Buffer, MaskBuffer, ObjectSize, Count,
		  FoundOffset, Mode);
}
#endif

static MDIInt32
mdiExecute (MDIHandleT Device)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiExecute (0x%x)\n", Device);
  stat = MDIExecute (Device);
  if (remote_debug > 1 && stat != MDISuccess)
    fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
  return stat;
}

static MDIInt32
mdiStep (MDIHandleT Device, MDIUint32 Steps, MDIUint32 Mode)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiStep (0x%x, %d, %d)\n", 
			Device, Steps, Mode);
  stat = MDIStep (Device, Steps, Mode);
  if (remote_debug > 1 && stat != MDISuccess)
    fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
  return stat;
}

static MDIInt32
mdiStop (MDIHandleT Device)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiStop (0x%x)\n", Device);
  stat = MDIStop (Device);
  if (remote_debug > 1 && stat != MDISuccess)
    fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
  return stat;
}

static MDIInt32
mdiReset (MDIHandleT Device, MDIUint32 Flag)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiReset (0x%x, Flg 0x%x)\n", 
			Device, Flag);
  stat = MDIReset (Device, Flag);
  if (remote_debug > 1 && stat != MDISuccess)
    fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
  return stat;
}

static MDIInt32
mdiCacheQuery (MDIHandleT Device, MDICacheInfoT * CacheInfo)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiCacheQuery (0x%x, -)\n", Device);
  stat = MDICacheQuery (Device, CacheInfo);
  if (remote_debug > 1 && stat != MDISuccess)
    fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
  return stat;
}

static MDIInt32
mdiCacheFlush (MDIHandleT Device, MDIUint32 Type, MDIUint32 Flag)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiCacheFlush (0x%x, Type %d, Flg 0x%x)\n",
			Device, Type, Flag);
  stat = MDICacheFlush (Device, Type, Flag);
  if (remote_debug > 1 && stat != MDISuccess)
    fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
  return stat;
}

static MDIInt32
mdiRunState (MDIHandleT Device, MDIInt32 WaitTime, MDIRunStateT * runstate)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiRunState (0x%x, Wait %d, -)\n", 
			Device, WaitTime);
  stat = MDIRunState (Device, WaitTime, runstate);
  if (remote_debug > 1)
    {
      fprintf_unfiltered (mdi_logfp, "returned: ");
      if (stat != MDISuccess)
	fprintf_unfiltered (mdi_logfp, "%s\n", mdierrcode (stat));
      else
	mdiDisplayRunState (runstate);
    }
  return stat;
}

static MDIInt32
mdiSetBp (MDIHandleT Device, MDIBpDataT * BpData)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, 
			"mdiSetBp (0x%x, {%s, Enb %d, %s, 0x%s-0x%s, PassC %d, Pass2Go %d})\n",
			Device,
			bptypeName (BpData->Type), BpData->Enabled,
			resrcName (BpData->Resource),
			phex_nz (BpData->Range.Start,
				 sizeof BpData->Range.Start),
			phex_nz (BpData->Range.End,
				 sizeof BpData->Range.End),
			BpData->PassCount,
			BpData->PassesToGo);
  stat = MDISetBp (Device, BpData);
  if (remote_debug > 1)
    {
      if (stat != MDISuccess)
	fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
      else 
	fprintf_unfiltered (mdi_logfp, "returned: BpId=0x%x\n", BpData->Id);
    }	
  return stat;
}

#if unused
static MDIInt32
mdiSetSWBp (MDIHandleT Device, MDIResourceT SrcResource, MDIOffsetT SrcOffset,
	    MDIBpIdT * BpId)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiSetSWBp (0x%x, %s, 0x%s, -)\n",
			Device, resrcName (SrcResource), 
			phex_nz (SrcOffset, sizeof SrcOffset));
  stat = MDISetSWBp (Device, SrcResource, SrcOffset, BpId);
  if (remote_debug > 1)
    {
      if (stat != MDISuccess)
	fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
      else 
	fprintf_unfiltered (mdi_logfp, "returned: BpId=0x%x\n", *BpId);
    }
  return stat;
}
#endif

static MDIInt32
mdiClearBp (MDIHandleT Device, MDIBpIdT BpID)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiClearBp (0x%x, BpID 0x%x)\n", 
			Device, BpID);
  stat = MDIClearBp (Device, BpID);
  if (remote_debug > 1 && stat != MDISuccess)
    fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
  return stat;
}

#if unused
static MDIInt32
mdiEnableBp (MDIHandleT Device, MDIBpIdT BpID)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiEnableBp (0x%x, BpID 0x%x)\n", 
			Device, BpID);
  stat = MDIEnableBp (Device, BpID);
  if (remote_debug > 1 && stat != MDISuccess)
    fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
  return stat;
}

static MDIInt32
mdiDisableBp (MDIHandleT Device, MDIBpIdT BpID)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiDisableBp ()\n");
  stat = MDIDisableBp (Device, BpID);
  if (remote_debug > 1 && stat != MDISuccess)
    fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
  return stat;
}

static MDIInt32
mdiBpQuery (MDIHandleT Device, MDIInt32 * HowMany, MDIBpDataT * BpData)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiBpQuery (0x%x, &%d, -)\n",
			Device, *HowMany);
  stat = MDIBpQuery (Device, HowMany, BpData);
  if (remote_debug > 1)
    {
      if (stat != MDISuccess)
	fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
      else
	fprintf_unfiltered (mdi_logfp, "returned: *HowMany = %d\n", *HowMany);
    }
  return stat;
}
#endif

static MDIInt32
mdiHwBpQuery (MDIHandleT Device, MDIInt32 *HowMany, MDIBpInfoT *BpInfo)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiHwBpQuery (0x%x, &%d, -)\n",
			Device, *HowMany);
  if (MDIHwBpQuery)
    stat = MDIHwBpQuery (Device, HowMany, BpInfo);
  else
    stat = MDIErrUnsupported;
  if (remote_debug > 1)
    {
      if (stat != MDISuccess)
	fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
      else
	fprintf_unfiltered (mdi_logfp, "returned: *HowMany = %d\n", *HowMany);
    }
  return stat;
}

static MDIInt32
mdiDoCommand (MDIHandleT Device, char *Buffer)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiDoCommand (0x%x, \"%s\")\n",
			Device, Buffer);
  stat = MDIDoCommand (Device, Buffer);
  if (remote_debug > 1 && stat != MDISuccess)
    fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
  return stat;
}

static MDIInt32
mdiAbort (MDIHandleT Device)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiAbort (0x%x)\n", Device);
  stat = MDIAbort (Device);
  if (remote_debug > 1 && stat != MDISuccess)
    fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
  return stat;
}

#if unused
static MDIInt32
mdiTraceEnable (MDIHandleT Device)
{
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiTraceEnable (-)\n");
  return MDITraceEnable (Device);
}

static MDIInt32
mdiTraceDisable (MDIHandleT Device)
{
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiTraceDisable (-)\n");
  return MDITraceDisable (Device);
}

static MDIInt32
mdiTraceClear (MDIHandleT Device)
{
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiTraceClear (-)\n");
  return MDITraceClear (Device);
}

static MDIInt32
mdiTraceStatus (MDIHandleT Device, MDIUint32 * Status)
{
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiTraceStatus (-, -)\n");
  return MDITraceStatus (Device, Status);
}

static MDIInt32
mdiTraceCount (MDIHandleT Device, MDIUint32 * FrameCount)
{
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiTraceCount (-, -)\n");
  return MDITraceCount (Device, FrameCount);
}

static MDIInt32
mdiTraceRead (MDIHandleT Device, MDIUint32 FirstFrame, MDIUint32 FrameCount,
	      MDIUint32 IncludeInstructions, MDITrcFrameT * Frames)
{
  if (remote_debug > 1)
    fprintf_filtered
      (mdi_logfp,
       "mdiTraceRead (-, FirstFrame %d, FrameCnt %d, Insts? %d, -)\n",
       FirstFrame, FrameCount, IncludeInstructions);
  return MDITraceRead (Device, FirstFrame, FrameCount, IncludeInstructions,
		       Frames);
}
#endif

static MDIInt32
mdiSetTC (MDIHandleT Device, MDITCIdT TCId)
{
  MDIInt32 stat;

  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiSetTC (0x%x, TCId 0x%x)\n", 
			Device, TCId);
  if (MDISetTC)
    stat = MDISetTC (Device, TCId);
  else if (MDITCSet)
    stat = MDITCSet (Device, TCId);
  else
    stat = MDIErrUnsupported;
  if (remote_debug > 1 && stat != MDISuccess)
    fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
  return stat;
}

static MDIInt32
mdiGetTC (MDIHandleT Device, MDITCIdT *TCId)
{
  MDIInt32 stat;

  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiGetTC (0x%x, -)\n", Device);
  if (MDIGetTC)
    stat = MDIGetTC (Device, TCId);
  else if (MDITCGet)
    stat = MDITCGet (Device, TCId);
  else
    stat = MDIErrUnsupported;
  if (remote_debug > 1)
    {
      if (stat != MDISuccess)
	fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
      else
	fprintf_unfiltered (mdi_logfp, "returned: *TCId = 0x%x\n", *TCId);
    }
  return stat;
}

static MDIInt32
mdiTCQuery (MDIHandleT Device, MDIInt32 *HowMany, MDITCDataT *TCData)
{
  MDIInt32 stat;

  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiTCQuery (0x%x, &%d, -)\n", 
			Device, *HowMany);
  if (MDITCQuery)
    stat = MDITCQuery (Device, HowMany, TCData);
  else if (MDIQueryTC)
    stat = MDIQueryTC (Device, HowMany, TCData);
  else
    stat = MDIErrUnsupported;
  if (remote_debug > 1)
    {
      if (stat != MDISuccess)
	fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
      else
	fprintf_unfiltered (mdi_logfp, "returned: *HowMany = %d\n", *HowMany);
    }
  return stat;
}

static MDIInt32
mdiSetRunMode (MDIHandleT Device, MDITCIdT TCId, MDIUint32 StepMode,
	       MDIUint32 SuspendMode)
{
  MDIInt32 stat;

  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp,
			"mdiSetRunMode (0x%x, TCId 0x%x, %d, %u)\n", 
			Device, TCId, StepMode, SuspendMode);
  if (MDISetRunMode)
    stat = MDISetRunMode (Device, TCId, StepMode, SuspendMode);
  else if (MDISetTCRunMode)
    stat = MDISetTCRunMode (Device, TCId, StepMode != MDINoStep, SuspendMode);
  else if (MDITCSetRunMode)
    stat = MDITCSetRunMode (Device, TCId, StepMode != MDINoStep, SuspendMode);
  else
    stat = MDIErrUnsupported;
  if (remote_debug > 1 && stat != MDISuccess)
    fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
  return stat;
}

static MDIInt32
mdiTeamCreate (MDIHandleT MDIHandle, MDITeamIdT *TeamId)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiTeamCreate (0x%x, -)\n", MDIHandle);
  if (MDITeamCreate)
    stat = MDITeamCreate (MDIHandle, TeamId);
  else if (MDICreateTeam)
    stat = MDICreateTeam (MDIHandle, TeamId);
  else
    stat = MDIErrUnsupported;
  if (remote_debug > 1)
    {
      if (stat != MDISuccess)
	fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
      else
	fprintf_unfiltered (mdi_logfp, "returned: *TeamId = 0x%x\n", *TeamId);
    }
  return stat;
}

static MDIInt32
mdiQueryTeams (MDIHandleT MDIHandle, MDIInt32 *HowMany, MDITeamIdT *TeamId)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiQueryTeams (0x%x, &%d, -)\n",
			MDIHandle, *HowMany);
  if (MDIQueryTeams)
    stat = MDIQueryTeams (MDIHandle, HowMany, TeamId);
  else
    stat = MDIErrUnsupported;
  if (remote_debug > 1)
    {
      if (stat != MDISuccess)
	fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
      else
	fprintf_unfiltered (mdi_logfp, "returned: *HowMany = %d\n", *HowMany);
    }
  return stat;
}

static MDIInt32
mdiTeamClear (MDIHandleT MDIHandle, MDITeamIdT TeamId)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiTeamClear (0x%x, TeamId 0x%x)\n",
			MDIHandle, TeamId);
  if (MDITeamClear)
    stat = MDITeamClear (MDIHandle, TeamId);
  else if (MDIClearTeam)
    stat = MDIClearTeam (MDIHandle, TeamId);
  else
    stat = MDIErrUnsupported;
  if (remote_debug > 1 && stat != MDISuccess)
    fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
  return stat;
}

static MDIInt32
mdiTeamDestroy (MDIHandleT MDIHandle, MDITeamIdT TeamId)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiTeamDestroy (0x%x, TeamId 0x%x)\n",
			MDIHandle, TeamId);
  if (MDITeamDestroy)
    stat = MDITeamDestroy (MDIHandle, TeamId);
  else if (MDIDestroyTeam)
    stat = MDIDestroyTeam (MDIHandle, TeamId);
  else
    stat = MDIErrUnsupported;
  if (remote_debug > 1 && stat != MDISuccess)
    fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
  return stat;
}

static MDIInt32
mdiTMAttach (MDIHandleT MDIHandle, MDITeamIdT TeamId, MDITMDataT *TMData)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiTMAttach "
			"(0x%x, TeamId 0x%x, { 0x%x, TGId 0x%x, 0x%x })\n",
			MDIHandle, TeamId,
			TMData->MDIHandle, TMData->TGId, TMData->DevId);
  if (MDITMAttach)
    stat = MDITMAttach (MDIHandle, TeamId, TMData);
  else if (MDIAttachTM)
    stat = MDIAttachTM (MDIHandle, TeamId, TMData);
  else
    stat = MDIErrUnsupported;
  if (remote_debug > 1 && stat != MDISuccess)
    fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
  return stat;
}

static MDIInt32
mdiTMDetach (MDIHandleT MDIHandle, MDITeamIdT TeamId, MDITMDataT *TMData)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiTMDetach "
			"(0x%x, TeamId 0x%x, { 0x%x, TGId 0x%x, 0x%x })\n",
			MDIHandle, TeamId,
			TMData->MDIHandle, TMData->TGId, TMData->DevId);
  if (MDITMDetach)
    stat = MDITMDetach (MDIHandle, TeamId, TMData);
  else if (MDIDetachTM)
    stat = MDIDetachTM (MDIHandle, TeamId, TMData);
  else
    stat = MDIErrUnsupported;
  if (remote_debug > 1 && stat != MDISuccess)
    fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
  return stat;
}

static MDIInt32
mdiQueryTM (MDIHandleT MDIHandle, MDITeamIdT TeamId, MDIInt32 *HowMany,
	    MDITMDataT *TMData)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiQueryTM (0x%x, TeamId 0x%x, &%d, -)\n",
			MDIHandle, TeamId, *HowMany);
  if (MDIQueryTM)
    stat = MDIQueryTM (MDIHandle, TeamId, HowMany, TMData);
  else if (MDITMQuery)
    stat = MDITMQuery (MDIHandle, TeamId, HowMany, TMData);
  else
    stat = MDIErrUnsupported;
  if (remote_debug > 1)
    {
      if (stat != MDISuccess)
	fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
      else
	fprintf_unfiltered (mdi_logfp, "returned: *HowMany = %d\n", *HowMany);
    }
  return stat;
}

static MDIInt32
mdiTeamExecute (MDIHandleT MDIHandle, MDITeamIdT TeamId)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mdiTeamExecute (0x%x, TeamId 0x%x)\n",
			MDIHandle, TeamId);
  if (MDITeamExecute)
    stat = MDITeamExecute (MDIHandle, TeamId);
  else
    stat = MDIErrUnsupported;
  if (remote_debug > 1 && stat != MDISuccess)
    fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
  return stat;
}

static MDIInt32
mipssim_SetConfigFile (MDIHandleT handle, char *name)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mipssim_SetConfigFile (0x%x, \"%s\")\n",
			handle, name);
  stat = MIPSsim_SetConfigFile (handle, name);
  if (remote_debug > 1 && stat != MDISuccess)
    fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
  return stat;
}

static MDIInt32
mipssim_CreateProfile (MDIHandleT handle, MDIInt32 *pid, 
		       MDIUint64 start, MDIUint64 size)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, 
			"mipssim_CreateProfile (0x%x, -, 0x%s, 0x%s)\n", 
			handle, 
			phex_nz (start, sizeof start), 
			phex_nz (size, sizeof size));
  stat = MIPSsim_CreateProfile (handle, pid, start, size);
  if (remote_debug > 1)
    {
      if (stat != MDISuccess)
	fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
      else
	fprintf_unfiltered (mdi_logfp, "returned: *pid = %d\n", *pid);
    }
  return stat;
}

static MDIInt32
mipssim_DestroyProfile (MDIHandleT handle, MDIInt32 id)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mipssim_DestroyProfile (0x%x, 0x%x)\n", 
			handle, id);
  stat = MIPSsim_DestroyProfile (handle, id);
  if (remote_debug > 1 && stat != MDISuccess)
    fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
  return stat;
}

static MDIInt32
mipssim_StartProfile (MDIHandleT handle, MDIInt32 id)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mipssim_StartProfile (0x%x, 0x%x)\n", 
			handle, id);
  stat = MIPSsim_StartProfile (handle, id);
  if (remote_debug > 1 && stat != MDISuccess)
    fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
  return stat;
}

static MDIInt32
mipssim_StopProfile (MDIHandleT handle, MDIInt32 id)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mipssim_StopProfile (0x%X, 0x%x)\n", 
			handle, id);
  stat = MIPSsim_StopProfile (handle, id);
  if (remote_debug > 1 && stat != MDISuccess)
    fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
  return stat;
}

#if unused
static MDIInt32
mipssim_ClearProfile (MDIHandleT handle, MDIInt32 id)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mipssim_ClearProfile (0x%x, 0x%x)\n", 
			handle, id);
  stat = MIPSsim_ClearProfile (handle, id);
  if (remote_debug > 1 && stat != MDISuccess)
    fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
  return stat;
}
#endif

static MDIInt32
mipssim_FetchProfile (MDIHandleT handle, MDIInt32 id,
		      MDIUint32 **cntbufp, MDIUint64 *cyclesp)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mipssim_FetchProfile (0x%x, 0x%x, -, -)\n",
			handle, id);
  stat = MIPSsim_FetchProfile (handle, id, cntbufp, cyclesp);
  if (remote_debug > 1 && stat != MDISuccess)
    fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
  return stat;
}

static MDIInt32
mipssim_FreeProfileData (MDIHandleT handle, MDIUint32 **cntbufp)
{
  MDIInt32 stat;
  if (remote_debug > 1)
    fprintf_unfiltered (mdi_logfp, "mipssim_FreeProfileData (0x%x, -)\n", 
			handle);
  stat = MIPSsim_FreeProfileData (handle, cntbufp);
  if (remote_debug > 1 && stat != MDISuccess)
    fprintf_unfiltered (mdi_logfp, "returned: %s\n", mdierrcode (stat));
  return stat;
}



static void
togdb_force_update (void)
{
#ifdef GDBTK
  if (gdbtk_interp != NULL)
    Tcl_Eval (gdbtk_interp, "gdbtk_update");
#endif
}

static void
check_mdi (void)
{
  if (mdi_desc == NULL)
    error ("MDI connection not opened yet, use the 'target mdi' command.");
};

static int
isa2int (char *str)
{
  const struct isa_xlate_struct *p;

  for (p = isa_xlate; p->isa && strcmp (p->isa, str); p++)
    continue;
  return p->num;
}


static void
dump_mem (void *buffer, int elsize, int cnt)
{
  unsigned char *buf = buffer;
  int col = 0;
  int width;
  int n;

  if (elsize > sizeof(ULONGEST))
    {
      fprintf_unfiltered (mdi_logfp, "Cannot print objects of size %d\n", 
			elsize);
      return;
    }

  if (elsize == 0)
    elsize = 1;

  n = (cnt * elsize > 128) ? 128 / elsize : cnt;

  width = 16 / elsize;

  while (n-- > 0)
    {
      ULONGEST val = extract_unsigned_integer (buf, elsize);
      buf += elsize;
      cnt -= 1;

      if (col == 0)
	fprintf_unfiltered (mdi_logfp, "\t");

      if (elsize > 1)
	fprintf_unfiltered (mdi_logfp, "%s ", phex (val, elsize));
      else
	fprintf_unfiltered (mdi_logfp, "%02x ", (unsigned int)val & 0xff);
      
      if (++col == width || n == 0)
	{
	  /* Output ascii dump for byte size elements. */
	  if (elsize == 1)
	    {
	      int limit = col;
	      buf -= col;	/* backup over printed bytes */
	      /* Space across to ascii column. */
	      for ( ; col < width + 1; col++)
		fprintf_unfiltered (mdi_logfp, "   ");
	      for (col = 0; col < limit; col++)
		{
		  fputc_unfiltered (isprint (*buf) ? *buf : '.', mdi_logfp);
		  buf++;
		}
	    }
	  fprintf_unfiltered (mdi_logfp, "\n");
	  col = 0;
	}
    }

  if (col != 0)
    fprintf_unfiltered (mdi_logfp, "\n");
  if (cnt != 0)
    fprintf_unfiltered (mdi_logfp, "\tetc...\n");
}


static const char *
mdierrcode (MDIInt32 err)
{
  const char *es;
  static char buf[64];
  int i;

  for (i = 0; (es = errorcodes[i].str) && errorcodes[i].errorcode != err; i++)
    continue;

  if (es)
    return es;

  sprintf (buf, "MDIerr=%d", err);
  return buf;
}

static void
mdierr (const char *file, int line, MDIInt32 err, const char *what)
{
  const char *es = mdierrcode (err);
  if (remote_debug)
    error ("%s:%d: %s failed (%s)", file, line, what, es);
  else
    error ("MDI %s failed (%s)", what, es);
}

#define MDIERR(op, what) \
  do { \
    MDIInt32 __stat = (op); \
    if (__stat != MDISuccess) \
      mdierr (__FILE__, __LINE__, __stat, what); \
  } while (0)

static void
mdiwarn (char *file, int line, MDIInt32 err, const char *what)
{
  const char *es = mdierrcode (err);

  if (remote_debug)
    warning ("%s:%d: %s failed (%s)", file, line, what, es);
  else
    warning ("MDI %s failed (%s)", what, es);
}

#define MDIWARN(op, what) \
  do { \
    MDIInt32 __stat = (op); \
    if (__stat != MDISuccess) \
      mdiwarn (__FILE__, __LINE__, __stat, what); \
  } while (0)



static MDIInt32 __stdcall
mdiPeriodic (MDIHandleT handle)
{
  int stop = 0;

  /* Only call ui_loop_hook if this callback is from mdi_wait or
     mdi_resume(step==1).
     
     Unfortunately MDIRead/MDIWrite also call the periodic callback
     and that can lead to problems where the GUI tries to refresh the
     memory window etc. */

  if (deprecated_ui_loop_hook != NULL && (mdi_wait_active || mdi_step_active))
    stop = deprecated_ui_loop_hook (0);

  if (stop)
    quit_flag = 1;
  if (quit_flag)		/* gdb's idea of quit */
    {
      MDIInt32 mdistat;
      if (mdi_step_active)
	mdistat = mdiAbort (handle);
      /* XXX how do we stop a normal wait? */
    }

  if (remote_debug)
    {
      static int progress = 0;
      if ((progress++ % 30) == 0)
	{
	  printf_unfiltered (".");
	  gdb_flush (gdb_stdout);
	  if (quit_flag)
	    printf_unfiltered ("mdiPeriodic: quitting\n");
	}
    }

  return MDISuccess;
}



static char *
check_for_template (const char *dir)
{
  size_t dirlen;
  char *buf;
  struct stat s;

  if (remote_debug)
    fprintf_unfiltered (mdi_logfp, "checking \"%s\" for mipssim.cfg\n",
			dir);

  dirlen = strlen (dir);
  buf = (char *) xmalloc (dirlen + sizeof("/mipssim.cfg"));
  sprintf (buf, "%s/mipssim.cfg", dir);

  if (stat (buf, &s) == 0 && S_ISREG (s.st_mode))
    return buf;

  xfree (buf);
  return NULL;
}


static void
mdi_setconfigfile (struct mdi_state *mdi, MDIDDataT *ddata, char *cfg)
{
  struct cleanup *cleanups = NULL;
  MDIInt32 mdistat;

  if (! MIPSsim_SetConfigFile)
    return;

  if (! cfg || ! *cfg)
    {
      char tmpfile[sizeof ("/tmp/gmdiXXXXXX")];
      char devtmpfile[sizeof ("/tmp/gmddXXXXXX")];
      char line[256];
      int matches = 0;
      char *template = NULL;
      char *dname;
      char *end;
      int cfgfd;
      int copy;
      FILE *infile, *outfile;
      size_t dlen;
      MDIInt32 major, minor, patch;
      
      if (remote_debug)
	fprintf_unfiltered (mdi_logfp, "gdb_program_name=\"%s\"\n",
			    gdb_program_name);
      
      end = strrchr (gdb_program_name, '/');
#if defined(_WIN32) || defined(__CYGWIN32__)
      {
	/* We could have \foo\bar, or /foo\bar.  */
	char *bslash = strrchr (gdb_program_name, '\\');
	if (bslash > end)
	  end = bslash;
      }
#endif
      
      if (end)
	{
	  /* Make a copy of program_name in dir.
	     Leave room for later "/../share".  */
	  size_t dirlen = end - gdb_program_name;
	  char *dir = (char *) alloca (dirlen + sizeof ("/../share"));
	  strncpy (dir, gdb_program_name, dirlen);
	  dir[dirlen] = '\0';
	  template = check_for_template (dir);
	  if (! template)
	    {
	      /* Try relative path - look in /../share.  */
	      strcpy (dir + dirlen, "/../share");
	      template = check_for_template (dir);
	    }
	}
      
      if (! template)
	{
	  /* Try looking for it in the directories on the search path. */
	  char *path = getenv ("PATH");
	  if (path)
	    {
	      char *sp;
	      path = xstrdup (path);
	      for (sp = strtok (path, ":"); ! template && sp;
		   sp = strtok (NULL, ":"))
		{
		  size_t dirlen = strlen (sp);
		  char *dir;
		  if (dirlen == 0)
		    continue;
		  /* look in /path/../share.  */
		  dir = (char *) xmalloc (dirlen + sizeof ("/../share"));
		  strcpy (dir, sp);
		  strcpy (dir + dirlen, "/../share");
		  template = check_for_template (dir);
		  xfree (dir);
		}
	      xfree (path);
	    }
	}

      if (! template)
	/* Try in the "standard" directory. */
	template = check_for_template (MIPSSIM_LIBRARY);

      if (! template)
	error ("Cannot find MIPSsim config file template: mipssim.cfg");
      
      cleanups = make_cleanup (xfree, template);

      infile = fopen (template, FOPEN_RT);
      if (! infile)
	error ("Cannot open MIPSsim config template: %s", template);
      
      strcpy (tmpfile, "/tmp/gmdiXXXXXX");
      cfgfd = mkstemp (tmpfile);
      if (cfgfd < 0)
	error ("Cannot create temp MIPSsim config file");

      if (remote_debug)
	printf_unfiltered ("Generating MIPSsim cfg from %s to %s for \"%s\"\n",
			   template, tmpfile, ddata->DName);

      mdi->cfgfile = cfg = xstrdup (tmpfile);

      outfile = fdopen (cfgfd, FOPEN_WT);
      if (! outfile)
	error ("Cannot write to temp MIPSsim config file");
      

      /* Get MIPSsim version number */
      major = MIPSsim_ZeroPerfCounter ? 4 : 3;
      minor =0; patch=0;
      if (MIPSsim_GetVersion)
	(void) MIPSsim_GetVersion (mdi->MDIHandle, &major, &minor, &patch);

      dname = ddata->DName;

      /* skip unneeded leading part of name from MIPSsim 3.x */
      if (strncmp (dname, "MIPS32_", sizeof "MIPS32_" - 1) == 0)
	dname += sizeof "MIPS32_" - 1;
      else if (strncmp (dname, "MIPS64_", sizeof "MIPS64_" - 1) == 0)
	dname += sizeof "MIPS64_" - 1;

      /* Construct string to match a regexp pattern against which
	 includes MISPsim version number followed by CPU name. */
      asprintf (&dname, "%d.%d.%d;%s", major, minor, patch, dname);
      if (! dname)
	nomem (0);

      cleanups = make_cleanup (xfree, dname);
      dlen = strlen (dname);
      copy = 1;

      while (fgets (line, sizeof (line), infile))
	{
	  char *lp;

	  for (lp = line; *lp && isspace (*lp); lp++)
	    continue;
	  if (! *lp || *lp == '#')
	    continue;
	  
	  if (*lp == '{')
	    {
	      char *ep;
	      int plen;
	      int invert = 0;
	      for (ep = ++lp; *ep && *ep != '}'; ep++)
		continue;
	      if (*ep != '}')
		continue;
	      if (*lp == '!')
		{
		  ++lp;
		  invert = 1;
		}
	      plen = ep - lp;
	      if (! strncasecmp (lp, "COMMON", plen))
		copy = 1;
	      else if (! strncasecmp (lp, "EB", plen))
		copy = ddata->Endian == MDIEndianBig;
	      else if (! strncasecmp (lp, "EL", plen))
		copy = ddata->Endian == MDIEndianLittle;
	      else if (! strncasecmp (lp, "DEBUG0", plen))
		copy = remote_debug == 0;
	      else if (! strncasecmp (lp, "DEBUG1", plen))
		copy = remote_debug == 1;
	      else if (! strncasecmp (lp, "DEBUG2", plen))
		copy = remote_debug >= 2;
	      else 
		{
		  char * patstr;
		  regex_t pattern;
		  int ec;

		  /* For input {xxx} make regex */
		  if (asprintf (&patstr, "^(%.*s)([^-a-z0-9]|$)", plen, lp) == 0)
		    nomem (0);

		  if ((ec = regcomp (&pattern, patstr, REG_ICASE | REG_EXTENDED)) == 0)
		    {
		      copy = re_search (&pattern, dname, dlen,
					0, dlen, NULL) >= 0;
		      if (copy)
			++matches;
		      regfree (&pattern);
		    }
		  else
		    {
		      char errorbuffer[512];
		      regerror (ec, NULL, errorbuffer, sizeof errorbuffer);
		      warning ("Error in regexp /%s/: %s", patstr,
			       errorbuffer);
		      copy = 0;
		    }
		  free (patstr);
		}
	      copy ^= invert;
	      continue;
	    }
	  
	  if (copy)
	    fputs (line, outfile);
	}
      
      fclose (infile);

      if (mdi_devcfgfile)
	{
	  char *devcfg = mdi_devcfgfile;
#ifdef __CYGWIN32__
	  /* Convert the Cygwin name to a Windows name. */
	  char winpath[MAX_PATH];
	  if (cygwin_conv_to_win32_path (devcfg, winpath) == 0)
	    devcfg = winpath;
#endif
	  if (remote_debug)
	    printf_unfiltered ("Adding \"set DEV_CFG %s\" to config file\n", devcfg);
	  fprintf (outfile, "set DEV_CFG %s\n", devcfg);
	}

      fclose (outfile);
      if (matches == 0)
	error ("No entries matching MIPSsim CPU '%s' in %s",
	       dname, template);
    }

#ifdef __CYGWIN32__
  /* Convert the Cygwin name to a Windows name. */
  {
    char winpath[MAX_PATH];
    if (cygwin_conv_to_win32_path (cfg, winpath) == 0)
      cfg = winpath;
  }
#endif

  mdistat = mipssim_SetConfigFile (mdi->MDIHandle, cfg);
  if (mdistat != MDISuccess)
    error ("MIPSsim cannot access config file: \"%s\"", cfg);

  if (cleanups)
    do_cleanups (cleanups);
}



static MDIInt32
SelectDevice (MDIDDataT * ddata, MDIInt32 n, int device)
{
  int i;
  char *s;
  unsigned long value;

  if (device > n)
    error ("Bad device number %d, only %d devices available.", device, n);

  if (device > 0 && device <= n)
    return device - 1;

  if (n == 1)
    return (0);

#ifdef GDBTK
  /* Using command_line_input when GUI is active breaks the console window. */
  if (gdbtk_interp != NULL)
    error ("Use \"set mdi device\" to select MDI device.");
#endif

retry:
  printf_filtered ("Select Device:\n");
  for (i = 0; i < n; i++)
    {
      /* Skip devices called "RESERVED" */
      if (strncasecmp (ddata[i].DName, "RESERVED", sizeof "RESERVED" - 1) == 0)
	continue;
      printf_filtered ("  %02d) %s\n", i + 1, ddata[i].DName);
    }

  reinitialize_more_filter ();
  s = command_line_input ("Enter Number (or q to quit) > ", 0,
			  "mdidevice-choice");

  if (s == NULL)
    error ("Unexpected EOF.");

  while (*s == ' ' || *s == '\t')
    ++s;
  if (*s == 'q')
    throw_exception (RETURN_QUIT);

  value = strtoul (s, &s, 0);
  if (value < 1 || value > n || *s)
    goto retry;

  return (value - 1);
}

static MDIInt32
SelectTarget (MDITGDataT * tgdata, MDIInt32 n, int target)
{
  int i;
  char *s;
  unsigned long value;

  if (target > n)
    error ("Bad target number %d, only %d targets available.", target, n);

  if (target > 0 && target <= n)
    return target - 1;

  if (n == 1)
    return 0;

#ifdef GDBTK
  /* Using command_line_input when GUI is active breaks the console window. */
  if (gdbtk_interp != NULL)
    error ("Use \"set mdi target\" to select MDI target group.");
#endif

retry:
  printf_filtered ("Select Target Group:\n");
  for (i = 0; i < n; i++)
    {
      /* Skip targets called "RESERVED" */
      if (strncasecmp (tgdata[i].TGName, "RESERVED",
		       sizeof "RESERVED" - 1) == 0)
	continue;    
      printf_filtered ("  %02d) %s\n", i + 1, tgdata[i].TGName);
    }

  reinitialize_more_filter ();
  s = command_line_input ("Enter Number (or q to quit) > ", 0,
			  "mditarget-choice");
  if (s == NULL)
    error ("Unexpected EOF.");

  while (*s == ' ' || *s == '\t')
    ++s;
  if (*s == 'q')
    throw_exception (RETURN_QUIT);

  value = strtoul (s, &s, 0);
  if (value < 1 || value > n || *s)
    goto retry;

  return (value - 1);
}

static RETSIGTYPE (*ofunc) ();
static RETSIGTYPE mdi_interrupt (int signo);
static void (*interrupt_cleanup)(void);

static void
open_cleanup (void)
{
}

static void
haltDev (struct mdi_state *mdi)
{
  MDIInt32 mdistat;
  MDIRunStateT runstate;
  struct timeval now, when;
  int timedout, interrupted, forever;
  int stopcount;
  int i;

  /* Wait the specified number of seconds for CPU to halt */
  gettimeofday (&now, NULL);
  when = now;
  when.tv_sec += mdi_connecttimeout;
  forever = (mdi_connecttimeout == UINT_MAX);
  timedout = interrupted = 0;

  /* Install our SIGINT signal handler. */
  interrupt_cleanup = open_cleanup;
  ofunc = signal (SIGINT, mdi_interrupt);

  stopcount = 0;
  i = 0;

  do {
    mdistat = mdiRunState (mdi->gmdata[i].DevHandle, 100, &runstate);
    if (mdistat != MDISuccess)
      break;

    if ((runstate.Status & MDIStatusMask) == MDIStatusRunning
	|| runstate.Status == MDIStatusDisabled
	|| runstate.Status == MDIStatusVPEDisabled)
      {
	mdistat = mdiStop (mdi->gmdata[i].DevHandle);
	if (mdistat != MDISuccess)
	  break;

	mdistat = mdiRunState (mdi->gmdata[i].DevHandle, 100, &runstate);
	if (mdistat != MDISuccess)
	  break;
      }

    if (runstate.Status == MDIStatusDisabled
	|| runstate.Status == MDIStatusVPEDisabled)
      {
	mdi->gmdata[i].disabled = 1;
	if (mdi->gmcount == 1)
	  forever = 1;
      }
    else if ((runstate.Status & MDIStatusMask) == MDIStatusRunning)
      forever = (mdi_connecttimeout == UINT_MAX);
    else
      {
	mdi->gmdata[i].disabled = 0;
	stopcount++;
	if (stopcount == 1)
	  mdi->gmindex = i;
	if (stopcount == mdi->gmcount)
	  break;
      }

    i = (i + 1) % mdi->gmcount;

    /* Only time out after a full poll of all devices.  */
    if (i == 0 && (!forever || stopcount > 0))
      {
	gettimeofday (&now, NULL);
	if (timercmp (&now, &when, >))
	  {
	    timedout = 1;
	    break;
	  }
      }
    /* See if the user has asked us to stop. */
    if (quit_flag || (deprecated_ui_loop_hook && deprecated_ui_loop_hook (0)))
      {
	interrupted = 1;
	break;
      }
    sleep_ms (100);
  } while (1);

  /* Restore old SIGINT handler. */
  signal (SIGINT, ofunc);

  if (timedout && stopcount == 0)
    error ("Cannot halt CPU");
  else if (interrupted)
    error ("Interrupted while waiting for the device");
  else
    MDIERR (mdistat, "halting CPU");
}

static void
resetDev (struct mdi_state *mdi, int connreset)
{
  MDIInt32 mdistat;
  MDIRunStateT runstate;
  struct timeval now, when;

  if (connreset >= 0)
    {
      /* Reset the device, wait "connectreset" seconds, then stop it. */
      mdistat = mdiReset (mdi->gmdata[0].DevHandle, MDIFullReset);
      if (mdistat == MDISuccess && connreset > 0  && ! mdi->mipssim_p)
	{
	  /* Start CPU execution. */
	  if (mdi->gmcount > 1)
	    {
	      mdistat = mdiTeamExecute (mdi->MDIHandle, mdi->gid);
	      if (mdistat == MDIErrUnsupported)
		{
		  int i;

		  for (i = 0; i < mdi->gmcount; i++)
		    {
		      mdistat = mdiExecute (mdi->gmdata[i].DevHandle);
		      if (mdistat != MDIErrDisabled)
			MDIWARN (mdistat, "starting execution");
		    }
		  mdistat = MDISuccess;
		}
	    }
	  else
	    mdistat = mdiExecute (mdi->gmdata[0].DevHandle);
	  if (mdistat == MDISuccess)
	    {
	      /* Keep polling MDI until "connectreset" seconds elapsed. */
	      gettimeofday (&now, NULL);
	      when = now;
	      when.tv_sec += connreset;
	      do {
		/* Poll MDI - may be necessary for some probes. */
		(void) mdiRunState (mdi->gmdata[0].DevHandle, 200, &runstate);
		sleep_ms (200);
		gettimeofday (&now, NULL);
	      } while (timercmp (&now, &when, <));
	      /* Halt CPU. */
	      (void) mdiStop (mdi->gmdata[0].DevHandle);
	    }
	}
    }
}

static MDIInt32
getDevs (MDIHandleT tg, int index, MDIDDataT **ddata, MDIInt32 *devices)
{
  MDIInt32 mdistat;

  *ddata = NULL;
  *devices = 0;

  mdistat = mdiDQuery (tg, devices, NULL);
  if (mdistat != MDISuccess)
    return mdistat;

  *ddata = (MDIDDataT *) xmalloc (*devices * sizeof (MDIDDataT));
  mdistat = mdiDQuery (tg, devices, *ddata);

  return mdistat;
}

static void
updateDev (struct mdi_state *mdi, int index, MDIDeviceIdT device)
{
  struct cleanup *cleanups;
  MDIDDataT *ddata = NULL;
  MDIInt32 devices, i;
  MDIInt32 mdistat;

  mdistat = getDevs (mdi->gmdata[index].TGHandle, index, &ddata, &devices);
  cleanups = make_cleanup (xfree, ddata);
  MDIERR (mdistat, "device query");

  for (i = 0; i < devices; i++)
    if (ddata[i].Id == device)
      memmove (&mdi->gmdata[index].DeviceData, ddata + i, sizeof (MDIDDataT));

  do_cleanups (cleanups);
}

static void
openDev (struct mdi_state *mdi, int index, char *configfile)
{
  struct cleanup *cleanups;
  MDIDDataT *ddata = NULL;
  MDIDDataT *dp;
  MDIInt32 devices;
  MDIDeviceIdT device;
  MDIInt32 mdistat;

  mdi->gmdata[index].disabled = 1;

  device = mdi->gmdata[index].tmdata.DevId + 1;

  mdistat = getDevs (mdi->gmdata[index].TGHandle, index, &ddata, &devices);
  cleanups = make_cleanup (xfree, ddata);
  MDIERR (mdistat, "device query");

  if (devices == 0)
    error ("No MDI devices available");

  device = SelectDevice (ddata, devices, device);
  dp = &ddata[device];

  if (remote_debug)
    fprintf_unfiltered (mdi_logfp,
			"selected dname '%s' fpart '%s' vendor '%s' vfamily '%s' vpart '%s' vpartrev '%s' vpartdata '%s'\n",
			dp->DName, dp->FPart, dp->Vendor, dp->VFamily,
			dp->VPart, dp->VPartRev, dp->VPartData);

  mdi->mipssim_p = (strcmp (dp->VPartData, "MIPSsim") == 0);
  if (mdi->mipssim_p && index == 0)
    mdi_setconfigfile (mdi, dp, configfile);

  /* Old versions used to force mdi_continueonclose to "0" for
     MIPSsim.  Not anymore, but keep this value as the default.  */
  if (mdi_continueonclose == -1)
    {
      if (mdi->mipssim_p)
	mdi_continueonclose = 0;
      else
	mdi_continueonclose = 1;
    }

  mdistat = mdiOpen (mdi->gmdata[index].TGHandle, dp->Id, MDIExclusiveAccess,
		     &mdi->gmdata[index].DevHandle);
  MDIERR (mdistat, "device open");

  if (dp->VPartData[0] && strcmp (dp->VPartData, "Unknown") != 0)
    printf_filtered ("Selected device %s on %s %s\n", dp->DName,
		     dp->Vendor, dp->VPartData);
  else
    printf_filtered ("Selected device %s\n", dp->DName);

  updateDev (mdi, index, dp->Id);

  do_cleanups (cleanups);
}


static void
openTG (struct mdi_state *mdi, int index)
{
  struct cleanup *cleanups;
  MDIInt32 mdistat = MDISuccess;
  MDITGDataT *tgdata = NULL;
  MDIInt32 targets;
  MDITGIdT target;

  if ((mdi->Config.MDICapability & MDICAP_TargetGroups) == 0)
    {
      mdi->gmdata[index].TGHandle = mdi->gmdata[index].tmdata.MDIHandle;
      return;
    }

  target = mdi->gmdata[index].tmdata.TGId + 1;
  targets = 0;
  mdistat = mdiTGQuery (mdi->gmdata[index].tmdata.MDIHandle, &targets, NULL);
  MDIERR (mdistat, "target group query");

  tgdata = (MDITGDataT *) xmalloc (targets * sizeof (MDITGDataT));
  cleanups = make_cleanup (xfree, tgdata);

  mdistat = mdiTGQuery (mdi->gmdata[index].tmdata.MDIHandle, &targets, tgdata);
  MDIERR (mdistat, "target group query");

  target = SelectTarget (tgdata, targets, target);

  mdistat = mdiTGOpen (mdi->gmdata[index].tmdata.MDIHandle,
		       tgdata[target].TGId, MDISharedAccess,
		       &mdi->gmdata[index].TGHandle);
  if (mdistat != MDISuccess)
    /* Failed shared access - try exclusive access. */
    mdistat = mdiTGOpen (mdi->gmdata[index].tmdata.MDIHandle,
			 tgdata[target].TGId, MDIExclusiveAccess,
			 &mdi->gmdata[index].TGHandle);
  MDIERR (mdistat, "target group open");

  if (remote_debug)
    printf_filtered ("Selected target group %s\n", tgdata[target].TGName);
  mdi->gmdata[index].tmdata.TGId = tgdata[target].TGId;

  do_cleanups (cleanups);
}

static int __stdcall
mdiDbgOutput (MDIHandleT handle, MDIInt32 Type, char *Buffer, MDIInt32 Count)
{
  if (remote_debug > 1)
    {
      fprintf_unfiltered (mdi_logfp, "MDICBOutput (0x%x, %s, -, %d) called\n",
			  handle, lookupName (cbtype_Names, Type), Count);
      fprintf_unfiltered (mdi_logfp, "data:");
      dump_mem (Buffer, 0, Count);
    }

  if (Type == MDIIOTypeMDIErr || Type == MDIIOTypeTgtErr)
    {
      ui_file_write (gdb_stdtargerr, Buffer, Count);
      gdb_flush (gdb_stdtargerr);
    }
  else if (! mdi_quiet)
    {
      ui_file_write (gdb_stdtarg, Buffer, Count);
      gdb_flush (gdb_stdtarg);
    }
  return MDISuccess;
}


static int __stdcall
mdiDbgInput (MDIHandleT handle, MDIInt32 Type, MDIInt32 Mode,
	     char **Buffer, MDIInt32 * Count)
{
  static char *lastline = NULL;
  char *line;
  extern FILE *instream;

  if (deprecated_readline_begin_hook)
    (*deprecated_readline_begin_hook) ("\nMDI library has requested input:\n");
  else if (input_from_terminal_p ())
    printf_filtered ("\nMDI library has requested input:\n");

  /* Make sure that all output has been output.  Some machines may let
     you get away with leaving out some of the gdb_flush, but not all.  */
  wrap_here ("");
  gdb_flush (gdb_stdout);
  gdb_flush (gdb_stderr);

  if (deprecated_readline_hook)
    line = (*deprecated_readline_hook) ("MDILIB> ");
  else if (input_from_terminal_p ())
    {
      line = gdb_readline_wrapper ("MDILIB> ");
      if (line && *line)	/* add to command history */
	add_history (line);
    }
  else
    line = gdb_readline (0);

  if (deprecated_readline_end_hook)
    (*deprecated_readline_end_hook) ();

  /* Avoid consuming too much memory. */
  if (lastline)
    {
      free (lastline);
      lastline = line;
    }

  *Buffer = line;
  *Count = strlen (line);
  return MDISuccess;
}



static void
mdi_dlopen (char *lib, struct mdi_state *mdi, int from_tty)
{
  MDIInt32 mdistat;
  MDIVersionRangeT vrange;
  MDIVersionT api;

  /* MDIInit must be called to load the shared library. */
  if (!mdiModule)
    {
      mdistat = MDIInit (lib, &mdiModule);
      if (mdistat == MDIErrLoadLib)
	{
	  if (lib)
	    error ("Cannot load MDI library: \"%s\".", lib);
	  if (from_tty)
	    error ("Unknown MDI library.\n"
		   "Please specify the library on the \"target\" command line or\n"
		   "by setting the 'mdi library' variable, or the $GDBMDILIB\n"
		   "environment variable.");
	  error ("Unknown MDI library.\n"
		 "Please set the 'mdi library' variable,\n"
		 "or the $GDBMDILIB environment variable.");
	}
      MDIERR (mdistat, "MDI initialisation");
    }

  /* Get API version range supported by library. */
  mdistat = mdiVersion (&vrange);
  MDIERR (mdistat, "MDI get versions");

  /* Use the highest API compatible with library. */
  api = ((vrange.newest < MDICurrentRevision)
	 ? vrange.newest : MDICurrentRevision);

  /* Make sure it's not too old. */
  if (api < vrange.oldest || api < MDIOldestRevision)
    {
      if (remote_debug)
	{
	  fprintf_unfiltered (mdi_logfp,
			      "MDI library API: oldest=%d.%d newest=%d.%d\n",
			      vrange.oldest >> 16, vrange.oldest & 0xffff,
			      vrange.newest >> 16, vrange.newest & 0xffff);
	  fprintf_unfiltered (mdi_logfp,
			      "Our API: oldest=%d.%d current=%d.%d\n",
			      MDIOldMajor, MDIOldMinor, MDIMajor, MDIMinor);
	}
      error ("MDI library API version mismatch");
    }

  sprintf (mdi->Config.User, "GNU gdb %.*s",
	   (int) sizeof (mdi->Config.User - sizeof "GNU gdb "), version);
  mdi->Config.MDICBOutput = mdiDbgOutput;
  mdi->Config.MDICBInput = mdiDbgInput;
  mdi->Config.MDICBPeriodic = mdiPeriodic;

  mdistat = mdiConnect (api, &mdi->MDIHandle, &mdi->Config);
  MDIERR (mdistat, "MDI connect");

  if (remote_debug)
    fprintf_unfiltered (mdi_logfp,
			"connected to library '%s', implementer '%s'\n",
			lib ? lib : "<default>", mdi->Config.Implementer);

  if (mdi_implementer)
    xfree (mdi_implementer);
  mdi_implementer = xstrdup (mdi->Config.Implementer);
}



#define CFG_M		0x80000000	/* next Config implemented */

/* MIPS32 Config0 register */
#define CFG0_MTMASK	0x00000380
#define  CFG0_MT_TLB	 (1<<7)

/* MIPS32 Config1 register */
#define CFG1_MMUSMASK	0x7e000000	/* mmu size - 1 */
#define CFG1_MMUSSHIFT	25
#define CFG1_ISMASK	0x01c00000	/* icache lines 64<<n */
#define CFG1_ISSHIFT	22
#define CFG1_ILMASK	0x00380000	/* icache line size 2<<n */
#define CFG1_ILSHIFT	19
#define CFG1_IAMASK	0x00070000	/* icache ways - 1 */
#define CFG1_IASHIFT	16
#define CFG1_DSMASK	0x0000e000	/* dcache lines 64<<n */
#define CFG1_DSSHIFT	13
#define CFG1_DLMASK	0x00001c00	/* dcache line size 2<<n */
#define CFG1_DLSHIFT	10
#define CFG1_DAMASK	0x00000380	/* dcache ways - 1 */
#define CFG1_DASHIFT	7

/* MIPS32 Config2 register */
#define CFG2_TUMASK	0x70000000	/* tertiary cache control */
#define CFG2_TUSHIFT	28
#define CFG2_TSMASK	0x0f000000	/* tcache sets per wway 64<<n */
#define CFG2_TSSHIFT	24
#define CFG2_TLMASK	0x00f00000	/* tcache line size 2<<n */
#define CFG2_TLSHIFT	20
#define CFG2_TAMASK	0x000f0000	/* tcache ways - 1 */
#define CFG2_TASHIFT	16
#define CFG2_SUMASK	0x0000f000	/* secondary cache control */
#define CFG2_SUSHIFT	12
#define CFG2_SSMASK	0x00000f00	/* scache sets per wway 64<<n */
#define CFG2_SSSHIFT	8
#define CFG2_SLMASK	0x000000f0	/* scache line size 2<<n */
#define CFG2_SLSHIFT	4
#define CFG2_SAMASK	0x0000000f	/* scache ways - 1 */
#define CFG2_SASHIFT	0

static void
mdi_probe_cpu (struct mdi_state *mdi, int index)
{
  enum mdiTlb tlb;
  int regsize;
  unsigned long cfg0 = 0, cfg1 = 0, cfg2 = 0;
  MDIInt32 mdistat;
  char data[4];
  int isa;

  isa = isa2int (mdi->gmdata[index].DeviceData.FISA);
  if (isa <= 2)
    {
      regsize = 4;
      tlb = R3000_TLB;
    }
  else if (isa == 32 || isa == 64)
    {
      if (isa == 32)
	{
	  regsize = 4;
	  tlb = R4000_32TLB;
	}
      else
	{
	  regsize = 8;
	  tlb = R4000_64TLB;
	}
      mdi->gmdata[index].ntlb = -1;

      /* Determine TLB size from Config0 and Config1 registers. */

      /* MIPS32 Config0 Register  (CP0 Register 16, Select 0). */
      mdistat = mdiRead (mdi->gmdata[index].DevHandle,
			 MDIMIPCP0, 16, data, 4, 1);
      cfg0 = (unsigned long) extract_signed_integer (data, 4);
      if (cfg0 & CFG_M)
	{
	  /* MIPS32 Config1 Register  (CP0 Register 16, Select 1). */
	  mdistat =
	    mdiRead (mdi->gmdata[index].DevHandle,
		     MDIMIPCP0, 16 | (1 << 5), data, 4, 1);
	  if (mdistat == MDISuccess)
	    cfg1 = (unsigned long) extract_signed_integer (data, 4);
	}

      if (mdistat == MDISuccess && (cfg0 & CFG0_MTMASK) == CFG0_MT_TLB
	  && cfg1 != 0)
	mdi->gmdata[index].ntlb = ((cfg1 & CFG1_MMUSMASK) >> CFG1_MMUSSHIFT)
				  + 1;
    }
  else
    {
      regsize = 8;
      tlb = R4000_64TLB;
    }

  if (mdi->gmdata[index].ntlb <= 0)
    tlb = NO_TLB;

  if (index != 0)
    {
      if (regsize != mdi->regsize)
	error ("Cannot support multiple devices of different register size.");
      if (tlb != mdi->tlb)
	error ("Cannot support multiple devices of different TLB type.");
    }

  mdi->regsize = regsize;
  mdi->tlb = tlb;

  if (mdi->gmdata[index].ntlb == 0)
    {
      /* Do a binary search to find the number of TLB entries. */
      int min = 0, max = 256;
      int half;
      MDIInt32 mdistat;

      while (max - min > 1)
	{
	  half = (max - min + 1) >> 1;
	  switch (mdi->tlb)
	    {
	    case R3000_TLB:
	      {
		MDIUint32 data[2];
		mdistat = mdiRead (mdi->gmdata[index].DevHandle, MDIMIPTLB,
				   (min + half) * 2, data, 4, 2);
	      }
	      break;
	    case R4000_32TLB:
	      {
		MDIUint32 data[4];
		mdistat = mdiRead (mdi->gmdata[index].DevHandle, MDIMIPTLB,
				   (min + half) * 4, data, 4, 4);
	      }
	      break;
	    case R4000_64TLB:
	      {
		MDIUint64 data[4];
		mdistat = mdiRead (mdi->gmdata[index].DevHandle, MDIMIPTLB,
				   (min + half) * 4, data, 8, 4);
	      }
	      break;
	    default:
	      mdistat = MDIErrNoResource;
	      break;
	    }
	  if (mdistat == MDISuccess)
	    min += half;
	  else
	    max = min + half;
	}

      mdi->gmdata[index].ntlb = max;
    }

  /* Probe cache layout. */
  if (isa == 32 || isa == 64)
    {
      MDICacheInfoT *cache = mdi->gmdata[index].CacheInfo;

      if (cfg1 & CFG_M)
	{
	  /* MIPS32 Config1 Register  (CP0 Register 16, Select 2). */
	  mdistat = mdiRead (mdi->gmdata[index].DevHandle, MDIMIPCP0,
			     16 | (2 << 5), data, 4, 1);
	  if (mdistat == MDISuccess)
	    cfg2 = (unsigned long) extract_signed_integer (data, 4);
	}

      if (cfg1 & CFG1_ILMASK)
	{
	  /* I-cache */
	  cache[0].Type = MDICacheTypeInstruction;
	  cache[0].LineSize = 2 << ((cfg1 & CFG1_ILMASK) >> CFG1_ILSHIFT);
	  cache[0].LinesPerSet = 64 << ((cfg1 & CFG1_ISMASK) >> CFG1_ISSHIFT);
	  cache[0].Sets = 1 + ((cfg1 & CFG1_IAMASK) >> CFG1_IASHIFT);
	}
      if (cfg1 & CFG1_DLMASK)
	{
	  /* D-cache */
	  cache[1].Type = MDICacheTypeData;
	  cache[1].LineSize = 2 << ((cfg1 & CFG1_DLMASK) >> CFG1_DLSHIFT);
	  cache[1].LinesPerSet = 64 << ((cfg1 & CFG1_DSMASK) >> CFG1_DSSHIFT);
	  cache[1].Sets = 1 + ((cfg1 & CFG1_DAMASK) >> CFG1_DASHIFT);
	}
      if (cfg2 & CFG2_SLMASK)
	{
	  /* S-cache */
	  cache[2].Type = MDICacheTypeUnified;
	  cache[2].LineSize = 2 << ((cfg2 & CFG2_SLMASK) >> CFG2_SLSHIFT);
	  cache[2].LinesPerSet = 64 << ((cfg2 & CFG2_SSMASK) >> CFG2_SSSHIFT);
	  cache[2].Sets = 1 + ((cfg2 & CFG2_SAMASK) >> CFG2_SASHIFT);
	}
      if (cfg2 & CFG2_TLMASK)
	{
	  /* T-cache */
	  cache[3].Type = MDICacheTypeUnified;
	  cache[3].LineSize = 2 << ((cfg2 & CFG2_TLMASK) >> CFG2_TLSHIFT);
	  cache[3].LinesPerSet = 64 << ((cfg2 & CFG2_TSMASK) >> CFG2_TSSHIFT);
	  cache[3].Sets = 1 + ((cfg2 & CFG2_TAMASK) >> CFG2_TASHIFT);
	}
    }
  else
    {
      /* Ask MDI, but quietly ignore any error. */
      (void) mdiCacheQuery (mdi->gmdata[index].DevHandle,
			    &mdi->gmdata[index].CacheInfo[0]);
    }
}



static void
mdi_cleanup (void *arg)
{
  struct mdi_state *mdi = arg;

  if (mdi_desc)
    {
      mdi_desc = NULL;		/* so mdi_close will do nothing */
      unpush_target (&mdi_ops);
    }
  if (mdi->cfgfile)
    {
      if (remote_debug == 0)
	unlink (mdi->cfgfile);
      xfree (mdi->cfgfile);
    }
  if (mdi->MDIHandle != MDINoHandle)
    mdiDisconnect (mdi->MDIHandle, MDICurrentState);
  xfree (mdi);

  if (mdi_logging)
    {
      ui_file_delete (mdi_logfp);
      mdi_logfp = gdb_stdlog;
      mdi_logging = 0;
    }
}


static int
mdi_first_ptid (struct thread_info *tp, void *data)
{
  return 1;
}

static void
mdi_set_inferior_ptid (void)
{
  MDIInt32 mdistat, TCId;
  struct thread_info *tp;

  inferior_ptid = mdi_ptid;
  mdi_find_new_threads ();
  mdistat = mdiGetTC (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle, &TCId);
  if (mdistat != MDISuccess)
    return;

  inferior_ptid = ptid_build (GET_PID (inferior_ptid),
			      mdi_desc->gmdata[mdi_desc->gmindex].DevHandle,
			      TCId);
  if (!in_thread_list (inferior_ptid))
    {
      tp = iterate_over_threads (mdi_first_ptid, NULL);
      if (tp)
	{
	  inferior_ptid = tp->ptid;
	  mdi_switch_to_thread ();
	}
    }
}

/* The open routine takes the rest of the parameters from the command,
   and (if successful) pushes a new target onto the stack.  Targets
   should supply this routine, if only to provide an error message.
   Called when selecting the simulator. EG: (gdb) target mdi name. */

static void
mdi_open (char *args, int from_tty)
{
  struct mdi_tm_data_list *tmdatalist = NULL, **listp = &tmdatalist;
  int group_mode = 0, team_mode = 0;
  struct cleanup *cleanups;
  int len;
  char *arg_buf;
  char **argv;
  MDIInt32 mdistat;
  struct mdi_state *mdi;
  char *lib = 0;
  char *configfile = 0;
  unsigned long device, target;
  int byte_order;
  int connreset;
  int i, j;

  if (remote_debug && mdi_logfile)
    {
      mdi_logfp = gdb_fopen (mdi_logfile, "w");
      if (mdi_logfp == NULL)
	perror_with_name (mdi_logfile);
      else
	mdi_logging = 1;
    }

  if (mdi_logfp == NULL)
    mdi_logfp = gdb_stdlog;

  if (remote_debug)
    fprintf_unfiltered (mdi_logfp, "mdi_open: args \"%s\"\n",
			args ? args : "(null)");

  /* XXX is this still needed */
  sr_set_device (xstrdup ("mdi"));

  device = mdi_device;
  target = mdi_target;
  connreset = mdi_connectreset;

  if (args)
    {
      int seendev = 0;
      char *argscopy, *p;
      
      argscopy = alloca (strlen (args) + 1);
      strcpy (argscopy, args);
      for (p = strtok (argscopy, " \t,{}"); p; p = strtok (NULL, " \t,{}"))
	{
	  if (isdigit (*p))
	    {
	      if (seendev)
		error ("Multiple MDI device numbers specified.");
	      device = strtoul (p, &p, 0);
	      if (*p == ':' && isdigit (p[1]))
		{
		  target = device;
		  device = strtoul (p + 1, &p, 0);
		}
	      if (*p)
		error ("Bad [TARGET:]DEVICE number.");
	      seendev = 1;
	    }
	  else if (strncmp (p, "cfg=", sizeof "cfg=" - 1) == 0)
	    {
	      if (configfile)
		error ("Multiple MIPSsim config files specified.");
	      p += sizeof "cfg=" - 1;
	      if (*p)
		configfile = p;
	    }
	  else if (strncmp (p, "group=", sizeof "group=" - 1) == 0)
	    {
	      unsigned long tm_device, tm_target;

	      if (team_mode)
		error ("Cannot mix group= and team= options.");

	      group_mode = 1;
	      p += sizeof "group=" - 1;
	      if (isdigit (*p))
		{
		  tm_target = mdi_target;
		  tm_device = strtoul (p, &p, 0);
		  if (*p == ':' && isdigit (p[1]))
		    {
		      tm_target = tm_device;
		      tm_device = strtoul (p + 1, &p, 0);
		    }
		  if (*p)
		    error ("Bad [TARGET:]DEVICE number.");
    
		  *listp = alloca (sizeof (**listp));
		  (*listp)->next = NULL;
		  (*listp)->tmdata.TGId = tm_target - 1;
		  (*listp)->tmdata.DevId = tm_device - 1;
		  listp = &(*listp)->next;
		}
	      else
		error ("Bad group= option.");
	    }
	  else if (strncmp (p, "rst=", sizeof "rst=" - 1) == 0)
	    {
	      p += sizeof "rst=" - 1;
	      if (strcasecmp (p, "off") == 0)
		connreset = -1;
	      else if (strcasecmp (p, "on") == 0)
		connreset = 0;
	      else
		{
		  connreset = strtol (p, &p, 0);
		  if (*p)
		    error ("Bad rst= option.");
		}
	    }
	  else if (strncmp (p, "team=", sizeof "team=" - 1) == 0)
	    {
	      unsigned long tm_device, tm_target;

	      if (group_mode)
		error ("Cannot mix team= and group= options.");

	      team_mode = 1;
	      p += sizeof "team=" - 1;
	      if (isdigit (*p))
		{
		  tm_target = mdi_target;
		  tm_device = strtoul (p, &p, 0);
		  if (*p == ':' && isdigit (p[1]))
		    {
		      tm_target = tm_device;
		      tm_device = strtoul (p + 1, &p, 0);
		    }
		  if (*p)
		    error ("Bad [TARGET:]DEVICE number.");
    
		  *listp = alloca (sizeof (**listp));
		  (*listp)->next = NULL;
		  (*listp)->tmdata.TGId = tm_target - 1;
		  (*listp)->tmdata.DevId = tm_device - 1;
		  listp = &(*listp)->next;
		}
	      else
		error ("Bad team= option.");
	    }
	  else if (*p)
	    {
	      if (lib)
		error ("Multiple MDI libraries specified.");
	      lib = p;
	    }
	}
    }

  if (!lib)
    lib = mdi_library;

  if (!configfile)
    configfile = mdi_configfile;

  /* reread files if they've changed */
  reopen_exec_file ();
  reread_symbols ();

  target_preopen (from_tty);

  unpush_target (&mdi_ops);

  mdi = (struct mdi_state *) xmalloc (sizeof (struct mdi_state));
  memset (mdi, 0, sizeof (struct mdi_state));
  mdi->MDIHandle = MDINoHandle;

  /* Failed coprocessor register masks. */
  memset (mdi_cpdreg_miss, 0, sizeof (mdi_cpdreg_miss));
  memset (mdi_cpcreg_miss, 0, sizeof (mdi_cpcreg_miss));
  memset (mdi_cpdreg_wp, 0, sizeof (mdi_cpdreg_wp));
  memset (mdi_cpcreg_wp, 0, sizeof (mdi_cpcreg_wp));
  mdi_hilo_miss = 0;
  mdi_dsp_miss = 0;

  /* To avoid confusion, don't access the well known CP0 regs directly. */
  mdi_cpdreg_miss[0][0] |= ((1 << 12)		/* Status */
			    | (1 << 8)		/* BadVaddr */
			    | (1 << 13));	/* Cause */

  /* Availability of MDIMIPFPR resource is unknown. */
  mdi_fpr_avail = -1;

  /* Availability of MDIHwBpQuery call is unknown. */
  mdi_hwbpq_avail = -1;

  cleanups = make_cleanup (mdi_cleanup, mdi);

  /* Connect to MDI library. */
  mdi_dlopen (lib, mdi, from_tty);

  /* Create a team if requested.  */
  if (tmdatalist)
    {
      struct mdi_tm_data_list tmdatathis;

      /* Add the device to be opened too.  */
      tmdatathis.next = tmdatalist;
      tmdatathis.tmdata.TGId = target - 1;
      tmdatathis.tmdata.DevId = device - 1;
      tmdatalist = &tmdatathis;

      if (mdi_team <= 0 || group_mode)
	{
	  mdistat = mdiTeamCreate (mdi->MDIHandle, &mdi_team);
	  if (mdistat != MDISuccess)
	    {
	      mdi_team = 0;
	      if (group_mode && mdistat != MDIErrUnsupported)
		mdi_team_status (mdistat);
	      else
		MDIWARN (mdistat, "creating team");
	    }
	  mdi_team += 1;
	}

      if (group_mode)
	mdi->gid = mdi_team - 1;

      for (listp = &tmdatalist; *listp; listp = &(*listp)->next)
	{
	  (*listp)->tmdata.MDIHandle = mdi->MDIHandle;
	  mdistat = mdiTMAttach (mdi->MDIHandle, mdi_team - 1,
				 &(*listp)->tmdata);

	  if (group_mode && mdistat != MDIErrUnsupported)
	    mdi_tm_status (mdistat, (*listp)->tmdata.TGId + 1,
			   (*listp)->tmdata.DevId + 1);
	  else
	    MDIWARN (mdistat, "adding team member");
	  if (group_mode || !mdi->gmcount)
	    mdi->gmdata[mdi->gmcount++].tmdata = (*listp)->tmdata;
	}
    }
  else
    {
      mdi->gmdata[mdi->gmcount].tmdata.MDIHandle = mdi->MDIHandle;
      mdi->gmdata[mdi->gmcount].tmdata.TGId = target - 1;
      mdi->gmdata[mdi->gmcount].tmdata.DevId = device - 1;
      mdi->gmcount++;
    }

  for (i = 0; i < mdi->gmcount; i++)
    {
      /* Reuse the target group handle if possible. */
      mdi->gmdata[i].TGHandle = MDINoHandle;
      for (j = 0; j < i; j++)
	if (mdi->gmdata[i].tmdata.TGId == mdi->gmdata[j].tmdata.TGId)
	  mdi->gmdata[i].TGHandle = mdi->gmdata[j].TGHandle;

      /* Connect to target group. */
      if (mdi->gmdata[i].TGHandle == MDINoHandle)
	openTG (mdi, i);

      /* Connect to device in group. */
      openDev (mdi, i, configfile);
    }

  /* Reset the primary device.  */
  resetDev (mdi, connreset);

  /* Halt the devices. */
  haltDev (mdi);

  for (i = 0; i < mdi->gmcount; i++)
    if (mdi->gmdata[i].DeviceData.Endian != mdi->gmdata[0].DeviceData.Endian)
      error ("Cannot support multiple devices of mixed endianness.");

  /* Set gdb byte order to match target. */
  byte_order = mdi->gmdata[0].DeviceData.Endian == MDIEndianBig
    ? BFD_ENDIAN_BIG : BFD_ENDIAN_LITTLE;

  if (TARGET_BYTE_ORDER != byte_order)
    {
      const char *mdi_order = (byte_order == BFD_ENDIAN_BIG)
	? "big" : "little";
      const char *gdb_order = (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG)
	? "big" : "little";
      int ok;

      if (selected_byte_order () == BFD_ENDIAN_UNKNOWN && ! exec_bfd)
	{
	  /* If there's no executable file, we might as well
	     change gdb's endianness now to match the CPU. */
	  struct gdbarch_info info;
	  gdbarch_info_init (&info);
	  info.byte_order = byte_order;
	  if (! gdbarch_update_p (info))
	    internal_error (__FILE__, __LINE__,
			    "can't switch gdb to %s endian byte order",
			    mdi_order);

#if XXX 
	  /* TBD  */
	  /* Note that after we clear target_byte_order_auto, gdb
	     won't change endianness automatically if we open a new
	     exec-file with a different endianness - and there'll be
	     no warning about that!! */
	  target_byte_order_auto = 0;
#endif

	  if (remote_debug)
	    fprintf_unfiltered (mdi_logfp, "switching gdb to %s endian\n",
				mdi_order);
	}
      else
	{
	  if (exec_bfd)
	    /* Endianness probably derived from file. */
	    ok = query ("The selected CPU is %s endian, but file \"%s\" is %s endian.\n\
Do you want to connect anyway? ",
			mdi_order, basename (get_exec_file (0)), gdb_order);
	  else
	    /* Endianness probably set explicitly with "set endian". */
	    ok = query ("The selected CPU is %s endian, but gdb is set to %s endian.\n\
Do you want to connect anyway? ",
			mdi_order, gdb_order);
	  if (!ok)
	    error ("GDB/MDI endianness mismatch.");
	}
    }

  /* Probe target CPU for other internal information. */
  for (i = 0; i < mdi->gmcount; i++)
    if (!mdi->gmdata[i].disabled)
      {
	mdi_probe_cpu (mdi, i);
	mdi->gmdata[i].probed = 1;
      }

  /* No access to DEPC, DESAVE or other debug mode registers,
     except on MIPSsim, where they aren't used by the debug
     monitor. */
  if (! mdi->mipssim_p)
    for (i = 0; i < 7; i++)
      mdi_cpdreg_miss[0][i] |= ((1 << 23) 	/* Debug, Trace, etc */
				| (1 << 24) 	/* DEPC */
				| (1 << 31)); 	/* DESAVE */

#if 0
  /* For now, write protect all the MT registers */
  mdi_cpdreg_wp[0][1] |= ((1 << 0) 	/* MVPControl */
			  | (1 << 1)	/* VPEControl */
			  | (1 << 4)	/* TCStatus */
			  | (1 << 6)	/* SRSConf0 */
			  | (1 << 15));	/* EBase */

  mdi_cpdreg_wp[0][2] |= ((1 << 0) 	/* MVPConf0 */
			  | (1 << 1)	/* VPEConf0 */
			  | (1 << 4)	/* TCPC */
			  | (1 << 6)	/* SRSConf1 */
			  | (1 << 12));	/* SRSCtl */

  mdi_cpdreg_wp[0][3] |= ((1 << 0) 	/* MVPConf1 */
			  | (1 << 1)	/* VPEConf1 */
			  | (1 << 4)	/* TCHalt */
			  | (1 << 6)	/* SRSConf2 */
			  | (1 << 16));	/* Config3 */

  mdi_cpdreg_wp[0][4] |= ((1 << 1)	/* YQMask */
			  | (1 << 4)	/* TCContext */
			  | (1 << 6));	/* SRSConf3 */

  mdi_cpdreg_wp[0][5] |= ((1 << 1)	/* VPESchedule */
			  | (1 << 4)	/* TCShedule */
			  | (1 << 6));	/* SRSConf4 */

  mdi_cpdreg_wp[0][6] |= ((1 << 1)	/* VPEScheFBack */
			  | (1 << 4)	/* TCScheFBack */
			  | (1 << 6));	/* SRSConf4 */
#endif

    
  /* No access to w/o 4KS family security registers */
  for (i = 1; i < 7; i++)
    mdi_cpdreg_miss[0][i] |= (1 << 22);

  /* We're connected, record the mdi descriptor. */
  mdi_desc = mdi;
  mdi_loaded = mdi_exited = mdi_signalled = 0;

  push_target (&mdi_ops);

  /* Without this, some commands which require an active target (such
     as kill) won't work.  This variable serves (at least) double duty
     as both the pid of the target process (if it has such), and as a
     flag indicating that a target is active.  These functions should
     be split out into seperate variables, especially since GDB will
     someday have a notion of debugging several processes.  */
  mdi_set_inferior_ptid ();

  discard_cleanups (cleanups);

  if (remote_debug)
    fprintf_unfiltered (mdi_logfp, "mdi_open(): connected\n");

  /* Let the user know. */
  printf_filtered ("Connected to MDI target\n");

#ifdef GDBTK
  /* The way the insight GUI works, it really needs a prompt... */
  if (gdbtk_interp) {
    if (*mdi_target_prompt == '\0') {
      xfree (mdi_target_prompt);
      mdi_target_prompt = xstrdup ("app>");
    }
  }
#endif

  /* Forget about any MDI breakpoints left over in our table. */
  mdi_reset_breakpoints ();

#ifdef GDB_TARGET_START_REMOTE
  GDB_TARGET_START_REMOTE ();
#endif

  mdi_starting = 1;

  /* Initialise gdb internals. */
  start_remote ();

  mdi_starting = 0;

  /* So that "run" doesn't complain */
  inferior_ptid = null_ptid;
}



static void
do_freeargv (void *arg)
{
  freeargv ((char **) arg);
}

/* This is called not only when we first attach, but also when the
   user types "run" after having attached.  */
static void
mdi_create_inferior (char *execfile, char *args, char **env, int from_tty)
{
  struct cleanup *cleanups;
  char *prog;
  char *arg_buf;
  char **argv = NULL;
  CORE_ADDR entry_point;
  int len;

  if (execfile == 0 || exec_bfd == 0)
    error ("No executable file specified.");

  if (mdi_argv)
    {
      freeargv (mdi_argv);
      mdi_argv = NULL;
    }

  prog = execfile[0] ? execfile : "''";
  len = strlen (prog) + strlen (args) + 2;
  arg_buf = (char *) alloca (len);
  strcpy (arg_buf, prog);
  if (args)
    {
      strcat (arg_buf, " ");
      strcat (arg_buf, args);
    }
  
  argv = buildargv (arg_buf);
  if (argv == NULL)
    nomem (0);

  cleanups = make_cleanup (do_freeargv, argv);

  if (mdi_entrypoint != 0)
    entry_point = mdi_entrypoint;
  else
    entry_point = (int) bfd_get_start_address (exec_bfd);

  if (mdi_rununcached)
    {
      /* Force to uncached address. */
#ifdef BFD64
      if (entry_point >= 0xffffffff80000000LL
	  && entry_point < 0xffffffffa0000000LL)
	entry_point |= 0x20000000;
#else
      if (entry_point >= 0x80000000 && entry_point < 0xa0000000)
	entry_point |= 0x20000000;
#endif
    }

  if (!mdi_loaded)
    {
      if (query ("You haven't downloaded the executable to the MDI target.\n\
Do you want to start executing at 0x%s anyway? ", paddr_nz (entry_point)))
	mdi_loaded = 1;
      else
	error ("No program loaded.");
    }

  
  /* Initialise host file i/o descriptors. */
  mdi_fdinit ();

  /* Rip out the breakpoints; we'll reinsert them after restarting. */
  remove_breakpoints ();

  mdi_set_inferior_ptid ();
  mdi_exited = mdi_signalled = 0;

  /* Now put the breakpoints back in.  This way we're safe if the
     restart function reloads the image. */
  insert_breakpoints ();

  /* Clean up from the last time we were running.  */
  clear_proceed_status ();

  /* Set argument registers and stack pointer to zero,
     as used by some startup code. 
     XXX Should we clear all registers for consistency? */
  write_register (A0_REGNUM + 0, 0);	/* $a0 */
  write_register (A0_REGNUM + 1, 0);	/* $a1 */
  write_register (A0_REGNUM + 2, 0);	/* $a2 */
  write_register (A0_REGNUM + 3, 0);	/* $a3 */
  write_register (29, 0);		/* $sp */

  /* Ensure interrupts are disabled. */
  write_register (FIRST_EMBED_REGNUM + 12, 
		  read_register (FIRST_EMBED_REGNUM + 12) & ~1);

  discard_cleanups (cleanups);
  mdi_argv = argv;

  /* NB: Entry point already set by mdi_create_inferior. */
  proceed (entry_point, TARGET_SIGNAL_DEFAULT, 0);
}



static void
mdi_close (int quitting)
{
  MDIInt32 mdistat;
  MDIUint32 flags;
  struct mdi_state *mdi;
  int i;

  if ((mdi = mdi_desc) == NULL)
    return;

  if (remote_debug)
    fprintf_unfiltered (mdi_logfp, "mdi_close: quitting %d\n", quitting);

  inferior_ptid = null_ptid;

  /* Clean up target file i/o. */
  mdi_fdcloseall ();

  /* Clear our hidden syscall breakpoints */
  mdi_remove_syscallbreakpoint ();

  /* Output any pending profile data. */
  mdi_profiling_close (1);

  /* Release target program argument vector. */
  if (mdi_argv)
    {
      freeargv (mdi_argv);
      mdi_argv = NULL;
    }

  flags = MDICurrentState;

#if 0
  if (mdi->mipssim_p)
    {
      flags = MDIResetState;
    }
#endif

  if (mdi_continueonclose)
    for (i = 0; i < mdi_desc->gmcount; i++)
      {
	mdistat = mdiSetRunMode (mdi_desc->gmdata[i].DevHandle,
				 -1, MDINoStep, 0);
	/* Let the target continue when we disconnect from it. */
	mdistat = mdiExecute (mdi->gmdata[i].DevHandle);
	if (mdistat != MDIErrDisabled)
	  MDIWARN (mdistat, "continue execution");
      }

  for (i = 0; i < mdi->gmcount; i++)
    {
      mdistat = mdiClose (mdi->gmdata[i].DevHandle, flags);
      if (mdistat != MDISuccess)
	{
	  MDIRunStateT runstate;

	  if (! mdi_continueonclose || mdistat != MDIErrTargetRunning)
	    MDIWARN (mdistat, "closing device");

	  /* Try stopping the target again. */
	  mdistat = mdiStop (mdi->gmdata[i].DevHandle);
	  MDIWARN (mdistat, "stopping CPU");

	  /* Let it report that it has stopped. */
	  mdiRunState (mdi->gmdata[i].DevHandle, 200, &runstate);

	  if (mdi_continueonclose)
	    {
	      mdistat = mdiSetRunMode (mdi_desc->gmdata[i].DevHandle,
				       -1, MDINoStep, 0);
	      /* Let the target continue when we disconnect from it. */
	      mdistat = mdiExecute (mdi->gmdata[i].DevHandle);
	      if (mdistat != MDIErrDisabled)
		MDIWARN (mdistat, "continue execution");
	    }

	  /* Try the close again. */
	  mdistat = mdiClose (mdi->gmdata[i].DevHandle, flags);
	  MDIWARN (mdistat, "closing device again");
	}
    }

  if (mdi->Config.MDICapability & MDICAP_TargetGroups)
    for (i = 0; i < mdi->gmcount; i++)
      if (mdi->gmdata[i].TGHandle != MDINoHandle)
	{
	  int j;

	  mdistat = mdiTGClose (mdi->gmdata[i].TGHandle, flags);
	  MDIWARN (mdistat, "target group close");

	  for (j = i + 1; j < mdi->gmcount; j++)
	    if (mdi->gmdata[i].tmdata.TGId == mdi->gmdata[j].tmdata.TGId)
	      mdi->gmdata[j].TGHandle = MDINoHandle;
	}

  if (mdi->gmcount > 1)
    {
      mdistat = mdiTeamClear (mdi->MDIHandle, mdi->gid);
      MDIWARN (mdistat, "clearing team");
      mdistat = mdiTeamDestroy (mdi->MDIHandle, mdi->gid);
      MDIWARN (mdistat, "destroying team");
    }

  mdistat = mdiDisconnect (mdi->MDIHandle, flags);
  MDIWARN (mdistat, "MDI disconnect");

  if (mdi->cfgfile)
    {
      if (remote_debug == 0)
	unlink (mdi->cfgfile);
      xfree (mdi->cfgfile);
    }

  if (mdi_releaseonclose)
    {
      /* Release MDI DLL. */
      MDIRelease (mdiModule);
      mdiModule = 0;
    }

  xfree (mdi);
  mdi_desc = NULL;

  if (mdi_logging)
    {
      ui_file_delete (mdi_logfp);
      mdi_logfp = gdb_stdlog;
      mdi_logging = 0;
    }
}



/* Takes a program previously attached to and detaches it.
   The program may resume execution (some targets do, some don't) and will
   no longer stop on signals, etc.  We better not have left any breakpoints
   in the program or it'll die when it hits one.  ARGS is arguments
   typed by the user (e.g. a signal to send the process).  FROM_TTY
   says whether to be verbose or not.  */

/* Terminate the open connection to the remote debugger.
   Use this when you want to detach and do something else with your gdb.  */

static void
mdi_detach (char *args, int from_tty)
{
  if (args)
    error ("Argument given to \"detach\" when remotely debugging.");

  if (remote_debug)
    printf_unfiltered ("mdi_detach\n");

  target_mourn_inferior ();

  /* XXX should we actually pop the target here - we could leave it open
     and all allow a following attach */
  pop_target ();		/* calls mdi_close to do the real work */

  if (from_tty)
    printf_filtered ("Ending MDI debugging\n");
}


static void
mdi_disconnect (char *args, int from_tty)
{
  if (args)
    error ("Argument given to \"disconnect\" when remotely debugging.");

  if (remote_debug)
    printf_unfiltered ("mdi_disconnect\n");

  target_mourn_inferior ();

  /* XXX need to tell mdi_close not to restart the app */
  pop_target ();		/* calls mdi_close to do the real work */

  if (from_tty)
    printf_filtered ("Ending MDI debugging\n");
}

static void
mdi_attach (char *args, int from_tty)
{
  MDIInt32 mdistat;
  MDIRunStateT runstate;
  int i;

  if (args && *args)
    error ("Can't pass arguments to MDI.");

  if (exec_bfd == 0)
    error ("No executable file specified");

  check_mdi ();

  if (from_tty)
    printf_unfiltered ("Attaching to MDI target...\n");

  for (i = 0; i < mdi_desc->gmcount; i++)
    {
      mdiStop (mdi_desc->gmdata[i].DevHandle);
      mdiRunState (mdi_desc->gmdata[i].DevHandle, 200, &runstate);
    }

  inferior_ptid = null_ptid;
  mdi_exited = mdi_signalled = 0;

  reinit_frame_cache ();

  printf_unfiltered ("MDI target is now halted\n");
}


static void
mdi_resume (ptid_t ptid, int step, enum target_signal ssignal)
{
  MDIInt32 mdistat;
  MDIUint32 stepmode = MDINoStep;
  MDIUint32 stepallmode = MDINoStep;
  MDIUint32 freezeallmode = 1;
  int i;

  if (mdi_exited)
    error ("The program is not being run.\nPlease %s.\n",
	   mdi_loaded ? "\"load\" and \"run\" it again"
	   : "use \"mdi reset\" to restart execution");

  /* Can only pass signals on when stopped in the signal handler. */
  if (mdi_signalled && ssignal != mdi_lastsig)
    mdi_writereg (V0_REGNUM, ssignal);
  mdi_signalled = 0;

  /* Set up "syscall" breakpoints if not already done. */
  if (!mdi_syscall_inserted)
    mdi_insert_syscallbreakpoint ();

  /* Start profiling now, if requested and not already profiling. */
  mdi_profiling_start ();

  if (mdi_mem_written)
    {
      /* We have written something to memory, which could possibly be
	 instructions: synchronise the i- and d-caches. */
      /* XXX Would be better if we could use an address range,
	 but the MDI API doesn't support this. */
      for (i = 0; i < mdi_desc->gmcount; i++)
	if (!mdi_desc->gmdata[i].disabled)
	  {
	    mdiCacheFlush (mdi_desc->gmdata[i].DevHandle, MDICacheTypeData,
			   MDICacheWriteBack | MDICacheInvalidate);
	    mdiCacheFlush (mdi_desc->gmdata[i].DevHandle, MDICacheTypeUnified,
			   MDICacheWriteBack | MDICacheInvalidate);
	    mdiCacheFlush (mdi_desc->gmdata[i].DevHandle,
			   MDICacheTypeInstruction, MDICacheInvalidate);
	  }
      mdi_mem_written = 0;
    }

  /* A hook for when we need to do something at the last moment before
     resumption.  */
  if (target_resume_hook)
    (*target_resume_hook) ();

  if (step)
    stepmode = mdi_stepinto ? MDIStepInto : MDIStepForward;
  if (step && mdi_threadstepall)
    stepallmode = stepmode;
  if ((step && mdi_threadstepall) || ptid_equal (ptid, pid_to_ptid (-1)))
    freezeallmode = 0;

  for (i = 0; i < mdi_desc->gmcount; i++)
    if (!mdi_desc->gmdata[i].disabled)
      mdistat = mdiSetRunMode (mdi_desc->gmdata[i].DevHandle,
			       -1, stepallmode, freezeallmode);

  if (!ptid_equal (inferior_ptid, null_ptid))
    mdistat = mdiSetRunMode (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle,
			     ptid_get_tid (inferior_ptid), stepmode, 0);
  else
    {
      MDIInt32 tid = -1;

      mdistat = mdiGetTC (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle, &tid);
      MDIWARN (mdistat, "getting current thread");
      mdistat = mdiSetRunMode (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle,
			       tid, stepmode, 0);
    }

  if (step)
    {
      if (remote_debug)
	fprintf_unfiltered (mdi_logfp, "mdi_resume: stepping at 0x%s\n", 
			    phex (mdi_readpc (), sizeof (ULONGEST)));

      mdi_step_active = 1;
      if (mdi_desc->gmcount > 1)
	{
	  mdistat = mdiTeamExecute (mdi_desc->MDIHandle, mdi_desc->gid);
	  if (mdistat == MDIErrUnsupported)
	    {
	      for (i = 1; i < mdi_desc->gmcount + 1; i++)
		{
		  int index = (mdi_desc->gmindex + i) % mdi_desc->gmcount;
		  mdistat = mdiExecute (mdi_desc->gmdata[index].DevHandle);
		  if (mdistat != MDIErrDisabled)
		    MDIWARN (mdistat, "starting single step");
		}
	      mdistat = MDISuccess;
	    }
	}
      else
	mdistat = mdiStep (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle,
			   1, stepmode);
      mdi_step_active = 0;
      MDIERR (mdistat, "starting single step");
    }
  else
    {
      if (remote_debug)
	fprintf_unfiltered (mdi_logfp, "mdi_resume: resuming at 0x%s\n", 
			    phex (mdi_readpc (), sizeof (ULONGEST)));

      if (mdi_desc->gmcount > 1)
	{
	  mdistat = mdiTeamExecute (mdi_desc->MDIHandle, mdi_desc->gid);
	  if (mdistat == MDIErrUnsupported)
	    {
	      for (i = 1; i < mdi_desc->gmcount + 1; i++)
		{
		  int index = (mdi_desc->gmindex + i) % mdi_desc->gmcount;
		  mdistat = mdiExecute (mdi_desc->gmdata[index].DevHandle);
		  if (mdistat != MDIErrDisabled)
		    MDIWARN (mdistat, "starting execution");
		}
	      mdistat = MDISuccess;
	    }
	}
      else
	mdistat = mdiExecute (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle);
      MDIERR (mdistat, "starting execution");
    }
}

static RETSIGTYPE mdi_interrupt_twice (int signo);
static void interrupt_query (void);

static RETSIGTYPE
mdi_interrupt (int signo)
{
  /* If this doesn't work, try more severe steps.  */
  signal (signo, mdi_interrupt_twice);

  if (remote_debug)
    {
      target_terminal_ours_for_output ();
      printf_unfiltered ("mdi_interrupt called\n");
      target_terminal_inferior ();
    }

  quit_flag = 1;
}

/* The user typed ^C twice.  */
static RETSIGTYPE
mdi_interrupt_twice (int signo)
{
  signal (signo, ofunc);

  if (remote_debug)
    {
      target_terminal_ours_for_output ();
      printf_unfiltered ("mdi_interrupt_twice called\n");
      target_terminal_inferior ();
    }

  interrupt_query ();

  signal (signo, mdi_interrupt);
}


/* Ask the user what to do when an interrupt is received.  */

static void
wait_cleanup (void)
{
  /* Stop it manually. */
  MDIRunStateT runstate;
  int i;

  mdi_wait_active = 0;
  for (i = 0; i < mdi_desc->gmcount; i++)
    {
      mdiStop (mdi_desc->gmdata[i].DevHandle);
      mdiRunState (mdi_desc->gmdata[i].DevHandle, 200, &runstate);
    }
  target_mourn_inferior ();
  throw_exception (RETURN_QUIT);
}

static void
interrupt_query (void)
{
  target_terminal_ours ();

  if (query ("Interrupted while waiting for the program.\n\
Give up (and stop debugging it)? "))
    interrupt_cleanup ();

  target_terminal_inferior ();
}



static int		mdi_profile_active;
static CORE_ADDR 	mdi_profile_lowpc;
static CORE_ADDR 	mdi_profile_highpc;
static HISTCOUNTER * 	mdi_hist;
static FILE * 		mdi_gfp;

struct mdi_profile_region {
  int		valid;
  MDIInt32 	id;
  CORE_ADDR	low;
  CORE_ADDR	high;
  MDIUint32 * 	counts;
} mdi_profreg[2];

#define ROUNDDOWN(x,y)	(((x)/(y))*(y))
#define ROUNDUP(x,y)	((((x)+(y)-1)/(y))*(y))


/* Cleanup if an error is thrown while writing profiling data. */

static void
mdi_profiling_cleanup (void *arg)
{
  MDIInt32 mdistat;
  int i;

  if (mdi_gfp)
    fclose (mdi_gfp);

  if (mdi_hist)
    xfree (mdi_hist);

  for (i = 0; i < 2; i++)
    {
      if (mdi_profreg[i].valid)
	{
	  if (mdi_profreg[i].counts)
	    (void) mipssim_FreeProfileData
	      (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle,
	       &mdi_profreg[i].counts);
	  (void) mipssim_DestroyProfile
	    (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle, mdi_profreg[i].id);
	  mdi_profreg[i].counts = 0;
	  mdi_profreg[i].valid = 0;
	}
    }
}


/* Only called via catch_errors, to avoid fatal errors. */

static int
mdi_profiling_close_1 (PTR writep)
{
  int write = *(int *)writep;
  struct cleanup *cleanups;
  MDIInt32 mdistat;
  MDIUint64 cycles;
  int i;

  if (!mdi_profile_active)
    return 0;
  mdi_profile_active = 0;
  
  for (i = 0; i < 2; i++)
    if (mdi_profreg[i].valid)
      {
	mdistat =
	  mipssim_StopProfile (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle,
			       mdi_profreg[i].id);
	MDIWARN (mdistat, "Stop Profiling");
      }

  cleanups = make_cleanup (mdi_profiling_cleanup, NULL);
  mdi_gfp = NULL;
  mdi_hist = NULL;

  if (write)
    {
      MDIUint64 max;
      MDIUint64 total;
      unsigned long monsize, moncount;
      unsigned long mdicount, mdiidx, histidx;
      unsigned int counts_per_bin;
      unsigned int scale;
      int tracelev;
      const char * counting;
      struct gmon_hdr hdr;
      struct gmon_hist_hdr {
	/* char lowpc[addrsize]; */
	/* char highpc[addrsize]; */
	char hist_size[4];
	char prof_rate[4];
	char dimen[15];
	char dimen_abbrev;
      } hist_hdr;
      u_char tag;

      tracelev = 0; 
      if (MIPSsim_GetTraceLevel)
	(void) MIPSsim_GetTraceLevel
	  (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle, &tracelev);
      counting = (tracelev & 1) ? "cycles" : "insns";

      /* Allocate gmon histogram buffer of textsize / HISTFRACTION, */
      monsize = (mdi_profile_highpc - mdi_profile_lowpc) / HISTFRACTION;
      mdi_hist = xmalloc (monsize);

      /* There is one gmon counter for every HISTFRACTION * sizeof
         (HISTCOUNTER) bytes of text space. (That's 1 for every 4
         bytes for the current settings). */
      moncount = monsize / sizeof (HISTCOUNTER);

      /* There is one MIPSsim counter for every 2 bytes of text. */
      mdicount = (mdi_profile_highpc - mdi_profile_lowpc) / 2;

      /* So that's how many MIPSsim counts for every gmon count? */
      counts_per_bin = mdicount / moncount;

      if (remote_debug)
	fprintf_unfiltered (mdi_logfp, 
			    "hist size=%ld count=%ld; mdi count=%ld; mdi/hist=%d\n",
			    monsize, moncount, mdicount, counts_per_bin);

      /* Fetch the MIPSsim cycle counts, and find the maximum MIPSsim
	 cycle count that will go in one gmon histogram bin. */
      max = 0;
      for (i = 0; i < 2; i++)
	{
	  if (! mdi_profreg[i].valid)
	    continue;

	  mdistat =
	    mipssim_FetchProfile
	      (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle,
	       mdi_profreg[i].id, &mdi_profreg[i].counts, &total);
	  MDIERR (mdistat, "Fetch Profile");

	  mdicount = (mdi_profreg[i].high - mdi_profreg[i].low) / 2;
	  for (mdiidx = 0; mdiidx < mdicount; )
	    {
	      MDIUint64 sum;
	      int c;
	      for (sum = 0, c = counts_per_bin; c != 0; c--)
		sum += mdi_profreg[i].counts[mdiidx++];
	      if (sum > max)
		max = sum;
	    }
	}

      /* Find a scale factor which will allow the largst MDI count to
	 fit into a 16-bit histogram bin. */
      /* TODO
	 At some point we should enhance gprof to support
	 32-bit histogram counters. */
      if (max > 65535)
	scale = (max + 65534) / 65535;
      else
	scale = 1;

      if (remote_debug)
	fprintf_unfiltered (mdi_logfp, "max=%lld scale=%d\n",
			    max, scale);

      /* Rescale the MDI counters and insert into the gmon "pc"
         histogram. Gprof doesn't support multiple histograms, so we
         have to merge all profile regions into a single output
         histogram. */
      histidx = 0;
      for (i = 0; i < 2; i++)
	{
	  unsigned long nextidx;

	  if (! mdi_profreg[i].valid)
	    continue;

	  /* Calculate first gmon histogram index for this region. */
	  nextidx = (mdi_profreg[i].low - mdi_profile_lowpc)
	    / (HISTFRACTION * sizeof(HISTCOUNTER));

	  /* Clear any hole between this and the last region. 
	     XXX Assumes region list is sorted by ascending address. */
	  if (histidx < nextidx)
	    memset (&mdi_hist[histidx], 0, 
		    (nextidx - histidx) * sizeof mdi_hist[0]);

	  /* Merge and scale the MDI counters into the gmon bins. */
	  mdicount = (mdi_profreg[i].high - mdi_profreg[i].low) / 2;
	  for (mdiidx = 0, histidx = nextidx; mdiidx < mdicount; histidx++)
	    {
	      MDIUint64 sum;
	      int c;
	      for (sum = 0, c = counts_per_bin; c != 0; c--)
		sum += mdi_profreg[i].counts[mdiidx++];
	      if (scale != 1)
		sum = (sum + scale / 2) / scale; /* rescale to 16 bits */
	      if (sum > 0xffff)
		sum = 0xffff;
	      store_unsigned_integer (&mdi_hist[histidx],
				      sizeof (mdi_hist[0]),
				      (ULONGEST) sum);
	    }
	}

      /* Write the results to the gmon.out file. */
      mdi_gfp = fopen (mdi_gmonfile, FOPEN_WB);
      if (! mdi_gfp)
	error ("Cannot open profile output file \"%s\": %s.", 
	       mdi_gmonfile, safe_strerror (errno));
	  
      if (remote_debug)
	fprintf_unfiltered (mdi_logfp, "writing to %s\n", mdi_gmonfile);

      /* Write file header. */
      memset (&hdr, 0, sizeof hdr);
      memcpy (hdr.cookie, GMON_MAGIC, sizeof hdr.cookie);
      store_unsigned_integer (&hdr.version, sizeof hdr.version, 
			      GMON_VERSION);
      if (fwrite (&hdr, sizeof hdr, 1, mdi_gfp) != 1)
	perror_with_name (mdi_gmonfile);

      /* Create histogram header. */

      /* Write histogram tag. */
      tag = GMON_TAG_TIME_HIST;
      if (fwrite (&tag, sizeof tag, 1, mdi_gfp) != 1)
	perror_with_name (mdi_gmonfile);

      {
	char hilo[2*8];
	int sz = register_size (current_gdbarch, 0);
	store_signed_integer (&hilo[0], sz, mdi_profile_lowpc);
	store_signed_integer (&hilo[sz], sz, mdi_profile_highpc);
	if (fwrite (hilo, sz, 2, mdi_gfp) != 2)
	  perror_with_name (mdi_gmonfile);
      }

      store_unsigned_integer (hist_hdr.hist_size, sizeof hist_hdr.hist_size,
			      moncount);
      store_signed_integer (hist_hdr.prof_rate, sizeof hist_hdr.prof_rate, 
			    scale == 1 ? 1 : -scale);
      strncpy (hist_hdr.dimen, counting, sizeof (hist_hdr.dimen));
      hist_hdr.dimen_abbrev = counting[0];

      /* Write tail of histogram header. */
      if (fwrite (&hist_hdr, sizeof hist_hdr, 1, mdi_gfp) != 1)
	perror_with_name (mdi_gmonfile);

      /* Write histogram data. */
      if (fwrite (mdi_hist, sizeof (HISTCOUNTER), moncount, mdi_gfp)
	  != moncount)
	perror_with_name (mdi_gmonfile);

      if (fclose (mdi_gfp) != 0)
	perror_with_name (mdi_gmonfile);
      mdi_gfp = NULL;

      if (remote_debug)
	fprintf_unfiltered (mdi_logfp, "profiling output done\n");
    }

  do_cleanups (cleanups);
  return 0;
}


static void
mdi_profiling_close (int write)
{
  /* Don't let errors bounce us all the way to the top level. */
  (void) catch_errors (mdi_profiling_close_1, &write,
		       "Couldn't output profiling data:\n",
		       RETURN_MASK_ALL);
}

static struct minimal_symbol *
mdi_lookup_symbols (const char *syms)
{
  struct minimal_symbol *ms;
  char *copy, *p;

  copy = alloca (strlen (syms) + 1);
  strcpy (copy, syms);
  for (p = strtok (copy, " \t,"); p; p = strtok (NULL, " \t,"))
    if ((ms = lookup_minimal_symbol (p, NULL, NULL)) != NULL)
      return ms;
  return NULL;
}


static void
mdi_profiling_start (void)
{
  MDIInt32 mdistat;
  struct minimal_symbol *fs, *es;
  struct symbol *s;
  int i;

  /* If already active, then nothing to do. */
  if (mdi_profile_active)
    return;

  /* Is profiling requested? */
  switch (mdi_profiling)
    {
    case AUTO_BOOLEAN_FALSE:
      /* Never */
      return;
    case AUTO_BOOLEAN_TRUE:
      /* Always */
      if (! mdi_desc->mipssim_p)
	{
	  mdi_profiling = AUTO_BOOLEAN_FALSE;
	  warning ("Profiling only supported on MIPSsim - disabled");
	  return;
	}
      if (! MIPSsim_FreeProfileData)
	{
	  mdi_profiling = AUTO_BOOLEAN_FALSE;
	  warning ("Profiling not available with this version of MIPSsim - disabled");
	  return;
	}
      break;
    case AUTO_BOOLEAN_AUTO:
      /* Only if _mcount symbol found */
      if (! mdi_desc->mipssim_p
	  || ! MIPSsim_FreeProfileData
	  || ! mdi_lookup_symbols (mdi_mcount_syms))
	return;
      break;
    }

  fs = mdi_lookup_symbols (mdi_ftext_syms);
  if (! fs)
    {
      mdi_profiling = AUTO_BOOLEAN_FALSE;
      warning ("Missing start of code symbol - profiling disabled");
      return;
    }

  es = mdi_lookup_symbols (mdi_etext_syms);
  if (! es)
    {
      mdi_profiling = AUTO_BOOLEAN_FALSE;
      warning ("Missing end of code symbol - profiling disabled");
      return;
    }  

  mdi_profile_lowpc = SYMBOL_VALUE_ADDRESS (fs) & ~(CORE_ADDR)1;
  mdi_profile_highpc = SYMBOL_VALUE_ADDRESS (es) & ~(CORE_ADDR)1;
  mdi_profile_lowpc = ROUNDDOWN (mdi_profile_lowpc, 
				 HISTFRACTION * sizeof(HISTCOUNTER));
  mdi_profile_highpc = ROUNDUP (mdi_profile_highpc, 
				HISTFRACTION * sizeof(HISTCOUNTER));

  if (mdi_profile_highpc <= mdi_profile_lowpc)
    {
      mdi_profiling = AUTO_BOOLEAN_FALSE;
      warning ("Cannot profile - lowpc (0x%s) >= highpc (0x%s) - disabled",
	     paddr_nz (mdi_profile_lowpc), paddr_nz (mdi_profile_highpc));
    }  

  if (remote_debug)
    fprintf_unfiltered (mdi_logfp, "profile start: lowpc=0x%s highpc=0x%s\n",
			paddr_nz (mdi_profile_lowpc), 
			paddr_nz (mdi_profile_highpc));

  /* Set up profiling region 0, region 1 is empty. */
  memset (mdi_profreg, 0, sizeof mdi_profreg);
  mdi_profreg[0].low = mdi_profile_lowpc;
  mdi_profreg[0].high = mdi_profile_highpc;
  
  /* If the _mcount symbol is in the middle of region 0, split
     it into two regions (0 & 1), excluding the _mcount function. */
  if (! mdi_profile_mcount
      && ((s = lookup_symbol ("_mcount", NULL, VAR_DOMAIN, NULL, NULL))
	  || (s = lookup_symbol ("__mcount", NULL, VAR_DOMAIN, NULL, NULL))))
    {
      CORE_ADDR mcount_beg = BLOCK_START (SYMBOL_BLOCK_VALUE (s));
      CORE_ADDR mcount_end = BLOCK_END (SYMBOL_BLOCK_VALUE (s));
      mcount_beg = ROUNDUP (mcount_beg & ~(CORE_ADDR)1,
			      HISTFRACTION * sizeof(HISTCOUNTER));
      mcount_end = ROUNDDOWN (mcount_end & ~(CORE_ADDR)1, 
			      HISTFRACTION * sizeof(HISTCOUNTER));
      if (mcount_beg < mcount_end
	  && mcount_beg >= mdi_profreg[0].low
	  && mcount_end < mdi_profreg[0].high)
	{
	  mdi_profreg[1].low = mcount_end;
	  mdi_profreg[1].high = mdi_profreg[0].high;
	  mdi_profreg[0].high = mcount_beg;
	}
    }
  
  if (mdi_desc->regsize == 4)
    {
      /* Inconsistent address sign-extension (sigh). */
      mdi_profile_lowpc &= (CORE_ADDR) 0xffffffff;
      mdi_profile_highpc &= (CORE_ADDR) 0xffffffff;
    }

  /* Create the MIPSsim profile regions and start counting. */
  mdistat = MDISuccess;
  for (i = 0; i < 2; i++)
    {
      if (mdi_profreg[i].low != mdi_profreg[i].high)
	{
	  if (mdi_desc->regsize == 4)
	    {
	      /* Inconsistent address sign-extension (sigh). */
	      mdi_profreg[i].low &= (CORE_ADDR) 0xffffffff;
	      mdi_profreg[i].high &= (CORE_ADDR) 0xffffffff;
	    }

	  /* Create profile region. */
	  mdistat =
	    mipssim_CreateProfile
	      (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle,
	       &mdi_profreg[i].id,
	       mdi_profreg[i].low,
	       mdi_profreg[i].high - mdi_profreg[i].low);
	  if (mdistat != MDISuccess)
	    break;

	  mdi_profreg[i].valid = 1;
	}
    }

  if (mdistat == MDISuccess)
    {
      for (i = 0; i < 2; i++)
	if (mdi_profreg[i].valid)
	  {
	    /* Start accumulating profile data. */
	    mdistat =
	      mipssim_StartProfile
		(mdi_desc->gmdata[mdi_desc->gmindex].DevHandle,
		 mdi_profreg[i].id);
	    if (mdistat != MDISuccess)
	      break;
	  }
    }

  if (mdistat != MDISuccess)
    {
      warning ("Cannot create MIPSsim profile for range 0x%s-0x%s.",
	       paddr_nz (mdi_profreg[i].low),
	       paddr_nz (mdi_profreg[i].high));

      for (i = 0; i < 2; i++)
	if (mdi_profreg[i].valid)
	  mipssim_DestroyProfile
	    (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle, mdi_profreg[i].id);
      return;
    }

  /* Finally try to enable cycle counting, if requested. */
  if (mdi_profile_cycles)
    {
      if (! catch_errors (mdi_cycles_enable, NULL,
			  "Couldn't enable cycle profiling:\n",
			  RETURN_MASK_ALL))
	mdi_profile_cycles = 0;
    }

  mdi_profile_active = 1;
}



static int
mdi_readstring (ULONGEST memaddr, char *mybuf, int len)
{
  MDIInt32 mdistat;

  /* Just read a full buffer's worth. */
  mdistat = mdiRead (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle,
		     mdi_memresource (memaddr), 
		     mdi_memoffset (memaddr),
		     mybuf, 0, len);
  return mdistat == MDISuccess;
}

static int
mdi_writestring (ULONGEST memaddr, char *mybuf, int len)
{
  MDIInt32 mdistat;

  /* Write a full buffer's worth. */
  mdistat = mdiWrite (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle,
		      mdi_memresource (memaddr), 
		      mdi_memoffset (memaddr),
		      mybuf, 0, len);
  return mdistat == MDISuccess;
}

#define MAXMDIFD 64
static struct mdifd
{
  int open;
  int sysfd;
}
mdifd[MAXMDIFD];


static void
mdi_fdcloseall (void)
{
  struct mdifd *mp;
  for (mp = &mdifd[3]; mp < &mdifd[MAXMDIFD]; mp++)
    if (mp->open)
      {
	close (mp->sysfd);
	mp->open = 0;
      }
}

static void
mdi_fdinit (void)
{
  int nfd;
  struct mdifd *mp;

  mdi_fdcloseall ();
  for (nfd = 0; nfd < MAXMDIFD; nfd++)
    {
      mp = &mdifd[nfd];
      if (nfd <= 2)
	{
	  mp->sysfd = nfd;
	  mp->open = 1;
	}
      else
	mp->open = 0;
    }
}


static int
mdi_fdalloc (int sysfd)
{
  struct mdifd *mp;
  int fd;

  if (sysfd < 0)
    return sysfd;
  for (fd = 2, mp = &mdifd[2]; fd < MAXMDIFD; fd++, mp++)
    {
      if (!mp->open)
	{
	  mp->sysfd = sysfd;
	  mp->open = 1;
	  break;
	}
    }
  if (fd >= MAXMDIFD)
    return -1;
  return fd;
}

static int
mdi_fdlookup (int fd)
{
  if (fd < 0 || fd >= MAXMDIFD || !mdifd[fd].open)
    return -1;
  return mdifd[fd].sysfd;
}


static int
mdi_fdopen (char *buf, int flags, int mode)
{
  int fd;
  int hflags = 0;

  /* Convert SDE flag bits into host flags bits. */
  switch (flags & 0x0003)
    {
    case 0:
      hflags = O_RDONLY;
      break;
    case 1:
      hflags = O_WRONLY;
      break;
    case 2:
      hflags = O_RDWR;
      break;
    default:
      return -1;
    }
#ifdef O_APPEND
  if (flags & 0x0008)
    hflags |= O_APPEND;
#endif
#ifdef O_CREAT
  if (flags & 0x0200)
    hflags |= O_CREAT;
#endif
#ifdef O_TRUNC
  if (flags & 0x0400)
    hflags |= O_TRUNC;
#endif
#ifdef O_EXCL
  if (flags & 0x0800)
    hflags |= O_EXCL;
#endif
#ifdef O_BINARY
  if (flags & 0x8000)
    hflags |= O_BINARY;
#endif

  fd = open (buf, hflags, mode);
  if (fd >= 0)
    fd = mdi_fdalloc (fd);
  return fd;
}


static int
mdi_fdclose (int fd)
{
  int status = 0;

  if (fd > 2)
    {
      int sysfd = mdi_fdlookup (fd);
      struct mdifd *mp;
      if (sysfd < 0)
	return sysfd;
      mp = &mdifd[fd];
      status = close (sysfd);
      mp->open = 0;
    }
  return status;
}

#if defined(HAVE_SIGSETJMP)
#define SIGJMP_BUF		sigjmp_buf
#define SIGSETJMP(buf)		sigsetjmp(buf, 1)
#define SIGLONGJMP(buf,val)	siglongjmp(buf,val)
#else
#define SIGJMP_BUF		jmp_buf
#define SIGSETJMP(buf)		setjmp(buf)
#define SIGLONGJMP(buf,val)	longjmp(buf,val)
#endif

static SIGJMP_BUF mdi_readline_env;

static RETSIGTYPE (*mdi_readline_prev_sigint) ();


static RETSIGTYPE
mdi_readline_signal_handler (int signo)
{
#if 0
  /* Don't want to invoke old handler. */
  (*mdi_readline_prev_sigint) (signo);
#endif
  SIGLONGJMP (mdi_readline_env, 1);
}


static int
mdi_read_stdin (char *buf, int len)
{
  static char *inbuf = NULL;
  char *tmpbuf = NULL;
  static int inlen = 0;
  static int inptr;
  char *bp = buf;

  char *line;
  extern FILE *instream;

  while (len > 0)
    {
      if (inlen <= 0)
	{
	  int result;

	  if (inbuf)
	    {
	      free (inbuf);
	      inbuf = NULL;
	    }

	  if (gdb_stdtargin == gdb_stdin && deprecated_readline_begin_hook)
	    (*deprecated_readline_begin_hook) ("");

	  /* Make sure that all output has been output.  Some machines
	     may let you get away with leaving out some of the
	     gdb_flush, but not all.  */
	  wrap_here ("");
	  gdb_flush (gdb_stdout);
	  gdb_flush (gdb_stderr);

	  if (gdb_stdtargin == gdb_stdin && deprecated_readline_hook)
	    tmpbuf = (*deprecated_readline_hook) (mdi_target_prompt);
	  else if (gdb_stdtargin != gdb_stdin || input_from_terminal_p ())
	    {
	      if (SIGSETJMP (mdi_readline_env))
		{
		  signal (SIGINT, mdi_readline_prev_sigint);
		  quit_flag = 1;
		}
	      else {

		mdi_readline_prev_sigint = signal (SIGINT,
						   mdi_readline_signal_handler);

		if (gdb_stdtargin == gdb_stdin)
		  {
		    tmpbuf = gdb_readline_wrapper (mdi_target_prompt);
		  }
		else
		  {
		    long ret;
		    tmpbuf = xmalloc (32768);
		    ret = ui_file_read (gdb_stdtargin, tmpbuf, 32767);
		    if (ret > 0)
		      tmpbuf[ret] = '\0';
		    else
		      {
			xfree (tmpbuf);
			tmpbuf = NULL;
		      }
		  }

		signal (SIGINT, mdi_readline_prev_sigint);
	      }
	    }
	  else
	    {
	      /* read one line from a file */
	      tmpbuf = gdb_readline (0);
	    }

	  if (gdb_stdtargin == gdb_stdin && deprecated_readline_end_hook)
	    (*deprecated_readline_end_hook) ();

	  if (remote_debug)
	    fprintf_unfiltered (mdi_logfp, "read \"%s\", quit_flag=%d\n",
				tmpbuf ? tmpbuf : "<null>", quit_flag);

	  if (quit_flag)
	    {
	      if (tmpbuf)
		free (tmpbuf);
	      quit_flag = 0;
	      errno = EINTR;
	      return -1;
	    }

	  if (tmpbuf)
	    {
	      /* Append \n. */
	      inlen = strlen (tmpbuf) + 1;
	      inbuf = (char *) malloc (inlen + 1);
	      memcpy (inbuf, tmpbuf, inlen - 1);
	      inbuf[inlen - 1] = '\n';
	      inbuf[inlen] = '\0';
	      free (tmpbuf);
	      inptr = 0;
	    }
	  else
	    {
	      /* EOF! */
	      inlen = 0;
	      break;
	    }
	}
      else
	{
	  int n = inlen > len ? len : inlen;
	  memcpy (bp, &inbuf[inptr], n);
	  bp += n;
	  len -= n;
	  inptr += n;
	  inlen -= n;
	  if (inlen == 0)
	    break;
	}
    }

  return bp - buf;
}


#define _MDI_SYSCALL_OPEN	0
#define _MDI_SYSCALL_CLOSE	1
#define _MDI_SYSCALL_READ	2
#define _MDI_SYSCALL_WRITE	3
#define _MDI_SYSCALL_GETCHAR	4
#define _MDI_SYSCALL_PUTCHAR	5
#define _MDI_SYSCALL_LSEEK32	6
#define _MDI_SYSCALL_GETTIME	7
#define _MDI_SYSCALL_EXIT	8
#define _MDI_SYSCALL_MOVED	9
#define _MDI_SYSCALL_GETARGS	10
#define _MDI_SYSCALL_ISATTY	11
#define _MDI_SYSCALL_PROFIL	12
#define _MDI_SYSCALL_SIGHOOK	13

#define MDI_SYS_EXITED		0
#define MDI_SYS_RESUME		1
#define MDI_SYS_SIGNALLED	2
#define MDI_SYS_INTERRUPT	3

/* Provide a host file i/o mechanism. */

static int
mdi_do_syscall (void)
{
  ULONGEST v0 = mdi_readreg (V0_REGNUM);
  ULONGEST a0 = mdi_readreg (A0_REGNUM);
  ULONGEST a1 = mdi_readreg (A0_REGNUM + 1);
  ULONGEST a2 = mdi_readreg (A0_REGNUM + 2);
#define BUFLEN 8192
  char buf[BUFLEN];
  int ptrsize = TARGET_PTR_BIT / 8;
  int result = MDI_SYS_RESUME;
  int rv;

  switch ((int) v0)
    {
    case _MDI_SYSCALL_OPEN:
      if (mdi_readstring (a0, buf, 1024))
	{
	  if (remote_debug > 0)
	    fprintf_unfiltered (mdi_logfp, 
				"mdi_syscall: open(\"%s\", 0x%x, 0%o)\n",
				buf, (int)a1, (int)a2);
	  rv = mdi_fdopen (buf, a1, a2);
	}
      else
	{
	  if (remote_debug > 0)
	    fprintf_unfiltered (mdi_logfp, 
				"mdi_syscall: open(memerr<0x%s>, 0x%x, 0%o)",
				phex_nz (a0, sizeof a0), (int)a1, (int)a2);
	  rv = -1;
	}
      if (remote_debug > 0)
	fprintf_unfiltered (mdi_logfp, "mdi_syscall: open -> %d\n", rv);
      mdi_writereg (V0_REGNUM, rv);
      break;

    case _MDI_SYSCALL_CLOSE:
      if (remote_debug > 0)
	fprintf_unfiltered (mdi_logfp, "mdi_syscall: close(%d)\n", (int)a1);
      rv = mdi_fdclose (a0);
      if (remote_debug > 0)
	fprintf_unfiltered (mdi_logfp, "mdi_syscall: close -> %d\n", rv);
      mdi_writereg (V0_REGNUM, rv);
      break;

    case _MDI_SYSCALL_READ:
      {
	int sysfd = mdi_fdlookup (a0);
	int remain = a2;
	int nread = 0;
	int n = 0;

	if (remote_debug > 0)
	  fprintf_unfiltered (mdi_logfp, 
			      "mdi_syscall: read(%d, 0x%s, %d)\n",
			      (int)a0, phex_nz (a1, sizeof a1), remain);
	if (sysfd < 0)
	  nread = -1;
	else if (remain > 0)
	  switch (sysfd)
	    {
	    case 0:
	      n = (remain > BUFLEN) ? BUFLEN : remain;
	      nread = mdi_read_stdin (buf, n);
	      if (nread < 0)
		{
		  mdi_writestring (a1, "\n", 1);
		  nread = 1;	/* to avoid end of file */
		  result = MDI_SYS_INTERRUPT;
		  break;
		}
	      if (nread > 0 && !mdi_writestring (a1, buf, nread))
		nread = -1;	/* memory error */
	      break;

	    case 1:
	    case 2:
	      nread = -1;	/* bad file descriptor */
	      break;

	    default:
	      while (remain > 0)
		{
		  n = (remain > BUFLEN) ? BUFLEN : remain;
		  n = read (sysfd, buf, n);
		  if (n > 0)
		    {
		      if (!mdi_writestring (a1, buf, n))
			n = -1;	/* memory error */
		    }
		  if (n <= 0)
		    {
		      if (nread == 0)
			nread = n;
		      break;
		    }
		  a1 += n;
		  remain -= n;
		  nread += n;
		}
	      break;
	    }

	if (remote_debug > 0)
	  fprintf_unfiltered (mdi_logfp, "mdi_syscall: read -> %d\n", nread);
	mdi_writereg (V0_REGNUM, nread);
      }
      break;

    case _MDI_SYSCALL_WRITE:
      {
	int sysfd = mdi_fdlookup (a0);
	int remain = a2;
	int nwritten = 0;
	int n;

	if (remote_debug > 0)
	  fprintf_unfiltered (mdi_logfp, 
			      "mdi_syscall: write(%d, 0x%s, %d)\n",
			      (int)a0, phex_nz (a1, sizeof a1), remain);
	if (sysfd <= 0)
	  nwritten = -1;
	else
	  {
	    while (remain > 0)
	      {
		n = (remain > BUFLEN) ? BUFLEN : remain;
		if (mdi_readstring (a1, buf, n))
		  {
		    if (sysfd == 1)
		      {
			ui_file_write (gdb_stdtarg, buf, n);
			gdb_flush (gdb_stdtarg);
		      }
		    else if (sysfd == 2)
		      {
			ui_file_write (gdb_stdtargerr, buf, n);
			gdb_flush (gdb_stdtargerr);
		      }
		    else
		      n = write (sysfd, buf, n);
		  }
		else
		  n = -1;		/* memory error */
		if (n <= 0)
		  {
		    if (nwritten == 0)
		      nwritten = n;
		    break;
		  }
		a1 += n;
		remain -= n;
		nwritten += n;
	      }
	  }
	if (remote_debug > 0)
	  fprintf_unfiltered (mdi_logfp, "mdi_syscall: write -> %d\n", nwritten);
	mdi_writereg (V0_REGNUM, nwritten);
      }
      break;

    case _MDI_SYSCALL_LSEEK32:
      {
	int sysfd = mdi_fdlookup (a0);
	int whence = a2;
	long offs;

	if (remote_debug > 0)
	  fprintf_unfiltered (mdi_logfp, 
			      "mdi_syscall: lseek32(%d, %ld, %d)\n",
			      (int)a0, (long)a1, whence);
	if (sysfd > 2)
	  {
	    switch (whence)
	      {
	      case 0:
		whence = SEEK_SET;
		break;
	      case 1:
		whence = SEEK_CUR;
		break;
	      case 2:
		whence = SEEK_END;
		break;
	      }
	    offs = lseek (sysfd, (long) a1, whence);
	  }
	else
	  offs = -1;
	if (remote_debug > 0)
	  fprintf_unfiltered (mdi_logfp, "mdi_syscall: lseek32 -> %ld\n", offs);
	mdi_writereg (V0_REGNUM, offs);
      }
      break;

    case _MDI_SYSCALL_ISATTY:
      {
	int sysfd = mdi_fdlookup (a0);
	rv = 0;
	if (remote_debug > 0)
	  fprintf_unfiltered (mdi_logfp, "mdi_syscall: isatty(%d)\n", (int)a0);
	if (sysfd >= 0)
	  {
	    switch (sysfd)
	      {
	      case 0:
		rv = ui_file_isatty (gdb_stdtargin);
		break;
	      case 1:
		rv = ui_file_isatty (gdb_stdtarg);
		break;
	      case 2:
		rv = ui_file_isatty (gdb_stdtargerr);
		break;
	      default:
		rv = isatty (sysfd);
		break;
	      }
	  }
	if (remote_debug > 0)
	  fprintf_unfiltered (mdi_logfp, "mdi_syscall: isatty -> %d\n", rv);
	mdi_writereg (V0_REGNUM, rv);
      }
      break;

    case _MDI_SYSCALL_GETCHAR:
      /* XXX Find a way to read a single character without line editing. */
      if (remote_debug > 0)
	fprintf_unfiltered (mdi_logfp, "mdi_syscall: getchar(%d)\n", (int)a0);
      rv = 0;
      if (a0 == 0)
	{
	  int n = mdi_read_stdin (buf, 1);
	  if (n < 0)
	    /* Return no character, and interrupt result. */
	    result = MDI_SYS_INTERRUPT;
	  else if (n == 1)
	    {
	      if (buf[0] == '\n')
		rv = '\r';
	      else
		rv = buf[0] & 0xff;
	    }
	}
      if (remote_debug > 0)
	fprintf_unfiltered (mdi_logfp, "mdi_syscall: getchar -> '%c' (0x%x)\n", 
			    isprint (rv) ? rv : '.', rv);
      mdi_writereg (V0_REGNUM, rv);
      break;

    case _MDI_SYSCALL_PUTCHAR:
      {
	char buf = a1;
	if (remote_debug > 0)
	  fprintf_unfiltered (mdi_logfp, "mdi_syscall: putchar('%c' (0x%x))\n",
			      isprint (buf) ? buf : '.', buf);
	ui_file_write (gdb_stdtarg, &buf, 1);
	gdb_flush (gdb_stdtarg);
      }
      break;

    case _MDI_SYSCALL_GETTIME:
      {
	struct timeval tv;
	gettimeofday (&tv, NULL);
	mdi_writereg (V0_REGNUM, tv.tv_sec);
	mdi_writereg (V0_REGNUM + 1, tv.tv_usec);
	if (remote_debug > 0)
	  fprintf_unfiltered (mdi_logfp, "mdi_syscall: gettime -> %ld.%06ld\n", 
			      tv.tv_sec, tv.tv_usec);
      }
      break;

    case _MDI_SYSCALL_GETARGS:
      {
	CORE_ADDR sp, argv;
	unsigned char buf[8];
	int argc, nbytes, i;

	/* Get total arg string length. */
	nbytes = 0;
	for (argc = 0; mdi_argv && mdi_argv[argc]; argc++)
	  nbytes += strlen (mdi_argv[argc]) + 1;

	/* Start writing arg strings at top of stack. */
	sp = mdi_readreg (29);

	if (remote_debug > 0)
	  fprintf_unfiltered (mdi_logfp, "mdi_syscall: getargs(), sp=0x%s\n",
			      phex_nz (sp, sizeof sp));

	/* Start writing argv[] array just below arg strings. */
	argv = (sp - nbytes) & ~7;

	/* Store terminating NULL in argv[argc]. */
	store_signed_integer (buf, ptrsize, 0);
	mdi_writestring (argv -= ptrsize, buf, ptrsize);

	/* Write arguments to stack in "reverse" order. */
	for (i = argc - 1; i >= 0; i--)
	  {
	    size_t len = strlen (mdi_argv[i]) + 1;
	    /* Write arg string. */
	    mdi_writestring (sp -= len, mdi_argv[i], len);
	    /* Write argv[] pointer. */
	    store_signed_integer (buf, ptrsize, sp);
	    mdi_writestring (argv -= ptrsize, buf, ptrsize);
	  }

	if (argc > 0)
	  {
	    /* Set $a0, $a1 to argc & argv */
	    mdi_writereg (A0_REGNUM + 0, argc);
	    mdi_writereg (A0_REGNUM + 1, argv);
	    /* Round stack pointer down to max stack alignment, then
	       leave space for max arg save area. */
	    sp = (argv & ~15) - 64;
	    mdi_writereg (29, sp);
	  }

	if (remote_debug > 0)
	  fprintf_unfiltered (mdi_logfp,
			      "mdi_syscall: getargs -> argc=%d argv=0x%s sp=0x%s\n",
			      argc, phex_nz (argv, sizeof argv),
			      phex_nz (sp, sizeof sp));
      }
      break;

    case _MDI_SYSCALL_PROFIL:
      /* Program control over MIPSsim profiling. */
      if (remote_debug > 0)
	fprintf_unfiltered (mdi_logfp, "mdi_syscall: profil(%s)\n",
			    a0 ? "start" : "stop");
      if (mdi_profile_active)
	{
	  int i;
	  for (i = 0; i < 2; i++)
	    {
	      if (mdi_profreg[i].valid)
		{
		  if (a0)
		    mipssim_StartProfile
		      (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle,
		       mdi_profreg[i].id);
		  else
		    mipssim_StopProfile
		      (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle,
		       mdi_profreg[i].id);
		}
	    }
	}
      break;

    case _MDI_SYSCALL_SIGHOOK:
      /* Program received signal - return signal number to handler */
      if (remote_debug > 0)
	fprintf_unfiltered (mdi_logfp, "mdi_syscall: sighook(signo %d)\n",
			    (int)a0);
      mdi_writereg (V0_REGNUM, a0);
      result = MDI_SYS_SIGNALLED;
      break;

    case _MDI_SYSCALL_EXIT:
      /* Program exited. */
      if (remote_debug > 0)
	fprintf_unfiltered (mdi_logfp, "mdi_syscall: exit(%d)\n", (int)a0);
      result = MDI_SYS_EXITED;
      break;

    case _MDI_SYSCALL_MOVED:
      if (remote_debug > 0)
	fprintf_unfiltered (mdi_logfp, "mdi_syscall: moved()\n");
      break;

    default:
      /* Just let target continue, MDI can reinsert breakpoints. */
      break;
    }

  if (result != MDI_SYS_EXITED)
    /* Skip rest of "syscall" function and return to caller. */
    mdi_writepc (mdi_readreg (RA_REGNUM));

  if (remote_debug > 0 && result != MDI_SYS_RESUME)
    fprintf_unfiltered (mdi_logfp, "mdi_syscall: not resuming, result=%s\n",
			(result == MDI_SYS_EXITED) ? "exited"
			: (result == MDI_SYS_SIGNALLED) ? "signalled"
			: (result == MDI_SYS_INTERRUPT) ? "interrupt"
			: "???");

  return result;
}


static CORE_ADDR
mdi_kva2phys (CORE_ADDR addr)
{
#if defined(BFD64) && defined(CC_HAS_LONG_LONG)
  if (0xffffffff80000000LL <= addr && addr < 0xffffffffc0000000LL)
    /* kseg0 / kseg1 */
    return addr & 0x1fffffffLL;
  if (0x8000000000000000LL <= addr && addr < 0xc000000000000000LL)
    /* xkphys */
    return addr & 0x07ffffffffffffffLL;
#else
  if ((CORE_ADDR) 0x80000000 <= addr && addr < (CORE_ADDR) 0xc0000000)
    return addr & (CORE_ADDR) 0x1fffffff;
#endif
  return addr;
}


/* Sleep for the specified number of milliseconds.
   If select () is interrupted, returns immediately;
   takes an error exit if select () fails for some other reason. */

static void
sleep_ms (long ms)
{
  struct timeval timeout;
  int status;

  timeout.tv_sec = ms / 1000;
  timeout.tv_usec = (ms % 1000) * 1000;

  status = select (0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &timeout);

  if (status < 0 && errno != EINTR)
    perror_with_name ("select");
}


static ptid_t
mdi_wait (ptid_t ptid, struct target_waitstatus *status)
{
  MDIInt32 mdistat;
  MDIRunStateT runstate;
  CORE_ADDR pc;
  int stop;
  int restarted;
  int quit_printed;
  int i;

  if (mdi_starting)
    {
      /* First time through, called from start_remote(), do nothing. */
      mdi_starting = 0;
      status->kind = TARGET_WAITKIND_STOPPED;
      status->value.sig = TARGET_SIGNAL_TRAP;
      return inferior_ptid;
    }

  if (remote_debug)
    fprintf_unfiltered (mdi_logfp, "mdi_wait:...\n");

  /* Ideally we should just call mdiRunState with an infinite wait
     time and rely on mdiAbort from the periodic callback.
     Unfortunately MIPSsim 3.x doesn't use the periodic callback, in
     fact it doesn't implement wait time either, it treats all calls
     as a poll, so we implement our own small sleep, to give other
     parts of the system a chance. 
     XXX Could we fix this up for MIPSsim 4.x? */

  stop = 0;

  do
    {
      int sleep_time;
      int not_running;
      int seen_reset;

      restarted = 0;

      i = (mdi_desc->gmindex + 1) % mdi_desc->gmcount;

      /* Install our SIGINT signal handler. */
      interrupt_cleanup = wait_cleanup;
      ofunc = signal (SIGINT, mdi_interrupt);

      sleep_time = 10;		/* start off with 10ms and increase */
      not_running = 0;
      seen_reset = 0;
      quit_printed = 0;

      do
	{
	  int disabled;

	  runstate.Status = MDIStatusNotRunning;

	  mdi_wait_active = 1;
	  mdistat = mdiRunState (mdi_desc->gmdata[i].DevHandle,
				 mdi_waittime, &runstate);
	  mdi_wait_active = 0;

	  if (mdistat != MDISuccess)
	    {
	      MDIWARN (mdistat, "fetching CPU run state");
	      break;
	    }

	  /* See if the user has asked us to stop. */
	  if (ptid_equal (ptid, pid_to_ptid (-1))
	      || mdi_desc->gmdata[i].DevHandle == ptid_get_lwp (ptid))
	    {
	      if (!stop && deprecated_ui_loop_hook != NULL)
		stop = deprecated_ui_loop_hook (0);
    
	      if (quit_flag)
		{
		  quit_flag = 0;
		  stop = 1;
		}
	    }

	  if (runstate.Status == MDIStatusNotRunning) 
	    {
	      /* Accumulate time spent NotRunning. */
	      not_running += mdi_waittime;
	      /* Time out after 5000ms,
	         or immediately if called from start_remote.  */
	      if (not_running >= 5000)
		break;
	    }

	  disabled = (runstate.Status == MDIStatusDisabled
		      || runstate.Status == MDIStatusVPEDisabled);

	  if ((runstate.Status & MDIStatusMask) != MDIStatusRunning
	      && runstate.Status != MDIStatusNotRunning
	      && !disabled)
	    /* Terminate wait loop when an interesting new state occurs. */
	    break;

	  if (runstate.Status & MDIStatusReset)
	    {
	      if (! seen_reset)
		warning ("CPU reset active");
	      seen_reset = 1;
	    }
	  else
	    {
	      seen_reset = 0;
	    }

	  if (stop && (!disabled || mdi_desc->gmcount == 1))
	    {
	      if (!quit_printed)
		{
		  printf_filtered ("Quit received: Stopping target\n");
		  quit_printed = 1;
		}
	      mdistat = mdiStop (mdi_desc->gmdata[i].DevHandle);
	      if (mdistat != MDISuccess)
		MDIWARN (mdistat, "stopping CPU");
	    }

	  /* Give the rest of the system a chance to do something. */
	  sleep_ms (sleep_time);

	  /* Accumulate time spent not running. */
	  not_running += sleep_time;

	  /* Keep increasing the length of the sleep, but limit to
	     200ms to avoid long GUI delays when hitting the stop
	     button. */
	  sleep_time *= 2;
	  if (sleep_time > 200)
	    sleep_time = 200;

	  if (!stop || disabled)
	    i = (i + 1) % mdi_desc->gmcount;
	}
      while (1);

      /* Restore old SIGINT handler. */
      signal (SIGINT, ofunc);

      mdi_desc->gmindex = i;

      /* This is a hook for when we need to do something (perhaps the
	 collection of trace data) every time the target stops.  */
      if (target_wait_loop_hook)
	(*target_wait_loop_hook) ();

      if (remote_debug)
	{
	  fprintf_unfiltered (mdi_logfp,
			      "mdi_wait: stopped at 0x%s, ",
			      phex (mdi_readpc (), sizeof (ULONGEST)));
	  mdiDisplayRunState (&runstate);
	}

      /* Check for a host i/o "system call". */
      if ((runstate.Status == MDIStatusBPHit
	   || runstate.Status == MDIStatusUsrBPHit
	   || runstate.Status == MDIStatusStepsDone)
	  && mdi_syscall_inserted
	  && (pc = mdi_kva2phys ((CORE_ADDR) mdi_readpc ()))
	  && (mdi_kva2phys (mdi_syscall) == pc 
	      || mdi_kva2phys (mdi_syscall_rom) == pc))
	{
	  int result = mdi_do_syscall ();
	  if (stop)
	    result = MDI_SYS_INTERRUPT;	/* catch a pending halt */
	  switch (result)
	    {
	    case MDI_SYS_RESUME:
	      /* I/O complete - restart target. */
	      mdistat = mdiExecute (mdi_desc->gmdata[i].DevHandle);
	      if (mdistat != MDISuccess)
		MDIWARN (mdistat, "resume execution after syscall");
	      else
		restarted = 1;
	      break;
	    case MDI_SYS_EXITED:
	      /* Target program exited. */
	      runstate.Status = MDIStatusExited;
	      break;
	    case MDI_SYS_INTERRUPT:
	      /* Interrupted by user. */
	      runstate.Status = MDIStatusHalted;
	      break;
	    case MDI_SYS_SIGNALLED:
	      /* Signal occurred. */
	      runstate.Status = MDIStatusHalted;
		mdi_signalled = 1;
	      break;
	    }
	}
    }
  while (restarted);

  for (i = (mdi_desc->gmindex + 1) % mdi_desc->gmcount; i != mdi_desc->gmindex;
       i = (i + 1) % mdi_desc->gmcount)
    {
      mdistat = mdiStop (mdi_desc->gmdata[i].DevHandle);
      if (mdistat != MDISuccess)
	MDIWARN (mdistat, "stopping CPU");
    }

  for (i = (mdi_desc->gmindex + 1) % mdi_desc->gmcount; i != mdi_desc->gmindex;
       i = (i + 1) % mdi_desc->gmcount)
    {
      MDIRunStateT rs;
      int sleep_time = 10;	/* start off with 10ms and increase */

      while (1)
	{
	  rs.Status = MDIStatusNotRunning;

	  mdi_wait_active = 1;
	  mdistat = mdiRunState (mdi_desc->gmdata[i].DevHandle,
				 mdi_waittime, &rs);
	  mdi_wait_active = 0;

	  if (mdistat != MDISuccess)
	    {
	      MDIWARN (mdistat, "fetching CPU run state");
	      break;
	    }

	  if ((rs.Status & MDIStatusMask) != MDIStatusRunning)
	    break;

	  /* Keep increasing the length of the sleep, but limit to
	     200ms to avoid long GUI delays when hitting the stop
	     button. */
	  sleep_time *= 2;
	  if (sleep_time > 200)
	    sleep_time = 200;
	}

	if (rs.Status == MDIStatusDisabled
	    || rs.Status == MDIStatusVPEDisabled)
	  {
	    mdi_desc->gmdata[i].disabled = 1;
	  }
	else
	  {
	    mdi_desc->gmdata[i].disabled = 0;
	    if (!mdi_desc->gmdata[i].probed)
	      {
		updateDev (mdi_desc, i, mdi_desc->gmdata[i].DeviceData.Id);
		mdi_probe_cpu (mdi_desc, i);
		mdi_desc->gmdata[i].probed = 1;
	      }
	  }
    }

  quit_flag = 0;		/* consume any left over quit signal */
  mdi_runstate = runstate;

  /* Update $cycles convenience variable. */
  if (mdi_desc->mipssim_p)
    mdi_cycles_update ();

  mdi_set_inferior_ptid ();

  mdi_curbpt.Type = 0;
  switch (runstate.Status & MDIStatusMask)
    {
    default:
      warning ("MDIRunState=%s", statusName (runstate.Status & MDIStatusMask));
      status->kind = TARGET_WAITKIND_SPURIOUS;
      status->value.sig = TARGET_SIGNAL_0;
      break;
    case MDIStatusDisabled:
      /* Can't happen... */
      internal_error (__FILE__, __LINE__,
		      "MDIRunState.Status = MDIStatusDisabled");
    case MDIStatusRunning:
      /* Can't happen... */
      internal_error (__FILE__, __LINE__,
		      "MDIRunState.Status = MDIStatusRunning");
    case MDIStatusNotRunning:
      if ((runstate.Status & (MDIStatusReset | MDIStatusWasReset)) == 0)
	warning ("MDI time out: MDIStatusNotRunning");
      /* fall through */
    case MDIStatusHalted:
      /* Report interesting reset events to user. */
      if (runstate.Status & MDIStatusReset)
	warning ("CPU held in reset");
      else if (runstate.Status & MDIStatusWasReset)
	warning ("CPU was reset");
      /* We requested target to stop. */
      status->kind = TARGET_WAITKIND_STOPPED;
      if (mdi_signalled)
	{
	  mdi_lastsig = (enum target_signal) mdi_readreg (A0_REGNUM);
	  if (mdi_lastsig <= TARGET_SIGNAL_FIRST || 
	      mdi_lastsig >= TARGET_SIGNAL_LAST)
	    mdi_lastsig = TARGET_SIGNAL_UNKNOWN;
	  status->value.sig = mdi_lastsig;
	}
      else
	{
	  status->value.sig = TARGET_SIGNAL_INT;
	  if (runstate.Status & MDIStatusDescription)
	    warning ("MDI halted: %s", (char *) runstate.Info.ptr);
	}
      break;
    case MDIStatusExited:
      {
	unsigned int ec = mdi_readreg (A0_REGNUM);
	if (ec >> 8)
	  {
	    mdi_lastsig = (enum target_signal) ((ec >> 8) & 0xff);
	    if (mdi_lastsig <= TARGET_SIGNAL_FIRST || 
		mdi_lastsig >= TARGET_SIGNAL_LAST)
	      mdi_lastsig = TARGET_SIGNAL_UNKNOWN;
	    status->kind = TARGET_WAITKIND_SIGNALLED;
	    status->value.sig = mdi_lastsig;
	  }
	else 
	  {
	    status->kind = TARGET_WAITKIND_EXITED;
	    status->value.integer = ec;
	  }
      }
      /* Cleanup target file i/o and write profiling data. */
      mdi_fdcloseall ();
      mdi_profiling_close (1);
      mdi_exited = 1;
      inferior_ptid = null_ptid;
      break;
    case MDIStatusStepsDone:
      status->kind = TARGET_WAITKIND_STOPPED;
      status->value.sig = TARGET_SIGNAL_TRAP;
      break;
    case MDIStatusBPHit:
      status->kind = TARGET_WAITKIND_STOPPED;
      status->value.sig = TARGET_SIGNAL_TRAP;
      {
	MDIBpDataT *bpt =
	  mdi_find_breakpoint (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle,
			       runstate.Info.value);
	if (bpt)
	  {
	    mdi_curbpt = *bpt;
	    break;
	  }
      }
      /* Fall through in case something has gone wrong in MDIlib.  */
    case MDIStatusUsrBPHit:
      status->kind = TARGET_WAITKIND_STOPPED;
      status->value.sig = TARGET_SIGNAL_TRAP;
      {
	MDIBpDataT *bpt = mdi_find_usr_breakpoint ((CORE_ADDR) mdi_readpc ());
	if (bpt)
	  mdi_curbpt = *bpt;
      }
      break;
    case MDIStatusException:
      status->kind = TARGET_WAITKIND_STOPPED;
      switch (runstate.Info.value)
	{
	case 0:		/* INTR */
	  status->value.sig = TARGET_SIGNAL_KILL;
	  break;
	case 1:		/* MOD */
	case 2:		/* TLBL/IBOUND */
	case 3:		/* TLBS/DBOUND */
	  status->value.sig = TARGET_SIGNAL_SEGV;
	  break;
	case 4:		/* ADEL */
	case 5:		/* ADES */
	case 6:		/* IBE/BE */
	case 7:		/* DBE */
	  /* NB some LSI processors signal watchpoints with a bus error */
	  status->value.sig = TARGET_SIGNAL_BUS;
	  break;
	case 8:		/* SYS */
	  status->value.sig = TARGET_SIGNAL_SYS;
	  break;
	case 9:		/* BP */
	  /* NB could decode break instruction to refine the signal */
	  status->value.sig = TARGET_SIGNAL_TRAP;
	  break;
	case 10:		/* RI */
	case 11:		/* CPU */
	  status->value.sig = TARGET_SIGNAL_ILL;
	  break;
	case 12:		/* OVF */
	  status->value.sig = TARGET_SIGNAL_FPE;	/* ??? */
	  break;
	case 13:		/* TRAP */
	  status->value.sig = TARGET_SIGNAL_TRAP;
	  break;
	case 14:		/* VCEI */
	case 15:		/* FPE */
	case 16:		/* CP2 */
	  status->value.sig = TARGET_SIGNAL_KILL;
	  break;
	case 17:		/* RES17 */
	case 18:		/* RES18 */
	case 19:		/* RES19 */
	case 20:		/* RES20 */
	case 21:		/* RES21 */
	case 22:		/* RES22 */
	  status->value.sig = TARGET_SIGNAL_KILL;
	  break;
	case 23:		/* WATCH */
	  status->value.sig = TARGET_SIGNAL_TRAP;
	  break;
	case 24:		/* MCHECK */
	case 25:		/* RES25 */
	case 26:		/* RES26 */
	case 27:		/* RES27 */
	case 28:		/* RES28 */
	case 29:		/* RES29 */
	case 30:		/* CACHEERR */
	case 31:		/* VCED */
	  status->value.sig = TARGET_SIGNAL_KILL;
	  break;
	default:
	  status->value.sig = TARGET_SIGNAL_KILL;
	  break;
	}
      break;
    case MDIStatusTraceFull:
      /* FIXME collect trace data. */
      status->kind = TARGET_WAITKIND_IGNORE;
      break;
    }

  return inferior_ptid;
}


static int
mdi_map_regno (int regno, MDIResourceT * rsrc, MDIOffsetT * offs, int write)
{
  const struct mips_regnum *regnum = mips_regnum (current_gdbarch);
  int size = mdi_desc->regsize;

  if (regno < 32)
    {
      *offs = regno;
      *rsrc = MDIMIPCPU;
    }
  else if (regno >= regnum->fp0
	   && regno < regnum->fp0 + 32)
    {
      size = register_size (current_gdbarch, regno);

      if (mdi_fpr_avail < 0)
	{
	  /* Check for availability of new MDI FPR resource. */
	  char tmp[8];
	  mdi_fpr_avail =
	    (mdiRead (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle, MDIMIPFPR,
		      0, tmp, register_size (current_gdbarch, regnum->fp0), 1)
	     == MDISuccess);
	}

      if (mdi_fpr_avail)
	/* Got new "software-model" MDI resource. */
	*rsrc = MDIMIPFPR;
      else
	{
	  /* Have to use old "hardware-model" MDI resource. */
	  /* XXX We don't really know if we're on CPU with 32 or 16
	     64-bit fp registers, so we don't know whether to divide
	     the regno by two or not.  We can't just tell from the
	     size, because MIPS32 has 32 single precision registers,
	     and MIPS I has 16. We'll assume MIPS32 and hope for the
	     best. */
	  if (size == 4)
	    /* 32-bit register mode: 0 - 15/31, assume 31. */
	    /* XXX Will the odd registers actually give us the
	       high part of the double? */
	    *rsrc = MDIMIPFP;
	  else
	    /* 64-bit register mode: 0 - 15/31, assume 31. */
	    *rsrc = MDIMIPDFP;
	}
      regno -= regnum->fp0;
      *offs = regno;
      if (mdi_cpdreg_miss[1][0] & (1 << regno))
	return 0;
    }
  else if (regno >= CP0_REGNUM && regno < CP0_REGNUM+32*8)
    {
      /* CP0 registers. */
      int reg = regno - CP0_REGNUM;
      /* Don't access cp0 registers where a previous read failed. */
      if (mdi_cpdreg_miss[0][reg / 32] & (1 << (reg % 32)))
	return 0;
      /* Don't allow writes to write protected registers. */
      if (write && (mdi_cpdreg_wp[0][reg / 32] & (1 << (reg % 32))))
	return 0;
      *offs = reg;
      *rsrc = MDIMIPCP0;
    }
  else if (regno >= CP2_REGNUM && regno < CP2_REGNUM+32*8)
    {
      /* CP2 registers. */
      int reg = regno - CP2_REGNUM;
      /* Don't access cp2 registers where a previous read failed. */
      if (mdi_cpdreg_miss[2][reg / 32] & (1 << (reg % 32)))
	return 0;
      /* Don't allow writes to write protected registers. */
      if (write && (mdi_cpdreg_wp[2][reg / 32] & (1 << (reg % 32))))
	return 0;
      *offs = reg;
      *rsrc = MDIMIPCP2;
    }
  else if (regno >= CP2CTL_REGNUM && regno < CP2CTL_REGNUM+32)
    {
      /* CP2 control registers. */
      int reg = regno - CP2CTL_REGNUM;
      /* Don't access cp2 control registers where a previous read failed. */
      if (mdi_cpcreg_miss[2] & (1 << reg))
	return 0;
      /* Don't allow writes to write protected registers. */
      if (write && (mdi_cpcreg_wp[2] & (1 << reg)))
	return 0;
      *offs = reg;
      *rsrc = MDIMIPCP2C;
    }
  else if (regno == regnum->hi)
    {
      *offs = 0;
      *rsrc = MDIMIPHILO;
    }
  else if (regno == regnum->lo)
    {
      *offs = 1;
      *rsrc = MDIMIPHILO;
    }
  else if (regnum->dspacc <= regno && regno < regnum->dspacc + 10)
    {
      int o = 2 + regno - regnum->dspacc;
      if (mdi_hilo_miss & (1 << o))
	return 0;
      *offs = o;
      *rsrc = MDIMIPHILO;
    }
  else if (regno == regnum->dspctl)
    {
      int o = regno - regnum->dspctl;
      if (mdi_dsp_miss & (1 << o))
	return 0;
      *offs = o;
      *rsrc = MDIMIPDSP;
    }
  else if (regno == regnum->badvaddr)
    {
      *offs = 8;
      *rsrc = MDIMIPCP0;
    }
  else if (regno == PS_REGNUM)
    {
      *offs = 12;
      *rsrc = MDIMIPCP0;
    }
  else if (regno == regnum->cause)
    {
      *offs = 13;
      *rsrc = MDIMIPCP0;
    }
  else if (regno == regnum->pc)
    {
      *offs = 0;
      *rsrc = MDIMIPPC;
    }
  else if (regno == regnum->fp_control_status)
    {
      *offs = 31;
      *rsrc = MDIMIPCP1C;
      if (mdi_cpcreg_miss[1] & (1 << 31))
	return 0;
    }
  else if (regno == regnum->fp_implementation_revision)
    {
      *offs = 0;
      *rsrc = MDIMIPCP1C;
      if (mdi_cpcreg_miss[1] & (1 << 0))
	return 0;
    }
  else
    return 0;

  return size;
}


static void
mdi_bad_register (MDIInt32 stat, MDIResourceT rsrc, MDIOffsetT offset, 
		  const char *what)
{
  unsigned int offs = offset;
  int coprocessor = 1;

  /* Record the failure, so we don't try again. */
  switch (rsrc)
    {
    case MDIMIPCP0:
      mdi_cpdreg_miss[0][offs / 32] |= 1 << (offs % 32);
      break;
    case MDIMIPFP:
    case MDIMIPDFP:
    case MDIMIPFPR:
      /* If an even register isn't available, assume no fpu. */
      if ((offs & 1) == 0)
	mdi_cpdreg_miss[1][0] = ~0;
      break;
    case MDIMIPCP1C:
      mdi_cpcreg_miss[1] |= 1 << (offs % 32);
      break;
    case MDIMIPCP2:
      mdi_cpdreg_miss[2][offs / 32] |= 1 << (offs % 32);
      break;
    case MDIMIPCP2C:
      mdi_cpcreg_miss[2] |= 1 << (offs % 32);
      break;
    case MDIMIPCP3:
      mdi_cpdreg_miss[3][offs / 32] |= 1 << (offs % 32);
      break;
    case MDIMIPCP3C:
      mdi_cpcreg_miss[3] |= 1 << (offs % 32);
      break;
    case MDIMIPDSP:
      mdi_dsp_miss |= 1 << offs;
      break;
    case MDIMIPHILO:
      mdi_hilo_miss |= 1 << offs;
      if (offs > 1)
	break;			/* DSP HI/LO/ACX may be missing */
      /* fall through */
    default:
      coprocessor = 0;
      break;
    }  

  /* Coprocessor registers are allowed to be missing. */
  if (coprocessor)
    {
      switch (stat) 
	{
	case MDIErrSrcResource:
	case MDIErrSrcCount:
	case MDIErrInvalidSrcOffset:
	case MDIErrDstResource:
	case MDIErrDstCount:
	case MDIErrInvalidDstOffset:
	case MDIErrFailure:	/* grrr, libs shouldn't return this */
	  /* Safe to ignore. */
	  return;
	}
    }

  fprintf_unfiltered (gdb_stderr, "MDI %s: register %s %d\n", 
		      what, resrcName (rsrc), offs);
  MDIERR (stat, what);
}


static void
mdi_fetch_register (int regno)
{
  unsigned LONGEST val;
  char *s;
  int err = 0;

  if (regno == -1)
    {
      for (regno = 0; regno < NUM_REGS; regno++)
	mdi_fetch_register (regno);
      return;
    }

  if (regno == ZERO_REGNUM)
    {
      val = 0;
    }
  else
    {
      char tmp[MAX_REGISTER_SIZE];
      int nr_bytes;
      MDIResourceT rsrc;
      MDIOffsetT offs;

      nr_bytes = mdi_map_regno (regno, &rsrc, &offs, 0);
      if (nr_bytes > 0)
	{
	  MDIInt32 mdistat;
	  mdistat = mdiRead (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle,
			     rsrc, offs, tmp, nr_bytes, 1);
	  if (mdistat == MDISuccess)
	    {
	      /* Register arrives in target byte order,
		 sign-extend the value at the same time. */
	      val = extract_signed_integer (tmp, nr_bytes);
	    }
	  else
	    {
	      mdi_bad_register (mdistat, rsrc, offs, "fetching register");
	      val = 0;
	      err = 1;
	    }
	}
      else
	{
	  val = 0;
	  err = 1;
	}
    }

  {
    char buf[MAX_REGISTER_SIZE];
    /* Gdb expects to see a value in the target byte ordering.  */
    store_unsigned_integer (buf, register_size (current_gdbarch, regno), val);
    regcache_raw_supply (current_regcache, regno, buf);
  }

  if (err)
    /* register value not available */
    set_register_cached (regno, -1);
}


static void
mdi_store_register (int regno)
{
  char tmp[MAX_REGISTER_SIZE];

  int nr_bytes;
  MDIResourceT rsrc;
  MDIOffsetT offs;

  if (regno == -1)
    {
      for (regno = 0; regno < NUM_REGS; regno++)
	mdi_store_register (regno);
      return;
    }

  nr_bytes = mdi_map_regno (regno, &rsrc, &offs, 1);
  if (nr_bytes > 0)
    {
      LONGEST val = read_register (regno);
      MDIInt32 mdistat;

      /* If h/w reg size is 64-bits, but s/w model is 32-bits, then
	 sign-extend from 32 to 64 bits. */
      if (nr_bytes == 8 && register_size (current_gdbarch, regno) == 4)
	val = (int) val;

      /* Send in target order. */
      store_signed_integer (tmp, nr_bytes, val);

      mdistat = mdiWrite (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle,
			  rsrc, offs, tmp, nr_bytes, 1);
      if (mdistat != MDISuccess)
	mdi_bad_register (mdistat, rsrc, offs, "storing register");
    }
}


static ULONGEST
mdi_readreg (int regno)
{
  ULONGEST val = 0;
  char tmp[MAX_REGISTER_SIZE];
  int nr_bytes;
  MDIResourceT rsrc;
  MDIOffsetT offs;

  nr_bytes = mdi_map_regno (regno, &rsrc, &offs, 0);
  if (nr_bytes > 0)
    {
      MDIInt32 mdistat;
      mdistat = mdiRead (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle,
			 rsrc, offs, tmp, nr_bytes, 1);
      if (mdistat == MDISuccess)
	{
	  /* Register arrives in target byte order,
	     sign-extend the value at the same time. */
	  val = extract_signed_integer (tmp, nr_bytes);
	}
      else
	{
	  /* Some registers may be missing. */
	  mdi_bad_register (mdistat, rsrc, offs, "reading register");
	}
    }
  return val;
}

static void
mdi_writereg (int regno, ULONGEST val)
{
  char tmp[MAX_REGISTER_SIZE];
  MDIResourceT rsrc;
  MDIOffsetT offs;
  MDIInt32 mdistat = MDISuccess;
  int nr_bytes;

  nr_bytes = mdi_map_regno (regno, &rsrc, &offs, 1);
  if (nr_bytes > 0)
    {
      /* Send in target order. */
      store_unsigned_integer (tmp, nr_bytes, val);
      mdistat = mdiWrite (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle,
			  rsrc, offs, tmp, nr_bytes, 1);
      MDIERR (mdistat, "writing register");
    }
}


static ULONGEST
mdi_readpc (void)
{
  return mdi_readreg (mips_regnum (current_gdbarch)->pc);
}

static void
mdi_writepc (ULONGEST v)
{
  mdi_writereg (mips_regnum (current_gdbarch)->pc, v);
}


static int
mapped_segment_p (CORE_ADDR addr)
{
#ifdef BFD64
  switch ((int)(addr >> 62) & 0x3)
    {
    case 0x3:	/* xkseg */
      if (addr == (bfd_signed_vma)(int)addr)
	/* 32-bit compatibility segment */
	break;
      /* xkseg -mapped */
      return 1; 	
    case 0x2:	
      /* xkphys - unmapped */
      return 0;
    case 0x1:	
      /* xksseg - mapped */
      return 1;
    case 0x0:	
      /* xkuseg/kuseg - mapped */
      return 1;
    }
#endif

  /* 32-bit address space */
  switch (((int)addr >> 30) & 0x3) {
  case 0x3:
    /* kseg2/ksseg/kseg3 - mapped */
    return 1;
  case 0x2:
    /* kseg 0/1 - unmapped */
    return 0;
  case 0x1:
  case 0x0:
    /* kuseg - mapped */
    return 1;
  }

  abort ();
}

static MDIResourceT
mdi_memresource (CORE_ADDR addr)
{
  MDIResourceT rsrc;

  switch (mdi_asidtype)
    {
    default:
    case ASIDNone:
      rsrc = MDIMIPGVIRTUAL;
      break;
    case ASIDAuto:
      if (! mapped_segment_p (addr))
	{
	  /* Use global address space for unmapped segments. */
	  rsrc = MDIMIPGVIRTUAL;
	  break;
	}
      /* fall through */
    case ASIDReg:
      /* FIXME: EntryHI */
      rsrc = MDIMIPVIRTUAL
	+ (read_register (FIRST_EMBED_REGNUM + 10) & 0xff);
      break;
    case ASIDFixed:
      rsrc = MDIMIPVIRTUAL + mdi_asid;
      break;
    }
  return rsrc;
}


static MDIOffsetT
mdi_memoffset (CORE_ADDR addr)
{
  MDIOffsetT offs = addr;
  if (mdi_desc->regsize == 4
      && (offs >> 32 == (MDIOffsetT) 0xffffffff))
    /* FS2 doesn't like 64 sig bits of address on a 32 bit CPU. */
    offs &= (MDIOffsetT) 0xffffffff;
  return offs;
}

static CORE_ADDR
mdi_memaddr (MDIOffsetT offs)
{
  CORE_ADDR addr = offs;
  if (mdi_desc->regsize == 4)
    /* FS2 doesn't like 64 sig bits of address on a 32 bit CPU. */
    addr = ((addr & (CORE_ADDR) 0xffffffff) ^ (CORE_ADDR) 0x80000000)
	   - (CORE_ADDR) 0x80000000;
  return addr;
}


static int
mdi_read_bytes (CORE_ADDR memaddr, char *myaddr, int len)
{
  if (mdiRead (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle,
	       mdi_memresource (memaddr),
	       mdi_memoffset (memaddr),
	       myaddr, 0, len) != MDISuccess)
    return 0;
  return len;
}


static int
mdi_write_bytes (CORE_ADDR memaddr, char *myaddr, int len)
{
  if (mdiWrite (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle, 
		mdi_memresource (memaddr),
		mdi_memoffset (memaddr),
		myaddr, 0, len) != MDISuccess)
    return 0;
  mdi_mem_written = 1;
  return len;
}


static int
mdi_xfer_memory (CORE_ADDR memaddr, char *buf, int len, int write,
		 struct mem_attrib *attrib,
		 struct target_ops *target)
{
  CORE_ADDR targ_addr;
  int targ_len;
  int res;

  /* Should this be the selected frame?  */
  gdbarch_remote_translate_xfer_address (current_gdbarch, current_regcache,
					 memaddr, len,
					 &targ_addr, &targ_len);
  if (targ_len <= 0)
    return 0;

  if (write)
    res = mdi_write_bytes (targ_addr, buf, targ_len);
  else
    res = mdi_read_bytes (targ_addr, buf, targ_len);
  return res;
}

static void
mdi_files_info (struct target_ops *ops)
{
  if (!mdi_desc)
    {
      printf_filtered ("\tNot attached.\n");
      return;
    }

  if (mdi_desc->gmdata[mdi_desc->gmindex].DeviceData.VPartData[0] 
      && strcmp (mdi_desc->gmdata[mdi_desc->gmindex].DeviceData.VPartData,
		 "Unknown") != 0)
    printf_filtered ("\tAttached to %s on %s %s\n",
		     mdi_desc->gmdata[mdi_desc->gmindex].DeviceData.DName,
		     mdi_desc->gmdata[mdi_desc->gmindex].DeviceData.Vendor,
		     mdi_desc->gmdata[mdi_desc->gmindex].DeviceData.VPartData);
  else
    printf_filtered ("\tAttached to %s\n",
		     mdi_desc->gmdata[mdi_desc->gmindex].DeviceData.DName);
  if (exec_bfd)
    {
      printf_filtered ("\tand running program %s.\n",
		       bfd_get_filename (exec_bfd));
    }
}

static void
mdi_kill (void)
{
  if (remote_debug)
    fprintf_unfiltered (mdi_logfp, "mdi_kill\n");

  /* XXX Should we forcibly stop any running MDI device here. */

  /* Just note that the inferior is no longer running. */
  target_mourn_inferior ();
}

/* Load an executable file into the target process.  This is expected to
   not only bring new code into the target process, but also to update
   GDB's symbol tables to match.  */

static void
mdi_load (char *prog, int fromtty)
{
  struct cleanup *cleanups;
  char *filename, *offset = NULL;
  char *ext;
  CORE_ADDR pc;
  CORE_ADDR entry;
  int i;

  if (remote_debug)
    fprintf_unfiltered (mdi_logfp, "mdi_load: prog \"%s\"\n", prog);

#if 0
  if (mdi_asidtype != ASIDNone)
    warning ("Using mapped addresses for load\n");
#endif

  /* Flush any leftover profiling data - don't write to file. */
  mdi_profiling_close (0);

  /* Forget about any previous "syscall" breakpoints. */
  mdi_remove_syscallbreakpoint ();

  /* Writeback and invalidate data caches. */
  for (i = 0; i < mdi_desc->gmcount; i++)
    if (!mdi_desc->gmdata[i].disabled)
      {
	mdiCacheFlush (mdi_desc->gmdata[i].DevHandle, MDICacheTypeData,
		       MDICacheWriteBack | MDICacheInvalidate);
	mdiCacheFlush (mdi_desc->gmdata[i].DevHandle, MDICacheTypeUnified,
		       MDICacheWriteBack | MDICacheInvalidate);
      }

  mdi_found_relf = 0;
  mdi_entrypoint = 0;
  mdi_loaded = 0;
  inferior_ptid = null_ptid;

  /* Reinitialise target file i/o. */
  mdi_fdinit ();

  filename = xmalloc (strlen (prog) + sizeof ".relf");
  cleanups = make_cleanup (xfree, filename);

  /* Copy arguments and terminate after file name. */
  strcpy (filename, prog);
  ext = strchr (filename, ' ');
  if (ext != NULL)
    {
      char *s;
      *ext = '\0';
      for (s = ext + 1; *s == ' ' || *s == '\t'; s++)
	continue;
      if (*s)
	offset = s;
    }
  else
    ext = filename + strlen (filename);

  /* Don't do enything if an offset is present, assume the
     user knows what their doing. */
  if (!offset)
    {
      /* Append .relf extension. */
      strcpy (ext, ".relf");
      /* If .relf file doesn't exist, revert to original name. */
      if (access (filename, F_OK) != 0)
	*ext = '\0';
      else
	mdi_found_relf = 1;
    }

  /* The generic_load function sets the PC, but we don't want to do this. */
  pc = read_pc ();
  generic_load (filename, fromtty);
  mdi_entrypoint = read_pc ();
  write_pc (pc);

  do_cleanups (cleanups);

  mdi_loaded = 1;
}


struct mdi_bpt
{
  struct mdi_bpt *next;
  MDIHandleT devhandle;
  MDIBpDataT mdibp;
};

static struct mdi_bpt *mdi_bpts;

static void
mdi_reset_breakpoints (void)
{
  struct mdi_bpt *bpt, *nbpt;

  for (bpt = mdi_bpts; bpt; bpt = nbpt)
    {
      nbpt = bpt->next;
      xfree (bpt);
    }
  mdi_bpts = NULL;
}


static MDIBpDataT *
mdi_find_breakpoint (MDIHandleT devhandle, MDIBpIdT id)
{
  struct mdi_bpt *bpt;

  for (bpt = mdi_bpts; bpt; bpt = bpt->next)
    if (bpt->devhandle == devhandle && bpt->mdibp.Id == id)
      return &bpt->mdibp;
  return NULL;
}


static MDIBpDataT *
mdi_find_usr_breakpoint (CORE_ADDR addr)
{
  struct mdi_bpt *bpt;

  for (bpt = mdi_bpts; bpt; bpt = bpt->next)
    if (bpt->mdibp.Type == MDIBPT_SWInstruction
	&& bpt->mdibp.Enabled == 1
	&& bpt->mdibp.Resource == mdi_memresource (addr)
	&& bpt->mdibp.Range.Start == mdi_memoffset (addr))
      return &bpt->mdibp;
  return NULL;
}


static int
mdi_insert_breakpoint1 (CORE_ADDR addr, MDIInt32 rsrc)
{
  struct mdi_bpt *bpt, *bpdata, **next;
  MDIInt32 mdistat;

  if (remote_debug)
    fprintf_unfiltered (mdi_logfp, "mdi_insert_breakpoint1: 0x%x\n",
			(unsigned int) addr);

  bpt = xcalloc (1, sizeof (struct mdi_bpt));
  /* Set the breakpoint for the first device only; others will get
     MDIStatusUsrBPHit.  */
  bpt->devhandle = mdi_desc->gmdata[0].DevHandle;
  bpt->mdibp.Id = 0;
  bpt->mdibp.Type = MDIBPT_SWInstruction;
  bpt->mdibp.Enabled = 1;
  bpt->mdibp.Resource = rsrc;
  bpt->mdibp.Range.Start = mdi_memoffset (addr);
  bpt->mdibp.PassCount = 1;

  next = &mdi_bpts;
  while (*next)
    {
      bpdata = *next;

      if (bpdata->devhandle == bpt->devhandle
	  && bpdata->mdibp.Type == bpt->mdibp.Type
	  && bpdata->mdibp.Resource == bpt->mdibp.Resource
	  && bpdata->mdibp.Range.Start == bpt->mdibp.Range.Start
	  && bpdata->mdibp.Enabled == 1)
	    bpt->mdibp.Enabled = ~0;

      next = &(*next)->next;
    }

  if (bpt->mdibp.Enabled != ~0)
    {
      mdistat = mdiSetBp (bpt->devhandle, &bpt->mdibp);
      if (mdistat != MDISuccess)
	{
	  xfree (bpt);
	  MDIWARN (mdistat, "setting breakpoint");
	  return EIO;
	}
    }

  bpt->next = mdi_bpts;
  mdi_bpts = bpt;

  return 0;
}

static int
mdi_insert_breakpoint (CORE_ADDR addr, char *save)
{
  return mdi_insert_breakpoint1 (addr, mdi_memresource (addr));
}

static void
mdi_insert_syscallbreakpoint (void)
{
  struct minimal_symbol *ms;

  mdi_syscall = mdi_syscall_rom = 0;
  if ((ms = lookup_minimal_symbol ("_mdi_syscall", NULL, NULL)))
    {
      mdi_syscall = SYMBOL_VALUE_ADDRESS (ms);
      
      /* If symbol is in RAM then add an additional breakpoint
	 at pre-relocation ROM address. */
      if (mdi_found_relf 
	  && (ms = lookup_minimal_symbol ("___reset", NULL, NULL)))
	mdi_syscall_rom = mdi_syscall
	  - SYMBOL_VALUE_ADDRESS (ms)
	  + mdi_entrypoint;
    }
  
  if (mdi_syscall)
    mdi_insert_breakpoint1 (mdi_syscall, MDIMIPGVIRTUAL);
  if (mdi_syscall_rom && mdi_syscall_rom != mdi_syscall)
    mdi_insert_breakpoint1 (mdi_syscall_rom, MDIMIPGVIRTUAL);
  mdi_syscall_inserted = 1;
}

static int
mdi_remove_bpt (MDIInt32 type, CORE_ADDR addr, MDIInt32 rsrc)
{
  MDIHandleT devhandle = mdi_desc->gmdata[0].DevHandle;
  MDIOffsetT off = mdi_memoffset (addr);
  struct mdi_bpt *bpt, *pbpt;
  MDIInt32 mdistat;

  /* Find matching breakpoint. */
  for (bpt = mdi_bpts, pbpt = NULL; bpt; pbpt = bpt, bpt = bpt->next)
    {
      if (bpt->devhandle == devhandle
	  && (bpt->mdibp.Type & MDIBPT_TypeMask) == type
	  && bpt->mdibp.Range.Start == off
	  && bpt->mdibp.Resource == rsrc)
	break;
    }

  if (!bpt)
    /* Not found. */
    return 0;

  if (bpt->mdibp.Enabled != ~0)
    {
      mdistat = mdiClearBp (bpt->devhandle, bpt->mdibp.Id);
      if (mdistat != MDISuccess)
	{
	  MDIWARN (mdistat, "clearing breakpoint");
	  return EIO;
	}
    }

  /* Unlink and free the breakpoint. */
  if (pbpt)
    pbpt->next = bpt->next;
  else
    mdi_bpts = bpt->next;
  xfree (bpt);

  return 0;
}

static int
mdi_remove_breakpoint1 (CORE_ADDR addr, MDIInt32 rsrc)
{
  return mdi_remove_bpt (MDIBPT_SWInstruction, addr, rsrc);
}

static int
mdi_remove_breakpoint (CORE_ADDR addr, char *save)
{
  return mdi_remove_breakpoint1 (addr, mdi_memresource (addr));
}

static void
mdi_remove_syscallbreakpoint (void)
{
  if (mdi_syscall)
    mdi_remove_breakpoint1 (mdi_syscall, MDIMIPGVIRTUAL);
  if (mdi_syscall_rom && mdi_syscall_rom != mdi_syscall)
    mdi_remove_breakpoint1 (mdi_syscall_rom, MDIMIPGVIRTUAL);
  mdi_syscall_rom = mdi_syscall = 0;
  mdi_syscall_inserted = 0;
}

static void
mdi_mourn_inferior (void)
{
  if (remote_debug)
    fprintf_unfiltered (mdi_logfp, "mdi_mourn_inferior:\n");

  remove_breakpoints ();
  generic_mourn_inferior ();
}

static void
mdi_switch_to_thread (void)
{
  MDIInt32 mdistat;
  MDIHandleT devhandle;
  MDITCIdT TCId;
  int i, j;

  j = mdi_desc->gmindex;
  devhandle = ptid_get_lwp (inferior_ptid);
  for (i = 0; i < mdi_desc->gmcount; i++)
    if (!mdi_desc->gmdata[i].disabled
	&& mdi_desc->gmdata[i].DevHandle == devhandle)
      {
	j = i;
	break;
      }

  TCId = ptid_get_tid (inferior_ptid);
  mdistat = mdiSetTC (mdi_desc->gmdata[j].DevHandle, TCId);
  if (mdistat == MDISuccess)
    mdi_desc->gmindex = j;
  else
    {
      warning ("Thread %d (%s) unavailable.",
	       pid_to_thread_id (inferior_ptid),
	       target_tid_to_str (inferior_ptid));

      /* Attempt to recover.  */
      mdistat = mdiGetTC (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle,
			  &TCId);
      if (mdistat != MDISuccess)
	/* Too bad...  */
	error ("Current thread went away!?");

      inferior_ptid =
	ptid_build (GET_PID (inferior_ptid),
		    mdi_desc->gmdata[mdi_desc->gmindex].DevHandle, TCId);
    }
}

static int
mdi_thread_alive (ptid_t ptid)
{
  MDIInt32 howmany;
  MDITCDataT *tcdata;
  MDIInt32 mdistat;
  int i, j;

  for (i = 0; i < mdi_desc->gmcount; i++)
    {
      if (mdi_desc->gmdata[i].disabled)
	continue;

      howmany = 0;

      mdistat = mdiTCQuery (mdi_desc->gmdata[i].DevHandle, &howmany, NULL);
      if (mdistat == MDIErrUnsupported && mdi_desc->gmcount > 1)
	{
	  if (ptid_get_lwp (ptid) == mdi_desc->gmdata[i].DevHandle
	      && ptid_get_tid (ptid) == 0)
	    return 1;
	}
      else if (mdistat != MDISuccess)
	return 0;

      if (howmany > 0)
	{
	  tcdata = alloca (howmany * sizeof (*tcdata));
	  mdistat = mdiTCQuery (mdi_desc->gmdata[i].DevHandle,
				&howmany, tcdata);
	  if (mdistat != MDISuccess)
	    return 0;				/* Shouldn't happen.  */
	  for (j = 0; j < howmany; j++)
	    {
	      int active;

	      active = mdi_threadviewinactive
		       || ((tcdata[j].Status != MDITCStatusHalted)
			   && (tcdata[j].Status != MDITCStatusFree));
	      if (active
		  && ptid_get_lwp (ptid) == mdi_desc->gmdata[i].DevHandle
		  && ptid_get_tid (ptid) == tcdata[j].TCId)
		return 1;
	    }
	}
    }
  return 0;
}

static void
mdi_find_new_threads (void)
{
  MDIInt32 howmany;
  MDITCDataT *tcdata;
  MDIInt32 mdistat;
  ptid_t ptid;
  int i, j;

  /* Don't add threads till a running program is available.  */
  if (ptid_equal (inferior_ptid, null_ptid))
    return;

  for (i = 0; i < mdi_desc->gmcount; i++)
    {
      if (mdi_desc->gmdata[i].disabled)
	continue;

      howmany = 0;

      mdistat = mdiTCQuery (mdi_desc->gmdata[i].DevHandle, &howmany, NULL);
      if (mdistat == MDIErrUnsupported && mdi_desc->gmcount > 1)
	{
	  ptid = ptid_build (GET_PID (inferior_ptid),
			     mdi_desc->gmdata[i].DevHandle, 0);
	  if (!in_thread_list (ptid))
	    {
	      add_thread (ptid);
	      printf_unfiltered ("[New %s]\n", target_pid_to_str (ptid));
	    }
	  continue;
	}
      else if (mdistat != MDISuccess)
	return;

      if (howmany > 0)
	{
	  tcdata = alloca (howmany * sizeof (*tcdata));
	  mdistat = mdiTCQuery (mdi_desc->gmdata[i].DevHandle,
				&howmany, tcdata);
	  if (mdistat != MDISuccess)
	    return;				/* Shouldn't happen.  */
	  for (j = 0; j < howmany; j++)
	    {
	      int active;

	      active = mdi_threadviewinactive
		       || ((tcdata[j].Status != MDITCStatusHalted)
			   && (tcdata[j].Status != MDITCStatusFree));
	      ptid = ptid_build (GET_PID (inferior_ptid),
				 mdi_desc->gmdata[i].DevHandle,
				 tcdata[j].TCId);
	      if (active && !in_thread_list (ptid))
		{
		  add_thread (ptid);
		  printf_unfiltered ("[New %s]\n", target_pid_to_str (ptid));
		}
	      else if (!active)
		delete_thread (ptid);
	    }
	}
    }
}

static char *
mdi_pid_to_str (ptid_t ptid)
{
  MDIHandleT devhandle;
  static char *buf = NULL;
  int i;

  devhandle = ptid_get_lwp (ptid);
  for (i = 0; i < mdi_desc->gmcount; i++)
    if (devhandle == mdi_desc->gmdata[i].DevHandle)
      {
	xfree (buf);
	if (mdi_desc->gmcount > 1)
	  buf = xstrprintf ("Thread Context %d:%d:%ld",
			    mdi_desc->gmdata[i].tmdata.TGId + 1,
			    mdi_desc->gmdata[i].tmdata.DevId + 1,
			    ptid_get_tid (ptid));
	else
	  buf = xstrprintf ("Thread Context %ld", ptid_get_tid (ptid));
	break;
      }
  return buf;
}

#if notyet
int
mdi_region_ok_for_hw_watchpoint (CORE_ADDR addr, int byte_count)
{
  int n = mdi_desc->nhwbpt;

  while (n > 0)
    {
      unsigned int cap = mdi_desc->hwbpt_cap[--n];
      if (cap & MIPS_WATCHPOINT_W)
	{
	  CORE_ADDR align;
	  
	  /* Mask/range supports any length. */
	  if (cap & (MIPS_WATCHPOINT_MASK | MIPS_WATCHPOINT_RANGE))
	    return 1;
	  
	  /* Watchpoint must fit into one watch register. */
	  /* XXX Could span several watchpoints but no MIPS cpu with
	     more than one watchpoint doesn't have a mask reg. */
	  align = (cap & MIPS_WATCHPOINT_DWORD) ? 8 : 4;
	  if ((addr & (align - 1)) + byte_count <= align)
	    return 1;
	}
    }

  return 0;
}


/* Check whether the hit read watchpoint overlaps with the 
   object spanned by VADDR and LEN.  (XXX assumes kseg0/1) */

int
mdi_data_address_match (CORE_ADDR vaddr, int len)
{
  CORE_ADDR wpaddr; 
  int wplen;
  CORE_ADDR abeg, aend;
  CORE_ADDR bbeg, bend;
  int res;

  if ((mdi_curbpt.Type & MDIBPT_TypeMask) != MDIBPT_HWData)
     return 0;

  wpaddr = mdi_curbpt.Range.Start;
  if (mdi_curbpt.Type & MDIBPT_HWFlg_AddrRange)
    wplen = mdi_curbpt.Range.End - wpaddr;
  else if (mdi_curbpt.Type & MDIBPT_HWFlg_AddrMask)
    wplen = mdi_curbpt.Range.End + 1;
  else 
    wplen = 8;
  
  /* Check for physical address overlap. */
  abeg = mdi_kva2phys (wpaddr);
  aend = abeg + wplen;
  bbeg = mdi_kva2phys (vaddr);
  bend = bbeg + len;
  res = (abeg < bend && bbeg < aend);

  if (remote_debug && res)
    printf_unfiltered ("Watchpoint match: W[0x%s,%d] D[0x%s,%d]\n", 
		       paddr_nz (wpaddr), wplen, 
		       paddr_nz (vaddr), len);
  return res;
}
#endif


static int
mdi_breakpoint_type_supported (MDIUint32 bptype)
{
  MDIInt32 howmany;
  MDIBpInfoT *bpinfo;
  MDIInt32 mdistat;
  int i;

  for (i = 0; i < mdi_desc->gmcount; i++)
    {
      if (mdi_desc->gmdata[i].disabled)
	return 0;

      howmany = 0;

      mdistat = mdiHwBpQuery (mdi_desc->gmdata[i].DevHandle, &howmany, NULL);
      if (mdistat == MDIErrUnsupported)
	{
	  if (mdi_hwbpq_avail < 0)
	    {
	      warning
		("Cannot query hardware breakpoint/watchpoint resources.");
	      warning ("Available functionality may be limited.");
	      mdi_hwbpq_avail = 0;
	    }
	  return 0;
	}
      mdi_hwbpq_avail = 1;
      if (mdistat != MDISuccess)
	return -1;
    }

  for (i = 0; i < mdi_desc->gmcount; i++)
    {
      int found = 0;
      int j;

      if (howmany > 0)
	{
	  bpinfo = alloca (howmany * sizeof (*bpinfo));
	  mdistat = mdiHwBpQuery (mdi_desc->gmdata[i].DevHandle,
				  &howmany, bpinfo);
	  if (mdistat != MDISuccess)
	    return -1;				/* Shouldn't happen.  */

	  for (j = 0; j < howmany; j++)
	    if ((bpinfo[j].Type & bptype) == bptype)
	      {
		found = 1;
		break;
	      }
	}

      if (!found)
        return -1;
    }

  return 1;
}

static int
mdi_can_use_hw_breakpoint (int type, int cnt, int othertype)
{
  MDIUint32 bptype = MDIBPT_HWType_Data;

  switch (type)
    {
    case bp_hardware_breakpoint:
      bptype = MDIBPT_HWType_Exec;
      break;
    case bp_hardware_watchpoint:
      bptype = MDIBPT_HWFlg_DataWrite | MDIBPT_HWType_Data;
      break;
    case bp_read_watchpoint:
      bptype = MDIBPT_HWFlg_DataRead | MDIBPT_HWType_Data;
      break;
    case bp_access_watchpoint:
      bptype = MDIBPT_HWFlg_DataWrite | MDIBPT_HWFlg_DataRead
	       | MDIBPT_HWType_Data;
      break;
    default:
      return 0;
    }

  return mdi_breakpoint_type_supported (bptype) >= 0;
}  

/* Find the widest mask that still spans the whole area specified.  */
static MDIRangeT
mdi_watchpoint_range_to_mask (MDIRangeT addrrange)
{
  MDIRangeT addrmask;
  MDIOffsetT select = ~0;
  MDIOffsetT mask, newmask;
  int shift;

  shift = sizeof (select) * 4;
  select <<= shift;

  mask = ((addrrange.Start ^ addrrange.End) << 1) + 1;
  do
    {
      newmask = mask & select;
      if (newmask)
	mask = newmask;
      shift >>= 1;
      select ^= select >> shift;
    }
  while (shift);
  mask = -mask;

  addrmask.Start = addrrange.Start & mask;
  addrmask.End = ~mask;

  return addrmask;
}

/*  Convert a mask into a range.  */
static MDIRangeT
mdi_watchpoint_mask_to_range (MDIRangeT addrmask)
{
  MDIRangeT addrrange;

  addrrange.Start = addrmask.Start;
  addrrange.End = (addrmask.Start & addrmask.End) | ~addrmask.End;

  return addrrange;
}

static int
mdi_insert_hw_any (MDIHandleT devhandle, MDIBpDataT bp)
{
  MDIBpDataT bpcopy;
  int maskok = 0, rangeok, needrange, usedmask = 0;
  MDIRangeT bprange = bp.Range;
  MDIRangeT bpmask = mdi_watchpoint_range_to_mask (bp.Range);
  MDIRangeT bpdatarange;
  MDIBpT bptype;
  struct mdi_bpt *bpdata, **next;
  MDIInt32 mdistat;

  bptype = bp.Type & ~(MDIBpT)MDIBPT_TypeMask;
  needrange = bprange.Start != bprange.End;
  if (!needrange)
    needrange == (mdi_breakpoint_type_supported (bptype) < 0);
  if (needrange)
    {
      rangeok = mdi_breakpoint_type_supported (bptype
					       | MDIBPT_HWFlg_AddrRange);
      if (rangeok < 0)
	{
	  maskok = mdi_breakpoint_type_supported (bptype
						  | MDIBPT_HWFlg_AddrMask);
	  if (maskok < 0)
	    return -1;
	}
    }
  else
    maskok = -1;

  next = &mdi_bpts;
  while (*next)
    {
      bpdata = *next;
      bptype = bpdata->mdibp.Type & ~(MDIBpT)(MDIBPT_HWFlg_AddrMask
					      | MDIBPT_HWFlg_AddrRange);

      bpdatarange = bpdata->mdibp.Range;
      if ((bpdata->mdibp.Type & MDIBPT_HWFlg_AddrMask) != 0)
	bpdatarange = mdi_watchpoint_mask_to_range (bpdatarange);

      if (bpdata->devhandle == devhandle
	  && bptype == bp.Type
	  && bpdata->mdibp.Resource == bp.Resource
	  && bpdatarange.Start == bprange.Start
	  && bpdata->mdibp.Enabled == 1)
	{
	  if (bpdatarange.End < bprange.End)
	    {
	      mdistat =
		mdiClearBp (bpdata->devhandle, bpdata->mdibp.Id);
	      if (mdistat != MDISuccess)
		return -1;

	      if ((bpdata->mdibp.Type & MDIBPT_HWFlg_AddrMask) != 0)
		usedmask = 1;
	      bpdata->mdibp.Enabled = ~0;
	    }
	  else
	    bp.Enabled = ~0;
	}

      next = &(*next)->next;
    }

  bpdata = xmalloc (sizeof (*bpdata));
  bpdata->next = NULL;
  bpdata->devhandle = devhandle;

  bpcopy = bp;
  mdistat = MDIErrFailure;

  if (!usedmask || maskok <= 0)
    {
      if (needrange)
	bpcopy.Type |= MDIBPT_HWFlg_AddrRange;
      else
	bpcopy.Range.End = 0;
      if (bpcopy.Enabled != ~0)
	{
	  mdistat = mdiSetBp (bpdata->devhandle, &bpcopy);
	  if (mdistat != MDISuccess)
	    bpcopy = bp;
	}
      else
	mdistat = MDISuccess;
    }

  if (mdistat != MDISuccess && maskok >= 0)
    {
      bpcopy.Type |= MDIBPT_HWFlg_AddrMask;
      bpcopy.Range = bpmask;
      if (bpcopy.Enabled != ~0)
	mdistat = mdiSetBp (bpdata->devhandle, &bpcopy);
      else
	mdistat = MDISuccess;
    }

  if (mdistat != MDISuccess)
    {
      xfree (bpdata);
      return -1;
    }

  bpdata->mdibp = bpcopy;
  *next = bpdata;

  return 0;
}

static int
mdi_remove_hw_any (MDIHandleT devhandle, MDIBpDataT bp)
{
  MDIRangeT bprange = bp.Range;
  MDIRangeT bpmask = mdi_watchpoint_range_to_mask (bp.Range);
  MDIBpT bptype;
  struct mdi_bpt *bpdata, **next;
  MDIInt32 mdistat;

  next = &mdi_bpts;
  while (*next)
    {
      bpdata = *next;
      bptype = bpdata->mdibp.Type & ~(MDIBpT)(MDIBPT_HWFlg_AddrMask
					      | MDIBPT_HWFlg_AddrRange);

      if (bpdata->devhandle == devhandle
	  && bptype == bp.Type
	  && bpdata->mdibp.Resource == bp.Resource
	  && (((bpdata->mdibp.Type & MDIBPT_HWFlg_AddrMask) != 0
	       && bpdata->mdibp.Range.Start == bpmask.Start
	       && bpdata->mdibp.Range.End == bpmask.End)
	      || ((bpdata->mdibp.Type & MDIBPT_HWFlg_AddrRange) != 0
	       && bpdata->mdibp.Range.Start == bprange.Start
	       && bpdata->mdibp.Range.End == bprange.End)
	      || ((bpdata->mdibp.Type
		   & (MDIBPT_HWFlg_AddrMask | MDIBPT_HWFlg_AddrRange)) == 0
	       && bpdata->mdibp.Range.Start == bprange.Start)))
	{
	  if (bpdata->mdibp.Enabled != ~0)
	    {
	      mdistat =
		mdiClearBp (bpdata->devhandle, bpdata->mdibp.Id);
	      if (mdistat != MDISuccess)
		return -1;
	    }

	  *next = bpdata->next;
	  xfree (bpdata);

	  return 0;
	}
      next = &(*next)->next;
    }

  return -1;
}


static MDIBpDataT
mdi_setup_hw_breakpoint (CORE_ADDR addr)
{
  MDIBpDataT bp;

  memset (&bp, 0, sizeof (bp));
  bp.Type = MDIBPT_HWInstruction;
  bp.Enabled = 1;
  bp.Resource = mdi_memresource (addr);
  bp.Range.Start = mdi_memoffset (addr);
  bp.Range.End = mdi_memoffset (addr);
  bp.PassCount = 1;

  return bp;
}

static int
mdi_insert_hw_breakpoint (CORE_ADDR addr, char *shadow)
{
  MDIBpDataT bp;
  int i;

  bp = mdi_setup_hw_breakpoint (addr);

  for (i = 0; i < mdi_desc->gmcount; i++)
    if (!mdi_desc->gmdata[i].disabled)
      if (mdi_insert_hw_any (mdi_desc->gmdata[i].DevHandle, bp) < 0)
	{
	  for (i--; i >= 0; i--)
	    if (!mdi_desc->gmdata[i].disabled)
	      mdi_remove_hw_any (mdi_desc->gmdata[i].DevHandle, bp);
	  return -1;
	}

  return 0;
}

static int
mdi_remove_hw_breakpoint (CORE_ADDR addr, char *shadow)
{
  MDIBpDataT bp;
  int i, result = 0;

  bp = mdi_setup_hw_breakpoint (addr);

  for (i = 0; i < mdi_desc->gmcount; i++)
    if (!mdi_desc->gmdata[i].disabled)
      if (mdi_remove_hw_any (mdi_desc->gmdata[i].DevHandle, bp) < 0)
	result = -1;

  return result;
}


static MDIBpDataT
mdi_setup_watchpoint (CORE_ADDR addr, int len, int type)
{
  MDIBpDataT bp;

  memset (&bp, 0, sizeof (bp));
  bp.Type = MDIBPT_HWData;
  bp.Enabled = 1;
  bp.Resource = mdi_memresource (addr);
  bp.PassCount = 1;

  switch (type)
    {
    case 0:
      bp.Type |= MDIBPT_HWFlg_DataWrite;
      break;
    case 1:
      bp.Type |= MDIBPT_HWFlg_DataRead;
      break;
    case 2:
    default:
      bp.Type |= MDIBPT_HWFlg_DataWrite | MDIBPT_HWFlg_DataRead;
      break;
    }

  bp.Range.Start = mdi_memoffset (addr);
  bp.Range.End = mdi_memoffset (addr) + len - 1;

  return bp;
}

static int
mdi_insert_watchpoint (CORE_ADDR addr, int len, int type)
{
  MDIBpDataT bp;
  int i;

  bp = mdi_setup_watchpoint (addr, len, type);

  for (i = 0; i < mdi_desc->gmcount; i++)
    if (!mdi_desc->gmdata[i].disabled)
      if (mdi_insert_hw_any (mdi_desc->gmdata[i].DevHandle, bp) < 0)
	{
	  for (i--; i >= 0; i--)
	    if (!mdi_desc->gmdata[i].disabled)
	      mdi_remove_hw_any (mdi_desc->gmdata[i].DevHandle, bp);
	  return -1;
	}

  return 0;
}

static int
mdi_remove_watchpoint (CORE_ADDR addr, int len, int type)
{
  MDIBpDataT bp;
  int i, result = 0;

  bp = mdi_setup_watchpoint (addr, len, type);

  for (i = 0; i < mdi_desc->gmcount; i++)
    if (!mdi_desc->gmdata[i].disabled)
      if (mdi_remove_hw_any (mdi_desc->gmdata[i].DevHandle, bp) < 0)
	result = -1;

  return result;
}


static int
mdi_stopped_by_watchpoint (void)
{
  return ((mdi_curbpt.Type & MDIBPT_TypeMask) == MDIBPT_HWData);
}

static CORE_ADDR
mdi_stopped_data_address (void)
{
	return mdi_memaddr (mdi_curbpt.Range.Start);
}


#define D(v,sbit,nbits) (((v)>>(sbit)) & ((1 << (nbits))-1))
#define DD(v,sbit,nbits) ((MDIUint32)(((v)>>(sbit)) & ((1LL << (nbits))-1)))

static MDIInt32 
show_tlb_command1 (int entry)
{
  MDIInt32 mdistat;

  check_mdi ();

  switch (mdi_desc->tlb)
    {
    case R3000_TLB:
      {
	MDIUint32 data[2];
	unsigned long lo, hi;
	if (entry < 0)
	  printf_filtered ("TLB  VPN        ASID  PFN        N D V G\n");
	else
	  {
	    mdistat = mdiRead (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle,
			       MDIMIPTLB, entry * 2, data, 4, 2);
	    if (mdistat != MDISuccess)
	      return mdistat;
	    lo = extract_unsigned_integer (&data[0], 4);
	    hi = extract_unsigned_integer (&data[1], 4);
	    if (remote_debug)
	      fprintf_unfiltered (mdi_logfp,
				  "%2d: hi=0x%08lx lo=0x%08lx\n",
				  entry, hi, lo);
	    printf_filtered ("%2d   0x%08lx %3ld   0x%08lx %ld %ld %ld %ld\n",
			     entry, D (hi, 6, 20), /* VPN */
			     D (hi, 0, 6),	/* ASID */
			     D (lo, 12, 20),	/* PFN */
			     D (lo, 11, 1),	/* N */
			     D (lo, 10, 1),	/* D */
			     D (lo, 9, 1),	/* V */
			     D (lo, 8, 1)	/* G */
	      );
	  }
      }
      break;

    case R4000_32TLB:
      {
	MDIUint32 data[4];
	unsigned long lo0, lo1, hi, mask;
	if (entry < 0)
	  printf_filtered
	    ("TLB  Mask    VPN2       ASID  R X PFN0       C D V G  R X PFN1       C D V G\n");
	else
	  {
	    mdistat = mdiRead (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle,
			       MDIMIPTLB, entry * 4, data, 4, 4);
	    if (mdistat != MDISuccess)
	      return mdistat;
	    lo0 = extract_unsigned_integer (&data[0], 4);
	    lo1 = extract_unsigned_integer (&data[1], 4);
	    hi = extract_unsigned_integer (&data[2], 4);
	    mask = extract_unsigned_integer (&data[3], 4);
	    if (remote_debug)
	      fprintf_unfiltered 
		(mdi_logfp,
		 "%2d: hi=0x%08lx lo0=0x%08lx lo1=0x%08lx mask=0x%08lx\n",
		 entry, hi, lo0, lo1, mask);
	    printf_filtered ("%2d   0x%05lx 0x%08lx %3ld   %ld %ld 0x%08lx %ld %ld %ld %ld  %ld %ld 0x%08lx %ld %ld %ld %ld\n",
			     entry, D (mask, 11, 18),	/* MASK */
#if 1
			     D (hi, 11, 21) << 11,	/* VPN2 (sigh smartmips) */
#else
			     D (hi, 13, 19),	/* VPN2 */
#endif
			     D (hi, 0, 8),	/* ASID */
			     D (lo0, 31, 1),	/* RI */
			     D (lo0, 30, 1),	/* XI */
			     D (lo0, 6, 24),	/* PFN */
			     D (lo0, 3, 3),	/* C */
			     D (lo0, 2, 1),	/* D */
			     D (lo0, 1, 1),	/* V */
			     D (lo0, 0, 1),	/* G */
			     D (lo1, 31, 1),	/* RI */
			     D (lo1, 30, 1),	/* XI */
			     D (lo1, 6, 24),	/* PFN */
			     D (lo1, 3, 3),	/* C */
			     D (lo1, 2, 1),	/* D */
			     D (lo1, 1, 1),	/* V */
			     D (lo1, 0, 1)	/* G */
	      );
	  }
      }
      break;
    case R4000_64TLB:
      {
	MDIUint64 data[4];
	unsigned long lo0, lo1, hi, mask;
	if (entry < 0)
	  printf_filtered
	    ("TLB  Mask    VPN2       ASID PFN0       C D V G PFN1       C D V G\n");
	else
	  {
	    mdistat = mdiRead (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle,
			       MDIMIPTLB, entry * 4, data, 8, 4);
	    if (mdistat != MDISuccess)
	      return mdistat;
	    lo0 = extract_unsigned_integer (&data[0], 8);
	    lo1 = extract_unsigned_integer (&data[1], 8);
	    hi = extract_unsigned_integer (&data[2], 8);
	    mask = extract_unsigned_integer (&data[3], 8);
	    if (remote_debug)
	      fprintf_unfiltered
		(mdi_logfp,
		 "%2d: hi=0x%08lx lo0=0x%08lx lo1=0x%08lx mask=0x%08lx\n", 
		 entry, hi, lo0, lo1, mask);
	    printf_filtered ("%2d   0x%05x 0x%08x %3d  %d %d 0x%08x %d %d %d %d 0x%08x %d %d %d %d\n",
			     entry, DD (mask, 11, 18),	/* MASK */
			     DD (hi, 13, 27),	/* VPN2 */
			     DD (hi, 0, 8),	/* ASID */
			     DD (lo0, 6, 24),	/* PFN */
			     DD (lo0, 3, 3),	/* C */
			     DD (lo0, 2, 1),	/* D */
			     DD (lo0, 1, 1),	/* V */
			     DD (lo0, 0, 1),	/* G */
			     DD (lo1, 31, 1),	/* RI */
			     DD (lo1, 30, 1),	/* XI */
			     DD (lo1, 6, 24),	/* PFN */
			     DD (lo1, 3, 3),	/* C */
			     DD (lo1, 2, 1),	/* D */
			     DD (lo1, 1, 1),	/* V */
			     DD (lo1, 0, 1)	/* G */
	      );
	  }
      }
      break;
    }
  return MDISuccess;
}

static void
show_tlb_command (char *args, int from_tty)
{
  MDIInt32 mdistat = MDISuccess;
  int i, n;

  check_mdi ();

  if (mdi_desc->gmdata[mdi_desc->gmindex].ntlb <= 0)
    error ("No TLB on this CPU");
  if (args == NULL || *args == '\0' || (strcmp (args, "all") == 0))
    {
      show_tlb_command1 (-1);
      for (i = 0; i < mdi_desc->gmdata[mdi_desc->gmindex].ntlb; i++)
	{
	  mdistat = show_tlb_command1 (i);
	  if (mdistat != MDISuccess)
	    break;
	}
    }
  else
    {
      n = strtoul (args, &args, 0);
      if (*args || n < 0 || n >= mdi_desc->gmdata[mdi_desc->gmindex].ntlb)
	{
	  error ("Usage: show mdi tlb [all | INDEX]");
	}
      show_tlb_command1 (-1);
      mdistat = show_tlb_command1 (n);
    }
  MDIERR (mdistat, "reading TLB");
}


static void
set_tlb_command (char *args, int from_tty)
{
  ULONGEST data[5];
  MDIInt32 mdistat;
  int i = 0;
  char *s = args;
  MDIOffsetT entry = -1;

  check_mdi ();

  if (mdi_desc->gmdata[mdi_desc->gmindex].ntlb <= 0)
    error ("No TLB on this CPU");

  if (s)
    {
      for ( ; i < 5 && *s != '\0'; i++)
	{
	  char *tail;
	  data[i] = strtoul (s, &tail, 0);
	  if (s == tail)
	    break;
	  s = tail;
	}
      while (*s == ' ' || *s == '\t')
	s++;
      if (*s != '\0')
	i = 999;
    }

  switch (mdi_desc->tlb)
    {
    case R3000_TLB:
      {
	MDIUint32 tlb[3];
	if (i != 3)
	  error ("Usage: set mdi tlb INDEX HI LO");
	entry = data[0];
	store_unsigned_integer (&tlb[0], sizeof (tlb[0]), data[1]);
	store_unsigned_integer (&tlb[1], sizeof (tlb[1]), data[2]);
	mdistat = mdiWrite (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle,
			    MDIMIPTLB, entry,
			    tlb, sizeof (tlb[0]), 2);
	MDIERR (mdistat, "writing TLB entry");
      }
      break;
    case R4000_32TLB:
      {
	MDIUint32 tlb[4];
	if (i != 5)
	  error ("Usage: set mdi tlb INDEX HI LO0 LO1 MASK");
	entry = data[0];
	store_unsigned_integer (&tlb[0], sizeof (tlb[0]), data[2]);
	store_unsigned_integer (&tlb[1], sizeof (tlb[1]), data[3]);
	store_unsigned_integer (&tlb[2], sizeof (tlb[2]), data[1]);
	store_unsigned_integer (&tlb[3], sizeof (tlb[3]), data[4]);
	mdistat = mdiWrite (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle,
			    MDIMIPTLB, entry * 4, tlb, sizeof (tlb[0]), 4);
	MDIERR (mdistat, "writing TLB entry");
      }
      break;
    case R4000_64TLB:
      {
	MDIUint64 tlb[4];
	if (i != 5)
	  error ("Usage: set mdi tlb INDEX HI LO0 LO1 MASK");
	entry = data[0];
	store_unsigned_integer (&tlb[0], sizeof (tlb[0]), data[2]);
	store_unsigned_integer (&tlb[1], sizeof (tlb[1]), data[3]);
	store_unsigned_integer (&tlb[2], sizeof (tlb[2]), data[1]);
	store_unsigned_integer (&tlb[3], sizeof (tlb[3]), data[4]);
	mdistat = mdiWrite (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle,
			    MDIMIPTLB, entry * 4, tlb, sizeof (tlb[0]), 4);
	MDIERR (mdistat, "writing TLB entry");
      }
      break;
    default:
      error ("Unknown TLB type");
    }
  show_tlb_command1 (-1);
  show_tlb_command1 (entry);
}

static void
show_cache1 (int which, char *args)
{
  MDIInt32 mdistat;
  MDICacheInfoT *cache = NULL;
  MDIUint32 info[16];		/* FIXME: caches with bigger line sizes ? */
  MDIOffsetT addr;
  MDIUint32 tag, parity;
  MDIResourceT tagrsrc = 0, datarsrc = 0;
  MDIOffsetT SrcOffset;
  MDIOffsetT offset;
  const char * name;
  char *tail;
  int set, nsets;
  int j;

  check_mdi ();

  switch (which)
    {
    case 0:
      name = "Icache";
      tagrsrc = MDIMIPPICACHET;
      datarsrc = MDIMIPPICACHE;
      break;
    case 1:
      name = "Dcache";
      tagrsrc = MDIMIPPDCACHET;
      datarsrc = MDIMIPPDCACHE;
      break;
    case 2:
      name = "Scache";
      tagrsrc = MDIMIPSUCACHET;
      datarsrc = MDIMIPSUCACHE;
      break;
    default:
      error ("Invalid cache");
    }

  cache = &mdi_desc->gmdata[mdi_desc->gmindex].CacheInfo[which];
  if (cache->Type == MDICacheTypeNone)
    error ("No information about %s", name);

  if (cache->LineSize / 4 >= sizeof (info) / sizeof (info[0]))
    internal_error (__FILE__, __LINE__, "cache line buffer size");

  if (args == NULL || ! *args)
    {
      printf_filtered ("%s: %d bytes (%d sets, %d lines/set, %d bytes/line)\n", 
		       name,
		       cache->LineSize * cache->LinesPerSet * cache->Sets,
		       cache->Sets, cache->LinesPerSet, cache->LineSize);
      return;
    }

  addr = value_as_address (parse_to_comma_and_eval (&args));

  if (*args)
    {
      if (*args == ',')
	args++;
      set = value_as_long (parse_to_comma_and_eval (&args));
      if (*args)
	error ("Usage: show mdi %s [OFFS[,SET]]", name);
      if (set >= cache->Sets)
	error ("Only %d sets in the %s.", cache->Sets, name);
      nsets = 1;
    }
  else
    {
      set = 0;
      nsets = cache->Sets;
    }

  /* Round address down to line boundary. */
  addr &= ~(cache->LineSize  - 1);			

  /* Reduce to offset in set (lose high order address bits). */
  addr &= (cache->LinesPerSet * cache->LineSize) - 1;

  while (nsets-- != 0)
    {
      /* Get tags and parity. */
      offset = (addr / cache->LineSize);	/* cache line number */
      SrcOffset = (offset + (set * cache->LinesPerSet)) * 2;
      mdistat = mdiRead (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle,
			 tagrsrc, SrcOffset, info, 4, 2);
      MDIERR (mdistat, "reading cache tags");

      tag = extract_unsigned_integer (&info[0], 4);
      parity = extract_unsigned_integer (&info[1], 4);
      set_internalvar (lookup_internalvar ("ctag"),
		       value_from_longest (builtin_type_int, (LONGEST) tag));
      set_internalvar (lookup_internalvar ("cparity"),
		       value_from_longest (builtin_type_int, (LONGEST) parity));

      printf_filtered ("%06x/%d  Tag=%08x  Parity=%08x    ",
		       (unsigned int) addr, set, tag, parity);

      /* Get data. */
      SrcOffset = addr + (set * cache->LinesPerSet * cache->LineSize);
      mdistat = mdiRead (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle,
			 datarsrc, SrcOffset, info, 1, cache->LineSize);
      MDIERR (mdistat, "reading cache data");

      for (j = 0; j < (cache->LineSize / 4); j++)
	{
	  ULONGEST v;
	  char cdata[8];
	  v = extract_unsigned_integer (&info[j], 4);
	  if ((j & 3) == 0)
	    printf_filtered ("\n");
	  printf_filtered (" %08x", (unsigned int) v);
	  sprintf (cdata, "cdata%d", j);
	  set_internalvar (lookup_internalvar (cdata),
			   value_from_longest (builtin_type_int, (LONGEST) v));
	}
      printf_filtered ("\n");
      
      set++;
    }
}


static void
show_cp0_command (char *args, int from_tty)
{
  const struct mips_regnum *regnum = mips_regnum (current_gdbarch);
  unsigned long regno, reg, bank;
  char *s;

  check_mdi ();

  if (args == NULL)
    {
    usage:
      error ("Usage: show mdi cp0 REG[/BANK]");
    }
  
  reg = strtoul (args, &s, 0);
  if (s == args)
    goto usage;

  if (*s == '/')
    {
      bank = strtoul (args = s + 1, &s, 0);
      if (s == args)
	goto usage;
    }
  else
    bank = 0;

  if (*s)
    goto usage;
  if (bank >= 8)
    goto usage;
  if (reg >= 32)
    goto usage;

  /* Do some judicious mapping for "well-known" CP0 registers. */
  regno = ((bank << 5) | reg);

  switch (regno)
    {
    case 8:			/* BadVaddr */
      regno = regnum->badvaddr;
      break;
    case 12:
      regno = PS_REGNUM;
      break;
    case 13:
      regno = regnum->cause;
      break;
    default:
      regno += FIRST_EMBED_REGNUM;
      break;
    }
  printf_filtered ("CP0 %lu/%lu ", reg, bank);
  gdbarch_print_registers_info (current_gdbarch, gdb_stdout, 
				deprecated_selected_frame, NUM_REGS + regno, 0);
}

static void
set_cp0_command (char *args, int from_tty)
{
  const struct mips_regnum *regnum = mips_regnum (current_gdbarch);
  unsigned long regno, reg, bank;
  MDIInt32 mdistat;
  ULONGEST val;
  char *s;

  check_mdi ();

  if (args == NULL)
    {
    usage:
      error ("Usage: set mdi cp0 REG[/BANK] VAL");
    }

  reg = strtoul (args, &s, 0);
  if (s == args)
    goto usage;

  if (*s == '/')
    {
      bank = strtoul (args = s + 1, &s, 0);
      if (s == args)
	goto usage;
    }
  else
    bank = 0;

  val = parse_and_eval_address_1 (&s);

  /* Set commands can have trailing space. */
  for (; *s != '\0'; s++)
    if (! isspace (*s))
      goto usage;

  if (bank >= 8)
    goto usage;
  if (reg >= 32)
    goto usage;

  /* Do some judicious mapping for "well-known" CP0 registers. */
  regno = ((bank << 5) | reg);
  switch (regno)
    {
    case 8:			/* BadVaddr */
      regno = regnum->badvaddr;
      break;
    case 12:
      regno = PS_REGNUM;
      break;
    case 13:
      regno = regnum->cause;
      break;
    default:
      regno += FIRST_EMBED_REGNUM;
      break;
    }
  write_register (regno, val);
}

static void
set_cache1 (int which, char *args)
{
  MDIInt32 mdistat;
  MDICacheInfoT *cache = NULL;
  MDIUint32 tagbuf[2];
  MDIUint32 info[16];
  MDIOffsetT addr;
  MDIUint32 tag, parity;
  MDIResourceT tagrsrc, datarsrc;
  MDIOffsetT DstOffset;
  MDIOffsetT offset;
  const char *name;
  char *tail;
  char *s;
  unsigned long set;
  int i;

  check_mdi ();

  switch (which)
    {
    case 0:
      name = "icache";
      tagrsrc = MDIMIPPICACHET;
      datarsrc = MDIMIPPICACHE;
      break;
    case 1:
      name = "dcache";
      tagrsrc = MDIMIPPDCACHET;
      datarsrc = MDIMIPPDCACHE;
      break;
    case 2:
      name = "scache";
      tagrsrc = MDIMIPSUCACHET;
      datarsrc = MDIMIPSUCACHE;
      break;
    default:
      error ("Invalid cache");
    }

  cache = &mdi_desc->gmdata[mdi_desc->gmindex].CacheInfo[which];
  if (cache->Type == MDICacheTypeNone)
    error ("No information about %s", name);

  if (cache->LineSize / 4 >= sizeof (info) / sizeof (info[0]))
    internal_error (__FILE__, __LINE__, "cache line buffer size");

  if (!args || ! *args)
    {
    usage:
      error ("Usage: set mdi %s ADDRESS,SET,TAG,PARITY,DATA,...", name);
    }

  addr = value_as_address (parse_to_comma_and_eval (&args));

  if (*args == ',') 
    args++;
  set = value_as_long (parse_to_comma_and_eval (&args));

  if (*args == ',') 
    args++;
  tag = value_as_long (parse_to_comma_and_eval (&args));

  if (*args == ',') 
    args++;
  parity = value_as_long (parse_to_comma_and_eval (&args));

  if (set >= cache->Sets)
    error ("Only %d sets in the %s.", cache->Sets, name);

  /* Round address down to line boundary. */
  addr &= ~(cache->LineSize  - 1);			

  /* Reduce to cache offset (lose high order address bits). */
  addr &= (cache->LinesPerSet * cache->LineSize) - 1;

  /* Parse data. */
  for (i = 0; i < cache->LineSize / 4; i++)
    {
      ULONGEST data;
      if (*args == ',') 
	args++;
      data = value_as_long (parse_to_comma_and_eval (&args));
      store_unsigned_integer (&info[i], 4, data);
    }

  /* Set commands can have trailing space. */
  for (; *args; args++)
    if (! isspace (*args))
      goto usage;

  /* Write Tags and Parity. */
  offset = (addr / cache->LineSize);	/* cache line number */
  DstOffset = (offset + (set * cache->LinesPerSet)) * 2;
  store_unsigned_integer (&tagbuf[0], 4, (ULONGEST) tag);
  store_unsigned_integer (&tagbuf[1], 4, (ULONGEST) parity);
  mdistat = mdiWrite (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle,
		      tagrsrc, DstOffset, tagbuf, 4, 2);
  MDIERR (mdistat, "writing cache tags");

  /* Write Data. */
  DstOffset = addr + (set * cache->LinesPerSet * cache->LineSize);
  mdistat = mdiWrite (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle,
		      datarsrc, DstOffset, info, 4, cache->LineSize / 4);
  MDIERR (mdistat, "writing cache data");
}

static void
show_scache_command (char *args, int from_tty)
{
  show_cache1 (2, args);
}

static void
set_scache_command (char *args, int from_tty)
{
  set_cache1 (2, args);
}

static void
show_dcache_command (char *args, int from_tty)
{
  show_cache1 (1, args);
}

static void
set_dcache_command (char *args, int from_tty)
{
  set_cache1 (1, args);
}

static void
show_icache_command (char *args, int from_tty)
{
  show_cache1 (0, args);
}

static void
set_icache_command (char *args, int from_tty)
{
  set_cache1 (0, args);
}

static void
mdi_rcmd (char *command, struct ui_file *outbuf)
{
  MDIInt32 mdistat;

  if (command == NULL)
    return;

  check_mdi ();

  mdistat = mdiDoCommand (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle,
			  command);
  if (mdistat == MDIErrUnsupported)
    error ("MDI library does not recognise this command");
  else
    MDIERR (mdistat, "DoCommand");
}


static void
mdi_log_command (const char *cmd)
{
  if (mdi_logging)
    {
      fprintf_unfiltered (mdi_logfp, "(gdb) %s\n", cmd);

      /* Make sure that the log file is as up-to-date as possible,
         in case we are getting ready to dump core or something. */
      gdb_flush (mdi_logfp);
    }

  if (mdi_logdocommand && *mdi_logdocommand)
    {
      char *logcmd, *p;
      int loglen;

      loglen = strlen (mdi_logdocommand);
      p = strstr (mdi_logdocommand, "%s");
      if (p)
	loglen += strlen (cmd) - (sizeof ("%s") - 1);
      logcmd = alloca (loglen + 1);

      if (p)
	{
	  memcpy (logcmd, mdi_logdocommand, p - mdi_logdocommand);
	  memcpy (logcmd + (p - mdi_logdocommand), cmd, strlen (cmd));
	  memcpy (logcmd + (p - mdi_logdocommand) + strlen (cmd),
		  p + (sizeof ("%s") - 1),
		  strlen (mdi_logdocommand) - (p - mdi_logdocommand) + 1);
	}
      else
	strcpy (logcmd, mdi_logdocommand);

      mdiDoCommand (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle, logcmd);
    }
}

static void
set_asid_command (char *args, int from_tty)
{
  unsigned long n;

  if (args == NULL || *args == '\0')
    error ("Usage: set mdi asid on|off|ASID");
  if (strcasecmp (args, "off") == 0)
    mdi_asidtype = ASIDNone;
  else if (strcasecmp (args, "on") == 0)
    mdi_asidtype = ASIDReg;
  else if (strcasecmp (args, "auto") == 0)
    mdi_asidtype = ASIDAuto;
  else
    {
      char *tail;
      n = strtoul (args, &tail, 0);
      for (; *tail && isspace (*tail); tail++)
	continue;
      if (*tail || n > 256)
	error ("ASID out of range: %s", args);
      mdi_asidtype = ASIDFixed;
      mdi_asid = n;
    }
}

static void
show_asid_command (char *args, int from_tty)
{
  switch (mdi_asidtype)
    {
    case ASIDNone:
      printf_filtered ("off\n");
      break;
    case ASIDReg:
      printf_filtered ("on\n");
      break;
    case ASIDAuto:
      printf_filtered ("auto\n");
      break;
    case ASIDFixed:
      printf_filtered ("%d\n", mdi_asid);
      break;
    }
}

static void
set_connectreset_command (char *args, int from_tty)
{
  unsigned long n;

  if (args == NULL || *args == '\0')
    error ("Usage: set mdi connectreset on|off|SECS");
  if (strcasecmp (args, "off") == 0)
    mdi_connectreset = -1;
  else if (strcasecmp (args, "on") == 0)
    mdi_connectreset = 0;
  else
    {
      char *tail;
      n = strtoul (args, &tail, 0);
      for (; *tail && isspace (*tail); tail++)
	continue;
      if (*tail || n > 60)
	error ("SECS out of range: %s", args);
      mdi_connectreset = n;
    }
}

static void
show_connectreset_command (char *args, int from_tty)
{
  if (mdi_connectreset < 0)
    {
      printf_filtered ("off\n");
    }
  else if (mdi_connectreset == 0)
    {
      printf_filtered ("on\n");
    }
  else 
    {
      printf_filtered ("%d\n", mdi_connectreset);
    }
}


static struct cmd_list_element *mditeamlist = NULL;

static void
mdi_team_status (MDIInt32 mdistat)
{
  switch (mdistat)
    {
    case MDISuccess:
      break;
    case MDIErrTeamId:
      error ("No such MDI team.");
      break;
    case MDIErrUnsupported:
      error ("MDI teams unsupported.");
      break;
    default:
      error ("MDI team operation failure.");
      break;
    }
}

static void
mdi_tm_status (MDIInt32 mdistat, MDITGIdT target, MDIDeviceIdT device)
{
  switch (mdistat)
    {
    case MDISuccess:
      break;
    case MDIErrAlreadyMember:
      printf_filtered ("Device %d:%d already a member.\n", target, device);
      break;
    case MDIErrNotMember:
      printf_filtered ("Device %d:%d not a member.\n", target, device);
      break;
    case MDIErrTeamId:
      error ("No such MDI team.");
      break;
    case MDIErrUnsupported:
      error ("MDI teams unsupported.");
      break;
    default:
      error ("MDI team member operation failure.");
      break;
    }
}

static void
mdi_team_command (char *args, int from_tty)
{
  unsigned long n;
  char *tail;

  if (args)
    {
      n = strtoul (args, &tail, 0);
      for (; *tail && isspace (*tail); tail++)
	continue;
      if (*tail)
	error ("Junk at end of command.");
      mdi_team = n;
    }
  else
    printf_filtered ("MDI team number is %d.\n", mdi_team);
}

static void
mdi_team_attach_command (char *args, int from_tty)
{
  unsigned long tm_device, tm_target;
  struct mdi_tm_data_list *tmdatalist = NULL, **listp = &tmdatalist;
  char *argscopy, *p;
  MDIInt32 mdistat;

  if (args)
    {
      check_mdi ();

      argscopy = alloca (strlen (args) + 1);
      strcpy (argscopy, args);
      for (p = strtok (argscopy, " \t"); p; p = strtok (NULL, " \t"))
	{
	  if (isdigit (*p))
	    {
	      tm_target = mdi_target;
	      tm_device = strtoul (p, &p, 0);
	      if (*p == ':' && isdigit (p[1]))
		{
		  tm_target = tm_device;
		  tm_device = strtoul (p + 1, &p, 0);
		}
	      if (*p)
		error ("Bad [TARGET:]DEVICE number.");

	      *listp = alloca (sizeof (**listp));
	      (*listp)->next = NULL;
	      (*listp)->tmdata.MDIHandle = mdi_desc->MDIHandle;
	      (*listp)->tmdata.TGId = tm_target - 1;
	      (*listp)->tmdata.DevId = tm_device - 1;
	      listp = &(*listp)->next;
	    }
	  else
	    error ("Junk at end of command.");
	}

      if (mdi_team <= 0)
	{
	  mdistat = mdiTeamCreate (mdi_desc->MDIHandle, &mdi_team);
	  if (mdistat != MDISuccess)
	    {
	      mdi_team = 0;
	      mdi_team_status (mdistat);
	    }
	  mdi_team += 1;
	}

      for (listp = &tmdatalist; *listp; listp = &(*listp)->next)
	{
	  mdistat = mdiTMAttach (mdi_desc->MDIHandle, mdi_team - 1,
				 &(*listp)->tmdata);
          mdi_tm_status (mdistat, (*listp)->tmdata.TGId + 1,
			 (*listp)->tmdata.DevId + 1);
	}
    }
  else
    error ("Argument required.");
}

static void
mdi_team_clear_command (char *args, int from_tty)
{
  MDIInt32 mdistat;

  if (args)
    error ("Junk at end of command.");

  check_mdi ();

  if (mdi_team <= 0)
    error ("No MDI team selected.");

  mdistat = mdiTeamClear (mdi_desc->MDIHandle, mdi_team - 1);
  mdi_team_status (mdistat);
  mdistat = mdiTeamDestroy (mdi_desc->MDIHandle, mdi_team - 1);
  mdi_team_status (mdistat);
  mdi_team = 0;
}

static void
mdi_team_detach_command (char *args, int from_tty)
{
  unsigned long tm_device, tm_target;
  struct mdi_tm_data_list *tmdatalist = NULL, **listp = &tmdatalist;
  char *argscopy, *p;
  MDIInt32 mdistat;

  if (args)
    {
      check_mdi ();

      if (mdi_team <= 0)
	error ("No MDI team selected.");

      argscopy = alloca (strlen (args) + 1);
      strcpy (argscopy, args);
      for (p = strtok (argscopy, " \t"); p; p = strtok (NULL, " \t"))
	{
	  if (isdigit (*p))
	    {
	      tm_target = mdi_target;
	      tm_device = strtoul (p, &p, 0);
	      if (*p == ':' && isdigit (p[1]))
		{
		  tm_target = tm_device;
		  tm_device = strtoul (p + 1, &p, 0);
		}
	      if (*p)
		error ("Bad [TARGET:]DEVICE number.");

	      *listp = alloca (sizeof (**listp));
	      (*listp)->next = NULL;
	      (*listp)->tmdata.MDIHandle = mdi_desc->MDIHandle;
	      (*listp)->tmdata.TGId = tm_target - 1;
	      (*listp)->tmdata.DevId = tm_device - 1;
	      listp = &(*listp)->next;
	    }
	  else
	    error ("Junk at end of command.");
	}

      for (listp = &tmdatalist; *listp; listp = &(*listp)->next)
	{
	  mdistat = mdiTMDetach (mdi_desc->MDIHandle, mdi_team - 1,
				 &(*listp)->tmdata);
          mdi_tm_status (mdistat, (*listp)->tmdata.TGId + 1,
			 (*listp)->tmdata.DevId + 1);
	}
    }
  else
    error ("Argument required.");
}

static void
mdi_team_list_command (char *args, int from_tty)
{
  MDIInt32 mdistat;
  MDIInt32 nteamids = 0;
  MDIInt32 ntms;
  MDITeamIdT *teamid;
  MDITMDataT *tmdata;
  int i, j;

  if (args)
    error ("Junk at end of command.");

  check_mdi ();

  mdistat = mdiQueryTeams (mdi_desc->MDIHandle, &nteamids, NULL);
  if (mdistat != MDISuccess || nteamids <= 0)
    return;

  teamid = alloca (nteamids * sizeof (*teamid));
  mdistat = mdiQueryTeams (mdi_desc->MDIHandle, &nteamids, teamid);
  if (mdistat != MDISuccess)
    return;					/* Shouldn't happen.  */

  for (i = 0; i < nteamids; i++)
    {
      ntms = 0;
      mdistat = mdiQueryTM (mdi_desc->MDIHandle, teamid[i], &ntms, NULL);
      if (mdistat != MDISuccess)
	continue;				/* Shouldn't happen.  */

      tmdata = xmalloc (ntms * sizeof (*tmdata));
      mdistat = mdiQueryTM (mdi_desc->MDIHandle, teamid[i], &ntms, tmdata);

      if (mdistat == MDISuccess)
	{
	  /* Got a team member list - output it. */
	  if (from_tty)
	    printf_filtered ("Team %02d:\n", teamid[i] + 1);
	  
	  for (j = 0; j < ntms; j++)
	      /* Output in human readable form if from_tty, else
		 in machine readable form for GDBTK. */
	      if (from_tty)
		printf_filtered ("    Dev %02d:%02d\n",
				 tmdata[j].TGId + 1,
				 tmdata[j].DevId + 1);
	      else
		printf_filtered ("%d|%d:%d\n",
				 teamid[i] + 1,
				 tmdata[j].TGId + 1,
				 tmdata[j].DevId + 1);
	}

      xfree (tmdata);
    }
}


static void
mdi_reset_command (char *arg, int from_tty)
{
  MDIInt32 mdistat;
  MDIUint32 flag = MDIFullReset;
  int i;

  if (arg != NULL && *arg != '\0')
    {
      if (strcasecmp (arg, "full") == 0)
	flag = MDIFullReset;
      else if (strcasecmp (arg, "device") == 0)
	flag = MDIDeviceReset;
      else if (strcasecmp (arg, "cpu") == 0)
	flag = MDICPUReset;
      else if (strcasecmp (arg, "periph") == 0)
	flag = MDIPeripheralReset;
      else
	error ("Usage: mdi reset [full | device | cpu | periph]");
    }

  check_mdi ();

  mdistat = mdiReset (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle, flag);
  for (i = 0; i < mdi_desc->gmcount; i++)
      (void) mdiStop (mdi_desc->gmdata[i].DevHandle);

  for (i = 0; i < mdi_desc->gmcount; i++)
    {
      MDIRunStateT runstate;

      mdi_desc->gmdata[i].disabled = 1;
      mdistat = mdiRunState (mdi_desc->gmdata[i].DevHandle, 100, &runstate);
      if (mdistat != MDISuccess)
	continue;

      if (runstate.Status != MDIStatusDisabled
	  && runstate.Status != MDIStatusVPEDisabled)
	mdi_desc->gmdata[i].disabled = 0;
    }

  /* Note that the inferior is no longer running. */
  target_mourn_inferior ();

  /* Reconnect to target. */
  mdi_set_inferior_ptid ();
  mdi_exited = mdi_signalled = 0;

  reinit_frame_cache ();

  togdb_force_update ();

  MDIERR (mdistat, "resetting device");
}

static void
mdi_regsync_command (char *arg, int from_tty)
{
  MDIInt32 mdistat;

  if (arg && *arg)
    error ("Usage: mdi regsync");

  check_mdi ();

  /* mdi_store_register (-1); */
  registers_changed ();
  togdb_force_update ();
}


/*
 * mdi cacheflush command
 */
static void
mdi_cacheflush_command (char *arg, int from_tty)
{
  MDIInt32 mdistat;
  int i;

  if (arg && *arg)
    error ("Usage: mdi cacheflush");

  check_mdi ();

  /* Writeback and invalidate all caches. */
  for (i = 0; i < mdi_desc->gmcount; i++)
    if (!mdi_desc->gmdata[i].disabled)
      {
	mdiCacheFlush (mdi_desc->gmdata[i].DevHandle, MDICacheTypeData,
		       MDICacheWriteBack | MDICacheInvalidate);
	mdiCacheFlush (mdi_desc->gmdata[i].DevHandle, MDICacheTypeUnified,
		       MDICacheWriteBack | MDICacheInvalidate);
	mdiCacheFlush (mdi_desc->gmdata[i].DevHandle, MDICacheTypeInstruction,
		       MDICacheInvalidate);
      }
}


static void
mdi_load_plugins_command (char *args, int from_tty)
{
  void *lib_handle = dlopen (args, RTLD_LAZY);
  if (!lib_handle)
    error ("Unable to load plugin extension \"%s\".  Error: %s\n",
	   args, dlerror());
}


static struct cmd_list_element *mdicycleslist = NULL;


/* Enable MIPSsim cycle counting, return 1 on success.
   Can be called via catch_errors to prevent fatal error. */

static int
mdi_cycles_enable (PTR dummy)
{
  static int cycle_failure = 0;
  int i;

  if (! mdi_desc->mipssim_p)
    error ("Cycle counting only available with MIPSsim");
  if (! MIPSsim_SetTraceLevel)
    error ("Cycle counting not supported on this version of MIPSsim");
  for (i = 0; i < mdi_desc->gmcount; i++)
    {
      int tracelev = 0;
      (void) MIPSsim_GetTraceLevel (mdi_desc->gmdata[i].DevHandle, &tracelev);
      if (! (tracelev & 1))
	{
	  if (MIPSsim_SetTraceLevel (mdi_desc->gmdata[i].DevHandle,
				     tracelev | 1)
	      != MDISuccess)
	    cycle_failure = 1;
	  else {
	    int newlev = 0;
	    (void) MIPSsim_GetTraceLevel (mdi_desc->gmdata[i].DevHandle,
					  &newlev);
	    if (! (newlev & 1))
	      cycle_failure = 1;
	  }
	  if (dummy)
	    /* reset to previous trace level */
	    (void) MIPSsim_SetTraceLevel (mdi_desc->gmdata[i].DevHandle,
					  tracelev);

	}
    }
  if (cycle_failure)
    error ("Cycle counting not available");
  return 1;
}

/* Set $cycles variable to current MIPSsim cycle counter */
static void
mdi_cycles_update (void)
{
  MDIUint64 cycles = 0, allcycles = 0;
  MDIInt32 stat;
  int i;
  
  for (i = 0; i < mdi_desc->gmcount; i++)
    {
      if (MIPSsim_GetPerfCounter)
	stat = MIPSsim_GetPerfCounter (mdi_desc->gmdata[i].DevHandle,
				       MIPS_E_PerfCntMode_ALL,
				       MIPS_PerfCntSel_Cycles0,
				       MIPS_PerfCntInd_Cycles0, &cycles);
      else if (MIPSsim_GetStats)
	stat = MIPSsim_GetStats (mdi_desc->gmdata[i].DevHandle,
				 MIPS_MDI_PerfCycles, &cycles);
      else
	stat = MDINotFound;

      if (stat != MDISuccess)
	return;

      allcycles += cycles;
    }

  set_internalvar (lookup_internalvar ("cycles"),
		   value_from_longest (builtin_type_uint64,
				       (LONGEST) allcycles));
}

static void
mdi_cycles_enable_command (char *args, int from_tty)
{
  check_mdi ();
  mdi_cycles_enable (NULL);
  mdi_cycles_update ();
}

static void
mdi_cycles_disable_command (char *args, int from_tty)
{
  check_mdi ();
  if (mdi_desc->mipssim_p && MIPSsim_SetTraceLevel)
    {
      int tracelev, i;

      for (i = 0; i < mdi_desc->gmcount; i++)
	{
	  (void) MIPSsim_GetTraceLevel (mdi_desc->gmdata[i].DevHandle,
					&tracelev);
	  if (tracelev & 1)
	    (void) MIPSsim_SetTraceLevel (mdi_desc->gmdata[i].DevHandle,
					  tracelev & ~1);
	}
    }
}

static void
mdi_cycles_clear_command (char *args, int from_tty)
{
  check_mdi ();

  /* Clear cycle counter */
  if (mdi_desc->mipssim_p)
    {
      int i;

      for (i = 0; i < mdi_desc->gmcount; i++)
	if (MIPSsim_ZeroPerfCounter)
	  MIPSsim_ZeroPerfCounter (mdi_desc->gmdata[i].DevHandle,
				   MIPS_E_PerfCntMode_ALL,
				   MIPS_PerfCntSel_Cycles0,
				   MIPS_PerfCntInd_Cycles0);
	else if (MIPSsim_ClearStats)
	  MIPSsim_ClearStats (mdi_desc->gmdata[i].DevHandle,
			      MIPS_MDI_PerfCycles);
    }

  /* Enable cycle counting and set $cycles */
  mdi_cycles_enable_command (args, from_tty);
}

static void
mdi_cycles_status_command (char *args, int from_tty)
{
  int tracelev;

  check_mdi ();

  /* Passing a non-zero argument causes the tracelevel to be reset
     after checking that it is possible to enable it. */
  mdi_cycles_enable ((PTR)1);

  (void) MIPSsim_GetTraceLevel (mdi_desc->gmdata[mdi_desc->gmindex].DevHandle,
				&tracelev);
  fprintf_filtered (gdb_stdout, "Cycle counting: %s\n",
		    (tracelev & 1) ? "enabled" : "disabled");
}

static void
mdi_cycles_command (char *args, int from_tty)
{
  printf_unfiltered ("\"mdi cycles\" must be followed by \"enable\", \"disable\", \"clear\" or \"status\".\n");
  mdi_cycles_status_command (args, from_tty);
}



/* Structure to maintain current state in mdi_show_devices_command()
   so that MDI connection can be cleanly closed if the user forces
   quit during the output. */

struct mdi_showdev_state
{
  struct mdi_state *	desc;
  int			TGshared;
  MDIHandleT 		TGhandle;
  MDITGDataT *		tgdata;
  MDIDDataT *		ddata;
};


static void
mdi_showdev_cleanup (void *arg)
{
  struct mdi_showdev_state *state = arg;
  MDIInt32 mdistat;

  /* Close any open target group (should be closed by Disconnect, but
     just to be sure). */
  if (! state->TGshared && state->TGhandle != MDINoHandle)
    mdiTGClose (state->TGhandle, MDICurrentState);

  /* Disconnect from DLL. */
  if (state->desc != mdi_desc
      && state->desc->MDIHandle != MDINoHandle)
    {
      mdistat = mdiDisconnect (state->desc->MDIHandle, MDICurrentState);
      MDIWARN (mdistat, "MDI disconnect");
      if (mdi_releaseonclose)
	{
	  /* Release MDI DLL. */
	  MDIRelease (mdiModule);
	  mdiModule = 0;
	}
    }

  /* Free local data arrays. */
  if (state->tgdata)
    xfree (state->tgdata);
  if (state->ddata)
    xfree (state->ddata);
  mdi_quiet = 0;
}


static void
mdi_show_devices_command (char *arg, int from_tty)
{
  char *lib = (arg && *arg) ? arg : mdi_library;
  struct mdi_state mdi_local;
  struct mdi_showdev_state state;
  struct cleanup *cleanups;
  MDIInt32 mdistat;
  int targetgroups_p;
  int ntargets;
  int ndevices;
  int t, d;

  memset (&state, 0, sizeof (state));
  state.TGhandle = MDINoHandle;
  if (!from_tty)
    mdi_quiet = 1;
  cleanups = make_cleanup (mdi_showdev_cleanup, &state);
  
  /* Get existing connection to MDI DLL. */
  if (!(state.desc = mdi_desc))
    {
      /* None: open a new temporary connection to DLL. */
      state.desc = &mdi_local;
      memset (&mdi_local, 0, sizeof (mdi_local));
      mdi_local.gmdata[mdi_local.gmindex].TGHandle = MDINoHandle;
      mdi_local.MDIHandle = MDINoHandle;
      mdi_dlopen (lib, &mdi_local, from_tty);
    }

  targetgroups_p = !!(state.desc->Config.MDICapability & MDICAP_TargetGroups);
  if (!targetgroups_p)
    {
      /* No target groups: create single dummy target group. */
      ntargets = 1;
      state.tgdata = (MDITGDataT *) xmalloc (sizeof (MDITGDataT));
      strcpy (state.tgdata[0].TGName, "Default");
    }
  else
    {
      /* Fetch target groups. */
      ntargets = 0;
      mdistat = mdiTGQuery (state.desc->MDIHandle, &ntargets, NULL);
      if (mdistat != MDISuccess)
	goto err;
      state.tgdata = (MDITGDataT *) xmalloc (ntargets * sizeof (MDITGDataT));
      mdistat = mdiTGQuery (state.desc->MDIHandle, &ntargets, state.tgdata);
      if (mdistat != MDISuccess)
	goto err;
    }

  for (t = 0; t < ntargets; t++)
    {
      /* Skip devices called "RESERVED" */
      if (strncasecmp (state.tgdata[t].TGName, "RESERVED",
		       sizeof "RESERVED" - 1) == 0)
	continue;    

      if (targetgroups_p)
	{
	  int i;

	  for (i = 0; i < state.desc->gmcount; i++)
	    {
	      if (state.tgdata[t].TGId == state.desc->gmdata[i].tmdata.TGId
		  && state.desc->gmdata[i].TGHandle != MDINoHandle)
		{
		  /* We've already got this TargetID open, use our
		     existing handle, since MDITGOpen may fail
		     if it's an exclusive open. */
		  state.TGshared = 1;
		  state.TGhandle = state.desc->gmdata[i].TGHandle;
		}
	    }
	  if (state.TGhandle == MDINoHandle)
	    {
	      state.TGshared = 0;
	      mdistat = mdiTGOpen (state.desc->MDIHandle,
				   state.tgdata[t].TGId, MDISharedAccess,
				   &state.TGhandle);
	      if (mdistat != MDISuccess)
		/* Failed shared access - try exclusive access. */
		mdistat = mdiTGOpen (state.desc->MDIHandle,
				     state.tgdata[t].TGId, MDIExclusiveAccess,
				     &state.TGhandle);
	      if (mdistat != MDISuccess)
		continue;
	    }
	}
      else
	{
	  /* No target groups, reuse MDI handle */
	  state.TGshared = 1;
	  state.TGhandle = state.desc->MDIHandle;
	}

      /* Fetch list of devices in this target group. */
      ndevices = 0;
      mdistat = mdiDQuery (state.TGhandle, &ndevices, NULL);
      if (mdistat == MDISuccess && ndevices > 0)
	{
	  state.ddata = (MDIDDataT *) xmalloc (ndevices * sizeof (MDIDDataT));
	  mdistat = mdiDQuery (state.TGhandle, &ndevices, state.ddata);

	  if (mdistat == MDISuccess)
	    {
	      state.desc->mipssim_p =
		(strcmp (state.ddata[0].VPartData, "MIPSsim") == 0);
	      
	      /* Got device list - output it. */
	      if (from_tty)
		printf_filtered ("Targ %02d: %s\n", t + 1,
				 state.tgdata[t].TGName);
	      
	      for (d = 0; d < ndevices; d++)
		{
		  /* Skip devices called "RESERVED" */
		  if (strncasecmp (state.ddata[d].DName, "RESERVED",
				   sizeof "RESERVED" - 1) == 0)
		    continue;
		  /* Output in human readable form if from_tty, else
		     in machine readable form for GDBTK. */
		  if (from_tty)
		    printf_filtered ("    Dev %02d: %s\n", d + 1,
				     state.ddata[d].DName);
		  else if (ntargets > 1 && ndevices > 1)
		    printf_filtered ("%d:%d|%s:%s\n", t + 1, d + 1,
				     state.tgdata[t].TGName,
				     state.ddata[d].DName);
		  else if (ntargets > 1)
		    printf_filtered ("%d:%d|%s\n", t + 1, d + 1,
				     state.tgdata[t].TGName);
		  else
		    printf_filtered ("%d:%d|%s\n", t + 1, d + 1,
				     state.ddata[d].DName);
		}
	    }
	}

      /* Done with this target group. */
      if (state.ddata)
	xfree (state.ddata);
      state.ddata = NULL;

      if (! state.TGshared && state.TGhandle != MDINoHandle)
	mdiTGClose (state.TGhandle, MDICurrentState);
      state.TGhandle = MDINoHandle;
    }

err:
  do_cleanups (cleanups);
}

static void
mdi_threadviewinactive_sfunc (char *args, int from_tty,
			      struct cmd_list_element *c)
{
  mdi_find_new_threads ();
}

/* Root of all "mdi "/"set mdi "/"show mdi " commands. This will eventually be
   used for all MDI specific commands.  */

static void
mdi_command (char *args, int from_tty)
{
  help_list (mdicmdlist, "mdi ", all_commands, gdb_stdout);
}

static void
show_mdi_command (char *args, int from_tty)
{
  help_list (showmdicmdlist, "show mdi ", all_commands, gdb_stdout);
}

static void
set_mdi_command (char *args, int from_tty)
{
  printf_unfiltered
    ("\"set mdi\" must be followed by an appropriate subcommand.\n");
  help_list (setmdicmdlist, "set mdi ", all_commands, gdb_stdout);
}


static void
init_mdi_ops (void)
{
  mdi_ops.to_shortname = "mdi";
  mdi_ops.to_longname = "MIPS Debug Interface";
  mdi_ops.to_doc = "Use MDI to access MIPS targets";
  mdi_ops.to_open = mdi_open;
  mdi_ops.to_close = mdi_close;
  mdi_ops.to_attach = mdi_attach;
  mdi_ops.to_detach = mdi_detach;
  mdi_ops.to_disconnect = mdi_disconnect;
  mdi_ops.to_resume = mdi_resume;
  mdi_ops.to_wait = mdi_wait;
  mdi_ops.to_fetch_registers = mdi_fetch_register;
  mdi_ops.to_store_registers = mdi_store_register;
  mdi_ops.to_prepare_to_store = gr_prepare_to_store;
  mdi_ops.to_xfer_memory = mdi_xfer_memory;
  mdi_ops.to_files_info = mdi_files_info;
  mdi_ops.to_insert_breakpoint = mdi_insert_breakpoint;
  mdi_ops.to_remove_breakpoint = mdi_remove_breakpoint;
  mdi_ops.to_can_use_hw_breakpoint = mdi_can_use_hw_breakpoint;
  mdi_ops.to_insert_hw_breakpoint = mdi_insert_hw_breakpoint;
  mdi_ops.to_remove_hw_breakpoint = mdi_remove_hw_breakpoint;
  mdi_ops.to_insert_watchpoint = mdi_insert_watchpoint;
  mdi_ops.to_remove_watchpoint = mdi_remove_watchpoint;
  mdi_ops.to_stopped_by_watchpoint = mdi_stopped_by_watchpoint;
  mdi_ops.to_have_continuable_watchpoint = 0;
  mdi_ops.to_stopped_data_address = mdi_stopped_data_address;
#ifdef notyet
  mdi_ops.to_region_size_ok_for_hw_watchpoint =
    mdi_region_size_ok_for_hw_watchpoint;
#endif
  mdi_ops.to_kill = mdi_kill;
  mdi_ops.to_load = mdi_load;
  mdi_ops.to_create_inferior = mdi_create_inferior;
  mdi_ops.to_mourn_inferior = mdi_mourn_inferior;
  mdi_ops.to_switch_to_thread = mdi_switch_to_thread;
  mdi_ops.to_thread_alive = mdi_thread_alive;
  mdi_ops.to_find_new_threads = mdi_find_new_threads;
  mdi_ops.to_pid_to_str = mdi_pid_to_str;
  mdi_ops.to_stop = 0; /* XXX */
  mdi_ops.to_rcmd = mdi_rcmd;
  mdi_ops.to_log_command = mdi_log_command;
  mdi_ops.to_stratum = process_stratum;
  mdi_ops.to_has_all_memory = 1;
  mdi_ops.to_has_memory = 1;
  mdi_ops.to_has_stack = 1;
  mdi_ops.to_has_registers = 1;
  mdi_ops.to_has_execution = 1;
  mdi_ops.to_has_thread_control = tc_schedlock;	/* can we lock scheduler?? */
  mdi_ops.to_magic = OPS_MAGIC;	/* Always the last thing */
}				/* init_mdi_ops */


static void
mdi_getenv_number (const char *env, int *resp, int min, int max)
{
  char *s;
  if ((s = getenv (env)) != NULL)
    {
      char *tail;
      long val;
      val = strtol (s, &tail, 0);
      if (val >= min && val <= max && !*tail)
	*resp = val;
      else
	warning ("Bad environment variable: %s=%s ignored", env, s);
    }
}


void
_initialize_remote_mdi (void)
{
  struct cmd_list_element *c;
  char *s;

  init_mdi_ops ();
  add_target (&mdi_ops);

  /* Default MDI log message stream. */
  mdi_logfp = gdb_stdlog;

  /* We need a fake non-null ptid to indicate the target is connected
     and running. */
  mdi_ptid = pid_to_ptid (42000);

  /* Pick up useful "global" settings from environment. */
  if ((s = getenv ("GDBMDILIB")) != NULL)
    mdi_library = xstrdup (s);
  if ((s = getenv ("GDBMIPSSIMCONFIG")) != NULL)
    mdi_configfile = xstrdup (s);
  if ((s = getenv ("GDBMIPSSIMDEVCFG")) != NULL)
    mdi_devcfgfile = xstrdup (s);
  mdi_getenv_number ("GDBMDITARGET", &mdi_target, 1, 99);
  mdi_getenv_number ("GDBMDIDEVICE", &mdi_device, 1, 99);
  mdi_getenv_number ("GDBMDICONNRST", &mdi_connectreset, -1, 99);

  mdi_ftext_syms = xstrdup ("_ftext");
  mdi_etext_syms = xstrdup ("_ecode _etext");
  mdi_mcount_syms = xstrdup ("_mcount __mcount");

  add_prefix_cmd ("mdi", no_class, set_mdi_command,
		  "Various MDI specific commands.",
		  &setmdicmdlist, "set mdi ", 0, &setlist);
  add_prefix_cmd ("mdi", no_class, show_mdi_command,
		  "Various MDI specific commands.",
		  &showmdicmdlist, "show mdi ", 0, &showlist);

  add_setshow_boolean_cmd ("stepinto", no_class, 
			   &mdi_stepinto, "\
Set MDI step-into mode.", "\
Show MDI step-into mode.", "\
When on, trap handlers can be stepped through.", "\
Step-into mode is %s.",
			   NULL, NULL,
			   &setmdicmdlist, &showmdicmdlist);

  add_setshow_boolean_cmd ("threadstepall", no_class, 
			   &mdi_threadstepall, "\
Set simultaneous thread stepping mode.", "\
Show simultaneous thread stepping mode.", "\
When on, all threads are stepped together with the scheduler locking\n\
mode enabled, otherwise single-stepping only enables execution in the\n\
selected thread.", "\
Simultaneous thread stepping mode is %s.",
			   NULL, NULL,
			   &setmdicmdlist, &showmdicmdlist);

  add_setshow_uinteger_cmd ("connecttimeout", no_class, 
			    &mdi_connecttimeout, "\
Set MDI connect timeout.", "\
Show MDI connect timeout.", "\
The number of seconds that the target is waited to halt in a response\n\
to such a request when a new connection has been made.", "\
MDI connect timeout is %s s.",
			   NULL, NULL,
			   &setmdicmdlist, &showmdicmdlist);

  add_setshow_boolean_cmd ("continueonclose", no_class, 
			   &mdi_continueonclose, "\
Set continue on MDI close mode.", "\
Show continue on MDI close mode.", "\
When on, the target will be restarted before detaching.", "\
Continue on MDI close mode is %s.",
			   NULL, NULL,
			   &setmdicmdlist, &showmdicmdlist);

  add_setshow_boolean_cmd ("rununcached", no_class, 
			   &mdi_rununcached, "\
Set program starts uncached.", "\
Show program starts uncached.", "\
When on, the loaded program's entrypoint address will be forced to\n\
an uncached MIPS segment, even if linked to a cacheable address.", "\
Program starts uncached is %s.",
			   NULL, NULL,
			   &setmdicmdlist, &showmdicmdlist);

  add_setshow_boolean_cmd ("threadviewinactive", no_class, 
			   &mdi_threadviewinactive, "\
Set view inactive threads.", "\
Show view inactive threads.", "\
When on, all threads are always visible, otherwise only ones that\n\
are activated and unhalted.", "\
View inactive threads is %s.",
			   &mdi_threadviewinactive_sfunc, NULL,
			   &setmdicmdlist, &showmdicmdlist);

  add_setshow_zinteger_cmd ("waittime", no_class, 
			    &mdi_waittime, "\
Set MDI poll period.", "\
Show MDI poll period.", "\
The number of milliseconds that the target is allowed to run for\n\
between gdb status polls.  Note that some MDI implementations do\n\
not have accurate time keeping.", "\
MDI poll period is %s ms.",
			   NULL, NULL,
			   &setmdicmdlist, &showmdicmdlist);

  add_setshow_boolean_cmd ("releaseonclose", no_class, 
			   &mdi_releaseonclose, "\
Set MDI release on close.", "\
Show MDI release on close.", "\
When on, the MDI library is released when the target is closed.", "\
MDI release on close is %s.",
			   NULL, NULL,
			   &setmdicmdlist, &showmdicmdlist);

  add_setshow_filename_cmd ("logfile", no_class,
			    &mdi_logfile, "\
Set MDI session log file.", "\
Show MDI session log file.", "\
The file to receive the MDI session debug log when \"debug remote\"\n\
has been set.", "\
MDI log file is %s.",
			    NULL, NULL,
			    &setmdicmdlist, &showmdicmdlist);

  add_setshow_string_cmd ("logdocommand", no_class, 
			   &mdi_logdocommand, "\
Set the remote command for logging gdb commands.", "\
Show the remote command for logging gdb commands.", "\
A textual command to be passed to the MDI target for logging gdb commands.\n\
The gdb command is supplied in place of %s if present in the string,\n\
otherwise the string is passed as is.", "\
The command for the MDI target for logging gdb commands is %s.",
			   NULL, NULL,
			   &setmdicmdlist, &showmdicmdlist);

  add_setshow_filename_cmd ("library", no_class, 
			   &mdi_library, "\
Set MDI interface library.", "\
Show MDI interface library.", "\
The name of the dynamically loaded MDI interface library.", "\
MDI interface library is %s.",
			   NULL, NULL,
			   &setmdicmdlist, &showmdicmdlist);

  add_setshow_filename_cmd ("configfile", no_class, 
			   &mdi_configfile, "\
Set MIPSsim config file name.", "\
Show MIPSsim config file name.", "\
The name of the configuration file to be loaded by MIPSsim.\n\
This can also be set by the GDBMIPSSIMCONFIG environment variable.\n\
If neither is set then GDB will auto-generate a configuration file.", "\
MIPSsim config file is %s.",
			   NULL, NULL,
			   &setmdicmdlist, &showmdicmdlist);

  add_setshow_filename_cmd ("devcfgfile", no_class, 
			   &mdi_devcfgfile, "\
Set MIPSsim device config file name.", "\
Show MIPSsim device config file name.", "\
The name of the device configuration file which will be added to GDB's\n\
auto-generated MIPSsim CPU config file.\n\
This can also be set by the GDBMIPSSIMDEVCFG environment variable.", "\
MIPSsim device config file is %s.",
			   NULL, NULL,
			   &setmdicmdlist, &showmdicmdlist);

  mdi_gmonfile = xstrdup ("gmon.out");
  add_setshow_filename_cmd ("gmonfile", no_class, 
			   &mdi_gmonfile, "\
Set MIPSsim gmon profile output file name.", "\
Show MIPSsim gmon profile output file name.", "\
The name of the file in which to store gmon profiling data,\n\
for use by gprof. Currently only supported by the MIPSsim target.", "\
MIPSsim gmon profile file file is %s.",
			    NULL, NULL,
			    &setmdicmdlist, &showmdicmdlist);

  add_setshow_auto_boolean_cmd ("profile", no_class, 
				&mdi_profiling, "\
Set MIPSsim profiling.", "\
Show MIPSsim profiling.", "\
When on, GDB will collect profiling data for the application\n\
and write it to the profile output file (\"show mdi gmonfile\").\n\
When auto, enable when _mcount symbol found in executable.", "\
MIPSsim profiling is %s.",
				NULL, NULL,
				&setmdicmdlist, &showmdicmdlist);

  add_setshow_boolean_cmd ("profile-mcount", no_class, 
			   &mdi_profile_mcount, "\
Set _mcount in MIPSsim profile.", "\
Show _mcount in MIPSsim profile.", "\
When on, GDB will include time spent in the _mcount function in the\n\
profile data.", "\
Include _mcount in profile is %s.",
			   NULL, NULL,
			   &setmdicmdlist, &showmdicmdlist);

  add_setshow_boolean_cmd ("profile-cycles", no_class, 
			   &mdi_profile_cycles, "\
Set cycle counting in when MIPSsim profiling.", "\
Show cycle counting in when MIPSsim profiling.", "\
When on, GDB will enable cycle accurate MIPSsim profiling, if supported\n\
by MIPSsim.", "\
Cycle accurate profiling is %s.",
			   NULL, NULL,
			   &setmdicmdlist, &showmdicmdlist);

  add_setshow_zinteger_cmd ("team", no_class, 
			   &mdi_team, "\
Set MDI team number.", "\
Show MDI team number.", "\
Selects the MDI team to be used.  If this variable is set to 0\n\
a new team will be created for new members.", "\
MDI team number is %s.",
			   NULL, NULL,
			   &setmdicmdlist, &showmdicmdlist);

  add_setshow_zinteger_cmd ("target", no_class, 
			   &mdi_target, "\
Set MDI target group number.", "\
Show MDI target group number.", "\
Selects the MDI target group to be used.  If this variable is set to 0\n\
you will be given a list of options during the connection setup.", "\
MDI target group number is %s.",
			   NULL, NULL,
			   &setmdicmdlist, &showmdicmdlist);

  add_setshow_zinteger_cmd ("device", no_class, 
			   &mdi_device, "\
Set MDI device number.", "\
Show MDI device number.", "\
Selects the MDI device to be used.  If this variable is set to 0 you will\n\
be given a list of options during the connection setup.", "\
MDI device number is %s.",
			   NULL, NULL,
			   &setmdicmdlist, &showmdicmdlist);

  mdi_target_prompt = xstrdup ("");
  add_setshow_string_cmd ("prompt", no_class, 
			   &mdi_target_prompt, "\
Set MDI target input prompt.", "\
Show MDI target input prompt.", "\
Sets the prompt used when the program running under MDI attempts to read\n\
from the console.", "\
MDI target input prompt is %s.",
			   NULL, NULL,
			   &setmdicmdlist, &showmdicmdlist);

  add_setshow_string_cmd ("ftext-symbols", no_class, 
			   &mdi_ftext_syms, "\
Set start of code section symbol name list.", "\
Show start of code section symbol list.", "\
Sets the list of symbol names which may indicate the start of the text or\n\
code section for profiling.", "\
Start of code section symbols names are %s.",
			   NULL, NULL,
			   &setmdicmdlist, &showmdicmdlist);

  add_setshow_string_cmd ("etext-symbols", no_class, 
			   &mdi_etext_syms, "\
Set end of code section symbol name list.", "\
Show end of code section symbol name list.", "\
Sets the list of symbol names which may indicate the end of the text or\n\
code section for profiling.", "\
End of code section symbols names are %s.",
			   NULL, NULL,
			   &setmdicmdlist, &showmdicmdlist);

  add_setshow_string_cmd ("mcount-symbols", no_class, 
			   &mdi_mcount_syms, "\
Set mcount symbol name list.", "\
Show mcount symbol name list.", "\
Sets the list of symbol names which may match the call-graph profiling\n\
function.", "\
The mcount symbol name list is %s.",
			   NULL, NULL,
			   &setmdicmdlist, &showmdicmdlist);

  add_show_cmd ("implementer", no_class, var_string,
		(void *)&mdi_implementer, "\
Show MDI library implementer name.\n\
Shows the name of the implementer of the current MDI library.",
			   &showmdicmdlist);

  add_cmd ("connectreset", class_support, set_connectreset_command, "\
Set reset on initial connection state.\n\
If \"off\" then gdb will not reset the target when connecting.\n\
If \"on\" then gdb will reset the target when connecting, and\n\
will leave it in reset.\n\
If a value N, then gdb will reset the target when connecting,\n\
start it executing, wait N seconds, and then stop the target.\n\
This is to allow the target's PROM monitor to initialise the\n\
memory controller, etc.",
	    &setmdicmdlist);

  add_cmd ("connectreset", class_support, show_connectreset_command, "\
Show reset on initial connection state.\n\
If \"off\" then gdb will not reset the target when connecting.\n\
If \"on\" then gdb will reset the target when connecting, and\n\
will leave it in reset.\n\
If a value N, then gdb will reset the target when connecting,\n\
start it executing, wait N seconds, and then stop the target.\n\
This is to allow the target's PROM monitor to initialise the\n\
memory controller, etc.",
	    &showmdicmdlist);

  add_cmd ("asid", class_support, set_asid_command, "\
Set target memory access mode.\n\
If ASID is \"off\", gdb will access target memory and set\n\
breakpoints using the global virtual address space.\n\
If ASID is \"on\", gdb will reference target memory using the\n\
address space id (asid) from the current ENTRYHI register.\n\
If ASID is \"auto\", gdb will reference target memory using the\n\
global mapping if the address is in an \"unmapped\" region, otherwise\n\
it will use the current asid from ENTRYHI.\n\
In all other cases ASID is an address space identifier\n\
which gdb will use to qualify a target virtual address.", 
	   &setmdicmdlist);

  add_cmd ("asid", class_support, show_asid_command, "\
Show target memory access mode.\n\
If ASID is \"off\", gdb will access target memory and set\n\
breakpoints using the global virtual address space.\n\
If ASID is \"on\", gdb will reference target memory using the\n\
address space id (asid) from the current ENTRYHI register.\n\
If ASID is \"auto\", gdb will reference target memory using the\n\
global mapping if the address is in an \"unmapped\" region, otherwise\n\
it will use the current asid from ENTRYHI.\n\
In all other cases ASID is a numeric address space identifier\n\
which gdb will use to qualify a target virtual address.",
	   &showmdicmdlist);

  add_cmd ("tlb", class_support, show_tlb_command, "\
Show TLB entries.\n\
Decodes and displays TLB entries.  If an entry is specified,\n\
only that entry will be displayed.  If no entry is selected then\n\
the entire TLB is diplayed.", 
	   &showmdicmdlist);

  add_cmd ("tlb", class_support, set_tlb_command, "\
Set TLB entry.\n\
Sets an individual TLB entry.  For R3000 style TLBs, then\n\
arguments are ENTRY,HI,LO.  For R4000 style TLBs, the arguments\n\
are ENTRY,HI,LO0,LO1,MASK.  The TLB entry is refetched and\n\
displayed after it has been written.", 
	   &setmdicmdlist);

  add_cmd ("cp0", class_obscure, show_cp0_command, "\
Show CP0 register.\n\
Display CP0 REG[/BANK].  If BANK is not given, bank 0 is assumed.\n\
If the function of the CP0 register is known, the symbolic\n\
register name will also be displayed.  This facility is intended\n\
as a last-resort for when gdb does not know about a particular\n\
CP0 register.", 
	   &showmdicmdlist);

  add_cmd ("cp0", class_support, set_cp0_command, "\
Set CP0 register.\n\
Sets CP0 REG[/BANK] to VALUE.  If BANK is not given, bank 0 is\n\
assumed.  This facility is intended as a last-resort for when\n\
gdb does not know about a particular CP0 register.",
	   &setmdicmdlist);

  add_cmd ("dcache", class_support, show_dcache_command, "\
Display data cache information.\n\
Without arguments the topology is shown.\n\
An argument of ADDRESS[,SET] is used to select a particular cache line\n\
and optionally a set.  Once a line has been selected, the tag and parity\n\
entries for the line are displayed, followed by the cache line data.",
	   &showmdicmdlist);

  add_cmd ("dcache", class_support, set_dcache_command, "\
Change a line in the data cache.\n\
An argument of ADDRESS,SET,TAG,PARITY,DATA,... follows.\n\
ADDRESS and SET are used to select a particular cache line.\n\
Once a line has been selected, TAG and PARITY are written to the\n\
the cache line tag and parity fields. The DATA values are\n\
written to the cache line itself.  There must be enough DATA\n\
values to fill the complete cache line (typically 4 or 8 words).\n\
It's unlikely that you will achieve anything useful with this\n\
command.", 
	   &setmdicmdlist);

  add_cmd ("icache", class_support, show_icache_command, "\
Display instruction cache information.\n\
Without arguments the topology is shown.\n\
An argument of ADDRESS[,SET] is used to select a particular cache line\n\
and optionally a set.  Once a line has been selected, the tag and parity\n\
entries for the line are displayed, followed by the cache line data.",
	   &showmdicmdlist);

  add_cmd ("icache", class_support, set_icache_command, "\
Change a line in the instruction cache.\n\
An argument of ADDRESS,SET,TAG,PARITY,DATA,... follows.\n\
ADDRESS and SET are used to select a particular cache line.\n\
Once a line has been selected, TAG and PARITY are written to the\n\
the cache line tag and parity fields. The DATA values are\n\
written to the cache line itself.  There must be enough DATA\n\
values to fill the complete cache line (typically 4 or 8 words).\n\
It's unlikely that you will achieve anything useful with this\n\
command.", 
	   &setmdicmdlist);

  add_cmd ("scache", class_support, show_scache_command, "\
Display secondary cache information.\n\
Without arguments the topology is shown.\n\
An argument of ADDRESS[,SET] is used to select a particular cache line\n\
and optionally a set.  Once a line has been selected, the tag and parity\n\
entries for the line are displayed, followed by the cache line data.",
	   &showmdicmdlist);

  add_cmd ("scache", class_support, set_scache_command, "\
Change a line in the secondary cache.\n\
An argument of ADDRESS,SET,TAG,PARITY,DATA,... follows.\n\
ADDRESS and SET are used to select a particular cache line.\n\
Once a line has been selected, TAG and PARITY are written to the\n\
the cache line tag and parity fields. The DATA values are\n\
written to the cache line itself.  There must be enough DATA\n\
values to fill the complete cache line (typically 4 or 8 words).\n\
It's unlikely that you will achieve anything useful with this\n\
command.", 
	   &setmdicmdlist);

  add_cmd ("devices", class_support, mdi_show_devices_command, "\
List all available MDI devices.", 
	   &showmdicmdlist);

  add_prefix_cmd ("mdi", class_obscure, mdi_command,
		  "Various MDI specific commands.",
		  &mdicmdlist, "mdi ", 0, &cmdlist);

  add_prefix_cmd ("team", class_obscure, mdi_team_command,
		  "MDI team support.\n\
With no arguments the currently selected team is shown.\n\
With a numerical argument the MDI team to be used is selected,\n\
with 0 meaning a new team will be created for new members\n\
Otherwise one of the subcommands can be used.",
		  &mditeamlist, "mdi team ", 1, &mdicmdlist);

  add_cmd ("list", class_obscure, mdi_team_list_command,
	   "List MDI teams.", &mditeamlist);
  add_cmd ("detach", class_obscure, mdi_team_detach_command,
	   "Detach from an MDI team.", &mditeamlist);
  add_cmd ("clear", class_obscure, mdi_team_clear_command,
	   "Clear an MDI team.", &mditeamlist);
  add_cmd ("attach", class_obscure, mdi_team_attach_command,
	   "Attach to an MDI team.", &mditeamlist);

  add_cmd ("reset", class_obscure, mdi_reset_command, "\
Reset MDI target.\n\
Resets CPU and associated peripherals.  GDB will attempt to\n\
remain synchronised with the CPU, but this probably won't do\n\
anything useful unless you are debugging the ROM startup code.",
	   &mdicmdlist);

  add_cmd ("regsync", class_obscure, mdi_regsync_command, "\
Flush cached registers.\n\
This is only required in unusual circumstances, for example if a coprocessor\n\
register may have changed due to a previous MDI operation.",
	   &mdicmdlist);

  add_cmd ("cacheflush", class_obscure, mdi_cacheflush_command, "\
Flush (writeback and invalidate) all CPU caches.", 
	   &mdicmdlist);

  add_cmd ("load-plugins", class_obscure, mdi_load_plugins_command, "\
Load gdb plug-in extensions",
	   &mdicmdlist);

  add_prefix_cmd ("cycles", class_obscure, mdi_cycles_command, "\
Control MIPSsim cycle counting.\n\
While cycle counting is enabled, the current cycle count is stored in\n\
the GDB $cycles convenience variable. To display the cycle count\n\
whenever the program stops, use \"display $cycles\".",
		  &mdicycleslist, "mdi cycles ", 0, &mdicmdlist);

  add_cmd ("enable", class_obscure, mdi_cycles_enable_command, "\
Enable MIPSsim cycle counting.",
	   &mdicycleslist);
  add_alias_cmd ("on", "enable", class_obscure, 1, &mdicycleslist);
  add_alias_cmd ("yes", "enable", class_obscure, 1, &mdicycleslist);
  add_alias_cmd ("start", "enable", class_obscure, 1, &mdicycleslist);

  add_cmd ("clear", class_obscure, mdi_cycles_clear_command, "\
Clear MIPSsim cycle count and enable counting.",
	   &mdicycleslist);
  add_alias_cmd ("0", "clear", class_obscure, 1, &mdicycleslist);

  add_cmd ("disable", class_obscure, mdi_cycles_disable_command, "\
Disable MIPSsim cycle counting.",
	   &mdicycleslist);
  add_alias_cmd ("off", "disable", class_obscure, 1, &mdicycleslist);
  add_alias_cmd ("no", "disable", class_obscure, 1, &mdicycleslist);
  add_alias_cmd ("stop", "disable", class_obscure, 1, &mdicycleslist);

  add_cmd ("status", class_obscure, mdi_cycles_status_command, "\
Report MIPSsim cycle counting status.",
	   &mdicycleslist);
}
