/* vms.h -- Header file for VMS (Alpha and Vax) support.
   Copyright 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2005, 2007,
   2008, 2009 Free Software Foundation, Inc.

   Main header file.

   Written by Klaus K"ampf (kkaempf@rmi.de)

   This file is part of BFD, the Binary File Descriptor library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#undef vms
#ifndef VMS_H
#define VMS_H

/* Constants starting with 'Exxx_' are for openVMS/Alpha (EVAX object
   language).  */

#define VMS_BLOCK_SIZE  512

/* VMS Text Information and Relocation Records (TIR/ETIR).  */

#define TIR_S_C_STA_GBL		0
#define TIR_S_C_STA_SB		1
#define TIR_S_C_STA_SW		2
#define TIR_S_C_STA_LW		3
#define TIR_S_C_STA_PB		4
#define TIR_S_C_STA_PW		5
#define TIR_S_C_STA_PL		6
#define TIR_S_C_STA_UB  	7
#define TIR_S_C_STA_UW  	8
#define TIR_S_C_STA_BFI 	9
#define TIR_S_C_STA_WFI 	10
#define TIR_S_C_STA_LFI 	11
#define TIR_S_C_STA_EPM 	12
#define TIR_S_C_STA_CKARG       13
#define TIR_S_C_STA_WPB 	14
#define TIR_S_C_STA_WPW 	15
#define TIR_S_C_STA_WPL 	16
#define TIR_S_C_STA_LSY 	17
#define TIR_S_C_STA_LIT 	18
#define TIR_S_C_STA_LEPM        19
#define TIR_S_C_MAXSTACOD       19
#define TIR_S_C_MINSTOCOD       20
#define TIR_S_C_STO_SB  	20
#define TIR_S_C_STO_SW  	21
#define TIR_S_C_STO_L   	22
#define TIR_S_C_STO_LW  	22
#define TIR_S_C_STO_BD  	23
#define TIR_S_C_STO_WD  	24
#define TIR_S_C_STO_LD  	25
#define TIR_S_C_STO_LI  	26
#define TIR_S_C_STO_PIDR        27
#define TIR_S_C_STO_PICR        28
#define TIR_S_C_STO_RSB 	29
#define TIR_S_C_STO_RSW 	30
#define TIR_S_C_STO_RL  	31
#define TIR_S_C_STO_VPS 	32
#define TIR_S_C_STO_USB 	33
#define TIR_S_C_STO_USW 	34
#define TIR_S_C_STO_RUB 	35
#define TIR_S_C_STO_RUW 	36
#define TIR_S_C_STO_B   	37
#define TIR_S_C_STO_W   	38
#define TIR_S_C_STO_RB  	39
#define TIR_S_C_STO_RW  	40
#define TIR_S_C_STO_RIVB        41
#define TIR_S_C_STO_PIRR        42
#define TIR_S_C_MAXSTOCOD       42
#define TIR_S_C_MINOPRCOD       50
#define TIR_S_C_OPR_NOP 	50
#define TIR_S_C_OPR_ADD 	51
#define TIR_S_C_OPR_SUB 	52
#define TIR_S_C_OPR_MUL 	53
#define TIR_S_C_OPR_DIV 	54
#define TIR_S_C_OPR_AND 	55
#define TIR_S_C_OPR_IOR 	56
#define TIR_S_C_OPR_EOR 	57
#define TIR_S_C_OPR_NEG 	58
#define TIR_S_C_OPR_COM 	59
#define TIR_S_C_OPR_INSV        60
#define TIR_S_C_OPR_ASH 	61
#define TIR_S_C_OPR_USH 	62
#define TIR_S_C_OPR_ROT 	63
#define TIR_S_C_OPR_SEL 	64
#define TIR_S_C_OPR_REDEF       65
#define TIR_S_C_OPR_DFLIT       66
#define TIR_S_C_MAXOPRCOD       66
#define TIR_S_C_MINCTLCOD       80
#define TIR_S_C_CTL_SETRB       80
#define TIR_S_C_CTL_AUGRB       81
#define TIR_S_C_CTL_DFLOC       82
#define TIR_S_C_CTL_STLOC       83
#define TIR_S_C_CTL_STKDL       84
#define TIR_S_C_MAXCTLCOD       84

