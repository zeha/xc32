dnl Process this file with autoconf to produce a configure script

AC_PREREQ(2.57)
AC_INIT(fileio.c)

# This works around the fact that libtool configuration may change LD
# for this particular configuration, but some shells, instead of
# keeping the changes in LD private, export them just because LD is
# exported.  We don't use libtool yet, but some day we might, so...
ORIGINAL_LD_FOR_MULTILIBS=$LD

dnl We use these options to decide which functions to include.
AC_ARG_WITH(target-subdir,
[  --with-target-subdir=SUBDIR      Configuring in a subdirectory for target])
AC_ARG_WITH(build-subdir,
[  --with-build-subdir=SUBDIR       Configuring in a subdirectory for build])
AC_ARG_WITH(cross-host,
[  --with-cross-host=HOST           Configuring with a cross compiler])

if test "${srcdir}" = "."; then
  if test -n "${with_build_subdir}"; then
    sdemdi_topdir="${srcdir}/../.."
    with_target_subdir=
  elif test -z "${with_target_subdir}"; then
    sdemdi_topdir="${srcdir}/.."
  else
    if test "${with_target_subdir}" != "."; then
      sdemdi_topdir="${srcdir}/${with_multisrctop}../.."
    else
      sdemdi_topdir="${srcdir}/${with_multisrctop}.."
    fi
  fi
else
  sdemdi_topdir="${srcdir}/.."
fi
AC_SUBST(sdemdi_topdir)
AC_CONFIG_AUX_DIR($sdemdi_topdir)

dnl Very limited version of automake's enable-maintainer-mode
AC_MSG_CHECKING([whether to enable maintainer-specific portions of Makefiles])
  dnl maintainer-mode is disabled by default
  AC_ARG_ENABLE(maintainer-mode,
[  --enable-maintainer-mode
                          enable make rules and dependencies not useful
                          (and sometimes confusing) to the casual installer],
      maintainer_mode=$enableval,
      maintainer_mode=no)

AC_MSG_RESULT($maintainer_mode)

if test "$maintainer_mode" = "yes"; then
  MAINT=''
  NOTMAINT='#'
else
  MAINT='#'
  NOTMAINT=''
fi
AC_SUBST(MAINT)dnl
AC_SUBST(NOTMAINT)dnl

AC_CANONICAL_HOST

dnl When we start using automake:
dnl AM_INIT_AUTOMAKE(sdemdi, 1.0)

dnl These must be called before AM_PROG_LIBTOOL, because it may want
dnl to call AC_CHECK_PROG.
AC_CHECK_TOOL(AR, ar)
AC_CHECK_TOOL(RANLIB, ranlib, :)

AC_PROG_CC

dnl When we start using automake:
dnl AM_MAINTAINER_MODE
dnl AC_EXEEXT

dnl When we start using automake:
dnl AM_PROG_INSTALL
AC_PROG_INSTALL

# We need multilib support, but only if configuring for the target.
AC_OUTPUT(Makefile,
[test -z "$CONFIG_HEADERS" || echo timestamp > stamp-h
if test -n "$CONFIG_FILES"; then
  if test -n "${with_build_subdir}" || test -n "${with_target_subdir}"; then
    # FIXME: We shouldn't need to set ac_file
    ac_file=Makefile
    LD="${ORIGINAL_LD_FOR_MULTILIBS}"
    . ${sdemdi_topdir}/config-ml.in
  fi
fi],
srcdir=${srcdir}
host=${host}
target=${target}
with_target_subdir=${with_target_subdir}
with_build_subdir=${with_build_subdir}
with_multisubdir=${with_multisubdir}
ac_configure_args="--enable-multilib ${ac_configure_args}"
CONFIG_SHELL=${CONFIG_SHELL-/bin/sh}
ORIGINAL_LD_FOR_MULTILIBS="${ORIGINAL_LD_FOR_MULTILIBS}"
sdemdi_topdir=${sdemdi_topdir}
)
