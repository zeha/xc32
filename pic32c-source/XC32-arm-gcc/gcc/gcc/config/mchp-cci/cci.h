/* Subroutines used for Microchip PIC32 MCU support
   Copyright (C) 2017, 2018
   Microchip Technology, Inc.

This file is part of Microchip XC compiler tool-chain.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#ifndef CCI

enum CCI_KIND {
  CCI_unknown,     /* should never happen, I don't like 0 being a valid enum */
  CCI_set_value,   /* For KEYWORD pass the address of the value you wish to set
                    *
                    *  KEYWORD = N;
                    *
                    * Great for setting flags or options...
                    */
  CCI_define,      /* straight define:
                    *
                    * #define KEYWORD=CCI_VAL
                    */
  CCI_attribute,   /* attribute define:
                    *
                    * #define KEYWORD=__attribute__((CCI_VAL)))
                    */
  CCI_attribute_n, /* attribute define:
                    *
                    * #define KEYWORD(P1,P2,...Pn)= \
                    *   __attribute__((CCI_VAL(P1,P2,...Pn)))
                    *
                    * if P1 (all or none) is specified in the conversion, then
                    *   that will be used otherwise the compiler will generate
                    *   the parenthetical expression after CCI_VAL
                    */
  CCI_attribute_v, /* attribute define:
                    *
                    * #define KEYWORD(...)=__attribute__((CCI_VAL(__VA_ARGS__)))
                    */
  CCI_attribute_nv, /* attribute define:
                    *
                    * #define KEYWORD(P1,P2,...Pn,...)= \
                    *   __attribute__((CCI_VAL(P1,P2,...Pn,__VA_ARGS__)))
                    *
                    * if P1 (all or none) is specified in the conversion, then
                    *   that will be used otherwise the compiler will generate
                    *   the parenthetical expression after CCI_VAL
                    */
  CCI_pragma       /* pragma define, maps a pragma to an already handled
                    *   native pragma, the CCI_for_xxx should be a function
                    *   name
                    */
};

// NOTE: -mext= (that is, mchp_import) is not currently supported by PIC32C so
// CCI_MCHP will depend only on the presence/absence of the -mcci option (TARGET_CCI)
//#define IMPORT_MCHP(V) ((mchp_import && strcasecmp(mchp_import,(V)) == 0))
#define CCI_MCHP (TARGET_CCI/* || IMPORT_MCHP("cci")*/)

#else /* CCI is defined */

// For use as an empty #define
#define TGT_EMPTY_DEF 0

// Two special cases for unsupported/not available CCI keywords on this target
// (the first, if encountered, generates a warning, the other one an error)
// NOTE: The warnings/errors are generated only on uses (i.e. not on declarations)
#define TGT_UNSUPPORTED "#unsup"
#define TGT_ERROR       "#error"

// NOTE: The following CCI extensions are implicitly predicated on CCI_MCHP
// (there's a single test for that in mchp_init_cci () that either enables all
// of them or none)

/*  CCI_KIND        CCI Keyword,       TGT Keyword,     N             */
/*  ------------------------------------------------------------------*/
CCI (define,        "__CCI__",         "1",             0)
CCI (attribute_n,   "__align",         "aligned",       1)
CCI (attribute_n,   "__at",            "address",       1)
CCI (define,        "__bank(X)",       TGT_EMPTY_DEF,   0)
CCI (attribute,     "__deprecate",     "deprecated",    0)
CCI (attribute,     "__eeprom",        TGT_UNSUPPORTED, 0)
CCI (attribute,     "__far",           TGT_UNSUPPORTED, 0)
CCI (attribute_n,   "__interrupt",     "interrupt",     1)
CCI (attribute,     "__near",          TGT_UNSUPPORTED, 0)
CCI (attribute,     "__pack",          "packed",        0)
CCI (attribute,     "__persistent",    "persistent",    0)
CCI (attribute_n,   "__section",       "section",       1)
CCI (define,        "__xdata",         TGT_EMPTY_DEF,   0)
CCI (define,        "__ydata",         TGT_EMPTY_DEF,   0)

#endif /* CCI */