#define ETIR_S_C_MINSTACOD 0		/* Minimum store code.		*/
#define ETIR_S_C_STA_GBL 0		/* Stack global symbol value.	*/
#define ETIR_S_C_STA_LW 1		/* Stack longword.		*/
#define ETIR_S_C_STA_QW 2		/* Stack quadword.		*/
#define ETIR_S_C_STA_PQ 3		/* Stack psect base + quadword off.  */
#define ETIR_S_C_STA_LI 4		/* Stack literal.		*/
#define ETIR_S_C_STA_MOD 5		/* Stack module.		*/
#define ETIR_S_C_STA_CKARG 6		/* Check Arguments.		*/
#define ETIR_S_C_MAXSTACOD 6		/* Maximum stack code.		*/
#define ETIR_S_C_MINSTOCOD 50		/* Minimum store code.		*/
#define ETIR_S_C_STO_B 50		/* Store byte.			*/
#define ETIR_S_C_STO_W 51		/* Store word.			*/
#define ETIR_S_C_STO_LW 52		/* Store longword.		*/
#define ETIR_S_C_STO_QW 53		/* Store quadword.		*/
#define ETIR_S_C_STO_IMMR 54		/* Store immediate Repeated.	*/
#define ETIR_S_C_STO_GBL 55		/* Store global.		*/
#define ETIR_S_C_STO_CA 56		/* Store code address.		*/
#define ETIR_S_C_STO_RB 57		/* Store relative branch.	*/
#define ETIR_S_C_STO_AB 58		/* Store absolute branch.	*/
#define ETIR_S_C_STO_OFF 59		/* Store offset within psect.	*/
#define ETIR_S_C_STO_IMM 61		/* Store immediate.		*/
#define ETIR_S_C_STO_GBL_LW 62		/* Store global Longword.	*/
#define ETIR_S_C_STO_LP_PSB 63		/* STO_LP_PSB not valid in level 2 use STC_LP_PSB.  */
#define ETIR_S_C_STO_HINT_GBL 64	/* Store 14 bit HINT at global address.  */
#define ETIR_S_C_STO_HINT_PS 65		/* Store 14 bit HINT at psect + offset */
#define ETIR_S_C_MAXSTOCOD 65		/* Maximum store code.		*/
#define ETIR_S_C_MINOPRCOD 100		/* Minimum operate code.	*/
#define ETIR_S_C_OPR_NOP 100		/* No-op.			*/
#define ETIR_S_C_OPR_ADD 101		/* Add.				*/
#define ETIR_S_C_OPR_SUB 102		/* Subtract.			*/
#define ETIR_S_C_OPR_MUL 103		/* Multiply.			*/
#define ETIR_S_C_OPR_DIV 104		/* Divide.			*/
#define ETIR_S_C_OPR_AND 105		/* Logical AND.			*/
#define ETIR_S_C_OPR_IOR 106		/* Logical inclusive OR.	*/
#define ETIR_S_C_OPR_EOR 107		/* Logical exclusive OR.	*/
#define ETIR_S_C_OPR_NEG 108		/* Negate.			*/
#define ETIR_S_C_OPR_COM 109		/* Complement.			*/
#define ETIR_S_C_OPR_INSV 110		/* Insert bit field.		*/
#define ETIR_S_C_OPR_ASH 111		/* Arithmetic shift.		*/
#define ETIR_S_C_OPR_USH 112		/* Unsigned shift.		*/
#define ETIR_S_C_OPR_ROT 113		/* Rotate.			*/
#define ETIR_S_C_OPR_SEL 114		/* Select one of three longwords on top of stack.   */
#define ETIR_S_C_OPR_REDEF 115		/* Redefine this symbol after pass 2.  */
#define ETIR_S_C_OPR_DFLIT 116		/* Define a literal.		*/
#define ETIR_S_C_MAXOPRCOD 116		/* Maximum operate code.	*/
#define ETIR_S_C_MINCTLCOD 150		/* Minimum control code.	*/
#define ETIR_S_C_CTL_SETRB 150		/* Set relocation base.		*/
#define ETIR_S_C_CTL_AUGRB 151		/* Augment relocation base.	*/
#define ETIR_S_C_CTL_DFLOC 152		/* Define debug location.	*/
#define ETIR_S_C_CTL_STLOC 153		/* Set debug location.		*/
#define ETIR_S_C_CTL_STKDL 154		/* Stack debug location.	*/
#define ETIR_S_C_MAXCTLCOD 154		/* Maximum control code.	*/
#define ETIR_S_C_MINSTCCOD 200		/* Minimum store-conditional code.   */
#define ETIR_S_C_STC_LP 200		/* Store-conditional Linkage Pair.   */
#define ETIR_S_C_STC_LP_PSB 201		/* Store-conditional Linkage Pair with Procedure Signature.  */
#define ETIR_S_C_STC_GBL 202		/* Store-conditional Address at global address.  */
#define ETIR_S_C_STC_GCA 203		/* Store-conditional Code Address at global address.  */
#define ETIR_S_C_STC_PS 204		/* Store-conditional Address at psect + offset.  */
#define ETIR_S_C_STC_NOP_GBL 205	/* Store-conditional NOP at address of global.  */
#define ETIR_S_C_STC_NOP_PS 206		/* Store-conditional NOP at pect + offset.  */
#define ETIR_S_C_STC_BSR_GBL 207	/* Store-conditional BSR at global address.  */
#define ETIR_S_C_STC_BSR_PS 208		/* Store-conditional BSR at pect + offset.  */
#define ETIR_S_C_STC_LDA_GBL 209	/* Store-conditional LDA at global address.  */
#define ETIR_S_C_STC_LDA_PS 210		/* Store-conditional LDA at psect + offset.  */
#define ETIR_S_C_STC_BOH_GBL 211	/* Store-conditional BSR or Hint at global address.  */
#define ETIR_S_C_STC_BOH_PS 212		/* Store-conditional BSR or Hint at pect + offset.  */
#define ETIR_S_C_STC_NBH_GBL 213	/* Store-conditional NOP,BSR or HINT at global address.  */
#define ETIR_S_C_STC_NBH_PS 214		/* Store-conditional NOP,BSR or HINT at psect + offset.  */
#define ETIR_S_C_MAXSTCCOD 214		/* Maximum store-conditional code.   */

#define ETIR_S_C_HEADER_SIZE 4		/* Size of the header of a command */

/* VMS Global Symbol Directory Records (GSD/EGSD).  */

