/* Subroutines used for Microchip PIC32C code generation.
   Copyright (C) 1989, 1990, 1991, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007
   Free Software Foundation, Inc.
   Contributed by Rathish C, rathish.chamukuttan@microchip.com

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "hash-table.h"
#include "tm.h"
#include "rtl.h"
#include "hash-set.h"
#include "machmode.h"
#include "vec.h"
#include "double-int.h"
#include "input.h"
#include "alias.h"
#include "symtab.h"
#include "wide-int.h"
#include "inchash.h"
#include "tree.h"
#include "memmodel.h"
#include "fold-const.h"
#include "stringpool.h"
#include "stor-layout.h"
#include "calls.h"
#include "varasm.h"
#include "obstack.h"
#include "regs.h"
#include "hard-reg-set.h"
#include "insn-config.h"
#include "conditions.h"
#include "output.h"
#include "insn-attr.h"
#include "flags.h"
#include "reload.h"
#include "function.h"
#include "hashtab.h"
#include "statistics.h"
#include "real.h"
#include "fixed-value.h"
#include "expmed.h"
#include "dojump.h"
#include "explow.h"
#include "emit-rtl.h"
#include "stmt.h"
#include "expr.h"
#include "insn-codes.h"
#include "optabs.h"
#include "diagnostic-core.h"
#include "recog.h"
#include "predict.h"
#include "dominance.h"
#include "cfg.h"
#include "cfgrtl.h"
#include "cfganal.h"
#include "lcm.h"
#include "cfgbuild.h"
#include "cfgcleanup.h"
#include "basic-block.h"
#include "hash-map.h"
#include "is-a.h"
#include "plugin-api.h"
#include "ipa-ref.h"
#include "cgraph.h"
#include "ggc.h"
#include "except.h"
#include "tm_p.h"
#include "target.h"
#include "target-def.h"
#include "debug.h"
#include "langhooks.h"
#include "bitmap.h"
#include "df.h"
#include "intl.h"
#include "libfuncs.h"
#include "params.h"
#include "opts.h"
#include "dumpfile.h"
#include "gimple-expr.h"
#include "builtins.h"
#include "tm-constrs.h"
#include "rtl-iter.h"
#include "sched-int.h"
#include "gimplify.h"
#include "version.h"

#include "pic32c-protos.h"
#include "pic32c.h"

#if !defined(SKIP_LICENSE_MANAGER)
#include "config/mchp-cci/xclm_public.h"
#include "config/mchp-cci/mchp_sha.h"
#endif  /* SKIP_LICENSE_MANAGER */


#ifndef MCHP_XCLM_FREE_LICENSE
 #define MCHP_XCLM_FREE_LICENSE 0x0
 #warning MCHP_XCLM_FREE_LICENSE not defined by API
#endif

/* Static Variables to modify the optimizatrion option levels */

static int require_cpp = 0;
static int nullify_O2 = 0;
static int nullify_Os = 0;
static int nullify_O3 = 0;
static int nullify_lto = 0;
//static const char *invalid = (const char*) "invalid";

static const char *disabled_option_message = NULL;
static int message_displayed = 0;
static int message_purchase_display = 0;
static const char *invalid_license = "due to an invalid XC32 license";

#define NULLIFY(X,S) \
    if (X) { \
      if ((S != NULL) && (disabled_option_message == NULL)) { \
          disabled_option_message = S; \
          message_displayed++;         \
        } \
    } \
    X

#define xstr(s) str(s)
#define str(s) #s


#ifdef MCHP_USE_LICENSE_CONF
#define MCHP_LICENSE_CONF_FILENAME "license.conf"
#define MCHP_LICENSEPATH_MARKER "license_dir"
#endif /* MCHP_USE_LICENSE_CONF */


/* Set the License manager */

#ifdef __MINGW32__
#define MCHP_MAX_LICENSEPATH_LINE_LENGTH 1024
#define MCHP_XCLM_FILENAME "xclm.exe"
#else
#define MCHP_XCLM_FILENAME "xclm"
#endif /* __MINGW32__*/


#ifdef SKIP_LICENSE_MANAGER
#if defined(MCHP_XCLM_VALID_CPP_FULL)
HOST_WIDE_INT mchp_pic32_license_valid = MCHP_XCLM_VALID_CPP_FULL;
#else
HOST_WIDE_INT mchp_pic32_license_valid = 0x6;
#endif /* MCHP_XCLM_VALID_CPP_FULL */
#else
HOST_WIDE_INT mchp_pic32_license_valid = -1;
#endif /* SKIP_LICENSE_MANAGER*/


