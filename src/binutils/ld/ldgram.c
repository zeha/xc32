
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 24 "ldgram.y"

/*

 */

#define DONTDECLARE_MALLOC

#include "sysdep.h"
#include "bfd.h"
#include "bfdlink.h"
#include "ld.h"
#include "ldexp.h"
#include "ldver.h"
#include "ldlang.h"
#include "ldfile.h"
#include "ldemul.h"
#include "ldmisc.h"
#include "ldmain.h"
#include "mri.h"
#include "ldctor.h"
#include "ldlex.h"

#ifndef YYDEBUG
#define YYDEBUG 1
#endif

static enum section_type sectype;
static lang_memory_region_type *region;

bfd_boolean ldgram_had_keep = FALSE;
char *ldgram_vers_current_lang = NULL;

#define ERROR_NAME_MAX 20
static char *error_names[ERROR_NAME_MAX];
static int error_index;
#define PUSH_ERROR(x) if (error_index < ERROR_NAME_MAX) error_names[error_index] = x; error_index++;
#define POP_ERROR()   error_index--;


/* Line 189 of yacc.c  */
#line 113 "ldgram.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     INT = 258,
     NAME = 259,
     LNAME = 260,
     OREQ = 261,
     ANDEQ = 262,
     RSHIFTEQ = 263,
     LSHIFTEQ = 264,
     DIVEQ = 265,
     MULTEQ = 266,
     MINUSEQ = 267,
     PLUSEQ = 268,
     OROR = 269,
     ANDAND = 270,
     NE = 271,
     EQ = 272,
     GE = 273,
     LE = 274,
     RSHIFT = 275,
     LSHIFT = 276,
     UNARY = 277,
     END = 278,
     ALIGN_K = 279,
     BLOCK = 280,
     BIND = 281,
     QUAD = 282,
     SQUAD = 283,
     LONG = 284,
     SHORT = 285,
     BYTE = 286,
     SECTIONS = 287,
     PHDRS = 288,
     INSERT_K = 289,
     AFTER = 290,
     BEFORE = 291,
     DATA_SEGMENT_ALIGN = 292,
     DATA_SEGMENT_RELRO_END = 293,
     DATA_SEGMENT_END = 294,
     SORT_BY_NAME = 295,
     SORT_BY_ALIGNMENT = 296,
     SIZEOF_HEADERS = 297,
     OUTPUT_FORMAT = 298,
     FORCE_COMMON_ALLOCATION = 299,
     OUTPUT_ARCH = 300,
     INHIBIT_COMMON_ALLOCATION = 301,
     SEGMENT_START = 302,
     INCLUDE = 303,
     MEMORY = 304,
     REGION_ALIAS = 305,
     NOLOAD = 306,
     DSECT = 307,
     COPY = 308,
     INFO = 309,
     OVERLAY = 310,
     DEFINED = 311,
     TARGET_K = 312,
     SEARCH_DIR = 313,
     MAP = 314,
     ENTRY = 315,
     NEXT = 316,
     SIZEOF = 317,
     ALIGNOF = 318,
     ADDR = 319,
     LOADADDR = 320,
     MAX_K = 321,
     MIN_K = 322,
     STARTUP = 323,
     HLL = 324,
     SYSLIB = 325,
     FLOAT = 326,
     NOFLOAT = 327,
     NOCROSSREFS = 328,
     ORIGIN = 329,
     FILL = 330,
     LENGTH = 331,
     CREATE_OBJECT_SYMBOLS = 332,
     INPUT = 333,
     OPTIONAL = 334,
     GROUP = 335,
     OUTPUT = 336,
     CONSTRUCTORS = 337,
     ALIGNMOD = 338,
     AT = 339,
     SUBALIGN = 340,
     PROVIDE = 341,
     PROVIDE_HIDDEN = 342,
     AS_NEEDED = 343,
     CHIP = 344,
     LIST = 345,
     SECT = 346,
     ABSOLUTE = 347,
     LOAD = 348,
     NEWLINE = 349,
     ENDWORD = 350,
     ORDER = 351,
     NAMEWORD = 352,
     ASSERT_K = 353,
     FORMAT = 354,
     PUBLIC = 355,
     DEFSYMEND = 356,
     BASE = 357,
     ALIAS = 358,
     TRUNCATE = 359,
     REL = 360,
     INPUT_SCRIPT = 361,
     INPUT_MRI_SCRIPT = 362,
     INPUT_DEFSYM = 363,
     CASE = 364,
     EXTERN = 365,
     START = 366,
     VERS_TAG = 367,
     VERS_IDENTIFIER = 368,
     GLOBAL = 369,
     LOCAL = 370,
     VERSIONK = 371,
     INPUT_VERSION_SCRIPT = 372,
     KEEP = 373,
     ONLY_IF_RO = 374,
     ONLY_IF_RW = 375,
     SPECIAL = 376,
     EXCLUDE_FILE = 377,
     CONSTANT = 378,
     INPUT_DYNAMIC_LIST = 379
   };
#endif
/* Tokens.  */
#define INT 258
#define NAME 259
#define LNAME 260
#define OREQ 261
#define ANDEQ 262
#define RSHIFTEQ 263
#define LSHIFTEQ 264
#define DIVEQ 265
#define MULTEQ 266
#define MINUSEQ 267
#define PLUSEQ 268
#define OROR 269
#define ANDAND 270
#define NE 271
#define EQ 272
#define GE 273
#define LE 274
#define RSHIFT 275
#define LSHIFT 276
#define UNARY 277
#define END 278
#define ALIGN_K 279
#define BLOCK 280
#define BIND 281
#define QUAD 282
#define SQUAD 283
#define LONG 284
#define SHORT 285
#define BYTE 286
#define SECTIONS 287
#define PHDRS 288
#define INSERT_K 289
#define AFTER 290
#define BEFORE 291
#define DATA_SEGMENT_ALIGN 292
#define DATA_SEGMENT_RELRO_END 293
#define DATA_SEGMENT_END 294
#define SORT_BY_NAME 295
#define SORT_BY_ALIGNMENT 296
#define SIZEOF_HEADERS 297
#define OUTPUT_FORMAT 298
#define FORCE_COMMON_ALLOCATION 299
#define OUTPUT_ARCH 300
#define INHIBIT_COMMON_ALLOCATION 301
#define SEGMENT_START 302
#define INCLUDE 303
#define MEMORY 304
#define REGION_ALIAS 305
#define NOLOAD 306
#define DSECT 307
#define COPY 308
#define INFO 309
#define OVERLAY 310
#define DEFINED 311
#define TARGET_K 312
#define SEARCH_DIR 313
#define MAP 314
#define ENTRY 315
#define NEXT 316
#define SIZEOF 317
#define ALIGNOF 318
#define ADDR 319
#define LOADADDR 320
#define MAX_K 321
#define MIN_K 322
#define STARTUP 323
#define HLL 324
#define SYSLIB 325
#define FLOAT 326
#define NOFLOAT 327
#define NOCROSSREFS 328
#define ORIGIN 329
#define FILL 330
#define LENGTH 331
#define CREATE_OBJECT_SYMBOLS 332
#define INPUT 333
#define OPTIONAL 334
#define GROUP 335
#define OUTPUT 336
#define CONSTRUCTORS 337
#define ALIGNMOD 338
#define AT 339
#define SUBALIGN 340
#define PROVIDE 341
#define PROVIDE_HIDDEN 342
#define AS_NEEDED 343
#define CHIP 344
#define LIST 345
#define SECT 346
#define ABSOLUTE 347
#define LOAD 348
#define NEWLINE 349
#define ENDWORD 350
#define ORDER 351
#define NAMEWORD 352
#define ASSERT_K 353
#define FORMAT 354
#define PUBLIC 355
#define DEFSYMEND 356
#define BASE 357
#define ALIAS 358
#define TRUNCATE 359
#define REL 360
#define INPUT_SCRIPT 361
#define INPUT_MRI_SCRIPT 362
#define INPUT_DEFSYM 363
#define CASE 364
#define EXTERN 365
#define START 366
#define VERS_TAG 367
#define VERS_IDENTIFIER 368
#define GLOBAL 369
#define LOCAL 370
#define VERSIONK 371
#define INPUT_VERSION_SCRIPT 372
#define KEEP 373
#define ONLY_IF_RO 374
#define ONLY_IF_RW 375
#define SPECIAL 376
#define EXCLUDE_FILE 377
#define CONSTANT 378
#define INPUT_DYNAMIC_LIST 379




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 62 "ldgram.y"

  bfd_vma integer;
  struct big_int
    {
      bfd_vma integer;
      char *str;
    } bigint;
  fill_type *fill;
  char *name;
  const char *cname;
  struct wildcard_spec wildcard;
  struct wildcard_list *wildcard_list;
  struct name_list *name_list;
  int token;
  union etree_union *etree;
  struct phdr_info
    {
      bfd_boolean filehdr;
      bfd_boolean phdrs;
      union etree_union *at;
      union etree_union *flags;
    } phdr;
  struct lang_nocrossref *nocrossref;
  struct lang_output_section_phdr_list *section_phdr;
  struct bfd_elf_version_deps *deflist;
  struct bfd_elf_version_expr *versyms;
  struct bfd_elf_version_tree *versnode;



