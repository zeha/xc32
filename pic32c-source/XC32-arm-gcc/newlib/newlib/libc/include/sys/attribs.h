/*********************************************************************
 *
 *              Macros for function and variable attributes
 *
 *********************************************************************
 * Filename:        sys/attribs.h
 *
 * Processor:       PIC32C
 *
 * Compiler:        MPLAB XC32
 *
 * Company:         Microchip Technology Inc.
 *
 * Software License Agreement
 *
 * Copyright (c) 2017, Microchip Technology Inc. and its subsidiaries ("Microchip")
 * All rights reserved.
 *
 * This software is developed by Microchip Technology Inc. and its
 * subsidiaries ("Microchip").
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1.      Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2.      Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * 3.      Microchip's name may not be used to endorse or promote products
 * derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MICROCHIP "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * MICROCHIP BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING BUT NOT LIMITED TO
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWSOEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *-------------------------------------------------------------------------*/

#pragma once
#ifndef __ATTRIBS_H
#define __ATTRIBS_H

/*****************************************************************************
 * The following macro -- __section__ -- is used to specify a named section
 * for the attributed object. The object will be placed into that section.
 *
 * Example:
 *   int __section__(".fred") foo (void) { return bar; }
 * This creates a function named foo that will be placed into section
 * ".fred"
 *****************************************************************************/
#define __section__(n) __attribute__ ((section (n)))

/*****************************************************************************
 * The following macro -- __unique_section__ -- is used to specify that the
 * section name for the attributed object should be unique, just as if
 * the command line option -ffunction-sections or -fdata-sections had been
 * specified. In addition, if a "section" attribute is also present, the name
 * of the section will be used as the prefix for the unique section name
 * generated.
 *
 * Example:
 *   int __unique_section__ foo (void) { return bar; }
 * Function "foo" will be placed into a uniquely named text section just
 * as if the translation unit were compiled with -ffunction-sections.
 *****************************************************************************/
#define __unique_section__ __attribute__ ((unique_section))

/*****************************************************************************
 * The following macro -- __ramfunc__ -- is used to specify a function that
 * will execute from data memory. This function may be called from other
 * functions located within data memory.
 *
 * Example:
 *   int __ramfunc__ foo (void);
 *   int __ramfunc__ foo (void) { return bar; }
 * This creates a function named foo that will execute from data memory.
 * This function may be called from other functions located within data
 * memory.
 *****************************************************************************/
#define __ramfunc__ __attribute__ ((space(data),unique_section,noinline))

#endif /* __ATTRIBS_H */