#ifndef SKIP_LICENSE_MANAGER

static char*
get_license_manager_path (void)
{
  extern struct cl_decoded_option *save_decoded_options;
  char *xclmpath = NULL;
  char* bindir = NULL;
  FILE *fptr = NULL;
  struct stat filestat;
  int xclmpath_length;

#ifdef MCHP_USE_LICENSE_CONF
  char *conf_dir, *conf_fname;
  FILE *fptr = NULL;
  char line [MCHP_MAX_LICENSEPATH_LINE_LENGTH] = {0};

  /* MCHP_LICENSE_CONF_FILENAME must reside in the same directory as xc32-gcc */
  conf_dir = make_relative_prefix(save_decoded_options[0].arg,
                                  "/lib/gcc/pic32c/"
                                  str(BUILDING_GCC_MAJOR) "."
                                  str(BUILDING_GCC_MINOR) "."
                                  str(BUILDING_GCC_PATCHLEVEL),
                                  "/bin");

  /* alloc space for the filename: directory + '/' + MCHP_LICENSE_CONF_FILENAME
   */
  conf_fname = (char*)alloca (strlen (conf_dir) + 1 +
                              strlen (MCHP_LICENSE_CONF_FILENAME) + 1);
  strcpy (conf_fname, conf_dir);
  if (conf_fname [strlen (conf_fname) - 1] != '/'
      && conf_fname [strlen (conf_fname) - 1] != '\\')
    strcat (conf_fname, "/");
  strcat (conf_fname, MCHP_LICENSE_CONF_FILENAME);

  if ((fptr = fopen (conf_fname, "rb")) != NULL)
    {
      while (get_line (line, sizeof (line), fptr) != NULL)
        {
          char *pch0, *pch1;
          /* Find the line with the license directory */
          if (strstr (line, MCHP_LICENSEPATH_MARKER))
            {
              /* Find the quoted string on that line */
              pch0 = strchr (line,'"') +1;
              pch1 = strrchr (line,'"');
              if ((pch1-pch0) > 2)
                strncpy (xclmpath, pch0, pch1-pch0);
              break;
            }
        }
      /* Append the xclm executable name to the directory. */
      if (xclmpath [strlen (xclmpath) - 1] != '/'
          && xclmpath [strlen (xclmpath) - 1] != '\\')
        strcat (xclmpath, "/");
      strcat (xclmpath, MCHP_XCLM_FILENAME);

    }

  if (fptr != NULL)
    {
      fclose (fptr);
      fptr = NULL;
    }
#endif /* MCHP_USE_LICENSE_CONF */

#ifdef MCHP_USE_LICENSE_CONF
  if (-1 == stat (xclmpath, &filestat))
#endif /* MCHP_USE_LICENSE_CONF */
    {
      /*  Try the compiler bin directory.
       *
       */
      bindir = make_relative_prefix(save_decoded_options[0].arg,
                                          "/lib/gcc/pic32c/"
                                          str(BUILDING_GCC_MAJOR) "."
                                          str(BUILDING_GCC_MINOR) "."
                                          str(BUILDING_GCC_PATCHLEVEL),
                                          "/bin");
#if defined(MCHP_DEBUG)
      fprintf(stderr, "bindir = %s\n", bindir);
#endif

      xclmpath_length = strlen(bindir) + 1 + strlen(MCHP_XCLM_FILENAME) + 1;
      xclmpath = (char*)xcalloc(xclmpath_length+1,sizeof(char));
      strncpy (xclmpath, bindir, xclmpath_length);
      /* Append the xclm executable name to the directory. */
      if (xclmpath [strlen (xclmpath) - 1] != '/'
          && xclmpath [strlen (xclmpath) - 1] != '\\')
        strcat (xclmpath, "/");
      strcat (xclmpath, MCHP_XCLM_FILENAME);

      if (-1 == stat (xclmpath, &filestat))
        {
          free (xclmpath);
          /*  Try the old common directory
           */
          xclmpath_length = strlen("/opt/Microchip/xclm/bin/") + strlen(MCHP_XCLM_FILENAME) + 1;
          xclmpath = (char*)xcalloc(xclmpath_length+1,sizeof(char));
          strncpy (xclmpath, "/opt/Microchip/xclm/bin/", xclmpath_length);
          /* Append the xclm executable name to the directory. */
          strcat (xclmpath, MCHP_XCLM_FILENAME);
        }
      if (-1 == stat (xclmpath, &filestat))
        {
          /*  Try the build directory
           */
          strncpy (xclmpath, "/build/xc32/xclm/bin/", xclmpath_length);
          /* Append the xclm executable name to the directory. */
          strcat (xclmpath, MCHP_XCLM_FILENAME);
        }
    }

#if defined(__MINGW32__)
  {
    char *convert;
    convert = xclmpath;
    while (*convert != '\0')
      {
        if (*convert == '\\')
          *convert = '/';
        convert++;
      }
  }
#endif /* __MINGW32__ */

  if (-1 == stat (xclmpath, &filestat))
    return NULL;

#if defined(MCHP_DEBUG)
  fprintf (stderr, "%d Final xclmpath: %s\n", __LINE__, xclmpath);
#endif /* MCHP_DEBUG */
  return xclmpath;
}
#ifdef MCHP_USE_LICENSE_CONF
#undef MCHP_MAX_LICENSEPATH_LINE_LENGTH
#undef MCHP_LICENSE_CONF_FILENAME
#undef MCHP_LICENSEPATH_MARKER
#endif /* MCHP_USE_LICENSE_CONF */