/* Line 214 of yacc.c  */
#line 428 "ldgram.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 440 "ldgram.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  17
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1866

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  148
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  128
/* YYNRULES -- Number of rules.  */
#define YYNRULES  364
/* YYNRULES -- Number of states.  */
#define YYNSTATES  776

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   379

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   146,     2,     2,     2,    34,    21,     2,
      37,   143,    32,    30,   141,    31,     2,    33,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    16,   142,
      24,     6,    25,    15,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   144,     2,   145,    20,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    56,    19,    57,   147,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     7,     8,     9,    10,    11,    12,    13,    14,    17,
      18,    22,    23,    26,    27,    28,    29,    35,    36,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     6,     9,    12,    15,    18,    20,    21,
      26,    27,    30,    34,    35,    38,    43,    45,    47,    50,
      52,    57,    62,    66,    69,    74,    78,    83,    88,    93,
      98,   103,   106,   109,   112,   117,   122,   125,   128,   131,
     134,   135,   141,   144,   145,   149,   152,   153,   155,   159,
     161,   165,   166,   168,   172,   173,   176,   178,   181,   185,
     186,   189,   192,   193,   195,   197,   199,   201,   203,   205,
     207,   209,   211,   213,   218,   223,   228,   233,   242,   247,
     249,   251,   256,   257,   263,   264,   270,   275,   276,   282,
     287,   292,   296,   300,   307,   309,   313,   316,   318,   322,
     325,   326,   332,   333,   341,   342,   349,   351,   355,   358,
     360,   364,   367,   372,   375,   378,   379,   384,   387,   388,
     396,   398,   400,   402,   404,   410,   415,   420,   428,   436,
     444,   452,   461,   464,   466,   470,   472,   474,   478,   483,
     485,   486,   492,   495,   497,   499,   501,   506,   508,   513,
     518,   519,   528,   529,   535,   538,   540,   541,   543,   545,
     547,   549,   551,   553,   555,   558,   559,   561,   563,   565,
     567,   569,   571,   573,   575,   577,   579,   583,   587,   594,
     601,   603,   604,   609,   611,   612,   616,   618,   619,   627,
     628,   634,   638,   642,   643,   647,   649,   652,   654,   657,
     662,   667,   671,   675,   677,   682,   686,   687,   689,   691,
     692,   695,   699,   700,   703,   706,   710,   715,   718,   721,
     724,   728,   732,   736,   740,   744,   748,   752,   756,   760,
     764,   768,   772,   776,   780,   784,   788,   794,   798,   802,
     807,   809,   811,   816,   821,   826,   831,   836,   841,   846,
     853,   860,   867,   872,   879,   884,   886,   893,   900,   907,
     912,   917,   921,   922,   927,   928,   933,   934,   939,   940,
     942,   944,   946,   947,   948,   949,   950,   951,   952,   972,
     973,   974,   975,   976,   977,   996,   997,   998,  1006,  1007,
    1013,  1015,  1017,  1019,  1021,  1023,  1027,  1028,  1031,  1035,
    1038,  1045,  1056,  1059,  1061,  1062,  1064,  1067,  1068,  1069,
    1073,  1074,  1075,  1076,  1077,  1089,  1094,  1095,  1098,  1099,
    1100,  1107,  1109,  1110,  1114,  1120,  1121,  1125,  1126,  1129,
    1131,  1134,  1139,  1142,  1143,  1146,  1147,  1153,  1155,  1158,
    1163,  1169,  1176,  1178,  1181,  1182,  1185,  1190,  1195,  1204,
    1206,  1208,  1212,  1216,  1217,  1227,  1228,  1236,  1238,  1242,
    1244,  1248,  1250,  1254,  1255
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     149,     0,    -1,   122,   165,    -1,   123,   153,    -1,   133,
     264,    -1,   140,   259,    -1,   124,   151,    -1,     4,    -1,
      -1,   152,     4,     6,   220,    -1,    -1,   154,   155,    -1,
     155,   156,   110,    -1,    -1,   105,   220,    -1,   105,   220,
     141,   220,    -1,     4,    -1,   106,    -1,   112,   158,    -1,
     111,    -1,   116,     4,     6,   220,    -1,   116,     4,   141,
     220,    -1,   116,     4,   220,    -1,   115,     4,    -1,   107,
       4,   141,   220,    -1,   107,     4,   220,    -1,   107,     4,
       6,   220,    -1,    38,     4,     6,   220,    -1,    38,     4,
     141,   220,    -1,    99,     4,     6,   220,    -1,    99,     4,
     141,   220,    -1,   108,   160,    -1,   109,   159,    -1,   113,
       4,    -1,   119,     4,   141,     4,    -1,   119,     4,   141,
       3,    -1,   118,   220,    -1,   120,     3,    -1,   125,   161,
      -1,   126,   162,    -1,    -1,    64,   150,   157,   155,    36,
      -1,   127,     4,    -1,    -1,   158,   141,     4,    -1,   158,
       4,    -1,    -1,     4,    -1,   159,   141,     4,    -1,     4,
      -1,   160,   141,     4,    -1,    -1,     4,    -1,   161,   141,
       4,    -1,    -1,   163,   164,    -1,     4,    -1,   164,     4,
      -1,   164,   141,     4,    -1,    -1,   166,   167,    -1,   167,
     168,    -1,    -1,   200,    -1,   177,    -1,   251,    -1,   211,
      -1,   212,    -1,   214,    -1,   216,    -1,   179,    -1,   266,
      -1,   142,    -1,    73,    37,     4,   143,    -1,    74,    37,
     150,   143,    -1,    97,    37,   150,   143,    -1,    59,    37,
       4,   143,    -1,    59,    37,     4,   141,     4,   141,     4,
     143,    -1,    61,    37,     4,   143,    -1,    60,    -1,    62,
      -1,    94,    37,   172,   143,    -1,    -1,    95,   169,    37,
     176,   143,    -1,    -1,    96,   170,    37,   172,   143,    -1,
      75,    37,   150,   143,    -1,    -1,    64,   150,   171,   167,
      36,    -1,    89,    37,   217,   143,    -1,   126,    37,   162,
     143,    -1,    48,    49,     4,    -1,    48,    50,     4,    -1,
      66,    37,     4,   141,     4,   143,    -1,     4,    -1,   172,
     141,     4,    -1,   172,     4,    -1,     5,    -1,   172,   141,
       5,    -1,   172,     5,    -1,    -1,   104,    37,   173,   172,
     143,    -1,    -1,   172,   141,   104,    37,   174,   172,   143,
      -1,    -1,   172,   104,    37,   175,   172,   143,    -1,     4,
      -1,   172,   141,     4,    -1,   172,     4,    -1,     5,    -1,
     172,   141,     5,    -1,   172,     5,    -1,    46,    56,   178,
      57,    -1,   178,   226,    -1,   178,   179,    -1,    -1,    76,
      37,     4,   143,    -1,   198,   197,    -1,    -1,   114,   180,
      37,   220,   141,     4,   143,    -1,     4,    -1,    32,    -1,
      15,    -1,   181,    -1,   138,    37,   183,   143,   181,    -1,
      54,    37,   181,   143,    -1,    55,    37,   181,   143,    -1,
      54,    37,    55,    37,   181,   143,   143,    -1,    54,    37,
      54,    37,   181,   143,   143,    -1,    55,    37,    54,    37,
     181,   143,   143,    -1,    55,    37,    55,    37,   181,   143,
     143,    -1,    54,    37,   138,    37,   183,   143,   181,   143,
      -1,   183,   181,    -1,   181,    -1,   184,   199,   182,    -1,
     182,    -1,     4,    -1,   144,   184,   145,    -1,   182,    37,
     184,   143,    -1,   185,    -1,    -1,   134,    37,   187,   185,
     143,    -1,   198,   197,    -1,    93,    -1,   142,    -1,    98,
      -1,    54,    37,    98,   143,    -1,   186,    -1,   193,    37,
     218,   143,    -1,    91,    37,   194,   143,    -1,    -1,   114,
     189,    37,   220,   141,     4,   143,   197,    -1,    -1,    64,
     150,   190,   192,    36,    -1,   191,   188,    -1,   188,    -1,
      -1,   191,    -1,    41,    -1,    42,    -1,    43,    -1,    44,
      -1,    45,    -1,   218,    -1,     6,   194,    -1,    -1,    14,
      -1,    13,    -1,    12,    -1,    11,    -1,    10,    -1,     9,
      -1,     8,    -1,     7,    -1,   142,    -1,   141,    -1,     4,
       6,   218,    -1,     4,   196,   218,    -1,   102,    37,     4,
       6,   218,   143,    -1,   103,    37,     4,     6,   218,   143,
      -1,   141,    -1,    -1,    65,    56,   201,    57,    -1,   202,
      -1,    -1,   202,   199,   203,    -1,   203,    -1,    -1,     4,
     204,   208,    16,   206,   199,   207,    -1,    -1,    64,   150,
     205,   201,    36,    -1,    90,     6,   218,    -1,    92,     6,
     218,    -1,    -1,    37,   209,   143,    -1,   210,    -1,   209,
     210,    -1,     4,    -1,   146,     4,    -1,    84,    37,   150,
     143,    -1,    85,    37,   213,   143,    -1,    85,    37,   143,
      -1,   213,   199,   150,    -1,   150,    -1,    86,    37,   215,
     143,    -1,   215,   199,   150,    -1,    -1,    87,    -1,    88,
      -1,    -1,     4,   217,    -1,     4,   141,   217,    -1,    -1,
     219,   220,    -1,    31,   220,    -1,    37,   220,   143,    -1,
      77,    37,   220,   143,    -1,   146,   220,    -1,    30,   220,
      -1,   147,   220,    -1,   220,    32,   220,    -1,   220,    33,
     220,    -1,   220,    34,   220,    -1,   220,    30,   220,    -1,
     220,    31,   220,    -1,   220,    29,   220,    -1,   220,    28,
     220,    -1,   220,    23,   220,    -1,   220,    22,   220,    -1,
     220,    27,   220,    -1,   220,    26,   220,    -1,   220,    24,
     220,    -1,   220,    25,   220,    -1,   220,    21,   220,    -1,
     220,    20,   220,    -1,   220,    19,   220,    -1,   220,    15,
     220,    16,   220,    -1,   220,    18,   220,    -1,   220,    17,
     220,    -1,    72,    37,     4,   143,    -1,     3,    -1,    58,
      -1,    79,    37,     4,   143,    -1,    78,    37,     4,   143,
      -1,    80,    37,     4,   143,    -1,    81,    37,     4,   143,
      -1,   139,    37,     4,   143,    -1,   108,    37,   220,   143,
      -1,    38,    37,   220,   143,    -1,    38,    37,   220,   141,
     220,   143,    -1,    51,    37,   220,   141,   220,   143,    -1,
      52,    37,   220,   141,   220,   143,    -1,    53,    37,   220,
     143,    -1,    63,    37,     4,   141,   220,   143,    -1,    39,
      37,   220,   143,    -1,     4,    -1,    82,    37,   220,   141,
     220,   143,    -1,    83,    37,   220,   141,   220,   143,    -1,
     114,    37,   220,   141,     4,   143,    -1,    90,    37,     4,
     143,    -1,    92,    37,     4,   143,    -1,   100,    25,     4,
      -1,    -1,   100,    37,   220,   143,    -1,    -1,    38,    37,
     220,   143,    -1,    -1,   101,    37,   220,   143,    -1,    -1,
     135,    -1,   136,    -1,   137,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     4,   227,   242,   222,   223,   224,   228,   225,
      56,   229,   192,    57,   230,   245,   221,   246,   195,   231,
     199,    -1,    -1,    -1,    -1,    -1,    -1,    71,   232,   243,
     244,   222,   224,   233,    56,   234,   247,    57,   235,   245,
     221,   246,   195,   236,   199,    -1,    -1,    -1,    96,   237,
     242,   238,    56,   178,    57,    -1,    -1,    64,   150,   239,
     178,    36,    -1,    67,    -1,    68,    -1,    69,    -1,    70,
      -1,    71,    -1,    37,   240,   143,    -1,    -1,    37,   143,
      -1,   220,   241,    16,    -1,   241,    16,    -1,    40,    37,
     220,   143,   241,    16,    -1,    40,    37,   220,   143,    39,
      37,   220,   143,   241,    16,    -1,   220,    16,    -1,    16,
      -1,    -1,    89,    -1,    25,     4,    -1,    -1,    -1,   246,
      16,     4,    -1,    -1,    -1,    -1,    -1,   247,     4,   248,
      56,   192,    57,   249,   246,   195,   250,   199,    -1,    47,
      56,   252,    57,    -1,    -1,   252,   253,    -1,    -1,    -1,
       4,   254,   256,   257,   255,   142,    -1,   220,    -1,    -1,
       4,   258,   257,    -1,   100,    37,   220,   143,   257,    -1,
      -1,    37,   220,   143,    -1,    -1,   260,   261,    -1,   262,
      -1,   261,   262,    -1,    56,   263,    57,   142,    -1,   272,
     142,    -1,    -1,   265,   268,    -1,    -1,   267,   132,    56,
     268,    57,    -1,   269,    -1,   268,   269,    -1,    56,   271,
      57,   142,    -1,   128,    56,   271,    57,   142,    -1,   128,
      56,   271,    57,   270,   142,    -1,   128,    -1,   270,   128,
      -1,    -1,   272,   142,    -1,   130,    16,   272,   142,    -1,
     131,    16,   272,   142,    -1,   130,    16,   272,   142,   131,
      16,   272,   142,    -1,   129,    -1,     4,    -1,   272,   142,
     129,    -1,   272,   142,     4,    -1,    -1,   272,   142,   126,
       4,    56,   273,   272,   275,    57,    -1,    -1,   126,     4,
      56,   274,   272,   275,    57,    -1,   130,    -1,   272,   142,
     130,    -1,   131,    -1,   272,   142,   131,    -1,   126,    -1,
     272,   142,   126,    -1,    -1,   142,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   162,   162,   163,   164,   165,   166,   170,   174,   174,
     184,   184,   197,   198,   202,   203,   204,   207,   210,   211,
     212,   214,   216,   218,   220,   222,   224,   226,   228,   230,
     232,   234,   235,   236,   238,   240,   242,   244,   246,   247,
     249,   248,   252,   254,   258,   259,   260,   264,   266,   270,
     272,   277,   278,   279,   284,   284,   289,   291,   293,   298,
     298,   304,   305,   310,   311,   312,   313,   314,   315,   316,
     317,   318,   319,   320,   322,   324,   326,   329,   331,   333,
     335,   337,   339,   338,   343,   342,   346,   349,   348,   352,
     356,   357,   359,   361,   366,   369,   372,   375,   378,   381,
     385,   384,   389,   388,   393,   392,   399,   402,   405,   408,
     411,   414,   420,   424,   425,   426,   430,   432,   433,   433,
     441,   445,   449,   456,   462,   468,   474,   480,   486,   492,
     498,   504,   513,   522,   533,   542,   553,   561,   565,   572,
     574,   573,   580,   581,   585,   586,   591,   596,   597,   602,
     606,   606,   610,   609,   616,   617,   620,   622,   626,   628,
     630,   632,   634,   639,   646,   648,   652,   654,   656,   658,
     660,   662,   664,   666,   671,   671,   676,   680,   688,   692,
     700,   700,   704,   707,   707,   710,   711,   716,   715,   721,
     720,   727,   735,   743,   744,   748,   749,   753,   755,   760,
     765,   766,   771,   773,   779,   781,   783,   787,   789,   795,
     798,   807,   818,   818,   824,   826,   828,   830,   832,   834,
     837,   839,   841,   843,   845,   847,   849,   851,   853,   855,
     857,   859,   861,   863,   865,   867,   869,   871,   873,   875,
     877,   879,   882,   884,   886,   888,   890,   892,   894,   896,
     898,   900,   902,   904,   913,   915,   917,   919,   921,   923,
     925,   931,   932,   936,   937,   941,   942,   946,   947,   951,
     952,   953,   954,   957,   961,   964,   970,   972,   957,   979,
     981,   983,   988,   990,   978,  1000,  1002,  1000,  1008,  1007,
    1014,  1015,  1016,  1017,  1018,  1022,  1023,  1024,  1028,  1029,
    1034,  1035,  1040,  1041,  1046,  1047,  1052,  1054,  1059,  1062,
    1075,  1079,  1084,  1086,  1077,  1094,  1097,  1099,  1103,  1104,
    1103,  1113,  1158,  1161,  1173,  1182,  1185,  1192,  1192,  1204,
    1205,  1209,  1213,  1222,  1222,  1236,  1236,  1246,  1247,  1251,
    1255,  1259,  1266,  1270,  1278,  1281,  1285,  1289,  1293,  1300,
    1304,  1308,  1312,  1317,  1316,  1330,  1329,  1339,  1343,  1347,
    1351,  1355,  1359,  1365,  1367
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "INT", "NAME", "LNAME", "'='", "OREQ",
  "ANDEQ", "RSHIFTEQ", "LSHIFTEQ", "DIVEQ", "MULTEQ", "MINUSEQ", "PLUSEQ",
  "'?'", "':'", "OROR", "ANDAND", "'|'", "'^'", "'&'", "NE", "EQ", "'<'",
  "'>'", "GE", "LE", "RSHIFT", "LSHIFT", "'+'", "'-'", "'*'", "'/'", "'%'",
  "UNARY", "END", "'('", "ALIGN_K", "BLOCK", "BIND", "QUAD", "SQUAD",
  "LONG", "SHORT", "BYTE", "SECTIONS", "PHDRS", "INSERT_K", "AFTER",
  "BEFORE", "DATA_SEGMENT_ALIGN", "DATA_SEGMENT_RELRO_END",
  "DATA_SEGMENT_END", "SORT_BY_NAME", "SORT_BY_ALIGNMENT", "'{'", "'}'",
  "SIZEOF_HEADERS", "OUTPUT_FORMAT", "FORCE_COMMON_ALLOCATION",
  "OUTPUT_ARCH", "INHIBIT_COMMON_ALLOCATION", "SEGMENT_START", "INCLUDE",
  "MEMORY", "REGION_ALIAS", "NOLOAD", "DSECT", "COPY", "INFO", "OVERLAY",
  "DEFINED", "TARGET_K", "SEARCH_DIR", "MAP", "ENTRY", "NEXT", "SIZEOF",
  "ALIGNOF", "ADDR", "LOADADDR", "MAX_K", "MIN_K", "STARTUP", "HLL",
  "SYSLIB", "FLOAT", "NOFLOAT", "NOCROSSREFS", "ORIGIN", "FILL", "LENGTH",
  "CREATE_OBJECT_SYMBOLS", "INPUT", "OPTIONAL", "GROUP", "OUTPUT",
  "CONSTRUCTORS", "ALIGNMOD", "AT", "SUBALIGN", "PROVIDE",
  "PROVIDE_HIDDEN", "AS_NEEDED", "CHIP", "LIST", "SECT", "ABSOLUTE",
  "LOAD", "NEWLINE", "ENDWORD", "ORDER", "NAMEWORD", "ASSERT_K", "FORMAT",
  "PUBLIC", "DEFSYMEND", "BASE", "ALIAS", "TRUNCATE", "REL",
  "INPUT_SCRIPT", "INPUT_MRI_SCRIPT", "INPUT_DEFSYM", "CASE", "EXTERN",
  "START", "VERS_TAG", "VERS_IDENTIFIER", "GLOBAL", "LOCAL", "VERSIONK",
  "INPUT_VERSION_SCRIPT", "KEEP", "ONLY_IF_RO", "ONLY_IF_RW", "SPECIAL",
  "EXCLUDE_FILE", "CONSTANT", "INPUT_DYNAMIC_LIST", "','", "';'", "')'",
  "'['", "']'", "'!'", "'~'", "$accept", "file", "filename", "defsym_expr",
  "$@1", "mri_script_file", "$@2", "mri_script_lines",
  "mri_script_command", "$@3", "ordernamelist", "mri_load_name_list",
  "mri_abs_name_list", "casesymlist", "extern_name_list", "$@4",
  "extern_name_list_body", "script_file", "$@5", "ifile_list", "ifile_p1",
  "$@6", "$@7", "$@8", "input_list", "@9", "@10", "@11", "optional_list",
  "sections", "sec_or_group_p1", "statement_anywhere", "$@12",
  "wildcard_name", "wildcard_spec", "exclude_name_list", "file_NAME_list",
  "input_section_spec_no_keep", "input_section_spec", "$@13", "statement",
  "$@14", "$@15", "statement_list", "statement_list_opt", "length",
  "fill_exp", "fill_opt", "assign_op", "end", "assignment", "opt_comma",
  "memory", "memory_spec_list_opt", "memory_spec_list", "memory_spec",
  "$@16", "$@17", "origin_spec", "length_spec", "attributes_opt",
  "attributes_list", "attributes_string", "startup", "high_level_library",
  "high_level_library_NAME_list", "low_level_library",
  "low_level_library_NAME_list", "floating_point_support",
  "nocrossref_list", "mustbe_exp", "$@18", "exp", "memspec_at_opt",
  "opt_at", "opt_align", "opt_subalign", "sect_constraint", "section",
  "$@19", "$@20", "$@21", "$@22", "$@23", "$@24", "$@25", "$@26", "$@27",
  "$@28", "$@29", "$@30", "$@31", "type", "atype", "opt_exp_with_type",
  "opt_exp_without_type", "opt_nocrossrefs", "memspec_opt", "phdr_opt",
  "overlay_section", "$@32", "$@33", "$@34", "phdrs", "phdr_list", "phdr",
  "$@35", "$@36", "phdr_type", "phdr_qualifiers", "phdr_val",
  "dynamic_list_file", "$@37", "dynamic_list_nodes", "dynamic_list_node",
  "dynamic_list_tag", "version_script_file", "$@38", "version", "$@39",
  "vers_nodes", "vers_node", "verdep", "vers_tag", "vers_defns", "@40",
  "@41", "opt_semicolon", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,    61,   261,   262,   263,
     264,   265,   266,   267,   268,    63,    58,   269,   270,   124,
      94,    38,   271,   272,    60,    62,   273,   274,   275,   276,
      43,    45,    42,    47,    37,   277,   278,    40,   279,   280,
     281,   282,   283,   284,   285,   286,   287,   288,   289,   290,
     291,   292,   293,   294,   295,   296,   123,   125,   297,   298,
     299,   300,   301,   302,   303,   304,   305,   306,   307,   308,
     309,   310,   311,   312,   313,   314,   315,   316,   317,   318,
     319,   320,   321,   322,   323,   324,   325,   326,   327,   328,
     329,   330,   331,   332,   333,   334,   335,   336,   337,   338,
     339,   340,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   368,
     369,   370,   371,   372,   373,   374,   375,   376,   377,   378,
     379,    44,    59,    41,    91,    93,    33,   126
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   148,   149,   149,   149,   149,   149,   150,   152,   151,
     154,   153,   155,   155,   156,   156,   156,   156,   156,   156,
     156,   156,   156,   156,   156,   156,   156,   156,   156,   156,
     156,   156,   156,   156,   156,   156,   156,   156,   156,   156,
     157,   156,   156,   156,   158,   158,   158,   159,   159,   160,
     160,   161,   161,   161,   163,   162,   164,   164,   164,   166,
     165,   167,   167,   168,   168,   168,   168,   168,   168,   168,
     168,   168,   168,   168,   168,   168,   168,   168,   168,   168,
     168,   168,   169,   168,   170,   168,   168,   171,   168,   168,
     168,   168,   168,   168,   172,   172,   172,   172,   172,   172,
     173,   172,   174,   172,   175,   172,   176,   176,   176,   176,
     176,   176,   177,   178,   178,   178,   179,   179,   180,   179,
     181,   181,   181,   182,   182,   182,   182,   182,   182,   182,
     182,   182,   183,   183,   184,   184,   185,   185,   185,   186,
     187,   186,   188,   188,   188,   188,   188,   188,   188,   188,
     189,   188,   190,   188,   191,   191,   192,   192,   193,   193,
     193,   193,   193,   194,   195,   195,   196,   196,   196,   196,
     196,   196,   196,   196,   197,   197,   198,   198,   198,   198,
     199,   199,   200,   201,   201,   202,   202,   204,   203,   205,
     203,   206,   207,   208,   208,   209,   209,   210,   210,   211,
     212,   212,   213,   213,   214,   215,   215,   216,   216,   217,
     217,   217,   219,   218,   220,   220,   220,   220,   220,   220,
     220,   220,   220,   220,   220,   220,   220,   220,   220,   220,
     220,   220,   220,   220,   220,   220,   220,   220,   220,   220,
     220,   220,   220,   220,   220,   220,   220,   220,   220,   220,
     220,   220,   220,   220,   220,   220,   220,   220,   220,   220,
     220,   221,   221,   222,   222,   223,   223,   224,   224,   225,
     225,   225,   225,   227,   228,   229,   230,   231,   226,   232,
     233,   234,   235,   236,   226,   237,   238,   226,   239,   226,
     240,   240,   240,   240,   240,   241,   241,   241,   242,   242,
     242,   242,   243,   243,   244,   244,   245,   245,   246,   246,
     247,   248,   249,   250,   247,   251,   252,   252,   254,   255,
     253,   256,   257,   257,   257,   258,   258,   260,   259,   261,
     261,   262,   263,   265,   264,   267,   266,   268,   268,   269,
     269,   269,   270,   270,   271,   271,   271,   271,   271,   272,
     272,   272,   272,   273,   272,   274,   272,   272,   272,   272,
     272,   272,   272,   275,   275
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     2,     2,     2,     2,     1,     0,     4,
       0,     2,     3,     0,     2,     4,     1,     1,     2,     1,
       4,     4,     3,     2,     4,     3,     4,     4,     4,     4,
       4,     2,     2,     2,     4,     4,     2,     2,     2,     2,
       0,     5,     2,     0,     3,     2,     0,     1,     3,     1,
       3,     0,     1,     3,     0,     2,     1,     2,     3,     0,
       2,     2,     0,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     4,     4,     4,     4,     8,     4,     1,
       1,     4,     0,     5,     0,     5,     4,     0,     5,     4,
       4,     3,     3,     6,     1,     3,     2,     1,     3,     2,
       0,     5,     0,     7,     0,     6,     1,     3,     2,     1,
       3,     2,     4,     2,     2,     0,     4,     2,     0,     7,
       1,     1,     1,     1,     5,     4,     4,     7,     7,     7,
       7,     8,     2,     1,     3,     1,     1,     3,     4,     1,
       0,     5,     2,     1,     1,     1,     4,     1,     4,     4,
       0,     8,     0,     5,     2,     1,     0,     1,     1,     1,
       1,     1,     1,     1,     2,     0,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     3,     3,     6,     6,
       1,     0,     4,     1,     0,     3,     1,     0,     7,     0,
       5,     3,     3,     0,     3,     1,     2,     1,     2,     4,
       4,     3,     3,     1,     4,     3,     0,     1,     1,     0,
       2,     3,     0,     2,     2,     3,     4,     2,     2,     2,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     5,     3,     3,     4,
       1,     1,     4,     4,     4,     4,     4,     4,     4,     6,
       6,     6,     4,     6,     4,     1,     6,     6,     6,     4,
       4,     3,     0,     4,     0,     4,     0,     4,     0,     1,
       1,     1,     0,     0,     0,     0,     0,     0,    19,     0,
       0,     0,     0,     0,    18,     0,     0,     7,     0,     5,
       1,     1,     1,     1,     1,     3,     0,     2,     3,     2,
       6,    10,     2,     1,     0,     1,     2,     0,     0,     3,
       0,     0,     0,     0,    11,     4,     0,     2,     0,     0,
       6,     1,     0,     3,     5,     0,     3,     0,     2,     1,
       2,     4,     2,     0,     2,     0,     5,     1,     2,     4,
       5,     6,     1,     2,     0,     2,     4,     4,     8,     1,
       1,     3,     3,     0,     9,     0,     7,     1,     3,     1,
       3,     1,     3,     0,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,    59,    10,     8,   333,   327,     0,     2,    62,     3,
      13,     6,     0,     4,     0,     5,     0,     1,    60,    11,
       0,   344,     0,   334,   337,     0,   328,   329,     0,     0,
       0,     0,     0,    79,     0,    80,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   207,   208,     0,     0,
      82,    84,     0,     0,     0,   118,     0,    72,    61,    64,
      70,     0,    63,    66,    67,    68,    69,    65,    71,     0,
      16,     0,     0,     0,     0,    17,     0,     0,     0,    19,
      46,     0,     0,     0,     0,     0,     0,    51,    54,     0,
       0,     0,   350,   361,   349,   357,   359,     0,     0,   344,
     338,   357,   359,     0,     0,   330,   212,   173,   172,   171,
     170,   169,   168,   167,   166,   212,   115,   316,     0,     0,
       0,     0,     7,    87,   184,     0,     0,     0,     0,     0,
       0,     0,   206,   209,     0,     0,     0,     0,     0,     0,
       0,    54,   175,   174,   117,     0,     0,    40,     0,   240,
     255,     0,     0,     0,     0,     0,     0,     0,     0,   241,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    14,     0,    49,    31,
      47,    32,    18,    33,    23,     0,    36,     0,    37,    52,
      38,    39,     0,    42,    12,     9,     0,     0,     0,     0,
     345,     0,     0,   332,   176,     0,   177,     0,     0,    91,
      92,     0,     0,    62,   187,     0,     0,   181,   186,     0,
       0,     0,     0,     0,     0,   201,   203,   181,   181,   209,
       0,    94,    97,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    13,     0,     0,   218,   214,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     217,   219,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    25,     0,     0,    45,     0,     0,
       0,    22,     0,     0,    56,    55,   355,     0,     0,   339,
     352,   362,   351,   358,   360,     0,   331,   213,   273,   112,
       0,   279,   285,   114,   113,   318,   315,   317,     0,    76,
      78,   335,   193,   189,   182,   180,     0,     0,    73,    74,
      86,   116,   199,   200,     0,   204,     0,   209,   210,    89,
     100,    96,    99,     0,     0,    81,    94,    97,     0,     0,
       0,    75,   212,   212,     0,    90,     0,    27,    28,    43,
      29,    30,   215,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   238,   237,   235,   234,   233,   228,   227,
     231,   232,   230,   229,   226,   225,   223,   224,   220,   221,
     222,    15,    26,    24,    50,    48,    44,    20,    21,    35,
      34,    53,    57,     0,     0,   346,   347,     0,   342,   340,
       0,   296,   288,     0,   296,     0,     0,    88,     0,     0,
     184,   185,     0,   202,   205,   211,     0,   104,    95,    98,
       0,    96,    99,     0,    83,    85,     0,     0,     0,   336,
      41,     0,   248,   254,     0,     0,   252,     0,   239,   216,
     243,   242,   244,   245,     0,     0,   259,   260,   247,     0,
     246,     0,    58,   363,   360,   353,   343,   341,     0,     0,
     296,     0,   264,   115,   303,     0,   304,   286,   321,   322,
       0,   197,     0,     0,   195,     0,     0,    93,     0,     0,
     102,    95,    98,   178,   179,     0,     0,     0,     0,     0,
       0,     0,     0,   236,   364,     0,     0,     0,   290,   291,
     292,   293,   294,   297,     0,     0,     0,     0,   299,     0,
     266,     0,   302,   305,   264,     0,   325,     0,   319,     0,
     198,   194,   196,     0,   181,   190,   101,     0,     0,   119,
     249,   250,   251,   253,   256,   257,   258,   356,     0,   363,
     295,     0,   298,     0,     0,   268,   289,   268,   115,     0,
     322,     0,     0,    77,   212,     0,   105,     0,   348,     0,
     296,     0,     0,     0,   274,   280,     0,     0,   323,     0,
     320,   191,     0,   188,   103,   354,     0,     0,   263,     0,
       0,   272,     0,   287,   326,   322,   212,     0,   300,   265,
       0,   269,   270,   271,     0,   281,   324,   192,     0,   267,
     275,   310,   296,   156,     0,     0,   136,   122,   121,   158,
     159,   160,   161,   162,     0,     0,     0,     0,   143,   145,
     150,     0,     0,   144,     0,   123,     0,   139,   147,   155,
     157,     0,     0,     0,   311,   282,   301,     0,     0,   152,
     212,     0,   140,     0,   120,     0,   135,   181,     0,   154,
     276,   212,   142,     0,   307,     0,     0,     0,     0,     0,
       0,     0,     0,   156,     0,   163,     0,     0,   133,     0,
       0,   137,     0,   181,   307,     0,   156,     0,   262,     0,
       0,   146,     0,   125,     0,     0,   126,     0,   149,     0,
     120,     0,     0,   132,   134,   138,   262,   148,     0,   306,
       0,   308,     0,     0,     0,     0,     0,   153,     0,   141,
     124,   308,   312,     0,   165,     0,     0,     0,     0,     0,
       0,   165,   308,   261,   212,     0,   283,   128,   127,     0,
     129,   130,     0,   277,   165,   164,   309,   181,   131,   151,
     181,   313,   284,   278,   181,   314
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     6,   123,    11,    12,     9,    10,    19,    90,   245,
     182,   181,   179,   190,   191,   192,   305,     7,     8,    18,
      58,   135,   136,   213,   234,   446,   558,   509,   359,    59,
     207,   323,   140,   655,   656,   699,   677,   657,   658,   697,
     659,   671,   693,   660,   661,   662,   694,   756,   115,   144,
      61,   702,    62,   216,   217,   218,   332,   440,   554,   603,
     439,   503,   504,    63,    64,   227,    65,   228,    66,   230,
     695,   205,   250,   731,   540,   575,   594,   624,   324,   431,
     611,   633,   704,   770,   433,   612,   631,   684,   767,   434,
     545,   493,   534,   491,   492,   496,   544,   708,   744,   634,
     683,   752,   774,    67,   208,   327,   435,   582,   499,   548,
     580,    15,    16,    26,    27,   103,    13,    14,    68,    69,
      23,    24,   430,    97,    98,   527,   424,   525
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -655
static const yytype_int16 yypact[] =
{
     130,  -655,  -655,  -655,  -655,  -655,    68,  -655,  -655,  -655,
    -655,  -655,    94,  -655,    15,  -655,    74,  -655,   869,  1641,
     100,   101,    83,    15,  -655,   111,    74,  -655,   576,    97,
     141,    64,   110,  -655,   184,  -655,   220,   179,   208,   234,
     243,   249,   254,   259,   269,   278,  -655,  -655,   291,   293,
    -655,  -655,   305,   309,   315,  -655,   316,  -655,  -655,  -655,
    -655,   109,  -655,  -655,  -655,  -655,  -655,  -655,  -655,   125,
    -655,   273,   220,   350,   715,  -655,   351,   352,   354,  -655,
    -655,   359,   361,   362,   715,   364,   367,   368,  -655,   374,
     270,   715,  -655,   375,  -655,   355,   366,   326,   242,   101,
    -655,  -655,  -655,   328,   246,  -655,  -655,  -655,  -655,  -655,
    -655,  -655,  -655,  -655,  -655,  -655,  -655,  -655,   385,   388,
     390,   393,  -655,  -655,    48,   395,   398,   220,   220,   400,
     220,    28,  -655,   401,    30,   371,   372,   220,   407,   412,
     380,  -655,  -655,  -655,  -655,   363,    45,  -655,    55,  -655,
    -655,   715,   715,   715,   381,   383,   384,   389,   391,  -655,
     396,   397,   399,   402,   403,   409,   410,   411,   414,   415,
     417,   419,   420,   421,   715,   715,   874,   392,  -655,   284,
    -655,   286,    27,  -655,  -655,   489,  1817,   296,  -655,  -655,
     297,  -655,   428,  -655,  -655,  1817,   404,   111,   111,   307,
     117,   405,   317,   117,  -655,   715,  -655,   310,    61,  -655,
    -655,    17,   320,  -655,  -655,   220,   408,    10,  -655,   325,
     324,   333,   334,   335,   336,  -655,  -655,    95,   133,    41,
     338,  -655,  -655,   431,    16,    38,    30,   340,   488,   490,
     715,   358,    15,   715,   715,  -655,   715,   715,  -655,  -655,
    1078,   715,   715,   715,   715,   715,   493,   494,   715,   498,
     499,   500,   504,   715,   715,   505,   506,   715,   715,   507,
    -655,  -655,   715,   715,   715,   715,   715,   715,   715,   715,
     715,   715,   715,   715,   715,   715,   715,   715,   715,   715,
     715,   715,   715,   715,  1817,   509,   510,  -655,   511,   715,
     715,  1817,   261,   517,  -655,    44,  -655,   382,   387,  -655,
    -655,   518,  -655,  -655,  -655,   -34,  -655,  1817,   576,  -655,
     220,  -655,  -655,  -655,  -655,  -655,  -655,  -655,   519,  -655,
    -655,   932,   495,  -655,  -655,  -655,    48,   521,  -655,  -655,
    -655,  -655,  -655,  -655,   220,  -655,   220,   401,  -655,  -655,
    -655,  -655,  -655,   497,    88,  -655,   394,   406,    50,   416,
      20,  -655,  -655,  -655,  1459,  -655,    13,  1817,  1817,  1664,
    1817,  1817,  -655,  1058,  1098,  1479,  1499,  1118,   413,   430,
    1138,   433,   435,   437,   450,  1519,  1539,   451,   452,  1158,
    1584,   453,  1777,  1663,  1564,  1832,  1038,   819,  1023,  1023,
     457,   457,   457,   457,   303,   303,   271,   271,  -655,  -655,
    -655,  1817,  1817,  1817,  -655,  -655,  -655,  1817,  1817,  -655,
    -655,  -655,  -655,   526,   111,   219,   117,   479,  -655,  -655,
      -9,   561,  -655,   643,   561,   715,   422,  -655,     4,   528,
      48,  -655,   459,  -655,  -655,  -655,    30,  -655,  -655,  -655,
     508,   461,   462,    91,  -655,  -655,   463,   464,   542,  -655,
    -655,   715,  -655,  -655,   715,   715,  -655,   715,  -655,  -655,
    -655,  -655,  -655,  -655,   715,   715,  -655,  -655,  -655,   544,
    -655,   715,  -655,   432,   534,  -655,  -655,  -655,   230,   514,
    1689,   539,   458,  -655,  -655,  1797,   468,  -655,  1817,    35,
     558,  -655,   604,     2,  -655,   520,   573,  -655,    24,    30,
    -655,   472,   473,  -655,  -655,   474,  1185,  1205,  1225,  1245,
    1265,  1285,   477,  1817,   117,   554,   111,   111,  -655,  -655,
    -655,  -655,  -655,  -655,   478,   715,    19,   607,  -655,   588,
     589,   339,  -655,  -655,   458,   570,   592,   594,  -655,   491,
    -655,  -655,  -655,   626,   496,  -655,  -655,    33,    30,  -655,
    -655,  -655,  -655,  -655,  -655,  -655,  -655,  -655,   503,   432,
    -655,  1312,  -655,   715,   611,   548,  -655,   548,  -655,   715,
      35,   715,   512,  -655,  -655,   560,  -655,    79,   117,   593,
     250,  1332,   715,   618,  -655,  -655,   614,  1352,  -655,  1372,
    -655,  -655,   650,  -655,  -655,  -655,   620,   642,  -655,  1392,
     715,   188,   616,  -655,  -655,    35,  -655,   715,  -655,  -655,
    1412,  -655,  -655,  -655,   621,  -655,  -655,  -655,  1439,  -655,
    -655,  -655,   633,   776,    75,   660,   654,  -655,  -655,  -655,
    -655,  -655,  -655,  -655,   646,   647,   220,   649,  -655,  -655,
    -655,   651,   652,  -655,    26,  -655,   655,  -655,  -655,  -655,
     776,   622,   656,   109,  -655,  -655,  -655,    18,   240,  -655,
    -655,   661,  -655,    70,  -655,   662,  -655,   121,    26,  -655,
    -655,  -655,  -655,   631,   672,   665,   666,   562,   667,   566,
     674,   675,   571,   776,   584,  -655,   715,     8,  -655,    12,
     224,  -655,    26,   178,   672,   586,   776,   709,   630,    70,
      70,  -655,    70,  -655,    70,    70,  -655,   695,  -655,  1604,
     591,   595,    70,  -655,  -655,  -655,   630,  -655,   679,  -655,
     707,  -655,   596,   597,    32,   598,   599,  -655,   733,  -655,
    -655,  -655,  -655,   739,   120,   601,   605,    70,   606,   612,
     613,   120,  -655,  -655,  -655,   743,  -655,  -655,  -655,   615,
    -655,  -655,   109,  -655,   120,  -655,  -655,   496,  -655,  -655,
     496,  -655,  -655,  -655,   496,  -655
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -655,  -655,   -71,  -655,  -655,  -655,  -655,   515,  -655,  -655,
    -655,  -655,  -655,  -655,   609,  -655,  -655,  -655,  -655,   546,
    -655,  -655,  -655,  -655,  -217,  -655,  -655,  -655,  -655,  -655,
    -440,   -13,  -655,  -383,  -579,    49,    84,    66,  -655,  -655,
     104,  -655,  -655,  -655,  -602,  -655,    11,  -654,  -655,  -637,
    -584,  -214,  -655,   329,  -655,   434,  -655,  -655,  -655,  -655,
    -655,  -655,   268,  -655,  -655,  -655,  -655,  -655,  -655,  -220,
    -104,  -655,   -74,    46,   231,  -655,   197,  -655,  -655,  -655,
    -655,  -655,  -655,  -655,  -655,  -655,  -655,  -655,  -655,  -655,
    -655,  -655,  -655,  -483,   342,  -655,  -655,    73,  -612,  -655,
    -655,  -655,  -655,  -655,  -655,  -655,  -655,  -655,  -655,  -498,
    -655,  -655,  -655,  -655,   753,  -655,  -655,  -655,  -655,  -655,
     541,   -19,  -655,   682,   -10,  -655,  -655,   215
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -336
static const yytype_int16 yytable[] =
{
     176,   147,   204,   336,   100,    60,   501,   537,   501,   348,
     186,   206,   720,   344,   346,   104,   674,   195,   358,   360,
     351,   352,   674,   637,   351,   352,   682,   637,   351,   352,
     674,   297,   122,   637,   231,   232,   674,   351,   352,   546,
     638,   637,   356,   357,   638,   229,  -183,   637,   422,   663,
     638,   243,   214,   541,   451,   452,   221,   222,   638,   224,
     226,   246,   675,   645,   638,   325,   237,  -183,    17,    21,
     459,    21,   685,   686,   674,   676,   663,   248,   249,   664,
     675,   645,   598,   351,   352,   637,   528,   529,   530,   531,
     532,   717,   448,   449,   428,   511,   512,   763,    20,   676,
     270,   271,   638,   294,   728,    92,    91,   607,   429,   663,
     771,   301,   215,   118,   119,    92,   687,   626,   326,   486,
     353,   310,   663,   724,   353,   769,   754,   445,   353,   751,
      25,   317,   665,   487,   233,   547,   755,   353,   596,    99,
     764,    22,   233,    22,   333,   551,   652,   120,   502,   635,
     502,   335,   654,   116,   353,   722,   688,   354,   328,   355,
     329,   354,   533,   455,   652,   354,   364,   556,   298,   367,
     368,   225,   370,   371,   354,   747,   586,   373,   374,   375,
     376,   377,   347,   353,   380,   423,   244,   307,   308,   385,
     386,   453,   450,   389,   390,   450,   247,   117,   392,   393,
     394,   395,   396,   397,   398,   399,   400,   401,   402,   403,
     404,   405,   406,   407,   408,   409,   410,   411,   412,   413,
     354,   121,   604,   310,   122,   417,   418,    93,   674,   508,
      94,    95,    96,   149,   150,   124,   335,    93,   343,   637,
      94,   101,   102,   311,   674,   125,   312,   313,   314,   432,
     142,   143,     1,     2,     3,   637,   638,   145,   456,   457,
     151,   152,   335,     4,   419,   420,   701,   153,   154,   155,
       5,   126,   638,   443,   335,   444,   345,   146,   685,   686,
     127,   156,   157,   158,   689,   692,   128,   536,   159,   606,
     698,   129,   557,   160,   690,   691,   130,   528,   529,   530,
     531,   532,   161,   288,   289,   290,   131,   162,   163,   164,
     165,   166,   167,   168,   318,   132,   723,   689,    60,   335,
     169,   725,   170,   621,   622,   623,   732,   733,   133,   698,
     134,   735,   736,   286,   287,   288,   289,   290,   171,   740,
     585,   587,   137,   318,   172,   311,   138,   100,   312,   313,
     484,   723,   139,   141,   148,   177,   178,   490,   180,   495,
     490,   498,   688,   183,   759,   184,   185,   319,   187,   173,
     188,   197,   189,   533,   320,   576,   174,   175,   193,   196,
     194,   321,   198,   199,   200,   202,    42,   516,   203,   209,
     517,   518,   210,   519,   211,   149,   150,   212,   292,   219,
     520,   521,   220,   320,   223,   229,   322,   523,   235,   236,
     321,   238,    53,    54,   483,    42,   239,   240,   251,   242,
     252,   253,   151,   152,    55,   295,   254,   296,   255,   153,
     154,   155,   304,   256,   257,   322,   258,   302,   303,   259,
     260,    53,    54,   156,   157,   158,   261,   262,   263,   309,
     159,   264,   265,    55,   266,   160,   267,   268,   269,   316,
     306,   571,   315,   330,   161,   334,   337,   338,   350,   162,
     163,   164,   165,   166,   167,   168,   339,   340,   341,   342,
     601,   349,   169,   361,   170,   284,   285,   286,   287,   288,
     289,   290,   149,   150,   362,   299,   363,   378,   379,   591,
     171,   365,   381,   382,   383,   597,   172,   599,   384,   387,
     388,   391,   627,   414,   415,   416,   568,   569,   609,   151,
     152,   421,   427,   436,   425,   442,   153,   154,   155,   426,
     482,   173,   438,   293,   447,   485,   620,  -106,   174,   175,
     156,   157,   158,   628,   505,   510,   515,   159,   522,  -109,
     526,   535,   160,   772,   467,   538,   773,   543,   539,   454,
     775,   161,   549,   500,   149,   150,   162,   163,   164,   165,
     166,   167,   168,   468,   524,   669,   470,   705,   471,   169,
     472,   170,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   151,   152,   473,   476,   477,   480,   171,   488,   154,
     155,   489,   507,   172,  -108,  -111,   513,   514,   550,   555,
     553,   567,   156,   157,   158,  -107,  -110,   559,   318,   159,
     566,   570,   719,   572,   160,   573,   578,   574,   173,   579,
     300,   581,   584,   161,   583,   174,   175,   335,   162,   163,
     164,   165,   166,   167,   168,   588,   149,   150,   592,   593,
     605,   169,   602,   170,   600,   610,   616,   617,   618,   494,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   171,
     536,   613,   625,   151,   152,   172,   666,   630,   320,   680,
     153,   154,   155,   667,   668,   321,   670,   706,   672,   673,
      42,  -120,   678,   681,   156,   157,   158,   707,   696,   700,
     173,   159,   709,   710,   712,   711,   160,   174,   175,   713,
     322,   714,   715,   729,   716,   161,    53,    54,   149,   150,
     162,   163,   164,   165,   166,   167,   168,   718,    55,   727,
     730,   737,   743,   169,  -136,   170,   742,   750,   739,   745,
     746,   748,   749,   753,   757,   151,   152,   766,   758,   760,
     241,   171,   153,   154,   155,   761,   762,   172,   768,   331,
     369,   734,   703,   721,   679,   765,   156,   157,   158,   506,
     441,   552,   741,   159,   595,   577,   497,   726,   160,   105,
     636,   201,   173,   366,   589,     0,     0,   161,     0,   174,
     175,   637,   162,   163,   164,   165,   166,   167,   168,     0,
       0,     0,     0,     0,     0,   169,     0,   170,   638,     0,
       0,     0,     0,     0,     0,     0,     0,   639,   640,   641,
     642,   643,     0,   171,     0,     0,     0,     0,     0,   172,
     644,   645,     0,     0,     0,     0,     0,     0,     0,     0,
     646,   278,   279,   280,   281,   282,   283,   284,   285,   286,
     287,   288,   289,   290,   173,     0,     0,     0,     0,     0,
       0,   174,   175,     0,     0,     0,     0,   647,     0,   648,
       0,     0,     0,    28,   649,     0,     0,     0,    53,    54,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   272,
     650,   273,   274,   275,   276,   277,   278,   279,   280,   281,
     282,   283,   284,   285,   286,   287,   288,   289,   290,     0,
     651,     0,     0,     0,   652,    29,    30,    31,   653,     0,
     654,     0,     0,     0,     0,     0,     0,     0,    32,    33,
      34,    35,     0,    36,    37,    38,    28,     0,     0,     0,
       0,     0,    39,    40,    41,    42,     0,     0,     0,     0,
       0,     0,     0,    43,    44,    45,    46,    47,    48,     0,
       0,     0,     0,    49,    50,    51,    52,     0,   437,     0,
       0,    53,    54,     0,     0,     0,     0,     0,    29,    30,
      31,     0,     0,    55,     0,     0,     0,     0,     0,     0,
       0,    32,    33,    34,    35,    56,    36,    37,    38,     0,
       0,  -335,     0,     0,     0,    39,    40,    41,    42,     0,
       0,    57,     0,     0,     0,   291,    43,    44,    45,    46,
      47,    48,     0,     0,     0,     0,    49,    50,    51,    52,
       0,     0,     0,     0,    53,    54,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    55,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,    56,   277,
     278,   279,   280,   281,   282,   283,   284,   285,   286,   287,
     288,   289,   290,   272,    57,   273,   274,   275,   276,   277,
     278,   279,   280,   281,   282,   283,   284,   285,   286,   287,
     288,   289,   290,   272,     0,   273,   274,   275,   276,   277,
     278,   279,   280,   281,   282,   283,   284,   285,   286,   287,
     288,   289,   290,   272,     0,   273,   274,   275,   276,   277,
     278,   279,   280,   281,   282,   283,   284,   285,   286,   287,
     288,   289,   290,   272,     0,   273,   274,   275,   276,   277,
     278,   279,   280,   281,   282,   283,   284,   285,   286,   287,
     288,   289,   290,   272,     0,   273,   274,   275,   276,   277,
     278,   279,   280,   281,   282,   283,   284,   285,   286,   287,
     288,   289,   290,   272,     0,   273,   274,   275,   276,   277,
     278,   279,   280,   281,   282,   283,   284,   285,   286,   287,
     288,   289,   290,     0,     0,     0,     0,     0,     0,   461,
     272,   462,   273,   274,   275,   276,   277,   278,   279,   280,
     281,   282,   283,   284,   285,   286,   287,   288,   289,   290,
     272,   372,   273,   274,   275,   276,   277,   278,   279,   280,
     281,   282,   283,   284,   285,   286,   287,   288,   289,   290,
     272,   463,   273,   274,   275,   276,   277,   278,   279,   280,
     281,   282,   283,   284,   285,   286,   287,   288,   289,   290,
     272,   466,   273,   274,   275,   276,   277,   278,   279,   280,
     281,   282,   283,   284,   285,   286,   287,   288,   289,   290,
     272,   469,   273,   274,   275,   276,   277,   278,   279,   280,
     281,   282,   283,   284,   285,   286,   287,   288,   289,   290,
     272,   478,   273,   274,   275,   276,   277,   278,   279,   280,
     281,   282,   283,   284,   285,   286,   287,   288,   289,   290,
       0,     0,     0,     0,     0,     0,     0,   272,   560,   273,
     274,   275,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   272,   561,   273,
     274,   275,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   272,   562,   273,
     274,   275,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   272,   563,   273,
     274,   275,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   272,   564,   273,
     274,   275,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   272,   565,   273,
     274,   275,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,     0,     0,     0,
       0,     0,     0,     0,   272,   590,   273,   274,   275,   276,
     277,   278,   279,   280,   281,   282,   283,   284,   285,   286,
     287,   288,   289,   290,   272,   608,   273,   274,   275,   276,
     277,   278,   279,   280,   281,   282,   283,   284,   285,   286,
     287,   288,   289,   290,   272,   614,   273,   274,   275,   276,
     277,   278,   279,   280,   281,   282,   283,   284,   285,   286,
     287,   288,   289,   290,   272,   615,   273,   274,   275,   276,
     277,   278,   279,   280,   281,   282,   283,   284,   285,   286,
     287,   288,   289,   290,   272,   619,   273,   274,   275,   276,
     277,   278,   279,   280,   281,   282,   283,   284,   285,   286,
     287,   288,   289,   290,   272,   629,   273,   274,   275,   276,
     277,   278,   279,   280,   281,   282,   283,   284,   285,   286,
     287,   288,   289,   290,     0,     0,     0,     0,     0,     0,
       0,     0,   632,   275,   276,   277,   278,   279,   280,   281,
     282,   283,   284,   285,   286,   287,   288,   289,   290,   272,
     458,   273,   274,   275,   276,   277,   278,   279,   280,   281,
     282,   283,   284,   285,   286,   287,   288,   289,   290,   272,
     464,   273,   274,   275,   276,   277,   278,   279,   280,   281,
     282,   283,   284,   285,   286,   287,   288,   289,   290,     0,
     465,     0,     0,     0,     0,    70,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     474,     0,     0,     0,     0,     0,     0,     0,    70,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    71,
     475,   274,   275,   276,   277,   278,   279,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,     0,     0,
     460,     0,    71,     0,   272,    72,   273,   274,   275,   276,
     277,   278,   279,   280,   281,   282,   283,   284,   285,   286,
     287,   288,   289,   290,     0,   479,   536,     0,    72,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      73,     0,     0,     0,     0,   738,    74,    75,    76,    77,
      78,   -43,    79,    80,    81,     0,    82,    83,     0,    84,
      85,    86,     0,    73,     0,     0,    87,    88,    89,    74,
      75,    76,    77,    78,     0,    79,    80,    81,     0,    82,
      83,     0,    84,    85,    86,     0,     0,     0,     0,    87,
      88,    89,   272,   481,   273,   274,   275,   276,   277,   278,
     279,   280,   281,   282,   283,   284,   285,   286,   287,   288,
     289,   290,   272,   542,   273,   274,   275,   276,   277,   278,
     279,   280,   281,   282,   283,   284,   285,   286,   287,   288,
     289,   290,   272,     0,   273,   274,   275,   276,   277,   278,
     279,   280,   281,   282,   283,   284,   285,   286,   287,   288,
     289,   290,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290
};

static const yytype_int16 yycheck[] =
{
      74,    72,   106,   217,    23,    18,     4,   490,     4,   229,
      84,   115,     4,   227,   228,    25,     4,    91,   235,   236,
       4,     5,     4,    15,     4,     5,   663,    15,     4,     5,
       4,     4,     4,    15,     4,     5,     4,     4,     5,     4,
      32,    15,     4,     5,    32,     4,    36,    15,     4,   633,
      32,     6,     4,   493,     4,     5,   127,   128,    32,   130,
     131,     6,    54,    55,    32,     4,   137,    57,     0,    56,
      57,    56,    54,    55,     4,   654,   660,   151,   152,     4,
      54,    55,   580,     4,     5,    15,    67,    68,    69,    70,
      71,   693,     4,     5,   128,     4,     5,   751,     4,   678,
     174,   175,    32,   177,   706,     4,     6,   590,   142,   693,
     764,   185,    64,    49,    50,     4,    98,   615,    57,   128,
     104,     4,   706,   702,   104,   762,     6,   347,   104,   741,
      56,   205,    57,   142,   104,   100,    16,   104,   578,    56,
     752,   128,   104,   128,   215,   143,   138,    37,   146,   632,
     146,   141,   144,    56,   104,   143,   138,   141,   141,   143,
     143,   141,   143,   143,   138,   141,   240,   143,   141,   243,
     244,   143,   246,   247,   141,   143,   143,   251,   252,   253,
     254,   255,   141,   104,   258,   141,   141,   197,   198,   263,
     264,   141,   104,   267,   268,   104,   141,    56,   272,   273,
     274,   275,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     141,    37,   143,     4,     4,   299,   300,   126,     4,   446,
     129,   130,   131,     3,     4,    56,   141,   126,   143,    15,
     129,   130,   131,   126,     4,    37,   129,   130,   131,   320,
     141,   142,   122,   123,   124,    15,    32,   132,   362,   363,
      30,    31,   141,   133,     3,     4,   145,    37,    38,    39,
     140,    37,    32,   344,   141,   346,   143,     4,    54,    55,
      37,    51,    52,    53,   667,   668,    37,    37,    58,    39,
     673,    37,   509,    63,    54,    55,    37,    67,    68,    69,
      70,    71,    72,    32,    33,    34,    37,    77,    78,    79,
      80,    81,    82,    83,     4,    37,   699,   700,   331,   141,
      90,   143,    92,   135,   136,   137,   709,   710,    37,   712,
      37,   714,   715,    30,    31,    32,    33,    34,   108,   722,
     554,   558,    37,     4,   114,   126,    37,   366,   129,   130,
     131,   734,    37,    37,     4,     4,     4,   431,     4,   433,
     434,   435,   138,     4,   747,     4,     4,    57,     4,   139,
       3,    16,     4,   143,    64,    36,   146,   147,     4,     4,
     110,    71,    16,    57,   142,    57,    76,   461,   142,     4,
     464,   465,     4,   467,     4,     3,     4,     4,     6,     4,
     474,   475,     4,    64,     4,     4,    96,   481,    37,    37,
      71,     4,   102,   103,   424,    76,     4,    37,    37,    56,
      37,    37,    30,    31,   114,   141,    37,   141,    37,    37,
      38,    39,     4,    37,    37,    96,    37,   141,   141,    37,
      37,   102,   103,    51,    52,    53,    37,    37,    37,   142,
      58,    37,    37,   114,    37,    63,    37,    37,    37,   142,
      56,   535,    57,   143,    72,    57,   141,   143,    37,    77,
      78,    79,    80,    81,    82,    83,   143,   143,   143,   143,
     584,   143,    90,   143,    92,    28,    29,    30,    31,    32,
      33,    34,     3,     4,     6,     6,     6,     4,     4,   573,
     108,   143,     4,     4,     4,   579,   114,   581,     4,     4,
       4,     4,   616,     4,     4,     4,   526,   527,   592,    30,
      31,     4,     4,     4,   142,     4,    37,    38,    39,   142,
       4,   139,    37,   141,    37,    56,   610,   143,   146,   147,
      51,    52,    53,   617,    16,    37,     4,    58,     4,   143,
      16,    37,    63,   767,   141,    16,   770,    89,   100,   143,
     774,    72,     4,   141,     3,     4,    77,    78,    79,    80,
      81,    82,    83,   143,   142,   646,   143,   681,   143,    90,
     143,    92,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    30,    31,   143,   143,   143,   143,   108,    37,    38,
      39,    40,   143,   114,   143,   143,   143,   143,     4,    36,
      90,    57,    51,    52,    53,   143,   143,   143,     4,    58,
     143,   143,   696,    16,    63,    37,    56,    38,   139,    37,
     141,    37,     6,    72,   143,   146,   147,   141,    77,    78,
      79,    80,    81,    82,    83,   142,     3,     4,    37,   101,
      57,    90,    92,    92,   142,    37,     6,    37,    16,    16,
       6,     7,     8,     9,    10,    11,    12,    13,    14,   108,
      37,    57,    56,    30,    31,   114,    16,    56,    64,    57,
      37,    38,    39,    37,    37,    71,    37,    56,    37,    37,
      76,    37,    37,    37,    51,    52,    53,    25,    37,    37,
     139,    58,    37,    37,    37,   143,    63,   146,   147,   143,
      96,    37,    37,     4,   143,    72,   102,   103,     3,     4,
      77,    78,    79,    80,    81,    82,    83,   143,   114,   143,
     100,    36,    25,    90,   143,    92,    57,     4,   143,   143,
     143,   143,   143,     4,   143,    30,    31,     4,   143,   143,
     141,   108,    37,    38,    39,   143,   143,   114,   143,   213,
     245,   712,   678,   697,   660,   754,    51,    52,    53,   440,
     336,   503,   726,    58,   577,   544,   434,   704,    63,    26,
       4,    99,   139,   242,   569,    -1,    -1,    72,    -1,   146,
     147,    15,    77,    78,    79,    80,    81,    82,    83,    -1,
      -1,    -1,    -1,    -1,    -1,    90,    -1,    92,    32,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    41,    42,    43,
      44,    45,    -1,   108,    -1,    -1,    -1,    -1,    -1,   114,
      54,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      64,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,   139,    -1,    -1,    -1,    -1,    -1,
      -1,   146,   147,    -1,    -1,    -1,    -1,    91,    -1,    93,
      -1,    -1,    -1,     4,    98,    -1,    -1,    -1,   102,   103,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    15,
     114,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    -1,
     134,    -1,    -1,    -1,   138,    46,    47,    48,   142,    -1,
     144,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    59,    60,
      61,    62,    -1,    64,    65,    66,     4,    -1,    -1,    -1,
      -1,    -1,    73,    74,    75,    76,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    84,    85,    86,    87,    88,    89,    -1,
      -1,    -1,    -1,    94,    95,    96,    97,    -1,    36,    -1,
      -1,   102,   103,    -1,    -1,    -1,    -1,    -1,    46,    47,
      48,    -1,    -1,   114,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    59,    60,    61,    62,   126,    64,    65,    66,    -1,
      -1,   132,    -1,    -1,    -1,    73,    74,    75,    76,    -1,
      -1,   142,    -1,    -1,    -1,   141,    84,    85,    86,    87,
      88,    89,    -1,    -1,    -1,    -1,    94,    95,    96,    97,
      -1,    -1,    -1,    -1,   102,   103,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   114,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,   126,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    15,   142,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    15,    -1,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    15,    -1,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    15,    -1,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    15,    -1,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    15,    -1,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    -1,    -1,    -1,    -1,    -1,    -1,   141,
      15,   143,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      15,   143,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      15,   143,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      15,   143,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      15,   143,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      15,   143,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    15,   143,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    15,   143,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    15,   143,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    15,   143,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    15,   143,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    15,   143,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    15,   143,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    15,   143,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    15,   143,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    15,   143,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    15,   143,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    15,   143,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   143,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    15,
     141,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    15,
     141,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    -1,
     141,    -1,    -1,    -1,    -1,     4,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     141,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     4,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    38,
     141,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    -1,    -1,
      36,    -1,    38,    -1,    15,    64,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    -1,   141,    37,    -1,    64,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      99,    -1,    -1,    -1,    -1,   141,   105,   106,   107,   108,
     109,   110,   111,   112,   113,    -1,   115,   116,    -1,   118,
     119,   120,    -1,    99,    -1,    -1,   125,   126,   127,   105,
     106,   107,   108,   109,    -1,   111,   112,   113,    -1,   115,
     116,    -1,   118,   119,   120,    -1,    -1,    -1,    -1,   125,
     126,   127,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    15,    -1,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,   122,   123,   124,   133,   140,   149,   165,   166,   153,
     154,   151,   152,   264,   265,   259,   260,     0,   167,   155,
       4,    56,   128,   268,   269,    56,   261,   262,     4,    46,
      47,    48,    59,    60,    61,    62,    64,    65,    66,    73,
      74,    75,    76,    84,    85,    86,    87,    88,    89,    94,
      95,    96,    97,   102,   103,   114,   126,   142,   168,   177,
     179,   198,   200,   211,   212,   214,   216,   251,   266,   267,
       4,    38,    64,    99,   105,   106,   107,   108,   109,   111,
     112,   113,   115,   116,   118,   119,   120,   125,   126,   127,
     156,     6,     4,   126,   129,   130,   131,   271,   272,    56,
     269,   130,   131,   263,   272,   262,     6,     7,     8,     9,
      10,    11,    12,    13,    14,   196,    56,    56,    49,    50,
      37,    37,     4,   150,    56,    37,    37,    37,    37,    37,
      37,    37,    37,    37,    37,   169,   170,    37,    37,    37,
     180,    37,   141,   142,   197,   132,     4,   150,     4,     3,
       4,    30,    31,    37,    38,    39,    51,    52,    53,    58,
      63,    72,    77,    78,    79,    80,    81,    82,    83,    90,
      92,   108,   114,   139,   146,   147,   220,     4,     4,   160,
       4,   159,   158,     4,     4,     4,   220,     4,     3,     4,
     161,   162,   163,     4,   110,   220,     4,    16,    16,    57,
     142,   271,    57,   142,   218,   219,   218,   178,   252,     4,
       4,     4,     4,   171,     4,    64,   201,   202,   203,     4,
       4,   150,   150,     4,   150,   143,   150,   213,   215,     4,
     217,     4,     5,   104,   172,    37,    37,   150,     4,     4,
      37,   162,    56,     6,   141,   157,     6,   141,   220,   220,
     220,    37,    37,    37,    37,    37,    37,    37,    37,    37,
      37,    37,    37,    37,    37,    37,    37,    37,    37,    37,
     220,   220,    15,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,   141,     6,   141,   220,   141,   141,     4,   141,     6,
     141,   220,   141,   141,     4,   164,    56,   272,   272,   142,
       4,   126,   129,   130,   131,    57,   142,   220,     4,    57,
      64,    71,    96,   179,   226,     4,    57,   253,   141,   143,
     143,   167,   204,   150,    57,   141,   199,   141,   143,   143,
     143,   143,   143,   143,   199,   143,   199,   141,   217,   143,
      37,     4,     5,   104,   141,   143,     4,     5,   172,   176,
     172,   143,     6,     6,   220,   143,   268,   220,   220,   155,
     220,   220,   143,   220,   220,   220,   220,   220,     4,     4,
     220,     4,     4,     4,     4,   220,   220,     4,     4,   220,
     220,     4,   220,   220,   220,   220,   220,   220,   220,   220,
     220,   220,   220,   220,   220,   220,   220,   220,   220,   220,
     220,   220,   220,   220,     4,     4,     4,   220,   220,     3,
       4,     4,     4,   141,   274,   142,   142,     4,   128,   142,
     270,   227,   150,   232,   237,   254,     4,    36,    37,   208,
     205,   203,     4,   150,   150,   217,   173,    37,     4,     5,
     104,     4,     5,   141,   143,   143,   218,   218,   141,    57,
      36,   141,   143,   143,   141,   141,   143,   141,   143,   143,
     143,   143,   143,   143,   141,   141,   143,   143,   143,   141,
     143,    16,     4,   272,   131,    56,   128,   142,    37,    40,
     220,   241,   242,   239,    16,   220,   243,   242,   220,   256,
     141,     4,   146,   209,   210,    16,   201,   143,   172,   175,
      37,     4,     5,   143,   143,     4,   220,   220,   220,   220,
     220,   220,     4,   220,   142,   275,    16,   273,    67,    68,
      69,    70,    71,   143,   240,    37,    37,   241,    16,   100,
     222,   178,    16,    89,   244,   238,     4,   100,   257,     4,
       4,   143,   210,    90,   206,    36,   143,   172,   174,   143,
     143,   143,   143,   143,   143,   143,   143,    57,   272,   272,
     143,   220,    16,    37,    38,   223,    36,   222,    56,    37,
     258,    37,   255,   143,     6,   199,   143,   172,   142,   275,
     143,   220,    37,   101,   224,   224,   178,   220,   257,   220,
     142,   218,    92,   207,   143,    57,    39,   241,   143,   220,
      37,   228,   233,    57,   143,   143,     6,    37,    16,   143,
     220,   135,   136,   137,   225,    56,   257,   218,   220,   143,
      56,   234,   143,   229,   247,   241,     4,    15,    32,    41,
      42,    43,    44,    45,    54,    55,    64,    91,    93,    98,
     114,   134,   138,   142,   144,   181,   182,   185,   186,   188,
     191,   192,   193,   198,     4,    57,    16,    37,    37,   150,
      37,   189,    37,    37,     4,    54,   182,   184,    37,   188,
      57,    37,   197,   248,   235,    54,    55,    98,   138,   181,
      54,    55,   181,   190,   194,   218,    37,   187,   181,   183,
      37,   145,   199,   184,   230,   218,    56,    25,   245,    37,
      37,   143,    37,   143,    37,    37,   143,   192,   143,   220,
       4,   185,   143,   181,   182,   143,   245,   143,   192,     4,
     100,   221,   181,   181,   183,   181,   181,    36,   141,   143,
     181,   221,    57,    25,   246,   143,   143,   143,   143,   143,
       4,   246,   249,     4,     6,    16,   195,   143,   143,   181,
     143,   143,   143,   195,   246,   194,     4,   236,   143,   197,
     231,   195,   199,   199,   250,   199
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{


    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 8:

/* Line 1455 of yacc.c  */
#line 174 "ldgram.y"
    { ldlex_defsym(); }
    break;

  case 9:

/* Line 1455 of yacc.c  */
#line 176 "ldgram.y"
    {
		  ldlex_popstate();
		  lang_add_assignment(exp_assop((yyvsp[(3) - (4)].token),(yyvsp[(2) - (4)].name),(yyvsp[(4) - (4)].etree)));
		}
    break;

  case 10:

/* Line 1455 of yacc.c  */
#line 184 "ldgram.y"
    {
		  ldlex_mri_script ();
		  PUSH_ERROR (_("MRI style script"));
		}
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 189 "ldgram.y"
    {
		  ldlex_popstate ();
		  mri_draw_tree ();
		  POP_ERROR ();
		}
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 204 "ldgram.y"
    {
			einfo(_("%P%F: unrecognised keyword in MRI style script '%s'\n"),(yyvsp[(1) - (1)].name));
			}
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 207 "ldgram.y"
    {
			config.map_filename = "-";
			}
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 213 "ldgram.y"
    { mri_public((yyvsp[(2) - (4)].name), (yyvsp[(4) - (4)].etree)); }
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 215 "ldgram.y"
    { mri_public((yyvsp[(2) - (4)].name), (yyvsp[(4) - (4)].etree)); }
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 217 "ldgram.y"
    { mri_public((yyvsp[(2) - (3)].name), (yyvsp[(3) - (3)].etree)); }
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 219 "ldgram.y"
    { mri_format((yyvsp[(2) - (2)].name)); }
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 221 "ldgram.y"
    { mri_output_section((yyvsp[(2) - (4)].name), (yyvsp[(4) - (4)].etree));}
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 223 "ldgram.y"
    { mri_output_section((yyvsp[(2) - (3)].name), (yyvsp[(3) - (3)].etree));}
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 225 "ldgram.y"
    { mri_output_section((yyvsp[(2) - (4)].name), (yyvsp[(4) - (4)].etree));}
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 227 "ldgram.y"
    { mri_align((yyvsp[(2) - (4)].name),(yyvsp[(4) - (4)].etree)); }
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 229 "ldgram.y"
    { mri_align((yyvsp[(2) - (4)].name),(yyvsp[(4) - (4)].etree)); }
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 231 "ldgram.y"
    { mri_alignmod((yyvsp[(2) - (4)].name),(yyvsp[(4) - (4)].etree)); }
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 233 "ldgram.y"
    { mri_alignmod((yyvsp[(2) - (4)].name),(yyvsp[(4) - (4)].etree)); }
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 237 "ldgram.y"
    { mri_name((yyvsp[(2) - (2)].name)); }
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 239 "ldgram.y"
    { mri_alias((yyvsp[(2) - (4)].name),(yyvsp[(4) - (4)].name),0);}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 241 "ldgram.y"
    { mri_alias ((yyvsp[(2) - (4)].name), 0, (int) (yyvsp[(4) - (4)].bigint).integer); }
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 243 "ldgram.y"
    { mri_base((yyvsp[(2) - (2)].etree)); }
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 245 "ldgram.y"
    { mri_truncate ((unsigned int) (yyvsp[(2) - (2)].bigint).integer); }
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 249 "ldgram.y"
    { ldlex_script (); ldfile_open_command_file((yyvsp[(2) - (2)].name)); }
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 251 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 253 "ldgram.y"
    { lang_add_entry ((yyvsp[(2) - (2)].name), FALSE); }
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 258 "ldgram.y"
    { mri_order((yyvsp[(3) - (3)].name)); }
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 259 "ldgram.y"
    { mri_order((yyvsp[(2) - (2)].name)); }
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 265 "ldgram.y"
    { mri_load((yyvsp[(1) - (1)].name)); }
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 266 "ldgram.y"
    { mri_load((yyvsp[(3) - (3)].name)); }
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 271 "ldgram.y"
    { mri_only_load((yyvsp[(1) - (1)].name)); }
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 273 "ldgram.y"
    { mri_only_load((yyvsp[(3) - (3)].name)); }
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 277 "ldgram.y"
    { (yyval.name) = NULL; }
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 284 "ldgram.y"
    { ldlex_expression (); }
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 286 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 290 "ldgram.y"
    { ldlang_add_undef ((yyvsp[(1) - (1)].name)); }
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 292 "ldgram.y"
    { ldlang_add_undef ((yyvsp[(2) - (2)].name)); }
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 294 "ldgram.y"
    { ldlang_add_undef ((yyvsp[(3) - (3)].name)); }
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 298 "ldgram.y"
    { ldlex_both(); }
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 300 "ldgram.y"
    { ldlex_popstate(); }
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 321 "ldgram.y"
    { lang_add_target((yyvsp[(3) - (4)].name)); }
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 323 "ldgram.y"
    { ldfile_add_library_path ((yyvsp[(3) - (4)].name), FALSE); }
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 325 "ldgram.y"
    { lang_add_output((yyvsp[(3) - (4)].name), 1); }
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 327 "ldgram.y"
    { lang_add_output_format ((yyvsp[(3) - (4)].name), (char *) NULL,
					    (char *) NULL, 1); }
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 330 "ldgram.y"
    { lang_add_output_format ((yyvsp[(3) - (8)].name), (yyvsp[(5) - (8)].name), (yyvsp[(7) - (8)].name), 1); }
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 332 "ldgram.y"
    { ldfile_set_output_arch ((yyvsp[(3) - (4)].name), bfd_arch_unknown); }
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 334 "ldgram.y"
    { command_line.force_common_definition = TRUE ; }
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 336 "ldgram.y"
    { command_line.inhibit_common_definition = TRUE ; }
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 339 "ldgram.y"
    { lang_enter_group (); }
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 341 "ldgram.y"
    { lang_leave_group (); }
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 343 "ldgram.y"
    { lang_enter_group (); }
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 345 "ldgram.y"
    { lang_leave_group (); }
    break;

  case 86:

/* Line 1455 of yacc.c  */
#line 347 "ldgram.y"
    { lang_add_map((yyvsp[(3) - (4)].name)); }
    break;

  case 87:

/* Line 1455 of yacc.c  */
#line 349 "ldgram.y"
    { ldlex_script (); ldfile_open_command_file((yyvsp[(2) - (2)].name)); }
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 351 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 89:

/* Line 1455 of yacc.c  */
#line 353 "ldgram.y"
    {
		  lang_add_nocrossref ((yyvsp[(3) - (4)].nocrossref));
		}
    break;

  case 91:

/* Line 1455 of yacc.c  */
#line 358 "ldgram.y"
    { lang_add_insert ((yyvsp[(3) - (3)].name), 0); }
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 360 "ldgram.y"
    { lang_add_insert ((yyvsp[(3) - (3)].name), 1); }
    break;

  case 93:

/* Line 1455 of yacc.c  */
#line 362 "ldgram.y"
    { lang_memory_region_alias ((yyvsp[(3) - (6)].name), (yyvsp[(5) - (6)].name)); }
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 367 "ldgram.y"
    { lang_add_input_file((yyvsp[(1) - (1)].name),lang_input_file_is_search_file_enum,
				 (char *)NULL); }
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 370 "ldgram.y"
    { lang_add_input_file((yyvsp[(3) - (3)].name),lang_input_file_is_search_file_enum,
				 (char *)NULL); }
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 373 "ldgram.y"
    { lang_add_input_file((yyvsp[(2) - (2)].name),lang_input_file_is_search_file_enum,
				 (char *)NULL); }
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 376 "ldgram.y"
    { lang_add_input_file((yyvsp[(1) - (1)].name),lang_input_file_is_l_enum,
				 (char *)NULL); }
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 379 "ldgram.y"
    { lang_add_input_file((yyvsp[(3) - (3)].name),lang_input_file_is_l_enum,
				 (char *)NULL); }
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 382 "ldgram.y"
    { lang_add_input_file((yyvsp[(2) - (2)].name),lang_input_file_is_l_enum,
				 (char *)NULL); }
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 385 "ldgram.y"
    { (yyval.integer) = add_DT_NEEDED_for_regular; add_DT_NEEDED_for_regular = TRUE; }
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 387 "ldgram.y"
    { add_DT_NEEDED_for_regular = (yyvsp[(3) - (5)].integer); }
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 389 "ldgram.y"
    { (yyval.integer) = add_DT_NEEDED_for_regular; add_DT_NEEDED_for_regular = TRUE; }
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 391 "ldgram.y"
    { add_DT_NEEDED_for_regular = (yyvsp[(5) - (7)].integer); }
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 393 "ldgram.y"
    { (yyval.integer) = add_DT_NEEDED_for_regular; add_DT_NEEDED_for_regular = TRUE; }
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 395 "ldgram.y"
    { add_DT_NEEDED_for_regular = (yyvsp[(4) - (6)].integer); }
    break;

  case 106:

/* Line 1455 of yacc.c  */
#line 400 "ldgram.y"
    { lang_add_input_file((yyvsp[(1) - (1)].name),lang_input_file_is_optional_search_file_enum,
				 (char *)NULL); }
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 403 "ldgram.y"
    { lang_add_input_file((yyvsp[(3) - (3)].name),lang_input_file_is_optional_search_file_enum,
				 (char *)NULL); }
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 406 "ldgram.y"
    { lang_add_input_file((yyvsp[(2) - (2)].name),lang_input_file_is_optional_search_file_enum,
				 (char *)NULL); }
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 409 "ldgram.y"
    { lang_add_input_file((yyvsp[(1) - (1)].name),lang_input_file_is_optional_l_enum,
				 (char *)NULL); }
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 412 "ldgram.y"
    { lang_add_input_file((yyvsp[(3) - (3)].name),lang_input_file_is_optional_l_enum,
				 (char *)NULL); }
    break;

  case 111:

/* Line 1455 of yacc.c  */
#line 415 "ldgram.y"
    { lang_add_input_file((yyvsp[(2) - (2)].name),lang_input_file_is_optional_l_enum,
				 (char *)NULL); }
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 431 "ldgram.y"
    { lang_add_entry ((yyvsp[(3) - (4)].name), FALSE); }
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 433 "ldgram.y"
    {ldlex_expression ();}
    break;

  case 119:

/* Line 1455 of yacc.c  */
#line 434 "ldgram.y"
    { ldlex_popstate ();
		  lang_add_assignment (exp_assert ((yyvsp[(4) - (7)].etree), (yyvsp[(6) - (7)].name))); }
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 442 "ldgram.y"
    {
			  (yyval.cname) = (yyvsp[(1) - (1)].name);
			}
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 446 "ldgram.y"
    {
			  (yyval.cname) = "*";
			}
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 450 "ldgram.y"
    {
			  (yyval.cname) = "?";
			}
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 457 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(1) - (1)].cname);
			  (yyval.wildcard).sorted = none;
			  (yyval.wildcard).exclude_name_list = NULL;
			}
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 463 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(5) - (5)].cname);
			  (yyval.wildcard).sorted = none;
			  (yyval.wildcard).exclude_name_list = (yyvsp[(3) - (5)].name_list);
			}
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 469 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(3) - (4)].cname);
			  (yyval.wildcard).sorted = by_name;
			  (yyval.wildcard).exclude_name_list = NULL;
			}
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 475 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(3) - (4)].cname);
			  (yyval.wildcard).sorted = by_alignment;
			  (yyval.wildcard).exclude_name_list = NULL;
			}
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 481 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(5) - (7)].cname);
			  (yyval.wildcard).sorted = by_name_alignment;
			  (yyval.wildcard).exclude_name_list = NULL;
			}
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 487 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(5) - (7)].cname);
			  (yyval.wildcard).sorted = by_name;
			  (yyval.wildcard).exclude_name_list = NULL;
			}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 493 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(5) - (7)].cname);
			  (yyval.wildcard).sorted = by_alignment_name;
			  (yyval.wildcard).exclude_name_list = NULL;
			}
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 499 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(5) - (7)].cname);
			  (yyval.wildcard).sorted = by_alignment;
			  (yyval.wildcard).exclude_name_list = NULL;
			}
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 505 "ldgram.y"
    {
			  (yyval.wildcard).name = (yyvsp[(7) - (8)].cname);
			  (yyval.wildcard).sorted = by_name;
			  (yyval.wildcard).exclude_name_list = (yyvsp[(5) - (8)].name_list);
			}
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 514 "ldgram.y"
    {
			  struct name_list *tmp;
			  tmp = (struct name_list *) xmalloc (sizeof *tmp);
			  tmp->name = (yyvsp[(2) - (2)].cname);
			  tmp->next = (yyvsp[(1) - (2)].name_list);
			  (yyval.name_list) = tmp;
			}
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 523 "ldgram.y"
    {
			  struct name_list *tmp;
			  tmp = (struct name_list *) xmalloc (sizeof *tmp);
			  tmp->name = (yyvsp[(1) - (1)].cname);
			  tmp->next = NULL;
			  (yyval.name_list) = tmp;
			}
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 534 "ldgram.y"
    {
			  struct wildcard_list *tmp;
			  tmp = (struct wildcard_list *) xmalloc (sizeof *tmp);
			  tmp->next = (yyvsp[(1) - (3)].wildcard_list);
			  tmp->spec = (yyvsp[(3) - (3)].wildcard);
			  (yyval.wildcard_list) = tmp;
			}
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 543 "ldgram.y"
    {
			  struct wildcard_list *tmp;
			  tmp = (struct wildcard_list *) xmalloc (sizeof *tmp);
			  tmp->next = NULL;
			  tmp->spec = (yyvsp[(1) - (1)].wildcard);
			  (yyval.wildcard_list) = tmp;
			}
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 554 "ldgram.y"
    {
			  struct wildcard_spec tmp;
			  tmp.name = (yyvsp[(1) - (1)].name);
			  tmp.exclude_name_list = NULL;
			  tmp.sorted = none;
			  lang_add_wild (&tmp, NULL, ldgram_had_keep);
			}
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 562 "ldgram.y"
    {
			  lang_add_wild (NULL, (yyvsp[(2) - (3)].wildcard_list), ldgram_had_keep);
			}
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 566 "ldgram.y"
    {
			  lang_add_wild (&(yyvsp[(1) - (4)].wildcard), (yyvsp[(3) - (4)].wildcard_list), ldgram_had_keep);
			}
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 574 "ldgram.y"
    { ldgram_had_keep = TRUE; }
    break;

  case 141:

/* Line 1455 of yacc.c  */
#line 576 "ldgram.y"
    { ldgram_had_keep = FALSE; }
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 582 "ldgram.y"
    {
 		lang_add_attribute(lang_object_symbols_statement_enum);
	      	}
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 587 "ldgram.y"
    {

		  lang_add_attribute(lang_constructors_statement_enum);
		}
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 592 "ldgram.y"
    {
		  constructors_sorted = TRUE;
		  lang_add_attribute (lang_constructors_statement_enum);
		}
    break;

  case 148:

/* Line 1455 of yacc.c  */
#line 598 "ldgram.y"
    {
			  lang_add_data ((int) (yyvsp[(1) - (4)].integer), (yyvsp[(3) - (4)].etree));
			}
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 603 "ldgram.y"
    {
			  lang_add_fill ((yyvsp[(3) - (4)].fill));
			}
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 606 "ldgram.y"
    {ldlex_expression ();}
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 607 "ldgram.y"
    { ldlex_popstate ();
			  lang_add_assignment (exp_assert ((yyvsp[(4) - (8)].etree), (yyvsp[(6) - (8)].name))); }
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 610 "ldgram.y"
    { ldlex_script (); ldfile_open_command_file((yyvsp[(2) - (2)].name)); }
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 612 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 158:

/* Line 1455 of yacc.c  */
#line 627 "ldgram.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].token); }
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 629 "ldgram.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].token); }
    break;

  case 160:

/* Line 1455 of yacc.c  */
#line 631 "ldgram.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].token); }
    break;

  case 161:

/* Line 1455 of yacc.c  */
#line 633 "ldgram.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].token); }
    break;

  case 162:

/* Line 1455 of yacc.c  */
#line 635 "ldgram.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].token); }
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 640 "ldgram.y"
    {
		  (yyval.fill) = exp_get_fill ((yyvsp[(1) - (1)].etree), 0, "fill value");
		}
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 647 "ldgram.y"
    { (yyval.fill) = (yyvsp[(2) - (2)].fill); }
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 648 "ldgram.y"
    { (yyval.fill) = (fill_type *) 0; }
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 653 "ldgram.y"
    { (yyval.token) = '+'; }
    break;

  case 167:

/* Line 1455 of yacc.c  */
#line 655 "ldgram.y"
    { (yyval.token) = '-'; }
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 657 "ldgram.y"
    { (yyval.token) = '*'; }
    break;

  case 169:

/* Line 1455 of yacc.c  */
#line 659 "ldgram.y"
    { (yyval.token) = '/'; }
    break;

  case 170:

/* Line 1455 of yacc.c  */
#line 661 "ldgram.y"
    { (yyval.token) = LSHIFT; }
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 663 "ldgram.y"
    { (yyval.token) = RSHIFT; }
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 665 "ldgram.y"
    { (yyval.token) = '&'; }
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 667 "ldgram.y"
    { (yyval.token) = '|'; }
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 677 "ldgram.y"
    {
		  lang_add_assignment (exp_assop ((yyvsp[(2) - (3)].token), (yyvsp[(1) - (3)].name), (yyvsp[(3) - (3)].etree)));
		}
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 681 "ldgram.y"
    {
		  lang_add_assignment (exp_assop ('=', (yyvsp[(1) - (3)].name),
						  exp_binop ((yyvsp[(2) - (3)].token),
							     exp_nameop (NAME,
									 (yyvsp[(1) - (3)].name)),
							     (yyvsp[(3) - (3)].etree))));
		}
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 689 "ldgram.y"
    {
		  lang_add_assignment (exp_provide ((yyvsp[(3) - (6)].name), (yyvsp[(5) - (6)].etree), FALSE));
		}
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 693 "ldgram.y"
    {
		  lang_add_assignment (exp_provide ((yyvsp[(3) - (6)].name), (yyvsp[(5) - (6)].etree), TRUE));
		}
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 716 "ldgram.y"
    { region = lang_memory_region_lookup ((yyvsp[(1) - (1)].name), TRUE); }
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 719 "ldgram.y"
    {}
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 721 "ldgram.y"
    { ldlex_script (); ldfile_open_command_file((yyvsp[(2) - (2)].name)); }
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 723 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 191:

/* Line 1455 of yacc.c  */
#line 728 "ldgram.y"
    {
		  region->origin = exp_get_vma ((yyvsp[(3) - (3)].etree), 0, "origin");
		  region->current = region->origin;
		}
    break;

  case 192:

/* Line 1455 of yacc.c  */
#line 736 "ldgram.y"
    {
		  region->length = exp_get_vma ((yyvsp[(3) - (3)].etree), -1, "length");
		}
    break;

  case 193:

/* Line 1455 of yacc.c  */
#line 743 "ldgram.y"
    { /* dummy action to avoid bison 1.25 error message */ }
    break;

  case 197:

/* Line 1455 of yacc.c  */
#line 754 "ldgram.y"
    { lang_set_flags (region, (yyvsp[(1) - (1)].name), 0); }
    break;

  case 198:

/* Line 1455 of yacc.c  */
#line 756 "ldgram.y"
    { lang_set_flags (region, (yyvsp[(2) - (2)].name), 1); }
    break;

  case 199:

/* Line 1455 of yacc.c  */
#line 761 "ldgram.y"
    { lang_startup((yyvsp[(3) - (4)].name)); }
    break;

  case 201:

/* Line 1455 of yacc.c  */
#line 767 "ldgram.y"
    { ldemul_hll((char *)NULL); }
    break;

  case 202:

/* Line 1455 of yacc.c  */
#line 772 "ldgram.y"
    { ldemul_hll((yyvsp[(3) - (3)].name)); }
    break;

  case 203:

/* Line 1455 of yacc.c  */
#line 774 "ldgram.y"
    { ldemul_hll((yyvsp[(1) - (1)].name)); }
    break;

  case 205:

/* Line 1455 of yacc.c  */
#line 782 "ldgram.y"
    { ldemul_syslib((yyvsp[(3) - (3)].name)); }
    break;

  case 207:

/* Line 1455 of yacc.c  */
#line 788 "ldgram.y"
    { lang_float(TRUE); }
    break;

  case 208:

/* Line 1455 of yacc.c  */
#line 790 "ldgram.y"
    { lang_float(FALSE); }
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 795 "ldgram.y"
    {
		  (yyval.nocrossref) = NULL;
		}
    break;

  case 210:

/* Line 1455 of yacc.c  */
#line 799 "ldgram.y"
    {
		  struct lang_nocrossref *n;

		  n = (struct lang_nocrossref *) xmalloc (sizeof *n);
		  n->name = (yyvsp[(1) - (2)].name);
		  n->next = (yyvsp[(2) - (2)].nocrossref);
		  (yyval.nocrossref) = n;
		}
    break;

  case 211:

/* Line 1455 of yacc.c  */
#line 808 "ldgram.y"
    {
		  struct lang_nocrossref *n;

		  n = (struct lang_nocrossref *) xmalloc (sizeof *n);
		  n->name = (yyvsp[(1) - (3)].name);
		  n->next = (yyvsp[(3) - (3)].nocrossref);
		  (yyval.nocrossref) = n;
		}
    break;

  case 212:

/* Line 1455 of yacc.c  */
#line 818 "ldgram.y"
    { ldlex_expression (); }
    break;

  case 213:

/* Line 1455 of yacc.c  */
#line 820 "ldgram.y"
    { ldlex_popstate (); (yyval.etree)=(yyvsp[(2) - (2)].etree);}
    break;

  case 214:

/* Line 1455 of yacc.c  */
#line 825 "ldgram.y"
    { (yyval.etree) = exp_unop ('-', (yyvsp[(2) - (2)].etree)); }
    break;

  case 215:

/* Line 1455 of yacc.c  */
#line 827 "ldgram.y"
    { (yyval.etree) = (yyvsp[(2) - (3)].etree); }
    break;

  case 216:

/* Line 1455 of yacc.c  */
#line 829 "ldgram.y"
    { (yyval.etree) = exp_unop ((int) (yyvsp[(1) - (4)].integer),(yyvsp[(3) - (4)].etree)); }
    break;

  case 217:

/* Line 1455 of yacc.c  */
#line 831 "ldgram.y"
    { (yyval.etree) = exp_unop ('!', (yyvsp[(2) - (2)].etree)); }
    break;

  case 218:

/* Line 1455 of yacc.c  */
#line 833 "ldgram.y"
    { (yyval.etree) = (yyvsp[(2) - (2)].etree); }
    break;

  case 219:

/* Line 1455 of yacc.c  */
#line 835 "ldgram.y"
    { (yyval.etree) = exp_unop ('~', (yyvsp[(2) - (2)].etree));}
    break;

  case 220:

/* Line 1455 of yacc.c  */
#line 838 "ldgram.y"
    { (yyval.etree) = exp_binop ('*', (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 221:

/* Line 1455 of yacc.c  */
#line 840 "ldgram.y"
    { (yyval.etree) = exp_binop ('/', (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 222:

/* Line 1455 of yacc.c  */
#line 842 "ldgram.y"
    { (yyval.etree) = exp_binop ('%', (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 223:

/* Line 1455 of yacc.c  */
#line 844 "ldgram.y"
    { (yyval.etree) = exp_binop ('+', (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 224:

/* Line 1455 of yacc.c  */
#line 846 "ldgram.y"
    { (yyval.etree) = exp_binop ('-' , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 225:

/* Line 1455 of yacc.c  */
#line 848 "ldgram.y"
    { (yyval.etree) = exp_binop (LSHIFT , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 226:

/* Line 1455 of yacc.c  */
#line 850 "ldgram.y"
    { (yyval.etree) = exp_binop (RSHIFT , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 227:

/* Line 1455 of yacc.c  */
#line 852 "ldgram.y"
    { (yyval.etree) = exp_binop (EQ , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 228:

/* Line 1455 of yacc.c  */
#line 854 "ldgram.y"
    { (yyval.etree) = exp_binop (NE , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 229:

/* Line 1455 of yacc.c  */
#line 856 "ldgram.y"
    { (yyval.etree) = exp_binop (LE , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 230:

/* Line 1455 of yacc.c  */
#line 858 "ldgram.y"
    { (yyval.etree) = exp_binop (GE , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 231:

/* Line 1455 of yacc.c  */
#line 860 "ldgram.y"
    { (yyval.etree) = exp_binop ('<' , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 232:

/* Line 1455 of yacc.c  */
#line 862 "ldgram.y"
    { (yyval.etree) = exp_binop ('>' , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 233:

/* Line 1455 of yacc.c  */
#line 864 "ldgram.y"
    { (yyval.etree) = exp_binop ('&' , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 234:

/* Line 1455 of yacc.c  */
#line 866 "ldgram.y"
    { (yyval.etree) = exp_binop ('^' , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 235:

/* Line 1455 of yacc.c  */
#line 868 "ldgram.y"
    { (yyval.etree) = exp_binop ('|' , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 236:

/* Line 1455 of yacc.c  */
#line 870 "ldgram.y"
    { (yyval.etree) = exp_trinop ('?' , (yyvsp[(1) - (5)].etree), (yyvsp[(3) - (5)].etree), (yyvsp[(5) - (5)].etree)); }
    break;

  case 237:

/* Line 1455 of yacc.c  */
#line 872 "ldgram.y"
    { (yyval.etree) = exp_binop (ANDAND , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 238:

/* Line 1455 of yacc.c  */
#line 874 "ldgram.y"
    { (yyval.etree) = exp_binop (OROR , (yyvsp[(1) - (3)].etree), (yyvsp[(3) - (3)].etree)); }
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 876 "ldgram.y"
    { (yyval.etree) = exp_nameop (DEFINED, (yyvsp[(3) - (4)].name)); }
    break;

  case 240:

/* Line 1455 of yacc.c  */
#line 878 "ldgram.y"
    { (yyval.etree) = exp_bigintop ((yyvsp[(1) - (1)].bigint).integer, (yyvsp[(1) - (1)].bigint).str); }
    break;

  case 241:

/* Line 1455 of yacc.c  */
#line 880 "ldgram.y"
    { (yyval.etree) = exp_nameop (SIZEOF_HEADERS,0); }
    break;

  case 242:

/* Line 1455 of yacc.c  */
#line 883 "ldgram.y"
    { (yyval.etree) = exp_nameop (ALIGNOF,(yyvsp[(3) - (4)].name)); }
    break;

  case 243:

/* Line 1455 of yacc.c  */
#line 885 "ldgram.y"
    { (yyval.etree) = exp_nameop (SIZEOF,(yyvsp[(3) - (4)].name)); }
    break;

  case 244:

/* Line 1455 of yacc.c  */
#line 887 "ldgram.y"
    { (yyval.etree) = exp_nameop (ADDR,(yyvsp[(3) - (4)].name)); }
    break;

  case 245:

/* Line 1455 of yacc.c  */
#line 889 "ldgram.y"
    { (yyval.etree) = exp_nameop (LOADADDR,(yyvsp[(3) - (4)].name)); }
    break;

  case 246:

/* Line 1455 of yacc.c  */
#line 891 "ldgram.y"
    { (yyval.etree) = exp_nameop (CONSTANT,(yyvsp[(3) - (4)].name)); }
    break;

  case 247:

/* Line 1455 of yacc.c  */
#line 893 "ldgram.y"
    { (yyval.etree) = exp_unop (ABSOLUTE, (yyvsp[(3) - (4)].etree)); }
    break;

  case 248:

/* Line 1455 of yacc.c  */
#line 895 "ldgram.y"
    { (yyval.etree) = exp_unop (ALIGN_K,(yyvsp[(3) - (4)].etree)); }
    break;

  case 249:

/* Line 1455 of yacc.c  */
#line 897 "ldgram.y"
    { (yyval.etree) = exp_binop (ALIGN_K,(yyvsp[(3) - (6)].etree),(yyvsp[(5) - (6)].etree)); }
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 899 "ldgram.y"
    { (yyval.etree) = exp_binop (DATA_SEGMENT_ALIGN, (yyvsp[(3) - (6)].etree), (yyvsp[(5) - (6)].etree)); }
    break;

  case 251:

/* Line 1455 of yacc.c  */
#line 901 "ldgram.y"
    { (yyval.etree) = exp_binop (DATA_SEGMENT_RELRO_END, (yyvsp[(5) - (6)].etree), (yyvsp[(3) - (6)].etree)); }
    break;

  case 252:

/* Line 1455 of yacc.c  */
#line 903 "ldgram.y"
    { (yyval.etree) = exp_unop (DATA_SEGMENT_END, (yyvsp[(3) - (4)].etree)); }
    break;

  case 253:

/* Line 1455 of yacc.c  */
#line 905 "ldgram.y"
    { /* The operands to the expression node are
			     placed in the opposite order from the way
			     in which they appear in the script as
			     that allows us to reuse more code in
			     fold_binary.  */
			  (yyval.etree) = exp_binop (SEGMENT_START,
					  (yyvsp[(5) - (6)].etree),
					  exp_nameop (NAME, (yyvsp[(3) - (6)].name))); }
    break;

  case 254:

/* Line 1455 of yacc.c  */
#line 914 "ldgram.y"
    { (yyval.etree) = exp_unop (ALIGN_K,(yyvsp[(3) - (4)].etree)); }
    break;

  case 255:

/* Line 1455 of yacc.c  */
#line 916 "ldgram.y"
    { (yyval.etree) = exp_nameop (NAME,(yyvsp[(1) - (1)].name)); }
    break;

  case 256:

/* Line 1455 of yacc.c  */
#line 918 "ldgram.y"
    { (yyval.etree) = exp_binop (MAX_K, (yyvsp[(3) - (6)].etree), (yyvsp[(5) - (6)].etree) ); }
    break;

  case 257:

/* Line 1455 of yacc.c  */
#line 920 "ldgram.y"
    { (yyval.etree) = exp_binop (MIN_K, (yyvsp[(3) - (6)].etree), (yyvsp[(5) - (6)].etree) ); }
    break;

  case 258:

/* Line 1455 of yacc.c  */
#line 922 "ldgram.y"
    { (yyval.etree) = exp_assert ((yyvsp[(3) - (6)].etree), (yyvsp[(5) - (6)].name)); }
    break;

  case 259:

/* Line 1455 of yacc.c  */
#line 924 "ldgram.y"
    { (yyval.etree) = exp_nameop (ORIGIN, (yyvsp[(3) - (4)].name)); }
    break;

  case 260:

/* Line 1455 of yacc.c  */
#line 926 "ldgram.y"
    { (yyval.etree) = exp_nameop (LENGTH, (yyvsp[(3) - (4)].name)); }
    break;

  case 261:

/* Line 1455 of yacc.c  */
#line 931 "ldgram.y"
    { (yyval.name) = (yyvsp[(3) - (3)].name); }
    break;

  case 262:

/* Line 1455 of yacc.c  */
#line 932 "ldgram.y"
    { (yyval.name) = 0; }
    break;

  case 263:

/* Line 1455 of yacc.c  */
#line 936 "ldgram.y"
    { (yyval.etree) = (yyvsp[(3) - (4)].etree); }
    break;

  case 264:

/* Line 1455 of yacc.c  */
#line 937 "ldgram.y"
    { (yyval.etree) = 0; }
    break;

  case 265:

/* Line 1455 of yacc.c  */
#line 941 "ldgram.y"
    { (yyval.etree) = (yyvsp[(3) - (4)].etree); }
    break;

  case 266:

/* Line 1455 of yacc.c  */
#line 942 "ldgram.y"
    { (yyval.etree) = 0; }
    break;

  case 267:

/* Line 1455 of yacc.c  */
#line 946 "ldgram.y"
    { (yyval.etree) = (yyvsp[(3) - (4)].etree); }
    break;

  case 268:

/* Line 1455 of yacc.c  */
#line 947 "ldgram.y"
    { (yyval.etree) = 0; }
    break;

  case 269:

/* Line 1455 of yacc.c  */
#line 951 "ldgram.y"
    { (yyval.token) = ONLY_IF_RO; }
    break;

  case 270:

/* Line 1455 of yacc.c  */
#line 952 "ldgram.y"
    { (yyval.token) = ONLY_IF_RW; }
    break;

  case 271:

/* Line 1455 of yacc.c  */
#line 953 "ldgram.y"
    { (yyval.token) = SPECIAL; }
    break;

  case 272:

/* Line 1455 of yacc.c  */
#line 954 "ldgram.y"
    { (yyval.token) = 0; }
    break;

  case 273:

/* Line 1455 of yacc.c  */
#line 957 "ldgram.y"
    { ldlex_expression(); }
    break;

  case 274:

/* Line 1455 of yacc.c  */
#line 961 "ldgram.y"
    { ldlex_popstate (); ldlex_script (); }
    break;

  case 275:

/* Line 1455 of yacc.c  */
#line 964 "ldgram.y"
    {
			  lang_enter_output_section_statement((yyvsp[(1) - (9)].name), (yyvsp[(3) - (9)].etree),
							      sectype,
							      (yyvsp[(5) - (9)].etree), (yyvsp[(6) - (9)].etree), (yyvsp[(4) - (9)].etree), (yyvsp[(8) - (9)].token));
			}
    break;

  case 276:

/* Line 1455 of yacc.c  */
#line 970 "ldgram.y"
    { ldlex_popstate (); ldlex_expression (); }
    break;

  case 277:

/* Line 1455 of yacc.c  */
#line 972 "ldgram.y"
    {
		  ldlex_popstate ();
		  lang_leave_output_section_statement ((yyvsp[(17) - (17)].fill), (yyvsp[(14) - (17)].name), (yyvsp[(16) - (17)].section_phdr), (yyvsp[(15) - (17)].name));
		}
    break;

  case 278:

/* Line 1455 of yacc.c  */
#line 977 "ldgram.y"
    {}
    break;

  case 279:

/* Line 1455 of yacc.c  */
#line 979 "ldgram.y"
    { ldlex_expression (); }
    break;

  case 280:

/* Line 1455 of yacc.c  */
#line 981 "ldgram.y"
    { ldlex_popstate (); ldlex_script (); }
    break;

  case 281:

/* Line 1455 of yacc.c  */
#line 983 "ldgram.y"
    {
			  lang_enter_overlay ((yyvsp[(3) - (8)].etree), (yyvsp[(6) - (8)].etree));
			}
    break;

  case 282:

/* Line 1455 of yacc.c  */
#line 988 "ldgram.y"
    { ldlex_popstate (); ldlex_expression (); }
    break;

  case 283:

/* Line 1455 of yacc.c  */
#line 990 "ldgram.y"
    {
			  ldlex_popstate ();
			  lang_leave_overlay ((yyvsp[(5) - (16)].etree), (int) (yyvsp[(4) - (16)].integer),
					      (yyvsp[(16) - (16)].fill), (yyvsp[(13) - (16)].name), (yyvsp[(15) - (16)].section_phdr), (yyvsp[(14) - (16)].name));
			}
    break;

  case 285:

/* Line 1455 of yacc.c  */
#line 1000 "ldgram.y"
    { ldlex_expression (); }
    break;

  case 286:

/* Line 1455 of yacc.c  */
#line 1002 "ldgram.y"
    {
		  ldlex_popstate ();
		  lang_add_assignment (exp_assop ('=', ".", (yyvsp[(3) - (3)].etree)));
		}
    break;

  case 288:

/* Line 1455 of yacc.c  */
#line 1008 "ldgram.y"
    { ldlex_script (); ldfile_open_command_file((yyvsp[(2) - (2)].name)); }
    break;

  case 289:

/* Line 1455 of yacc.c  */
#line 1010 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 290:

/* Line 1455 of yacc.c  */
#line 1014 "ldgram.y"
    { sectype = noload_section; }
    break;

  case 291:

/* Line 1455 of yacc.c  */
#line 1015 "ldgram.y"
    { sectype = noalloc_section; }
    break;

  case 292:

/* Line 1455 of yacc.c  */
#line 1016 "ldgram.y"
    { sectype = noalloc_section; }
    break;

  case 293:

/* Line 1455 of yacc.c  */
#line 1017 "ldgram.y"
    { sectype = noalloc_section; }
    break;

  case 294:

/* Line 1455 of yacc.c  */
#line 1018 "ldgram.y"
    { sectype = noalloc_section; }
    break;

  case 296:

/* Line 1455 of yacc.c  */
#line 1023 "ldgram.y"
    { sectype = normal_section; }
    break;

  case 297:

/* Line 1455 of yacc.c  */
#line 1024 "ldgram.y"
    { sectype = normal_section; }
    break;

  case 298:

/* Line 1455 of yacc.c  */
#line 1028 "ldgram.y"
    { (yyval.etree) = (yyvsp[(1) - (3)].etree); }
    break;

  case 299:

/* Line 1455 of yacc.c  */
#line 1029 "ldgram.y"
    { (yyval.etree) = (etree_type *)NULL;  }
    break;

  case 300:

/* Line 1455 of yacc.c  */
#line 1034 "ldgram.y"
    { (yyval.etree) = (yyvsp[(3) - (6)].etree); }
    break;

  case 301:

/* Line 1455 of yacc.c  */
#line 1036 "ldgram.y"
    { (yyval.etree) = (yyvsp[(3) - (10)].etree); }
    break;

  case 302:

/* Line 1455 of yacc.c  */
#line 1040 "ldgram.y"
    { (yyval.etree) = (yyvsp[(1) - (2)].etree); }
    break;

  case 303:

/* Line 1455 of yacc.c  */
#line 1041 "ldgram.y"
    { (yyval.etree) = (etree_type *) NULL;  }
    break;

  case 304:

/* Line 1455 of yacc.c  */
#line 1046 "ldgram.y"
    { (yyval.integer) = 0; }
    break;

  case 305:

/* Line 1455 of yacc.c  */
#line 1048 "ldgram.y"
    { (yyval.integer) = 1; }
    break;

  case 306:

/* Line 1455 of yacc.c  */
#line 1053 "ldgram.y"
    { (yyval.name) = (yyvsp[(2) - (2)].name); }
    break;

  case 307:

/* Line 1455 of yacc.c  */
#line 1054 "ldgram.y"
    { (yyval.name) = DEFAULT_MEMORY_REGION; }
    break;

  case 308:

/* Line 1455 of yacc.c  */
#line 1059 "ldgram.y"
    {
		  (yyval.section_phdr) = NULL;
		}
    break;

  case 309:

/* Line 1455 of yacc.c  */
#line 1063 "ldgram.y"
    {
		  struct lang_output_section_phdr_list *n;

		  n = ((struct lang_output_section_phdr_list *)
		       xmalloc (sizeof *n));
		  n->name = (yyvsp[(3) - (3)].name);
		  n->used = FALSE;
		  n->next = (yyvsp[(1) - (3)].section_phdr);
		  (yyval.section_phdr) = n;
		}
    break;

  case 311:

/* Line 1455 of yacc.c  */
#line 1079 "ldgram.y"
    {
			  ldlex_script ();
			  lang_enter_overlay_section ((yyvsp[(2) - (2)].name));
			}
    break;

  case 312:

/* Line 1455 of yacc.c  */
#line 1084 "ldgram.y"
    { ldlex_popstate (); ldlex_expression (); }
    break;

  case 313:

/* Line 1455 of yacc.c  */
#line 1086 "ldgram.y"
    {
			  ldlex_popstate ();
			  lang_leave_overlay_section ((yyvsp[(9) - (9)].fill), (yyvsp[(8) - (9)].section_phdr));
			}
    break;

  case 318:

/* Line 1455 of yacc.c  */
#line 1103 "ldgram.y"
    { ldlex_expression (); }
    break;

  case 319:

/* Line 1455 of yacc.c  */
#line 1104 "ldgram.y"
    { ldlex_popstate (); }
    break;

  case 320:

/* Line 1455 of yacc.c  */
#line 1106 "ldgram.y"
    {
		  lang_new_phdr ((yyvsp[(1) - (6)].name), (yyvsp[(3) - (6)].etree), (yyvsp[(4) - (6)].phdr).filehdr, (yyvsp[(4) - (6)].phdr).phdrs, (yyvsp[(4) - (6)].phdr).at,
				 (yyvsp[(4) - (6)].phdr).flags);
		}
    break;

  case 321:

/* Line 1455 of yacc.c  */
#line 1114 "ldgram.y"
    {
		  (yyval.etree) = (yyvsp[(1) - (1)].etree);

		  if ((yyvsp[(1) - (1)].etree)->type.node_class == etree_name
		      && (yyvsp[(1) - (1)].etree)->type.node_code == NAME)
		    {
		      const char *s;
		      unsigned int i;
		      static const char * const phdr_types[] =
			{
			  "PT_NULL", "PT_LOAD", "PT_DYNAMIC",
			  "PT_INTERP", "PT_NOTE", "PT_SHLIB",
			  "PT_PHDR", "PT_TLS"
			};

		      s = (yyvsp[(1) - (1)].etree)->name.name;
		      for (i = 0;
			   i < sizeof phdr_types / sizeof phdr_types[0];
			   i++)
			if (strcmp (s, phdr_types[i]) == 0)
			  {
			    (yyval.etree) = exp_intop (i);
			    break;
			  }
		      if (i == sizeof phdr_types / sizeof phdr_types[0])
			{
			  if (strcmp (s, "PT_GNU_EH_FRAME") == 0)
			    (yyval.etree) = exp_intop (0x6474e550);
			  else if (strcmp (s, "PT_GNU_STACK") == 0)
			    (yyval.etree) = exp_intop (0x6474e551);
			  else
			    {
			      einfo (_("\
%X%P:%S: unknown phdr type `%s' (try integer literal)\n"),
				     s);
			      (yyval.etree) = exp_intop (0);
			    }
			}
		    }
		}
    break;

  case 322:

/* Line 1455 of yacc.c  */
#line 1158 "ldgram.y"
    {
		  memset (&(yyval.phdr), 0, sizeof (struct phdr_info));
		}
    break;

  case 323:

/* Line 1455 of yacc.c  */
#line 1162 "ldgram.y"
    {
		  (yyval.phdr) = (yyvsp[(3) - (3)].phdr);
		  if (strcmp ((yyvsp[(1) - (3)].name), "FILEHDR") == 0 && (yyvsp[(2) - (3)].etree) == NULL)
		    (yyval.phdr).filehdr = TRUE;
		  else if (strcmp ((yyvsp[(1) - (3)].name), "PHDRS") == 0 && (yyvsp[(2) - (3)].etree) == NULL)
		    (yyval.phdr).phdrs = TRUE;
		  else if (strcmp ((yyvsp[(1) - (3)].name), "FLAGS") == 0 && (yyvsp[(2) - (3)].etree) != NULL)
		    (yyval.phdr).flags = (yyvsp[(2) - (3)].etree);
		  else
		    einfo (_("%X%P:%S: PHDRS syntax error at `%s'\n"), (yyvsp[(1) - (3)].name));
		}
    break;

  case 324:

/* Line 1455 of yacc.c  */
#line 1174 "ldgram.y"
    {
		  (yyval.phdr) = (yyvsp[(5) - (5)].phdr);
		  (yyval.phdr).at = (yyvsp[(3) - (5)].etree);
		}
    break;

  case 325:

/* Line 1455 of yacc.c  */
#line 1182 "ldgram.y"
    {
		  (yyval.etree) = NULL;
		}
    break;

  case 326:

/* Line 1455 of yacc.c  */
#line 1186 "ldgram.y"
    {
		  (yyval.etree) = (yyvsp[(2) - (3)].etree);
		}
    break;

  case 327:

/* Line 1455 of yacc.c  */
#line 1192 "ldgram.y"
    {
		  ldlex_version_file ();
		  PUSH_ERROR (_("dynamic list"));
		}
    break;

  case 328:

/* Line 1455 of yacc.c  */
#line 1197 "ldgram.y"
    {
		  ldlex_popstate ();
		  POP_ERROR ();
		}
    break;

  case 332:

/* Line 1455 of yacc.c  */
#line 1214 "ldgram.y"
    {
		  lang_append_dynamic_list ((yyvsp[(1) - (2)].versyms));
		}
    break;

  case 333:

/* Line 1455 of yacc.c  */
#line 1222 "ldgram.y"
    {
		  ldlex_version_file ();
		  PUSH_ERROR (_("VERSION script"));
		}
    break;

  case 334:

/* Line 1455 of yacc.c  */
#line 1227 "ldgram.y"
    {
		  ldlex_popstate ();
		  POP_ERROR ();
		}
    break;

  case 335:

/* Line 1455 of yacc.c  */
#line 1236 "ldgram.y"
    {
		  ldlex_version_script ();
		}
    break;

  case 336:

/* Line 1455 of yacc.c  */
#line 1240 "ldgram.y"
    {
		  ldlex_popstate ();
		}
    break;

  case 339:

/* Line 1455 of yacc.c  */
#line 1252 "ldgram.y"
    {
		  lang_register_vers_node (NULL, (yyvsp[(2) - (4)].versnode), NULL);
		}
    break;

  case 340:

/* Line 1455 of yacc.c  */
#line 1256 "ldgram.y"
    {
		  lang_register_vers_node ((yyvsp[(1) - (5)].name), (yyvsp[(3) - (5)].versnode), NULL);
		}
    break;

  case 341:

/* Line 1455 of yacc.c  */
#line 1260 "ldgram.y"
    {
		  lang_register_vers_node ((yyvsp[(1) - (6)].name), (yyvsp[(3) - (6)].versnode), (yyvsp[(5) - (6)].deflist));
		}
    break;

  case 342:

/* Line 1455 of yacc.c  */
#line 1267 "ldgram.y"
    {
		  (yyval.deflist) = lang_add_vers_depend (NULL, (yyvsp[(1) - (1)].name));
		}
    break;

  case 343:

/* Line 1455 of yacc.c  */
#line 1271 "ldgram.y"
    {
		  (yyval.deflist) = lang_add_vers_depend ((yyvsp[(1) - (2)].deflist), (yyvsp[(2) - (2)].name));
		}
    break;

  case 344:

/* Line 1455 of yacc.c  */
#line 1278 "ldgram.y"
    {
		  (yyval.versnode) = lang_new_vers_node (NULL, NULL);
		}
    break;

  case 345:

/* Line 1455 of yacc.c  */
#line 1282 "ldgram.y"
    {
		  (yyval.versnode) = lang_new_vers_node ((yyvsp[(1) - (2)].versyms), NULL);
		}
    break;

  case 346:

/* Line 1455 of yacc.c  */
#line 1286 "ldgram.y"
    {
		  (yyval.versnode) = lang_new_vers_node ((yyvsp[(3) - (4)].versyms), NULL);
		}
    break;

  case 347:

/* Line 1455 of yacc.c  */
#line 1290 "ldgram.y"
    {
		  (yyval.versnode) = lang_new_vers_node (NULL, (yyvsp[(3) - (4)].versyms));
		}
    break;

  case 348:

/* Line 1455 of yacc.c  */
#line 1294 "ldgram.y"
    {
		  (yyval.versnode) = lang_new_vers_node ((yyvsp[(3) - (8)].versyms), (yyvsp[(7) - (8)].versyms));
		}
    break;

  case 349:

/* Line 1455 of yacc.c  */
#line 1301 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, (yyvsp[(1) - (1)].name), ldgram_vers_current_lang, FALSE);
		}
    break;

  case 350:

/* Line 1455 of yacc.c  */
#line 1305 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, (yyvsp[(1) - (1)].name), ldgram_vers_current_lang, TRUE);
		}
    break;

  case 351:

/* Line 1455 of yacc.c  */
#line 1309 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[(1) - (3)].versyms), (yyvsp[(3) - (3)].name), ldgram_vers_current_lang, FALSE);
		}
    break;

  case 352:

/* Line 1455 of yacc.c  */
#line 1313 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[(1) - (3)].versyms), (yyvsp[(3) - (3)].name), ldgram_vers_current_lang, TRUE);
		}
    break;

  case 353:

/* Line 1455 of yacc.c  */
#line 1317 "ldgram.y"
    {
			  (yyval.name) = ldgram_vers_current_lang;
			  ldgram_vers_current_lang = (yyvsp[(4) - (5)].name);
			}
    break;

  case 354:

/* Line 1455 of yacc.c  */
#line 1322 "ldgram.y"
    {
			  struct bfd_elf_version_expr *pat;
			  for (pat = (yyvsp[(7) - (9)].versyms); pat->next != NULL; pat = pat->next);
			  pat->next = (yyvsp[(1) - (9)].versyms);
			  (yyval.versyms) = (yyvsp[(7) - (9)].versyms);
			  ldgram_vers_current_lang = (yyvsp[(6) - (9)].name);
			}
    break;

  case 355:

/* Line 1455 of yacc.c  */
#line 1330 "ldgram.y"
    {
			  (yyval.name) = ldgram_vers_current_lang;
			  ldgram_vers_current_lang = (yyvsp[(2) - (3)].name);
			}
    break;

  case 356:

/* Line 1455 of yacc.c  */
#line 1335 "ldgram.y"
    {
			  (yyval.versyms) = (yyvsp[(5) - (7)].versyms);
			  ldgram_vers_current_lang = (yyvsp[(4) - (7)].name);
			}
    break;

  case 357:

/* Line 1455 of yacc.c  */
#line 1340 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, "global", ldgram_vers_current_lang, FALSE);
		}
    break;

  case 358:

/* Line 1455 of yacc.c  */
#line 1344 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[(1) - (3)].versyms), "global", ldgram_vers_current_lang, FALSE);
		}
    break;

  case 359:

/* Line 1455 of yacc.c  */
#line 1348 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, "local", ldgram_vers_current_lang, FALSE);
		}
    break;

  case 360:

/* Line 1455 of yacc.c  */
#line 1352 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[(1) - (3)].versyms), "local", ldgram_vers_current_lang, FALSE);
		}
    break;

  case 361:

/* Line 1455 of yacc.c  */
#line 1356 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, "extern", ldgram_vers_current_lang, FALSE);
		}
    break;

  case 362:

/* Line 1455 of yacc.c  */
#line 1360 "ldgram.y"
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[(1) - (3)].versyms), "extern", ldgram_vers_current_lang, FALSE);
		}
    break;



/* Line 1455 of yacc.c  */
#line 4917 "ldgram.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1675 of yacc.c  */
#line 1370 "ldgram.y"

void
yyerror(arg)
     const char *arg;
{
  if (ldfile_assumed_script)
    einfo (_("%P:%s: file format not recognized; treating as linker script\n"),
	   ldfile_input_filename);
  if (error_index > 0 && error_index < ERROR_NAME_MAX)
     einfo ("%P%F:%S: %s in %s\n", arg, error_names[error_index-1]);
  else
     einfo ("%P%F:%S: %s\n", arg);
}