#define GSD_S_K_ENTRIES 1
#define GSD_S_C_ENTRIES 1
#define GSD_S_C_PSC     0
#define GSD_S_C_SYM     1
#define GSD_S_C_EPM     2
#define GSD_S_C_PRO     3
#define GSD_S_C_SYMW    4
#define GSD_S_C_EPMW    5
#define GSD_S_C_PROW    6
#define GSD_S_C_IDC     7
#define GSD_S_C_ENV     8
#define GSD_S_C_LSY     9
#define GSD_S_C_LEPM    10
#define GSD_S_C_LPRO    11
#define GSD_S_C_SPSC    12
#define GSD_S_C_SYMV    13
#define GSD_S_C_EPMV    14
#define GSD_S_C_PROV    15
#define GSD_S_C_MAXRECTYP       15

#define EGSD_S_K_ENTRIES 2	/* Offset to first entry in record.	*/
#define EGSD_S_C_ENTRIES 2	/* Offset to first entry in record.	*/
#define EGSD_S_C_PSC 0		/* Psect definition.			*/
#define EGSD_S_C_SYM 1		/* Symbol specification.		*/
#define EGSD_S_C_IDC 2		/* Random entity check.			*/
#define EGSD_S_C_SPSC 5		/* Shareable image psect definition.	*/
#define EGSD_S_C_SYMV 6		/* Vectored (dual-valued) versions of SYM.  */
#define EGSD_S_C_SYMM 7		/* Masked versions of SYM.		*/
#define EGSD_S_C_SYMG 8		/* EGST - gst version of SYM.		*/
#define EGSD_S_C_MAXRECTYP 8	/* Maximum entry type defined.		*/

/* Program Section Definition.  */
#define GPS_S_M_PIC     1
#define GPS_S_M_LIB     2
#define GPS_S_M_OVR     4
#define GPS_S_M_REL     8
#define GPS_S_M_GBL     16
#define GPS_S_M_SHR     32
#define GPS_S_M_EXE     64
#define GPS_S_M_RD      128
#define GPS_S_M_WRT     256
#define GPS_S_M_VEC     512
#define GPS_S_K_NAME    9
#define GPS_S_C_NAME    9

#define EGPS_S_B_ALIGN    4
#define EGPS_S_W_FLAGS    6
#define EGPS_S_L_ALLOC    8
#define EGPS_S_B_NAMLNG  12

#define EGPS_S_V_PIC		0x0001
#define EGPS_S_V_LIB		0x0002
#define EGPS_S_V_OVR		0x0004
#define EGPS_S_V_REL		0x0008
#define EGPS_S_V_GBL		0x0010
#define EGPS_S_V_SHR		0x0020
#define EGPS_S_V_EXE		0x0040
#define EGPS_S_V_RD		0x0080
#define EGPS_S_V_WRT		0x0100
#define EGPS_S_V_VEC		0x0200
#define EGPS_S_V_NOMOD		0x0400
#define EGPS_S_V_COM		0x0800
#define EGPS_S_V_ALLOC_64BIT	0x1000

/* Symbol Defintion or Reference.  */
#define GSY_S_M_WEAK    1
#define GSY_S_M_DEF     2
#define GSY_S_M_UNI     4
#define GSY_S_M_REL     8

#define EGSY_S_W_FLAGS  6

#define EGSY_S_V_WEAK		0x0001
#define EGSY_S_V_DEF		0x0002
#define EGSY_S_V_UNI		0x0004
#define EGSY_S_V_REL		0x0008
#define EGSY_S_V_COMM		0x0010
#define EGSY_S_V_VECEP		0x0020
#define EGSY_S_V_NORM		0x0040
#define EGSY_S_V_QUAD_VAL	0x0080

#define LSY_S_M_DEF     2
#define LSY_S_M_REL     8

#define ENV_S_M_DEF     1
#define ENV_S_M_NESTED  2

/* Symbol Definition.  */
#define ESDF_S_L_VALUE     8
#define ESDF_S_L_PSINDX   28
#define ESDF_S_B_NAMLNG   32

/* Universal Symbol Definition.  */
#define EGST_S_W_FLAGS    6
#define EGST_S_Q_LP_1    16
#define EGST_S_Q_LP_2    24
#define EGST_S_L_PSINDX  32
#define EGST_S_B_NAMLNG  36

/* Symbol Reference.  */
#define ESRF_S_B_NAMLNG   8

/* Debugger symbol definitions:  These are done by hand,
   as no machine-readable version seems to be available.  */
#define DST_S_C_C		  7	/* Language == "C".	*/
#define DST_S_C_CXX		 15	/* Language == "C++".	*/
#define DST_S_C_EPILOG		127
#define DST_S_C_VERSION		153
#define	DST_S_C_SOURCE		155	/* Source file.		*/
#define DST_S_C_PROLOG		162
#define	DST_S_C_BLKBEG		176	/* Beginning of block.	*/
#define	DST_S_C_BLKEND		177	/* End of block.	*/
#define DST_S_C_ENTRY		181
#define DST_S_C_PSECT		184
#define	DST_S_C_LINE_NUM	185	/* Line Number.		*/
#define DST_S_C_LBLORLIT	186
#define DST_S_C_LABEL		187
#define	DST_S_C_MODBEG		188	/* Beginning of module. */
#define	DST_S_C_MODEND		189	/* End of module.	*/
#define	DST_S_C_RTNBEG		190	/* Beginning of routine.*/
#define	DST_S_C_RTNEND		191	/* End of routine.	*/