#undef MCHP_XCLM_FILENAME

#endif /* SKIP_LICENSE_MANAGER */



static int
pic32_get_license (int require_cpp)
{
  /*
   *  On systems where we have a licence manager, call it
   */

/* Misc. Return Codes */

#ifndef MCHP_XCLM_EXPIRED_DEMO
#define MCHP_XCLM_EXPIRED_DEMO 0x10
#endif /* MCHP_XCLM_EXPIRED_DEMO*/

#if defined(MCHP_XCLM_VALID_CPP_FREE)
#define PIC32_EXPIRED_LICENSE MCHP_XCLM_EXPIRED_DEMO
#define PIC32_FREE_LICENSE MCHP_XCLM_FREE_LICENSE
#define PIC32_VALID_STANDARD_LICENSE MCHP_XCLM_VALID_STANDARD_LICENSE
#define PIC32_VALID_PRO_LICENSE MCHP_XCLM_VALID_PRO_LICENSE
#define PIC32_NO_CPP_LICENSE MCHP_XCLM_NO_CPP_LICENSE
#define PIC32_VALID_CPP_FREE MCHP_XCLM_VALID_CPP_FREE
#define PIC32_VALID_CPP_FULL MCHP_XCLM_VALID_CPP_FULL
#else
#define PIC32_EXPIRED_LICENSE MCHP_XCLM_EXPIRED_DEMO
#define PIC32_FREE_LICENSE 0
#define PIC32_VALID_STANDARD_LICENSE 1
#define PIC32_VALID_PRO_LICENSE 2
#define PIC32_NO_CPP_LICENSE 4
#define PIC32_VALID_CPP_FREE 5
#define PIC32_VALID_CPP_FULL 6
#endif /* MCHP_XCLM_VALID_CPP_FREE */


#ifndef SKIP_LICENSE_MANAGER
  {
    char *exec;

#if XCLM_FULL_CHECKOUT
    char kopt[] = "-fcfc";
#else
    char kopt[] = "-checkout";
#endif /* XCLM_FULL_CHECKOUT */

    char productc[]   = "swxc32";
    char productcpp[] = "swxcpp32";
    char version[9] = "";
    char date[] = __DATE__;

#if XCLM_FULL_CHECKOUT
    char * args[] = { NULL, NULL, NULL, NULL, NULL, NULL};
#else
    char * args[] = { NULL, NULL, NULL, NULL, NULL};
#endif /* XCLM_FULL_CHECKOUT */

    //char *err_msg=(char*)"", *err_arg=(char*)"";
    const char *failure = NULL;
    int status = 0;
    int err = 0;
    int major_ver =0, minor_ver=0;
    //extern struct cl_decoded_option *save_decoded_options;
    struct stat filestat;
    int found_xclm = 0, xclm_tampered = 1;

    /* Get the version number string from the entire version string */
    if ((version_string != NULL) && *version_string)
      {
        char *Microchip;
        gcc_assert(strlen(version_string) < 80);
        Microchip = strrchr ((char*)version_string, 'v');
        if (Microchip)
          {
            while ((*Microchip) &&
                   ((*Microchip < '0') ||
                    (*Microchip > '9')))
              {
                Microchip++;
              }
            if (*Microchip)
              {
                major_ver = strtol (Microchip, &Microchip, 0);
              }
            if ((*Microchip) &&
                ((*Microchip=='_') || (*Microchip=='.')))
              {
                Microchip++;
                minor_ver = strtol(Microchip, &Microchip, 0);
              }
          }
        snprintf (version, 6, "%d.%02d", major_ver, minor_ver);
      }

    /* Arguments to pass to xclm */
    args[1] = kopt;
    if (require_cpp)
      args[2] = productcpp;
    else
      args[2] = productc;
    args[3] = version;
    
#if XCLM_FULL_CHECKOUT
    args[4] = date;
#endif /* XCLM_FULL_CHECKOUT */

    /* Get a path to the license manager to try */
    exec = get_license_manager_path();

#if defined(MCHP_DEBUG)
    fprintf (stderr, "exec is %s\n", exec);
#endif

    if (exec == NULL)
      {
         /*Set free edition if the license manager isn't available.*/
        mchp_pic32_license_valid=PIC32_FREE_LICENSE;
        warning (0, "Could not retrieve compiler license");
        inform (input_location, "Please reinstall the compiler");
      }
    else if (-1 == stat (exec, &filestat))
      {
         /*Set free edition if the license manager execution fails. */
        mchp_pic32_license_valid=PIC32_FREE_LICENSE;
        warning (0, "Could not retrieve compiler license");
        inform (input_location, "Please reinstall the compiler");
      }
    else
      {
        /* Found xclm */
          found_xclm = 1;
      }

#undef xstr
#undef str
#undef MCHP_XCLM_SHA256_DIGEST_QUOTED

#define xstr(s) str(s)
#define str(s) #s

#define MCHP_XCLM_SHA256_DIGEST_QUOTED xstr(MCHP_XCLM_SHA256_DIGEST)

    /* Verify SHA sum and call xclm to determine the license */
    if (found_xclm && mchp_pic32_license_valid==-1 && !TARGET_SKIP_LICENSE_CHECK)
      {
        /* Verify that xclm executable is untampered */
        xclm_tampered = mchp_sha256_validate(exec, (const unsigned char*)MCHP_XCLM_SHA256_DIGEST_QUOTED);
        if (xclm_tampered != 0)
          {
            /* Set free edition if the license manager SHA digest does not
               match. The free edition disables optimization options without an
               eval period. */
            mchp_pic32_license_valid=PIC32_FREE_LICENSE;
            warning (0, "Detected corrupt executable file");
            inform (input_location, "Please reinstall the compiler");
          }
        else
          {
            args[0] = exec;
            failure = pex_one(0, exec, args, "MPLAB XC32 Compiler", 0, 0, &status, &err);

            if (failure != NULL)
              {
                /* Set free edition if the license manager isn't available.
                   The free edition disables optimization options without an
                   eval period. */
                mchp_pic32_license_valid=PIC32_FREE_LICENSE;
                warning (0, "Could not retrieve compiler license (%s)", failure);
                inform (input_location, "Please reinstall the compiler");
              }
            else if (WIFEXITED(status))
              {
                mchp_pic32_license_valid = WEXITSTATUS(status);
                if (mchp_pic32_license_valid == PIC32_NO_CPP_LICENSE)
                  {
                     mchp_pic32_license_valid = PIC32_VALID_CPP_FREE;
                  }
              }
          }
      }
  }
  
#undef xstr
#undef str
#undef MCHP_XCLM_SHA256_DIGEST_QUOTED

#endif /* SKIP_LICENSE_MANAGER */

  return mchp_pic32_license_valid;
}


