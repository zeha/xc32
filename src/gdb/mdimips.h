/* MDI (Microprocessor Debug Interface) MIPS specific definitions

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

#ifndef MDI_MIPS_Specification_Definitions
#define MDI_MIPS_Specification_Definitions

/* Valid values for MDIDDataT.FClass: */
#define MDIMIP_FClass       "MIPS"
/* Valid values for MDIDDataT.FISA: */
#define MDIMIP_FISA_M1      "MIPSI"
#define MDIMIP_FISA_M2      "MIPSII"
#define MDIMIP_FISA_M3      "MIPSIII"
#define MDIMIP_FISA_M4      "MIPSIV"
#define MDIMIP_FISA_M5      "MIPSV"
#define MDIMIP_FISA_M32     "MIPS32"
#define MDIMIP_FISA_M64     "MIPS64"

/* Valid values for Resource */
#define MDIMIPCPU           1
#define MDIMIPPC            2
#define MDIMIPHILO          3
#define MDIMIPTLB           4
#define MDIMIPPICACHET      5
#define MDIMIPPUCACHET      5
#define MDIMIPPDCACHET      6
#define MDIMIPSICACHET      7
#define MDIMIPSUCACHET      7
#define MDIMIPSDCACHET      8
#define MDIMIP192ACC        9
#define MDIMIPCP0           10
#define MDIMIPCP0C          11
#define MDIMIPCP1           12
#define MDIMIPCP1C          13
#define MDIMIPCP2           14
#define MDIMIPCP2C          15
#define MDIMIPCP3           16
#define MDIMIPCP3C          17
#define MDIMIPFP            18
#define MDIMIPDFP           19
#define MDIMIPPICACHE       20
#define MDIMIPPUCACHE       20
#define MDIMIPPDCACHE       21
#define MDIMIPSICACHE       22
#define MDIMIPSUCACHE       22
#define MDIMIPSDCACHE       23
#define MDIMIPPHYSICAL      24
#define MDIMIPGVIRTUAL      25
#define MDIMIPEJTAG         26
#define MDIMIPSRS           27
#define MDIMIPFPR           28
#define MDIMIPDSP           29
#define MDIMIPTICACHET      30
#define MDIMIPTUCACHET      31
#define MDIMIPTDCACHET      32
#define MDIMIPTICACHE       33
#define MDIMIPTUCACHE       34
#define MDIMIPTDCACHE       35
#define MDIMIPITCVIRTUAL    36
#define MDIMIPHWR           37

#define MDIMIPVIRTUAL       0x00001000  /* 0x10xx: 0x1000+ASID value */

/* For MDISetBp(), MDISetSWBp(), and MDITraceRead(), setting the low
   order address bit to 1 means that the addressed instruction is a
   MIPS16 instruction. */
#define MDIMIP_Flg_MIPS16   1

/* Select codes for MIPSsim_GetStats () */
#define MIPS_MDI_PerfCycles               1
#define MIPS_MDI_PerfInstructionsExecuted 2
#define MIPS_MDI_PerfBranchesTaken        3
#define MIPS_MDI_PerfLoadInstructions     4
#define MIPS_MDI_PerfStoreInstructions    5
#define MIPS_MDI_PerfIUncachedAccesses    6
#define MIPS_MDI_PerfPICacheMisses        7
#define MIPS_MDI_PerfPICacheHits          8
#define MIPS_MDI_PerfDUncachedAccesses    9
#define MIPS_MDI_PerfPDCacheMisses        10
#define MIPS_MDI_PerfPDCacheHits          11
#define MIPS_MDI_PerfPDCacheLoadMisses    12
#define MIPS_MDI_PerfUnmappedAccesses     13
#define MIPS_MDI_PerfTLBMisses            14
#define MIPS_MDI_PerfTLBHits              15
#define MIPS_MDI_PerfISpramHits           16
#define MIPS_MDI_PerfDSpramHits           17

/* MIPSsim 4.x performance counter modes */
#define MIPS_E_PerfCntMode_UM      0x01
#define MIPS_E_PerfCntMode_SM      0x02
#define MIPS_E_PerfCntMode_KM      0x04
#define MIPS_E_PerfCntMode_EXL     0x08
#define MIPS_E_PerfCntMode_ERL     0x10
#define MIPS_E_PerfCntMode_EXLERL  0x20
#define MIPS_E_PerfCntMode_ALL     0xffff

/* MIPSsim 4.x performance counters (a selection) */
#define MIPS_PerfCntSel_Cycles0           0
#define MIPS_PerfCntInd_Cycles0           0
#define MIPS_PerfCntSel_Instr0            0
#define MIPS_PerfCntInd_Instr0            1

#endif