/* These are used with DST_S_C_LINE_NUM.  */
#define DST_S_C_LINE_NUM_HEADER_SIZE 4

#define DST_S_C_DELTA_PC_W	 1	/* Incr PC.		*/
#define DST_S_C_INCR_LINUM	 2	/* Incr Line #.		*/
#define DST_S_C_INCR_LINUM_W	 3	/* Incr Line #.		*/
#define DST_S_C_SET_LINUM_INCR	 4
#define DST_S_C_SET_LINUM_INCR_W 5
#define DST_S_C_RESET_LINUM_INCR 6
#define DST_S_C_BEG_STMT_MODE	 7
#define DST_S_C_END_STMT_MODE	 8
#define DST_S_C_SET_LINE_NUM	 9	/* Set Line #.		*/
#define DST_S_C_SET_PC		10
#define DST_S_C_SET_PC_W	11
#define DST_S_C_SET_PC_L	12
#define DST_S_C_SET_STMTNUM	13
#define DST_S_C_TERM		14	/* End of lines.	*/
#define DST_S_C_TERM_W		15	/* End of lines.	*/
#define	DST_S_C_SET_ABS_PC	16	/* Set PC.		*/
#define	DST_S_C_DELTA_PC_L	17	/* Incr PC.		*/
#define DST_S_C_INCR_LINUM_L	18	/* Incr Line #.		*/
#define DST_S_C_SET_LINUM_B	19	/* Set Line #.		*/
#define DST_S_C_SET_LINUM_L	20	/* Set Line #.		*/
#define	DST_S_C_TERM_L		21	/* End of lines.	*/
/* These are used with DST_S_C_SOURCE */
#define DST_S_C_SRC_DECLFILE	 1	/* Declare source file.  */
#define DST_S_C_SRC_SETFILE	 2	/* Set source file.	 */
#define DST_S_C_SRC_SETREC_L	 3	/* Set record, longword value.  */
#define DST_S_C_SRC_SETREC_W	 4	/* Set record, word value.  */
#define DST_S_C_SRC_SETLNUM_L	 5	/* Set line, longword value.  */
#define DST_S_C_SRC_SETLNUM_W	 6	/* Set line, word value.  */
#define DST_S_C_SRC_INCRLNUM_B	 7	/* Increment line.  */
#define DST_S_C_SRC_DEFLINES_W	10	/* # of line, word counter.  */
#define DST_S_C_SRC_DEFLINES_B	11	/* # of line, byte counter.  */
#define DST_S_C_SRC_FORMFEED	16	/* ^L counts as a record.  */

#define DST_S_B_PCLINE_UNSBYTE	 1
#define DST_S_W_PCLINE_UNSWORD	 1
#define DST_S_L_PCLINE_UNSLONG	 1

#define DST_S_B_MODBEG_NAME	14
#define DST_S_L_RTNBEG_ADDRESS	 5
#define DST_S_B_RTNBEG_NAME	13
#define DST_S_L_RTNEND_SIZE	 5

/* These are used with DST_S_C_SOURCE.  */
#define DST_S_C_SOURCE_HEADER_SIZE 4

#define DST_S_B_SRC_DF_LENGTH	  1
#define DST_S_W_SRC_DF_FILEID	  3
#define DST_S_B_SRC_DF_FILENAME	 20
#define DST_S_B_SRC_UNSBYTE	  1
#define DST_S_W_SRC_UNSWORD	  1
#define DST_S_L_SRC_UNSLONG	  1

/* The following are the codes for the various data types.  Anything not on
   the list is included under 'advanced_type'.  */
#define DBG_S_C_UCHAR		0x02
#define DBG_S_C_USINT		0x03
#define DBG_S_C_ULINT		0x04
#define DBG_S_C_UQUAD		0x05
#define DBG_S_C_SCHAR		0x06
#define DBG_S_C_SSINT		0x07
#define DBG_S_C_SLINT		0x08
#define DBG_S_C_SQUAD		0x09
#define DBG_S_C_REAL4		0x0a
#define DBG_S_C_REAL8		0x0b	/* D_float double.  */
#define DBG_S_C_COMPLX4		0x0c	/* 2xF_float complex float.  */
#define DBG_S_C_COMPLX8		0x0d	/* 2xD_float complex double.  */
#define DBG_S_C_REAL8_G		0x1b	/* G_float double.  */
#define DBG_S_C_COMPLX8_G	0x1d	/* 2xG_float complex double.  */
#define DBG_S_C_FUNCTION_ADDR	0x17
#define DBG_S_C_ADVANCED_TYPE	0xa3
/*  Some of these are just for future reference.  [pr].  */
#define DBG_S_C_UBITA		0x01	/* Unsigned, aligned bit field.  */
#define DBG_S_C_UBITU		0x22	/* Unsigned, unaligned bit field.  */
#define DBG_S_C_SBITA		0x29	/* Signed, aligned bit field.  */
#define DBG_S_C_SBITU		0x2a	/* Signed, unaligned bit field.  */
#define DBG_S_C_CSTRING		0x2e	/* Asciz ('\0' terminated) string.  */
#define DBG_S_C_WCHAR		0x38	/* Wchar_t.  */
/*  These are descriptor class codes.  */
#define DSC_K_CLASS_S		0x01	/* Static (fixed length).  */
#define DSC_K_CLASS_D		0x02	/* Dynamic string (not via malloc!).  */
#define DSC_K_CLASS_A		0x04	/* Array.  */
#define DSC_K_CLASS_UBS		0x0d	/* Unaligned bit string.  */