void mchp_override_options_after_change(void) {
    if (nullify_Os)
      {
        /* Disable -Os optimization(s) */
        /* flag_web and flag_inline_functions already disabled */
        if (optimize_size)
          {
            optimize = 2;
          }
        NULLIFY(optimize_size, "Optimize for size") = 0;
      }
    if (nullify_O2)
      {
        int opt1_max;
        /* Disable -O2 optimizations */
        if (optimize > 1)
          {
            NULLIFY(optimize, "Optimization level > 1") = 1;
          }
        NULLIFY(flag_indirect_inlining, "indirect inlining") = 0;
        NULLIFY(flag_thread_jumps, "thread jumps") = 0;
        NULLIFY(flag_crossjumping, "crossjumping") = 0;
        NULLIFY(flag_optimize_sibling_calls, "optimize sibling calls") = 0;
        NULLIFY(flag_cse_follow_jumps, "cse follow jumps") = 0;
        NULLIFY(flag_gcse, "gcse") = 0;
        NULLIFY(flag_expensive_optimizations, "expensive optimizations") = 0;
        NULLIFY(flag_rerun_cse_after_loop, "cse after loop") = 0;
        NULLIFY(flag_caller_saves, "caller saves") = 0;
        NULLIFY(flag_peephole2, "peephole2") = 0;
#ifdef INSN_SCHEDULING
        NULLIFY(flag_schedule_insns, "schedule insns") = 0;
        NULLIFY(flag_schedule_insns_after_reload, "schedule insns after reload") = 0;
#endif
        NULLIFY(flag_strict_aliasing, "strict aliasing") = 0;
        NULLIFY(flag_strict_overflow, "strict overflow") = 0;
        NULLIFY(flag_reorder_functions, "reorder functions") = 0;
        NULLIFY(flag_tree_vrp, "tree vrp") = 0;
        NULLIFY(flag_tree_pre, "tree pre") = 0;
        NULLIFY(flag_tree_switch_conversion, "tree switch conversion") = 0;
        NULLIFY(flag_ipa_cp, "ipa cp") = 0;
        NULLIFY(flag_ipa_sra, "ipa sra") = 0;

        /* Just -O1/-O0 optimizations.  */
        opt1_max = (optimize <= 1);
        align_loops = opt1_max;
        align_jumps = opt1_max;
        align_labels = opt1_max;
        align_functions = opt1_max;
      }
    if (nullify_O3)
      {
        if (optimize >= 3)
          {
            NULLIFY(optimize, "Optimization level > 2") = 2;
          }
        /* Disable -O3 optimizations */
        NULLIFY(flag_predictive_commoning, "predictive commoning") = 0;
        NULLIFY(flag_inline_functions, "inline functions") = 0;
        NULLIFY(flag_unswitch_loops, "unswitch loops") = 0;
        NULLIFY(flag_gcse_after_reload, "gcse after reload") = 0;
        NULLIFY(flag_tree_vectorize, "tree vectorize") = 0;
        NULLIFY(flag_ipa_cp_clone, "ipa cp clone") = 0;
        flag_ipa_cp = 0;
       }

    if (nullify_lto)
      {
        NULLIFY(flag_lto, "Link-time optimization") = 0;
        NULLIFY(flag_whole_program, "Whole-program optimizations") = 0;
        NULLIFY(flag_generate_lto, "Link-time optimization") = 0;
      }
}

