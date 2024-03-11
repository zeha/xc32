/* Copyright (C) 1997-2018 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#include "version.h"

/* MERGE-NOTES:
   This had a bunch of hardcoded stuff. I don't want to port it. We
   can use the configuration to handle this. 
   In particular, we want to use PKGVERSION, set via --with-pkgversion
   in the configuration, to set the 'microchip' part of the output.
   */

/* This is the location of the online document giving instructions for
   reporting bugs.  If you distribute a modified version of GCC,
   please configure with --with-bugurl pointing to a document giving
   instructions for reporting bugs to you, not us.  (You are of course
   welcome to forward us bugs reported to you, if you determine that
   they are not bugs in your modifications.)  */

const char bug_report_url[] = BUGURL;

/* The complete version string, assembled from several pieces.
   BASEVER, DATESTAMP, DEVPHASE, and REVISION are defined by the
   Makefile.  */

#ifdef _BUILD_MCHP_
const char version_string[] = BASEVER;
#else
const char version_string[] = BASEVER DATESTAMP DEVPHASE REVISION;
#endif /* _BUILD_MCHP_ */
const char pkgversion_string[] = PKGVERSION;