/*  These are the codes that are used to generate the definitions of struct
    union and enum records.  */
#define DBG_S_C_ENUM_ITEM		0xa4
#define DBG_S_C_ENUM_START		0xa5
#define DBG_S_C_ENUM_END		0xa6
#define DBG_S_C_STRUCT_ITEM		DST_K_VFLAGS_BITOFFS	/* 0xff */
#define DBG_S_C_STRUCT_START		0xab
#define DBG_S_C_STRUCT_END		0xac
#define DST_K_TYPSPEC			0xaf	/* Type specification.  */
/* These codes are used in the generation of the symbol definition records.  */
#define DST_K_VFLAGS_NOVAL		0x80	/* Struct definition only.  */
#define DST_K_VFLAGS_DSC		0xfa	/* Descriptor used.  */
#define DST_K_VFLAGS_TVS		0xfb	/* Trailing value specified.  */
#define DST_K_VS_FOLLOWS		0xfd	/* Value spec follows.  */
#define DST_K_VFLAGS_BITOFFS		0xff	/* Value contains bit offset.  */
#define DST_K_VALKIND_LITERAL	0
#define DST_K_VALKIND_ADDR	1
#define DST_K_VALKIND_DESC	2
#define DST_K_VALKIND_REG	3
#define DST_K_REG_VAX_AP	0x0c	/* R12.  */
#define DST_K_REG_VAX_FP	0x0d	/* R13.  */
#define DST_K_REG_VAX_SP	0x0e	/* R14.  */
#define DST_V_VALKIND		0	/* Offset of valkind field.  */
#define DST_V_INDIRECT		2	/* Offset to indirect bit.  */
#define DST_V_DISP		3	/* Offset to displacement bit.  */
#define DST_V_REGNUM		4	/* Offset to register number.  */
#define DST_M_INDIRECT		(1<<DST_V_INDIRECT)
#define DST_M_DISP		(1<<DST_V_DISP)
#define DBG_C_FUNCTION_PARAM	/* 0xc9 */	\
	(DST_K_VALKIND_ADDR|DST_M_DISP|(DST_K_REG_VAX_AP<<DST_V_REGNUM))
#define DBG_C_LOCAL_SYM		/* 0xd9 */	\
	(DST_K_VALKIND_ADDR|DST_M_DISP|(DST_K_REG_VAX_FP<<DST_V_REGNUM))
/* Kinds of value specifications.   */
#define DST_K_VS_ALLOC_SPLIT	3	/* Split lifetime.  */
/* Kinds of type specifications.  */
#define DST_K_TS_ATOM		0x01	/* Atomic type specification.  */
#define DST_K_TS_DSC		0x02	/* Descriptor type spec.  */
#define DST_K_TS_IND		0x03	/* Indirect type specification.  */
#define DST_K_TS_TPTR		0x04	/* Typed pointer type spec.  */
#define DST_K_TS_PTR		0x05	/* Pointer type spec.  */
#define DST_K_TS_ARRAY		0x07	/* Array type spec.  */
#define DST_K_TS_NOV_LENG	0x0e	/* Novel length type spec.  */
/* These are the codes that are used in the suffix records to determine the
   actual data type.  */
#define DBG_S_C_BASIC			DST_K_TS_ATOM
#define DBG_S_C_BASIC_ARRAY		DST_K_TS_DSC
#define DBG_S_C_STRUCT			DST_K_TS_IND
#define DBG_S_C_POINTER			DST_K_TS_TPTR
#define DBG_S_C_VOID			DST_K_TS_PTR
#define DBG_S_C_COMPLEX_ARRAY		DST_K_TS_ARRAY

/* VMS Module Header Records (MHD/EMH).  */

#define MHD_S_C_MHD 0
#define MHD_S_C_LNM 1
#define MHD_S_C_SRC 2
#define MHD_S_C_TTL 3
#define MHD_S_C_CPR 4
#define MHD_S_C_MTC 5
#define MHD_S_C_GTX 6
#define MHD_S_C_MAXHDRTYP 6

#define EMH_S_C_MHD 0		/* Main header record.		*/
#define EMH_S_C_LNM 1		/* Language name and version.	*/
#define EMH_S_C_SRC 2		/* Source file specification.	*/
#define EMH_S_C_TTL 3		/* Title text of module.	*/
#define EMH_S_C_CPR 4		/* Copyright notice.		*/
#define EMH_S_C_MTC 5		/* Maintenance status.		*/
#define EMH_S_C_GTX 6		/* General text.		*/
#define EMH_S_C_MAXHDRTYP 6	/* Maximum allowable type.	*/

/* vms.c.  */

extern asymbol *_bfd_vms_make_empty_symbol (bfd *);
extern int _bfd_vms_slurp_object_records (bfd *abfd);

/* vms-gsd.c.  */

extern int _bfd_vms_slurp_gsd (bfd *abfd, int objtype);
extern int _bfd_vms_write_gsd (bfd *abfd, int objtype);
extern int _bfd_vms_slurp_dbg (bfd *abfd, int objtype);
extern int _bfd_vms_write_dbg (bfd *abfd, int objtype);
extern int _bfd_vms_slurp_tbt (bfd *abfd, int objtype);
extern int _bfd_vms_write_tbt (bfd *abfd, int objtype);

/* vms-misc.c.  */