static void mchp_print_license_warning (void)
{
    switch (mchp_pic32_license_valid)
      {
      case PIC32_EXPIRED_LICENSE:
        invalid_license = "because the XC32 evaluation period has expired";
        break;
      case PIC32_FREE_LICENSE:
        invalid_license = "because the free XC32 C compiler does not support this feature.";
        break;
      case PIC32_VALID_CPP_FREE:
        invalid_license = "because the free XC32 C++ compiler does not support this feature.";
        break;
      case PIC32_VALID_STANDARD_LICENSE:
        invalid_license = "because this feature requires the MPLAB XC32 PRO compiler";
        break;
      default:
        invalid_license = "due to an invalid XC32 license";
        break;
      }

    if (message_displayed && TARGET_LICENSE_WARNING)
      {
        /* Display a warning for the Standard option first */
        if (disabled_option_message != NULL)
          warning (0,"Compiler option (%s) ignored %s",
                   disabled_option_message, invalid_license);
        disabled_option_message = NULL;
        message_purchase_display++;
      }
}

void pic32c_subtarget_override_options(void)
{
 
  extern struct cl_decoded_option *save_decoded_options;
  require_cpp    = 0;

#ifndef SKIP_LICENSE_MANAGER
  nullify_O2     = 1;
  nullify_O3     = 1;
  nullify_Os     = 1;
  nullify_lto    = 1;
#endif /* SKIP_LICENSE_MANAGER */

  if (TARGET_SKIP_LICENSE_CHECK) {
    TARGET_LICENSE_WARNING = 0;
  }

  require_cpp = (strstr (save_decoded_options[0].arg, "cc1plus")!=NULL);
  if (TARGET_SKIP_LICENSE_CHECK)
  {
    mchp_pic32_license_valid = PIC32_VALID_CPP_FREE;
  }
  else 
  {
    mchp_pic32_license_valid = pic32_get_license (require_cpp);
  }

  if (require_cpp && !((mchp_pic32_license_valid == PIC32_VALID_CPP_FREE) ||
                       (mchp_pic32_license_valid == PIC32_VALID_CPP_FULL)))
  {
    error  ("MPLAB XC32 C++ license not activated");
    inform (input_location, "Visit http://www.microchip.com/MPLABXCcompilers to acquire a "
         "free C++ license");
  }
       
  if ((mchp_pic32_license_valid == PIC32_VALID_PRO_LICENSE) ||
      (mchp_pic32_license_valid == PIC32_VALID_CPP_FULL))
  {
    nullify_lto = nullify_O2 =  nullify_O3 = nullify_Os = 0;
  }
  else if (mchp_pic32_license_valid == PIC32_VALID_STANDARD_LICENSE)
  {
    nullify_O2 = 0;
  }
  
  /*
   *  On systems where we have a licence manager, call it
   */

  {
    /*
     * Prior to v1.40 (gcc upgrade from 4.5.2 to 4.8.3),
     * mchp_override_options_after_change() was called from optimization_option()
     * through OPTIMIZATION_OPTION target macro which is poisoned in 4.8.3. Hence
     * optimization_option() was removed. Call to  mchp_override_options_after_change()
     * added here to fix XC32-546
     */

    mchp_override_options_after_change();
    if (TARGET_LICENSE_WARNING)
    {
      mchp_print_license_warning();
    }
    
    if (nullify_lto)
    {
      if (optimize_size)
      {
        optimize = 2;
      }
      if (optimize >= 3)
      {
        NULLIFY(optimize, "Optimization level > 2") = 2;
      }
      
      NULLIFY(optimize_size, "Optimize for size") = 0;
      NULLIFY(flag_predictive_commoning, "predictive commoning") = 0;
      NULLIFY(flag_inline_functions, "inline functions") = 0;
      NULLIFY(flag_unswitch_loops, "unswitch loops") = 0;
      NULLIFY(flag_gcse_after_reload, "gcse after reload") = 0;
      NULLIFY(flag_tree_vectorize, "tree vectorize") = 0;
      NULLIFY(flag_ipa_cp_clone, "ipa cp clone") = 0;

      flag_ipa_cp = 0;

      NULLIFY(flag_lto, "Link-time optimization") = 0;
      NULLIFY(flag_whole_program, "Whole-program optimizations") = 0;
      NULLIFY(flag_generate_lto, "Link-time optimization") = 0;
    }
    
    if (message_displayed && TARGET_LICENSE_WARNING)
    {
      /*Now display a warning for the Pro option */
      if (disabled_option_message != NULL)
        warning (0,"Pro Compiler option (%s) ignored %s", disabled_option_message,
                   invalid_license);
      message_purchase_display++;
      message_displayed=0;
    }
    
    if ((message_purchase_display > 0) && (TARGET_LICENSE_WARNING))
    {
      inform (0, "Disable the option or visit http://www.microchip.com/MPLABXCcompilers "
                "to purchase a new MPLAB XC compiler license.");

      /*If the --nofallback option was specified, abort compilation. */
      if (TARGET_NO_FALLBACKLICENSE)
          error ("Unable to find a valid license, aborting");

      message_purchase_display = 0;
    }

    /*Require a Standard or Pro license */
    if (TARGET_NO_FALLBACKLICENSE && 
        ((mchp_pic32_license_valid == PIC32_FREE_LICENSE) || (mchp_pic32_license_valid == PIC32_VALID_CPP_FREE)))
      error ("Unable to find a valid license, aborting");
  }
 
#undef PIC32_EXPIRED_LICENSE
#undef PIC32_ACADEMIC_LICENSE
#undef PIC32_VALID_STANDARD_LICENSE
#undef PIC32_VALID_PRO_LICENSE

}

#undef NULLIFY
