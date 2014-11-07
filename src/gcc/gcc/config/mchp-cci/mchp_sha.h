/* Subroutines used for Microchip PIC32 MCU support
   Copyright (C) 2014  
   Microchip Technology, Inc.
   Contributed by S. Bekal, swaroopkumar.bekal@microchip.com

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
alng with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#ifndef MCHP_SHA_H
#define MCHP_SHA_H

/* SHA-256 digest of xclm executables for Win, Linux and OS X */
#if defined(__MINGW32__)
#define MCHP_XCLM_SHA256_DIGEST   "9ee3489491c426025cf8d302494f899a2a049c3f45a235be00a2d63e90df9cb7"
#elif defined(__linux__)
#define MCHP_XCLM_SHA256_DIGEST   "4fae1d53570ebffe637d8c5f77789ecbe690f67af65d0e3af8474bfee479239e"
#elif defined(__MACH__)
#define MCHP_XCLM_SHA256_DIGEST   "595a7a77c69ee1a9b784af8908fe665bd5bcfbdf81abad8079e48c9c4ef8934e" 
#else 
#error "Unknown host"
#endif

/* Function to calculate the SHA256 digest of given file
 * Parameters :
 *     - Absolute path to file
 *     - Buffer to hold SHA256 digest 
 * Return value :
 *     - 0 on success, non-zero on any error
*/
int mchp_sha256( const char *path, unsigned char output[32] );

/* Function to validate if the SHA256 digest of given file
 * mathces the suppiled value
 * Parameters :
 *     - Absolute path to file
 *     - Pre-calculated SHA256 digest of the file (null terminated)
 * Return value :
 *     - 0 on match, non-zero on mis-match 
*/
int mchp_sha256_validate( const char *path, unsigned const char *sha2);

#endif 