extern int _bfd_vms_get_object_record (bfd *abfd);
extern int _bfd_vms_get_first_record (bfd *abfd);

/* vms-hdr.c.  */

extern int _bfd_vms_slurp_hdr (bfd *abfd, int objtype);
extern int _bfd_vms_write_hdr (bfd *abfd, int objtype);
extern int _bfd_vms_slurp_eom (bfd *abfd, int objtype);
extern int _bfd_vms_write_eom (bfd *abfd, int objtype);
extern bfd_boolean _bfd_vms_find_nearest_dst_line
  (bfd *abfd, asection *section, asymbol **symbols, bfd_vma offset,
	   const char **file, const char **func, unsigned int *line);
extern int _bfd_vms_slurp_ihd
  (bfd *abfd, unsigned int *isd_offset, unsigned int *ihs_offset);
extern int _bfd_vms_slurp_isd (bfd *abfd, unsigned int offset);
extern int _bfd_vms_slurp_ihs (bfd *abfd, unsigned int offset);

/* vms-tir.c.  */

extern int _bfd_vms_slurp_tir (bfd *abfd, int objtype);
extern int _bfd_vms_write_tir (bfd *abfd, int objtype);
extern int _bfd_vms_slurp_lnk (bfd *abfd, int objtype);

extern int _bfd_vms_slurp_relocs (bfd *abfd);
extern int _bfd_vms_decode_relocs
  (bfd *abfd, arelent *relocs, asection *section, asymbol **symbols);

/* The r_type field in a reloc is one of the following values.  */
#define ALPHA_R_IGNORE		0
#define ALPHA_R_REFQUAD		1
#define ALPHA_R_BRADDR		2
#define ALPHA_R_HINT		3
#define ALPHA_R_SREL16		4
#define ALPHA_R_SREL32		5
#define ALPHA_R_SREL64		6
#define ALPHA_R_OP_PUSH		7
#define ALPHA_R_OP_STORE	8
#define ALPHA_R_OP_PSUB		9
#define ALPHA_R_OP_PRSHIFT	10
#define ALPHA_R_LINKAGE		11
#define ALPHA_R_REFLONG		12
#define ALPHA_R_CODEADDR	13
#define ALPHA_R_NOP		14
#define ALPHA_R_BSR		15
#define ALPHA_R_LDA		16
#define ALPHA_R_BOH		17

/* VMS Object Language (OBJ/EOBJ).  */

#define OBJ_S_C_HDR 0		/* VAX moule header record.		*/
#define OBJ_S_C_GSD 1		/* VAX glbal symbol definition record.	*/
#define OBJ_S_C_TIR 2		/* VAX tet information record.		*/
#define OBJ_S_C_EOM 3		/* VAX en of module record.		*/
#define OBJ_S_C_DBG 4		/* VAX Deugger information record.	*/
#define OBJ_S_C_TBT 5		/* VAX Trceback information record.	*/
#define OBJ_S_C_LNK 6		/* VAX liker options record.		*/
#define OBJ_S_C_EOMW 7		/* VAX en of module word-psect record.	*/
#define OBJ_S_C_MAXRECTYP 7	/* VAX Lat assigned record type.	*/

#define EOBJ_S_C_EMH 8		/* EVAX mdule header record.		*/
#define EOBJ_S_C_EEOM 9		/* EVAX ed of module record.		*/
#define EOBJ_S_C_EGSD 10	/* EVAX gobal symbol definition record. */
#define EOBJ_S_C_ETIR 11	/* EVAX txt information record.		*/
#define EOBJ_S_C_EDBG 12	/* EVAX Dbugger information record.	*/
#define EOBJ_S_C_ETBT 13	/* EVAX Taceback information record.	*/
#define EOBJ_S_C_MAXRECTYP 13	/* EVAX Lst assigned record type.	*/

#define OBJ_S_K_SUBTYP 1		 
#define OBJ_S_C_SUBTYP 1		 
#define EOBJ_S_K_SUBTYP 4		 
#define EOBJ_S_C_SUBTYP 4		 
#define OBJ_S_C_MAXRECSIZ 2048	/* Maximu legal record size.           */
#define EOBJ_S_C_MAXRECSIZ 8192 /* Maximu legal record size.           */
#define OBJ_S_C_STRLVL 0	/* Structre level.                     */
#define EOBJ_S_C_STRLVL 2	/* Structre level.                     */
#define OBJ_S_C_SYMSIZ 31	/* Maximu symbol length.		*/
#define EOBJ_S_C_SYMSIZ 64	/* Maximu symbol length.		*/
#define EOBJ_S_C_SECSIZ 31	/* Maximu section name length.		*/
#define OBJ_S_C_STOREPLIM -1	/* Maximu repeat count on store commands.  */
#define EOBJ_S_C_STOREPLIM -1	/* Maximu repeat count on store commands.  */
#define OBJ_S_C_PSCALILIM 9	/* Maximu p-sect alignment.            */
#define EOBJ_S_C_PSCALILIM 16	/* Maximu p-sect alignment.            */

#define EVAX_OFFSET	256	/* Type ofset for EVAX codes in switch.  */

/* Miscellaneous definitions.  */	 

#if __GNUC__				 
typedef unsigned long long uquad;	 
#else					 
typedef unsigned long uquad;		 
#endif					 

#define MAX_OUTREC_SIZE 4096
#define MIN_OUTREC_LUFT 64

/* VMS module header.  */		 

struct hdr_struct
{
  char hdr_b_strlvl;
  int hdr_l_arch1;
  int hdr_l_arch2;
  int hdr_l_recsiz;
  char *hdr_t_name;
  char *hdr_t_version;
  char *hdr_t_date;
  char *hdr_c_lnm;
  char *hdr_c_src;
  char *hdr_c_ttl;
  char *hdr_c_cpr;
};

#define EMH_S_W_HDRTYP   4
#define EMH_S_B_STRLVL   6
#define EMH_S_L_ARCH1    8
#define EMH_S_L_ARCH2   12
#define EH_S_L_RECSIZ  16
#define EMH_S_B_NAMLNG  20

#define EMH_DATE_LENGTH  17

/* VMS End-Of-Module records (EOM/EEOM).  */

struct eom_struct
{
  int eom_l_total_lps;
  short eom_w_comcod;
  bfd_boolean eom_has_transfer;
  char eom_b_tfrflg;
  int eom_l_psindx;
  int eom_l_tfradr;
};

#define EEOM_S_L_TOTAL_LPS   4
#define EEOM_S_W_COMCOD      8
#define EEOM_S_B_TFRFLG     10
#define EEOM_S_L_PSINDX     12
#define EEOM_S_L_TFRADR     16

/* VMS Image Header Records (IHD/EIHD).  */

#define EIHD_S_K_MAJORID	3	/* Major id constant	*/
#define EIHD_S_K_MINORID	0	/* Minor id constant	*/
#define EIHD_S_K_EXE		1	/* Executable image	*/

#define EIHD_S_L_SIZE		8
#define EIHD_S_L_ISDOFF		12
#define EIHD_S_L_SYMDBGOFF	20
#define EIHD_S_Q_SYMVVA		40
#define EIHD_S_L_IMGTYPE	52

/* VMS Image Section Description Records (ISD/EISD).  */

#define EISD_S_L_EISDSIZE	 8
#define EISD_S_L_SECSIZE	12
#define EISD_S_Q_VIR_ADDR	16
#define EISD_S_L_FLAGS		24
#define EISD_S_L_VBN		28
#define EISD_S_R_CONTROL	32
#define EISD_S_L_IDENT		36
#define EISD_S_T_GBLNAM		40

#define EISD_S_M_GBL		0x0001
#define EISD_S_M_CRF		0x0002
#define EISD_S_M_DZRO		0x0004
#define EISD_S_M_WRT		0x0008
#define EISD_S_M_INITALCODE	0x0010
#define EISD_S_M_BASED		0x0020
#define EISD_S_M_FIXUPVEC	0x0040
#define EISD_S_M_RESIDENT	0x0080
#define EISD_S_M_VECTOR		0x0100
#define EISD_S_M_PROTECT	0x0200
#define EISD_S_M_LASTCLU	0x0400
#define EISD_S_M_EXE		0x0800
#define EISD_S_M_NONSHRADR	0x1000
#define EISD_S_M_QUAD_LENGTH	0x2000
#define EISD_S_M_ALLOC_64BIT	0x4000

/* VMS Image Header Symbol Records (IHS/EIHS).  */

#define EIHS_S_L_DSTVBN		 8
#define EIHS_S_L_DSTSIZE	12
#define EIHS_S_L_GSTVBN		16
#define EIHS_S_L_GSTSIZE	20
#define EIHS_S_L_DMTVBN		24
#define EIHS_S_L_DMTBYTES	28

/* Debugger symbol definitions.  */

#define DBG_S_L_DMT_MODBEG       0
#define DBG_S_L_DST_SIZE         4
#define DBG_S_W_DMT_PSECT_COUNT  8
#define DBG_S_C_DMT_HEADER_SIZE 12

#define DBG_S_L_DMT_PSECT_START  0
#define DBG_S_L_DMT_PSECT_LENGTH 4
#define DBG_S_C_DMT_PSECT_SIZE   8


enum file_format_enum { FF_UNKNOWN, FF_FOREIGN, FF_NATIVE };
enum file_type_enum { FT_UNKNOWN, FT_MODULE, FT_IMAGE };

typedef struct vms_symbol_struct
{
  struct bfd_hash_entry bfd_hash;
  asymbol *symbol;
} vms_symbol_entry;

/* Stack value for push/pop commands.  */

struct stack_struct
{
  uquad value;
  int psect;
};

#define STACKSIZE 8192

/* A minimal decoding of DST compilation units.  We only decode
   what's needed to get to the line number information.  */

struct fileinfo
{
  char *name;
  unsigned int srec;
};

struct srecinfo
{
  struct srecinfo *next;
  unsigned int line;
  unsigned int sfile;
  unsigned int srec;
};

struct lineinfo
{
  struct lineinfo *next;
  bfd_vma address;
  unsigned int line;
};

struct funcinfo
{
  struct funcinfo *next;
  char *name;
  bfd_vma low;
  bfd_vma high;
};

struct module
{
  /* Chain the previously read compilation unit.  */
  struct module *next;

  /* The module name.  */
  char *name;

  /* The start offset and size of debug info in the DST section.  */
  unsigned int modbeg;
  unsigned int size;

  /* The lowest and highest addresses contained in this compilation
     unit as specified in the compilation unit header.  */
  bfd_vma low;
  bfd_vma high;

  /* The listing line table.  */
  struct lineinfo *line_table;

  /* The source record table.  */
  struct srecinfo *srec_table;

  /* A list of the functions found in this module.  */
  struct funcinfo *func_table;

  /* Current allocation of file_table.  */
  unsigned int file_table_count;

  /* An array of the files making up this module.  */
  struct fileinfo *file_table;
};

struct vms_private_data_struct
{
  bfd_boolean is_vax;
  bfd_boolean fixup_done;		/* Flag to indicate if all
					   section pointers and PRIV(sections)
					   are set up correctly.  */
  unsigned char *vms_buf;		/* record buffer */
  unsigned int buf_size;		/* size of record buffer  */
  unsigned char *vms_rec;		/* record pointer in record buffer */
  unsigned int rec_size;		/* record size  */
  enum file_format_enum file_format;

  struct hdr_struct hdr_data;		/* data from HDR/EMH record  */
  struct eom_struct eom_data;		/* data from EOM/EEOM record  */
  unsigned int section_count;		/* # of sections in following array  */
  asection **sections;			/* array of GSD/EGSD sections  */
  unsigned int gsd_sym_count;		/* # of GSD/EGSD symbols  */
  asymbol **symbols;			/* vector of GSD/EGSD symbols  */
  struct proc_value *procedure;

  struct stack_struct *stack;
  int stackptr;

  struct bfd_hash_table *vms_symbol_table;
  struct bfd_symbol **symcache;
  int symnum;

  asection *image_section;		/* section for image_ptr  */
  unsigned char *image_ptr;		/* a pointer to section->contents */

  unsigned char pdsc[8];		/* procedure descriptor */

  struct module *modules;		/* list of all compilation units */

  struct dst_info *dst_info;
  asection *dst_section;
  unsigned char *dst_ptr_end;
  unsigned int dst_ptr_offsets_count;	/* # of offsets in following array  */
  unsigned int *dst_ptr_offsets;	/* array of saved image_ptr offsets */

  /* Shared library support */
  bfd_vma symvva; /* relative virtual address of symbol vector */

  /* Output routine storage  */
  unsigned char *output_buf;		/* output data  */
  int push_level;
  int pushed_size;
  int length_pos;
  int output_size;
  int output_alignment;

  /* linkage index counter used by conditional store commands */
  int vms_linkage_index;

  /* see tc-alpha.c of gas for a description.  */
  int flag_hash_long_names;	/* -+, hash instead of truncate */
  int flag_show_after_trunc;	/* -H, show hashing/truncation */
};

#define PRIV(name) ((struct vms_private_data_struct *)abfd->tdata.any)->name

/* Used to keep extra VMS specific information for a given section.

   reloc_size holds the size of the relocation stream, note this
   is very different from the number of relocations as VMS relocations
   are variable length.

   reloc_stream is the actual stream of relocation entries.  */

struct vms_section_data_struct
{
  bfd_size_type reloc_size;
  unsigned char *reloc_stream;
  bfd_size_type reloc_offset;
  flagword vflags;
};

#define vms_section_data(sec) \
  ((struct vms_section_data_struct *)sec->used_by_bfd)
 
struct evax_private_udata_struct
{
  asymbol *bsym;
  asymbol *enbsym;
  char *origname;
  int lkindex;
};

#define SECTION_NAME_TEMPLATE "__SEC__%d"
					 
#if VMS_DEBUG				 
extern void _bfd_vms_debug (int, char *, ...) ATTRIBUTE_PRINTF_2;
extern void _bfd_hexdump   (int, unsigned char *, int, int);
					 
#define vms_debug _bfd_vms_debug	 
#endif					 
					 
extern struct bfd_hash_entry * _bfd_vms_hash_newfunc (struct bfd_hash_entry *, struct bfd_hash_table *, const char *);
extern void        _bfd_vms_get_header_values (bfd *, unsigned char *, int *, int *);
extern int         _bfd_vms_get_record  (bfd *abf);
extern int         _bfd_vms_next_record (bfd *abf);
extern char *      _bfd_vms_save_sized_string (unsigned char *, int);
extern char *      _bfd_vms_save_counted_string (unsigned char *);
extern void        _bfd_vms_push (bfd *, uquad, int);
extern uquad       _bfd_vms_pop (bfd *, int *);
extern void        _bfd_vms_output_begin (bfd *, int, int);
extern void        _bfd_vms_output_alignment (bfd *, int);
extern void        _bfd_vms_output_push (bfd *);
extern void        _bfd_vms_output_pop (bfd *);
extern void        _bfd_vms_output_flush (bfd *);
extern void        _bfd_vms_output_end (bfd *);
extern int         _bfd_vms_output_check (bfd *, int);
extern void        _bfd_vms_output_byte (bfd *, unsigned);
extern void        _bfd_vms_output_short (bfd *, unsigned);
extern void        _bfd_vms_output_long (bfd *, unsigned long);
extern void        _bfd_vms_output_quad (bfd *, uquad);
extern void        _bfd_vms_output_counted (bfd *, char *);
extern void        _bfd_vms_output_dump (bfd *, unsigned char *, int);
extern void        _bfd_vms_output_fill (bfd *, int, int);
extern char *      _bfd_vms_length_hash_symbol (bfd *, const char *, int);
extern vms_symbol_entry * _bfd_vms_enter_symbol (bfd *, char *);

#define EGPS_S_V_NO_SHIFT 16

extern void        bfd_vms_set_section_flags (bfd *, asection *, flagword);
#endif /* VMS_H */
