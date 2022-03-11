/*
** pic32c-allocate.c
**
** Copyright (c) 2016 Microchip Technology, Inc.
**
** This file is part of BFD, the Binary File Descriptor library.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
** MA 02110-1301, USA.
**
** This file contains pic32-specific memory allocation
** processing for the linker.
**
** It is included by the following target-specific
** emulation files:
**
**   ld/emultmpl/elf32pic32mx.em
**
*/

/*****************************************************************************
**
** Improved Memory Allocation Routines
**
** These functions implement the improved
** memory allocator .
**
*/

#include "pic32c-options.h"

/* useful constants and macros */
#define NEAR_BOUNDARY   0xa0010000

#define VALID_NEAR(addr, len) ((bfd_vma)(addr)+(len) <= NEAR_BOUNDARY)

#define VALID_LOW_ADDR(addr, len)  ((bfd_vma)(addr)+(len) <= exclude_addr)
#define VALID_HIGH_ADDR(addr, len)  ((bfd_vma)(addr) >= exclude_addr)

/* Create an id for each section attribute */
#define ATTR(a,b,c)  _##a,
 enum {
#include "pic32c-attributes.h"
 } ;

/* Create a bit mask for each section attribute */
#undef ATTR
#define ATTR(a,b,c)  a = (1<<_##a),
 enum {
#include "pic32c-attributes.h"
 } ;

/* this usage is intended for checking if a region_info member is present and valid,
   i.e. an additional NULL check for the pic32_region pointer. */
static int
pic32_valid_region(struct pic32_region *regn) {
    return (regn && valid_region(regn->region));
}

/* a hard coded magic number   the cache line size for jubilee is 16 bytes */
/* for now Jubilee is the only family with combines TCM, so I only need one of these.*/

/* ensure that the region has all of the needed_flags and none of the
   invalid_flags. basically, this copes with flags and not_flags. */
static int 
check_region_flags(const lang_memory_region_type *region, 
                   unsigned needed_flags, 
                   unsigned invalid_flags) {
    return ((region->flags & needed_flags) == needed_flags &&
            (region->not_flags & needed_flags) == 0 &&
            (invalid_flags == 0 ||
              ((region->flags & invalid_flags) == 0) ||
              ((region->not_flags & invalid_flags) != 0)));
}

/* not static, because it's needed (?) in elf32pic32c.em */
struct region_info region_info;

/* a hard coded magic number   the cache line size for jubilee is 16 bytes */
/* for now Jubilee is the only family with combines TCM, so I only need one of these.*/

#define JUBILEE_CACHE_LINE_ALIGNMENT_POWER 4

/* MERGE_NOTES: renamed params to avoid warnings for shadowing code/data globals */
enum tcm_style 
determine_tcm_style (lang_memory_region_type **itcm_region, lang_memory_region_type **dtcm_region) {

/*   the tcm style of the machine is determined by which memory regions are defined
 *   so this really comes from the linker script
 *
 *    if there is a memory region called "tcm" then we have TCM_COMBINED
 *    if either of dtcm or itcm exists we have TCM_SEPERATE
 *    if one of these co-exists with tcm we are in an error situation (not checked here)
 *    if none of these exist we have TCM_NONE    
 *
 *    input tcm sections will have the attribute itcm or dtcm depending if they are
 *    itcm_region or dtcm_region resp.   Plain "tcm" is not a valid attribute.
*/
    
    bfd_boolean found_itcm_or_dtcm = FALSE;
    *itcm_region = *dtcm_region = NULL;
    
    lang_memory_region_type *region = region_lookup("tcm");

    if (valid_region(region)) {
	*itcm_region = region;
	*dtcm_region = region;
	return TCM_COMBINED;
    }
    region = region_lookup("itcm");
    if (valid_region(region)) {
	*itcm_region = region;
	found_itcm_or_dtcm = TRUE;
    }
    region = region_lookup("dtcm");
    if (valid_region(region)) {
	*dtcm_region = region;
	found_itcm_or_dtcm = TRUE;
    }
	
    if (found_itcm_or_dtcm) return TCM_SEPARATE;
    return TCM_NONE;
}

/* debug helper to display regions as classified */
static void 
dump_single_region_info (const struct pic32_region *region, const char *name) {
    const lang_memory_region_type *regn = region->region;
    fprintf(stderr, "%12s %12s %12lx %12lx    ", name, regn->name_list.name, regn->origin, regn->length);
    fprintf(stderr, "%c", (region->flags.init_needed ? 'I' : '.'));
    fprintf(stderr, "%c", (region->flags.is_cleared ? 'Z' : '.'));
    fprintf(stderr, "%c", (region->flags.is_exec ? 'X' : '.'));
    fprintf(stderr, "%c", (region->flags.is_tcm ? 'T' : '.'));
    fprintf(stderr, "\n");
}

static void 
dump_region_info (const struct region_info *rinfo) {

    fprintf(stderr, "%12s %12s %12s %12s %8s\n", "region", "name", "origin", "length", "flags");

    if (pic32_valid_region(rinfo->code))
        dump_single_region_info(rinfo->code, "code");
    if (pic32_valid_region(rinfo->data)) 
        dump_single_region_info(rinfo->data, "data");
    if (pic32_valid_region(rinfo->code_tcm)) 
        dump_single_region_info(rinfo->code_tcm, "itcm");
    if (pic32_valid_region(rinfo->data_tcm)) 
        dump_single_region_info(rinfo->data_tcm, "dtcm");
    if (pic32_valid_region(rinfo->vectors))
      dump_single_region_info(rinfo->vectors, "vectors");
}

/* helper to output a region description to a memory info file. TCM reporting
   is disabled unless show_tcm is set, as apparently this can affect/break
   the IDE memory display? */
static void
report_single_region_info(FILE *fp, struct pic32_region *regn, const char *name) {
  unsigned long total_size = 0;
  unsigned long used_size = 0;
  struct pic32_section *s;

  if (!pic32_valid_region(regn) || !regn->region->length) 
      return;

  total_size = regn->region->length;

  for (s = pic32_section_list; s != NULL; s = s->next)
      if (s->sec)
          used_size += bfd_pic32_collect_section_size (s, regn->region);

  fprintf (fp, "    <memory name=\"%s\">\n"
               "      <units>bytes</units>\n"
               "      <length>%lu</length>\n"
               "      <used>%lu</used>\n"
               "      <free>%lu</free>\n"
               "    </memory>\n",
           name, total_size, used_size, total_size - used_size);
}

void
pic32_report_region_info (FILE *f, int show_tcm) {

    /* report code as 'rom' if not shared with data (or if readonly?) */
    /* XC32-1658: changed 'rom' to 'program' (the shell and MPLAB X look for this name) */
    if (pic32_valid_region(region_info.code) && 
        region_info.code != region_info.data) {
        report_single_region_info(f, region_info.code, "program");
    }
    /* XC32-1658: similarly, use 'data' instead of 'ram' */
    if (pic32_valid_region(region_info.data)) {
        report_single_region_info(f, region_info.data, "data");
    }

    if (show_tcm) {
        /* report tcm as 'tcm' when combined, else itcm/dtcm */
        if (pic32_valid_region(region_info.code_tcm) &&
            region_info.code_tcm == region_info.data_tcm) {
            report_single_region_info(f, region_info.code_tcm, "tcm");
        } 
        else {
            if (pic32_valid_region(region_info.code_tcm)) {
                report_single_region_info(f, region_info.code_tcm, "itcm");
            }
            if (pic32_valid_region(region_info.data_tcm)) {
                report_single_region_info(f, region_info.data_tcm, "dtcm");
            }
        }
    }
}

/* allocate and initialize a pic32_region for the underlying BFD region. */
static struct pic32_region *
pic32_new_region(struct memory_region_struct *region) {
    struct pic32_region *p = xmalloc(sizeof(struct pic32_region));
    memset(p, 0, sizeof(struct pic32_region));
    p->region = region;
    return p;
}


/* Populate region_info with information about regions that the BFA
   can allocate to. Returns true iff the BFA has any hope of working,
   i.e. there are regions for everything that could be allocated.
*/
static int
pic32_init_region_info(struct region_info *rinfo) {

    lang_memory_region_type *code_region, *data_region;

    /* wipe out whatever was there. */
    memset(rinfo, 0, sizeof *rinfo);

    /* first: check for 'ram'/'rom', the usual case. these
       are the expected regions, along with maybe TCM, for 
       cortex-M devices so far.
       the convention here is that if 'ram' exists, we expect
       a 'rom' as well. */
    data_region = region_lookup ("ram");
    code_region = region_lookup ("rom");

    if (valid_region(data_region)) {
        /* FIXME: our scripts use the 'r' flag for just about every section
           which seems to mean readonly, not readable (everything is readable).
           so we can't check that flag for data. */
        if (check_region_flags(data_region, (SEC_DATA), 0)) {
            if (pic32_debug) 
                fprintf(stderr, "classify: data -> 'ram'\n");
            rinfo->data = pic32_new_region(data_region);
            rinfo->data->flags.is_exec = check_region_flags(data_region, (SEC_CODE), 0);
            rinfo->data->flags.init_needed = check_region_flags(data_region, 0, (SEC_LOAD));
        }
    }
    else {
      /* try sram if ram isn't anywhere to be found. */
      data_region = region_lookup ("sram");
      if (valid_region(data_region)
          && check_region_flags(data_region, (SEC_DATA), 0)) {
        if (pic32_debug) 
          fprintf(stderr, "classify: data -> 'sram'\n");
        rinfo->data = pic32_new_region(data_region);
        rinfo->data->flags.is_exec = check_region_flags(data_region, (SEC_CODE), 0);
        rinfo->data->flags.init_needed = check_region_flags(data_region, 0, (SEC_LOAD));
      }
    }

    if (valid_region(code_region)) {
        if (check_region_flags(code_region, (SEC_CODE), 0)) {
            if (pic32_debug) 
                fprintf(stderr, "classify: code -> 'rom'\n");
            rinfo->code = pic32_new_region(code_region);
            rinfo->code->flags.is_exec = 1;
            rinfo->code->flags.init_needed = check_region_flags(code_region, 0, (SEC_LOAD));
        }
    }

    /* check for TCM regions and kind of TCM to handle. */

    rinfo->tcm_style = determine_tcm_style(&code_region, &data_region);

    if (rinfo->tcm_style != TCM_NONE) {

        /* currently determine_tcm_style guarantees the regions are real, but 
           we'll also check flags */
        /* see above TODO for a note about SEC_READONLY .. */
        if (valid_region(data_region) &&
            check_region_flags(data_region, (SEC_DATA), 0)) {
            rinfo->data_tcm = pic32_new_region(data_region);
            rinfo->data_tcm->flags.is_exec = check_region_flags(data_region, (SEC_CODE), 0);
            rinfo->data_tcm->flags.init_needed = check_region_flags(data_region, 0, (SEC_LOAD));
        }

        if (rinfo->tcm_style == TCM_COMBINED) {
            rinfo->code_tcm = rinfo->data_tcm;
        }
        else if (valid_region(code_region) && 
            check_region_flags(code_region, (SEC_CODE), 0)) {
            rinfo->code_tcm = pic32_new_region(code_region);
            rinfo->code_tcm->flags.is_exec = 1;
            rinfo->code_tcm->flags.init_needed = check_region_flags(code_region, 0, (SEC_LOAD));
        }
    }

    if (rinfo->code == NULL && rinfo->data != NULL) {
        /* if is_exec isn't set, we are out of luck. */
        if (!rinfo->data->flags.is_exec) {
            if (pic32_debug) {
                fprintf(stderr, "no rom region available, but ram is not executable\n");
            }
            return 0;
        }
        rinfo->code = rinfo->data;

        /* in this case we have to assume the section is loaded, as there's no
           ROM */
        rinfo->code->flags.init_needed = 0;
        rinfo->data->flags.init_needed = 0;

        if (pic32_debug) 
            fprintf(stderr, "classify: code and data will be in same region\n");
    }

    /* Now, we determine how to handle tcm-like relocations, i.e. moving .vectors
       around at runtime to something that isn't the code region. 
       If there's tcm regions, we trust the tcm-related options. 
       Otherwise, we check for 'sram', magically indicating SAMA5/MPU.
       TODO: set stack region as needed for stack-in-dtcm?
       */ 
    if (rinfo->tcm_style == TCM_NONE) {
        code_region = region_lookup("sram");

        /* we only want to relocate the vectors if we are not locating
           code in sram already. */
        if (valid_region(code_region) &&
            (rinfo->code != NULL && rinfo->code->region != code_region)) {
            if (check_region_flags(code_region, (SEC_CODE), 0)) {
                if (pic32_debug) 
                    fprintf(stderr, "classify: vectors -> 'sram'\n");
                rinfo->vectors = pic32_new_region(code_region);
                rinfo->vectors->flags.is_exec = 1;
                rinfo->vectors->flags.init_needed = check_region_flags(code_region, 0, (SEC_LOAD));
                pic32c_relocate_vectors = TRUE;
                pic32c_vectors_in_tcm = FALSE;
            }
        }
    }
    else if (pic32c_vectors_in_tcm) {
        rinfo->vectors = rinfo->code_tcm;
        pic32c_relocate_vectors = TRUE;
    }

    /* that's it for magic names, now just check if we can proceed at all. */
    if (!pic32_valid_region(rinfo->code) || !pic32_valid_region(rinfo->data)) {
        if (pic32_debug) {
            fprintf(stderr, "unable to determine code/data targets for allocation.\n");
            fprintf(stderr, "check memory region names/flags.\n");
            dump_region_info (rinfo);
        }
        return 0;
    }

    if (pic32_debug) {
      fprintf (stderr, "after classify:\n");
      dump_region_info (rinfo);
    }
    return 1;
}

/* create the free_block list for each known region as needed. this is moved out of
   the allocate_XXX_memory function in order to deal with code/data sharing. 
   we'll still keep the order itcm->dtcm->code->data. 
   this also initializes the vector/stack target region lists for relocation
   if they are in different region(s) than code/data/tcm. */
static void
pic32_build_free_blocks(struct region_info *rinfo) {

    /* build itcm/tcm free blocks */
    if (pic32_valid_region(rinfo->code_tcm))
        build_free_block_list(rinfo->code_tcm, 0);

    /* build dtcm free blocks if not shared with itcm/tcm */
    if (pic32_valid_region(rinfo->data_tcm)) {
        if (rinfo->data_tcm != rinfo->code_tcm)
            build_free_block_list(rinfo->data_tcm, 0);
    }

    /* do the same for code/data. */
    if (pic32_valid_region(rinfo->code))
        build_free_block_list(rinfo->code, 0);

    if (rinfo->data == rinfo->code) {
        // TODO: have to suspect the same cache line issue may affect shared code/data
        // for SAMA5/MPUs
        //align_free_blocks(rinfo->data->free_blocks, 5);
    }
    else {
        if (pic32_valid_region(rinfo->data))
            build_free_block_list(rinfo->data, 0);
    }

    /* if the vector/stack targets are not any of the above, build them now.
     */
    if (rinfo->vectors != NULL && rinfo->vectors->free_blocks == NULL) 
        build_free_block_list(rinfo->vectors, 0);
    if (rinfo->stack != NULL && rinfo->stack->free_blocks == NULL) 
        build_free_block_list(rinfo->stack, 0);
}

/* Free any allocated memory used by region_info. In particular,
   free all the free block lists. This is to be called from 
   bfd_pic32_finish, or at least at the end of the region_info's usefulness. */
void 
pic32_free_region_info(struct region_info *rinfo) {
    if (!rinfo) return;

    /* here we deal with sharing of free block lists - code/data may be shared
       between tcm and not-tcm flavours */
    if (pic32_valid_region(rinfo->code) && rinfo->code->free_blocks) {
        pic32_free_memory_list(&rinfo->code->free_blocks);
    }

    if (pic32_valid_region(rinfo->data) && rinfo->data->free_blocks) {
        pic32_free_memory_list(&rinfo->data->free_blocks);
    }

    if (pic32_valid_region(rinfo->code_tcm) && rinfo->code_tcm->free_blocks) {
        pic32_free_memory_list(&rinfo->code_tcm->free_blocks);
    }

    if (pic32_valid_region(rinfo->data_tcm) && rinfo->data_tcm->free_blocks) {
        pic32_free_memory_list(&rinfo->data_tcm->free_blocks);
    }

    /* and then do the same excercise for the regions themselves. */
    if (rinfo->code) {
        free (rinfo->code);
        if (rinfo->data == rinfo->code)
            rinfo->data = NULL;
        rinfo->code = NULL;
    }
    if (rinfo->data) {
        free (rinfo->data);
        rinfo->data = NULL;
    }
    if (rinfo->code_tcm) {
        free (rinfo->code_tcm);
        if (rinfo->data_tcm == rinfo->code_tcm)
            rinfo->data_tcm = NULL;
        rinfo->code_tcm = NULL;
    }
    if (rinfo->data_tcm) {
        free (rinfo->data_tcm);
        rinfo->data_tcm = NULL;
    }

    memset(rinfo, 0, sizeof *rinfo);
}

/* Create a bit mask to match any attribute */
#define all_attr 0xFFFFFFFF

/* \lghica coresident*/
//extern bfd_boolean pic32_coresident_app;

/* Memory allocator options  */
enum {
  NO_LOCATE_OPTION,
  FAVOR_LOW_ADDR,     /* scan free blocks from low addr only  */
  FAVOR_HIGH_ADDR,    /* scan free blocks from high addr only */
  EXCLUDE_LOW_ADDR,   /* avoid memory below LIMIT             */
  EXCLUDE_HIGH_ADDR,  /* avoid memory above LIMIT - 1         */
  LOCATE_HIGHEST,
};

/* Data structure for free program memory blocks */
extern struct pic32_memory *program_memory_free_blocks;

/* TODO: this is used as the relocated vector address, so the name should probably
   not mention itcm? */
extern bfd_vma pic32c_itcm_vectors_address;
extern bfd_boolean pic32c_relocate_vectors;

static void allocate_stack_in_dtcm (void);

static void
finish_section_info(struct pic32_section *s, lang_output_section_statement_type *os);

static int locate_options = 0;
static bfd_vma exclude_addr = 0;

static void
set_locate_options(int optionvalue, bfd_vma addr) {
  locate_options = optionvalue;
  exclude_addr = addr;
}

static void
reset_locate_options(void) {
  locate_options = NO_LOCATE_OPTION;
  exclude_addr = 0;
}

#define IS_LOCATE_OPTION(a)  ((locate_options & (a)) == (a))

static void
remove_section_from_bfd(bfd *abfd, asection *sec)
{
    asection *s, *prev;
    
    s = abfd->sections;
    if ((s == NULL) || (s->next == NULL)) return;
    
    /* as a sanity check, search for the section in the bfd */
    /* this can be removed or made optional if speed is an issue */
#if 1
    /* lookup 'sec' starting with 2nd bfd section (never remove the 1st one) */
    s = s->next;
    while (s != sec && s != NULL)
        s = s->next;
    /* didn't find it */
    if (s == NULL)
#else
    /* simple test for not removing the 1st section */
        if (sec->prev == NULL)
#endif
            return;
    
    prev = sec->prev;
    prev->next = sec->next;
    
    if (sec->next)
        sec->next->prev = prev;
    else
        abfd->section_last = prev; /*update the bfd->section_last field
                                    if the removed section is the
                                    last section.*/
    abfd->section_count -= 1;
    if (pic32_debug)
        fprintf(stderr,"    removing section %s\n", s->name);
    return;
} /* static void remove_section_from_bfd (...)*/




/*
 * reserve_space_for_vectors()
 *
 * This function reserve space at the beginning the given region for a copy of
 * .vectors section
 *
 * Called by: allocate_program_memory()
 *
 * Calls:     region_lookup()
 *            bfd_get_section_by_name()
 *            pic32_static_assign_memory()
 *
 */
static void 
reserve_space_for_vectors(struct pic32_region *regn)
{


    ///\ get .vectors
    asection                        *vectors_section =
    bfd_get_section_by_name(link_info.output_bfd, ".vectors");
    struct pic32_memory             *vectors_block = NULL;

    // ensure target region exists and is valid
    if (!pic32_valid_region(regn))
        return;

    if (pic32_debug) {
      fprintf(stderr, "allocating space for .vectors in %s\n", regn->region->name_list.name);
    }

    if (vectors_section == NULL)
        return;

    ///\ keep the itcm->origin for .dinit dump
    pic32c_itcm_vectors_address = regn->region->origin;

    ///\ allocate .vectors as an absolute section
    vectors_block = pic32_static_assign_memory(regn->free_blocks,
                                               vectors_section->size,
                                               pic32c_itcm_vectors_address);
    if (vectors_block == NULL)
        einfo("%X Could not allocate .vectors at the beginning of itcm\n");
    
    create_remainder_blocks(regn->free_blocks,vectors_block, vectors_section->size);
}


static void generate_flash_vectors_symbol(char * const name)
{
    // add a symbol for .vectors if it was in the ld script
    asection   *vectors_section =
                bfd_get_section_by_name(link_info.output_bfd, ".vectors");
    if (vectors_section != NULL) {
	_bfd_generic_link_add_one_symbol (&link_info, link_info.output_bfd, name,
					  BSF_GLOBAL, bfd_abs_section_ptr,
					  vectors_section->lma,
					  name, 1, 0, 0);
    }
}

static void generate_vectors_symbols (void)
{

    generate_flash_vectors_symbol("__svectors_nvm");
    
    ///\ generate itcm .vectors symbol
    _bfd_generic_link_add_one_symbol (&link_info, link_info.output_bfd, "__svectors",
                                BSF_GLOBAL, bfd_abs_section_ptr,
                                pic32c_itcm_vectors_address,
                                "__svectors", 1, 0, 0);
    
}


static void check_absolute_section(struct pic32_section *s)
{
    lang_memory_region_type *m;
//    bfd_boolean found = FALSE;
    if (s->sec == NULL) return;
    
    /* cannot do this - need to reach into ldlang to get a list of regions
     this check is a work in progress ... see XC32E-406
    for (m = lang_memory_region_list; m != NULL; m = m->next)
    {
	if ((s->sec->lma >= m->origin) && (s->sec->lma < (m->origin + m->length))) {
	    found = TRUE;
	    break;
	}
	
    }
    
    if (pic32c_debug) {
	if (found) printf("found absolute section %s in region %s \n",s->sec->name, m->name_list.name);
	else printf("Error: invalid address for absolute section %s \n",s->sec->name);
    }
    */

    if ((region_info.tcm_style == TCM_COMBINED) && pic32_valid_region(region_info.code_tcm)) {
	m = region_info.code_tcm->region;
	if ((s->sec->lma >= m->origin) && (s->sec->lma < (m->origin + m->length)))
	    einfo("Absolute section %s is in tcm memory.  Not permitted on this device. \n",s->sec->name);
    }
}






static void
report_allocation_error(struct pic32_section *s) {
#define PREFIX "/tmp"
    char *colon = "";
    char *filename = "";
    char *secname = "*";
    char *s1 = pic32_section_size_string(s->sec);
    char *s2 = pic32_section_attr_string(s->sec);

    /* report the file name, unless its a temp file */
    if (strncmp(s->sec->owner->filename, PREFIX, strlen(PREFIX)) != 0) {
      colon = ":";
      filename = (char *) s->sec->owner->filename;
    }

    /* report the section name, unless its based on a temp file */
    if (strncmp(s->sec->name, PREFIX, strlen(PREFIX)) != 0)
      secname = (char *) s->sec->name;

    einfo(_("%X%s%s Link Error: Could not allocate section %s, %s, %s\n"),
          filename, colon, secname, s1 ? s1 : "", s2 ? s2 : "");
 
    if (s1) free(s1);  /* free the malloc'ed strings */
    if (s2) free(s2);

    if (pic32_debug)
      fprintf(stderr,"\n    Error: Could not allocate section %s\n", s->sec->name);
}

/*
 * allocate_memory()
 *
 * This function attempts to locate sections using
 * a best-fit algorithm. The term "best-fit" implies
 * that the smallest suitable free block will be used
 * for each section. This approach makes efficient use
 * of the free block list and reduces fragmentation.
 *
 * Called by: ldemul_after_allocation()
 *
 * Calls:     allocate_program_memory()
 *            allocate_data_memory()
 *            bfd_map_over_sections()
 *
 * If any of the sub-processes fail, report it
 * and continue. This helps to suppress misleading
 * messages, and follows the general philosophy
 * of doing as much work as we can, despite the
 * occurrence of fatal errors.
 */
static void
allocate_memory() {
  int result;
  lang_output_section_statement_type **last_os;

#define ERR_STR " Link Error: Could not allocate "

  /* save the last output section statement
     after sequential allocation */
  last_os = (lang_output_section_statement_type **) lang_output_section_statement.tail;


  if (region_info.tcm_style == TCM_COMBINED) {

  
    /*
     * with combined TCM we need to enforce a rule that programmers cannot mark any item as both
     * tcm and absolute.   If this is allowed it becomes very complicated to make sure that data
     * and code do not co-exist on the same cache line
     */
    struct pic32_section *s, *prev, *next;

    // check all the absolute sections to see that they make sense
    prev = unassigned_sections;
    for (s = prev; s != NULL; s = next) {
      if ((s->sec != NULL) && PIC32_IS_ABSOLUTE_ATTR(s->sec)) {
          check_absolute_section(s);
      }
      next = s->next;
    }
  }


  
  if (pic32_debug) {
      pic32_print_section_list(unassigned_sections, "unassigned");
      pic32_print_section_list(memory_region_list, "memory region");
  }

  /*
   * Build an ordered list of output sections
   * that were placed by sequential allocator.
   * It will help identify any gaps between
   * output sections that are available.
   */
  pic32_init_section_list (&pic32_section_list);

  bfd_map_over_sections (link_info.output_bfd, &pic32_build_section_list, NULL);
  bfd_map_over_sections (link_info.output_bfd, &pic32_build_section_list_vma, NULL);
    
  if (pic32_debug) {
    pic32_print_section_list(unassigned_sections, "unassigned");
    pic32_print_section_list(memory_region_list, "memory region");
  }
    
    /* build the free blocks list for all known regions. */
    pic32_build_free_blocks(&region_info);

    result = allocate_tcm_memory();
    if (result != 0)
        einfo(_("%F%stcm memory\n"), ERR_STR );

    result = allocate_data_memory();
    if (result != 0)
        einfo(_("%F%sdata memory\n"), ERR_STR );

    result = allocate_program_memory();
    if (result != 0)
        einfo(_("%F%sprogram memory\n"), ERR_STR );

#ifdef ENABLE_USER_MEMORY
    if (has_user_defined_memory) {
        result = allocate_user_memory();
        if (result != 0)
            einfo(_("%F%suser-defined memory region\n"), ERR_STR );
    }
#endif

    // if vectors are not to be relocated, generate the symbols in the
    // default code region
    if (!pic32c_relocate_vectors) {
        generate_flash_vectors_symbol("__svectors_nvm");
        generate_flash_vectors_symbol("__svectors");
    }
  
    /* allocate the heap, if required */

  /* allocate the stack, unless the user has defined one */

  /* free the output section list */
  pic32_free_section_list(&pic32_section_list);

  /*
   * Scan over the output section statements
   * and merge any contiguous sections
   * with the same name, unless the
   * --unique option tells us not to.
   *
   * We start where the sequential allocator
   * finished, so that any merges are well understood.
   * For example, all of the best-fit output sections
   * contain a single input section.
   */
  {
    lang_statement_union_type *os, *next;
    asection *sec, *next_sec;
    unsigned int len, match, merge_sec = 0;
    bfd_vma sec_len, next_len = 0, merge_len = 0;
    char *p,*p2;

    if (pic32_debug)
      printf("\nScanning the output statements\n");

    for (os = (lang_statement_union_type *) *last_os;
         os != (lang_statement_union_type *) NULL;
         os = next) {

      /* clear the accumulator, if we didn't just merge */
      if (!merge_sec) merge_len = 0;

      merge_sec = 0;
      next = os->header.next;
      if (os->header.type == lang_output_section_statement_enum) {

        if (os->output_section_statement.bfd_section == NULL)
          /* --gc-sections has discarded this section */
          continue;

        sec = os->output_section_statement.bfd_section;
        if (os->output_section_statement.children.head->input_section.section->rawsize)
          sec_len = os->output_section_statement.children.head
                    ->input_section.section->rawsize;
        else
          sec_len = os->output_section_statement.children.head
                    ->input_section.section->size;

        if (!sec || !sec->name) continue;

        if (next && (next->header.type == lang_output_section_statement_enum)) {
          next_sec = next->output_section_statement.bfd_section;
	  if (next->output_section_statement.children.head) {
            if (next->output_section_statement.children.head->
                         input_section.section->rawsize)
              next_len = next->output_section_statement.children.head->
                         input_section.section->rawsize;
            else
              next_len = next->output_section_statement.children.head->
                         input_section.section->size;
          }

          if (!next_sec || !next_sec->name) continue;

          if (next->output_section_statement.children.head->
                input_section.section->size == 0) continue;

          /* if section address and len don't match, continue */
          if ((sec->lma + sec_len + merge_len) != next_sec->lma) continue;

          p = strchr(sec->name, '%');
          p2 = strchr(next_sec->name, '%');
          if (p && p2) {
            len = p - sec->name;
            if (len != (unsigned) (p2 - next_sec->name))
              continue;
            match = (strncmp(sec->name, next_sec->name, len) == 0);
          } else
            match = (strcmp(sec->name, next_sec->name) == 0);

          if (match && !config.unique_orphan_sections &&
              !pic32_unique_section(sec->name) &&
              (pic32_attribute_map(sec) == pic32_attribute_map(next_sec))) {

            if (pic32_debug) {
              printf("  Merging output sections %s and %s\n",
                      sec->name, next_sec->name);
              printf("    %s: addr = %lx, len = %lx\n",
                     sec->name, sec->lma, sec_len + merge_len);
              printf("    %s: addr = %lx, len = %lx\n",
                     next_sec->name, next_sec->lma, next_len);
            }

            merge_sec = 1; /* set a flag to indicate we're merging */
            merge_len += next_len;
            next->output_section_statement.children.head->
              input_section.section->output_section = NULL;

            lang_add_section (&os->output_section_statement.children,
                              next->output_section_statement.children.head
                              ->input_section.section,
                              NULL,
                              &os->output_section_statement);

            /* remove the merged section from output_bfd */
            remove_section_from_bfd(link_info.output_bfd, next_sec);
          }
        }
      }

      if (merge_sec) {
        if (!(os->header.next = next->header.next)) /* unlink the merged statement */
          /* update statement_list.tail if we just removed/merged the last stmt */
          statement_list.tail = &os->header.next;

        /* there's another (doubly) linked list containing only the output section */
        /* statements so also update its links and the tail address when the last */
        /* output statement is removed / merged */
        if (!(os->output_section_statement.next = next->output_section_statement.next))
          lang_output_section_statement.tail = (union lang_statement_union **) &os->output_section_statement.next;
        else
          os->output_section_statement.next->prev = (struct lang_output_section_statement_struct *) os;
        next = os;                            /* try to merge another one */
      }
    }
  }
} /* allocate_memory() */

/*
 * allocate_tcm_memory()
 *
 * This function attempts to allocate all itcm and dtcm input memory sections.
 *
 * Called by: allocate_memory()
 *
 * Calls:     build_alloc_section_list()
 *            build_free_block_list()
 *            locate_sections()
 *            pic32_section_list_length()
 *
 * Returns:   status code
 *            (0 = success)
 */
static int
allocate_tcm_memory() {

  unsigned int mask = itcm|dtcm ;
  int result = 0;
  
  if (pic32_debug)
    fprintf(stderr,"\nBuilding allocation list for regions \"itcm\", \"dtcm\" and \"tcm\"\n"
           "  attribute mask = %x\n", mask);

  build_alloc_section_list(mask);
  // if (pic32_section_list_length(alloc_section_list) == 0)
  //   return result;


  if (pic32_debug) {
      pic32_print_section_list(alloc_section_list, "tcm input");
  }
    
// first we allocate itcm sections to itcm_region
// then we allocate dtcm input sections to dtcm_region
// these two might be the same ... in that case we will need to check that no free block in free_blocks has any partial cache line.
// for now the cache line size is hard coded - there is only one such family (Jubilee) and the line size is 16 bytes
// in the more common case that itcm and dtcm are separate memory regions we will need to build a new free block list


  if (!pic32_valid_region(region_info.code_tcm)) {
      // do not even try if itcm_region is not defined ...
      if (pic32_debug)     fprintf(stderr,"no output region for itcm input sections  : not proceeding with BF allocation \n");
  }
  else {
      if (pic32c_vectors_in_tcm && pic32_valid_region(region_info.vectors))  {
	      reserve_space_for_vectors(region_info.vectors);
	      generate_vectors_symbols();
      } 

      reset_locate_options();
      
      result |= locate_sections(address, dtcm, region_info.code_tcm);   /* most restrictive  */
      result |= locate_sections(all_attr, dtcm, region_info.code_tcm);  /* less restrictive */
  }
  
  if (!pic32_valid_region(region_info.data_tcm)) {
      // do not even try if dtcm_region is not defined ...
      if (pic32_debug)     fprintf(stderr,"no output region for dtcm input sections  : not proceeding with BF allocation \n");
  }
  else {
      // 
      // make sure that the free blocks list does not contain any partial cache lines
      // TODO: make cache line size configurable
      if (region_info.tcm_style == TCM_COMBINED)
	  align_free_blocks(region_info.data_tcm->free_blocks, JUBILEE_CACHE_LINE_ALIGNMENT_POWER);
      
      
      reset_locate_options();
      
      result |= locate_sections(address, 0, region_info.data_tcm);   /* most restrictive  */
      result |= locate_sections(all_attr, 0, region_info.data_tcm);  /* least restrictive */
  }

  if (pic32c_stack_in_tcm)
      allocate_stack_in_dtcm();

  
  return result;
} /* allocate_tcm_memory() */



/*
 * allocate_program_memory()
 *
 * This function attempts to allocate program memory sections.
 *
 * Called by: allocate_memory()
 *
 * Calls:     build_alloc_section_list()
 *            build_free_block_list()
 *            locate_sections()
 *            pic32_section_list_length()
 *
 * Returns:   status code
 *            (0 = success)
 */
static int
allocate_program_memory() {
  unsigned int mask = code ;
  int result = 0;

  if (pic32_debug)
    fprintf(stderr,"\nBuilding allocation list for region \"program\" \n"
           "  attribute mask = %x\n", mask);

  build_alloc_section_list(mask);
  if (pic32_section_list_length(alloc_section_list) == 0)
    return result;

  if (pic32_debug) {
    pic32_print_section_list(alloc_section_list, "allocate_code");
  }

    
// now we  allocate the list of input sections we just built to ROM

  if (!pic32_valid_region(region_info.code)) {
      // do not bother going further if rom was not defined in the linker script ...
      if (pic32_debug) 
          fprintf(stderr, "no section named rom : not proceeding with BF allocation\n");

      return 0;
  }

  reset_locate_options();

  result |= locate_sections(address, itcm, region_info.code);   /* most restrictive  */
  result |= locate_sections(all_attr, itcm, region_info.code);  /* less restrictive */
  
  return result;
} /* allocate_program_memory() */



/*
 * allocate_data_memory()
 *
 * This function attempts to allocate data memory sections.
 *
 * Called by: allocate_memory()
 *
 * Calls:     build_section_list()
 *            build_free_block_list()
 *            locate_sections()
 *
 * Returns:   status code
 *            (0 = success)
 *
 * Notes: List "data_memory_free_blocks" is needed by
 *        bfd_pic32_finish() to allocate the stack
 *        and heap, so don't exit this function early.
 *        DZ: data_memory_free_blocks is now region_info.data->free_blocks, 
 *        but the same caveat about freeing the list applies.
 *
 *        EDS allocation is tricky. Although these sections
 *        can be allocated anywhere, we need to preserve
 *        low memory for the stack. In the first pass,
 *        we exclude memory below the stack limit altogether.
 *        In the second pass, we scan free blocks in reverse
 *        only, which helps when a block crosses the
 *        stack limit boundary. Eventually we'll take
 *        whatever memory we need to, and leave the rest
 *        for the stack and heap.
 *
 *        We also support allocation of stack and heap
 *        that are defined with section attributes.
 *
 *        See comments below for additional info
 *        about stack allocation.
 */
static int
allocate_data_memory() {
    struct pic32_section *s;
    unsigned int mask = data|bss|persist|stack|heap|ramfunc;
    int result = 0;

    if (pic32_debug)
        fprintf(stderr,"\nBuilding allocation list for region \"data\"\n"
               "  attribute mask = %x\n", mask);

    // build a list of data sections to be allocated -should not contain any tcm

    build_alloc_section_list(mask);

    if (pic32_debug) {
        pic32_print_section_list(alloc_section_list, "allocate_data");
    }

    if (!pic32_valid_region(region_info.data)) {
        // do not bother going further if  ram was not defined in the linker script ...
        if (pic32_debug) fprintf(stderr,"no section named ram : not proceeding with BF allocation\n");
        return 0; 
    }

    /* TODO: try to move the reserving of space for vectors outside of specific
       allocate functions. for now we do it here assuming allocate_tcm didn't, 
       based on the presence of the code_tcm region. */
    if (pic32c_relocate_vectors && !pic32c_vectors_in_tcm) {
      reserve_space_for_vectors(region_info.vectors);
      generate_vectors_symbols();
    }

    // now we allocate the sections on the list to ram
    // 
    reset_locate_options();

    result |= locate_sections(address, dtcm, region_info.data);
    result |= locate_sections(all_attr, stack|heap|dtcm, region_info.data);

#if 0
  /* user-defined heap */
  if (ramfunc_begin != 0) {
    set_locate_options(EXCLUDE_HIGH_ADDR, ramfunc_begin);
  }
  result |= locate_sections(heap, 0, region);

  /* user-defined stack */
  if (ramfunc_begin != 0) {
    set_locate_options(EXCLUDE_HIGH_ADDR, ramfunc_begin);
  }
  result |= locate_sections(stack, 0, region);
#else
  /* Don't support user-defined stack sections yet */
  for (s = alloc_section_list; s != NULL; s = s->next) {
      /* lghica co-resident */
#if 1
      if(s && (s->sec != NULL) && (s->sec->linked))
          continue;
#endif
    if (s->sec && (PIC32_IS_STACK_ATTR(s->sec) || PIC32_IS_HEAP_ATTR(s->sec))) {
      lang_output_section_statement_type *os;
      s->sec->lma = 0;
      s->sec->vma = 0;
      os = lang_output_section_statement_lookup (".stack", 0, TRUE);
      /* lang_add_section() will call init_os() if needed */
      lang_add_section (&os->children, s->sec, NULL, os);
      finish_section_info(s, os);
      pic32_remove_from_section_list(alloc_section_list,s);
      os->bfd_section->flags = s->sec->flags;
    }
  }

#endif

  /* if any sections are left in the allocation list, report an error */
  for (s = alloc_section_list; s != NULL; s = s->next) {
  if ((s->attributes != 0) && (s->sec->linked == 0) /* lghica co-resident*/)
  {
    report_allocation_error(s);
    result = 1;
    break;
  }
  }


  return result;
} /* allocate_data_memory() */

static bfd_vma
group_section_size(struct pic32_section *g)
{
  struct pic32_section *s,*next;
  bfd_vma result = 0;

  for (s = g; s != NULL; s = next) {
    next = s->next;
    if (s->sec == 0)
      continue;
    if (s->sec && (PIC32_IS_RAMFUNC_ATTR(s->sec)))
      result += s->sec->size;
  }
  return result;
}

#define OUTSIDE_REGION(addr, len, region)          \
  (((addr + len) <= region->origin)                \
   || (addr >= (region->origin + region->length)))

#define ACROSS_REGION(addr, len, region)                            \
  (((addr < region->origin) && ((addr + len) > region->origin)) ||  \
   ((addr < (region->origin + region->length)) &&                   \
    ((addr + len) > (region->origin + region->length))))


/*
 * locate_group_section()
 *
 * Helper function to locate a grouped section
 * using the free_blocks list.
 *
 * Called by: locate_sections()
 *
 * Returns:   status code
 *            (0 = success)
 */

static int
locate_group_section(struct pic32_section *s,
                     struct pic32_region *regn) {

  struct pic32_memory *b;
  bfd_vma len = group_section_size(s);
  bfd_vma addr = s->sec->lma;
  int result = 0;

  /* DEBUG */
  if (pic32_debug)
    fprintf(stderr,"  group section \"%s\", total size = %lx\n",
           s->sec->name, len);

  /* look for tricky user error */
  if (PIC32_IS_ABSOLUTE_ATTR(s->sec) && ACROSS_REGION(addr, len, regn->region))
        einfo(_(" Link Warning: absolute section \'%s\' crosses"
                " the boundary of region %s.\n"),
              s->sec->name, regn->region->name_list.name);

  if (len == 0)
    update_group_section_info(0,s,regn); /* trivial case */
  else if (PIC32_IS_ABSOLUTE_ATTR(s->sec) &&
           OUTSIDE_REGION(addr, len, regn->region)) {
    update_group_section_info(addr, s, regn);  /* falls outside region */
  }
  else {                          /* locate using free_blocks list */
    b = select_free_block(regn, s, len);
    if (b) {
      addr = b->addr + b->offset;
#if 0 ///\ LG
    /* Translate the address from kseg0 to kseg1 */
      if (PIC32_IS_COHERENT_ATTR(s->sec))
        addr |= 0x20000000u;
#endif
      update_group_section_info(addr,s,regn);
      create_remainder_blocks(regn->free_blocks,b,len);
      remove_free_block(regn->free_blocks, b);
    } else {
      if (locate_options != NO_LOCATE_OPTION) {
        if (pic32_debug)
          fprintf(stderr,"    \"%s\" location declined\n", s->sec->name);
        return 0;
      }
      result |= 1;
    }

    s->region = regn;
  }

  if (pic32_debug)
    fprintf(stderr,"    removing group from allocation list\n");
  pic32_remove_group_from_section_list(alloc_section_list);

  return result;
} /* locate_group_section() */


/*
 * locate_single_section()
 *
 * Helper function to locate a single section
 * using the free_blocks list.
 *
 * Called by: locate_sections()
 *
 * Returns:   status code
 *            (0 = success)
 */

static int
locate_single_section(struct pic32_section *s,
                      struct pic32_region *regn) {

  struct pic32_memory *b;
  bfd_vma len = s->sec->rawsize? s->sec->rawsize : s->sec->size;
  bfd_vma addr = s->sec->lma;
  int result = 0;

  s->region = NULL; /* initially unassigned */
  
  /* look for tricky user error */
  if (PIC32_IS_ABSOLUTE_ATTR(s->sec) && ACROSS_REGION(addr, len, regn->region))
        einfo(_(" Link Warning: absolute section \'%s\' crosses"
                " the boundary of region %s.\n"),
              s->sec->name, regn->region->name_list.name);
  
  if (len == 0)
    update_section_info(0,s,regn);       /* trivial case */
  else if (PIC32_IS_ABSOLUTE_ATTR(s->sec) &&
           OUTSIDE_REGION(addr, len, regn->region)) {
    update_section_info(addr, s, regn);  /* falls outside region */
  }
  else {                          /* locate using free_blocks list */
    b = select_free_block(regn, s, len);
    if (b) {
      addr = b->addr + b->offset;
      /* FIXME: this seems pretty device specific, but for MPUs we'll want
         to place coherent things in non-cached DDR sometimes.. */
      /* MERGE-NOTES: it is MIPS specific. getting rid of it just in case. */
#if 0
      if (PIC32_IS_COHERENT_ATTR(s->sec))
        addr |= 0x20000000;
#endif
      update_section_info(addr,s,regn);
      create_remainder_blocks(regn->free_blocks,b,len);
        /* lghica co-resident */
#if 0
        if (PIC32_IS_SHARED_ATTR(s->sec) && PIC32_IS_ABSOLUTE_ATTR(s->sec))
        {
            pic32_add_to_memory_list(shared_data_memory_blocks,
                                     b->addr+b->offset, len);
        }
#endif
      remove_free_block(regn->free_blocks,b);
    } else {
      if (locate_options != NO_LOCATE_OPTION) {
        if (pic32_debug)
          fprintf(stderr,"    \"%s\" location declined\n", s->sec->name);
        return 0;
      }
      result |= 1;
    }
  }

  if (pic32_debug)
    fprintf(stderr,"    removing %s from allocation list\n", s->sec->name);

  pic32_remove_from_section_list(alloc_section_list,s);

  return result;
} /* locate_single_section() */


/*
 * locate_sections()
 *
 * This function attempts to locate all sections
 * in the section list that match a specified
 * attribute mask. The block argument specifies
 * attributes that disqualify a section.
 *
 * Called by: allocate_program_memory()
 *            allocate_data_memory()
 *            allocate_eedata_memory()
 *            allocate_auxflash_memory()
 *
 * Calls:     select_free_block()
 *            update_section_info()
 *            create_remainder_blocks()
 *            remove_free_block()
 *            remove_alloc_section()
 *
 * Returns:   status code
 *            (0 = success)
 */
static int
locate_sections(unsigned int mask, unsigned int block,
                struct pic32_region *regn)
{
    struct pic32_section *s,*next;
    int result = 0;
    static int ramfunc_section_count = 0;
    
    if (pic32_debug) {
	fprintf(stderr,"\nLocating sections with mask %x, but not %x\n", mask, block);
	fprintf(stderr,"  locate_options = %x, exclude_addr = %lx\n",
	       locate_options, exclude_addr);
    }
    
    for (s = alloc_section_list; s != NULL; s = next) {
	
	next = s->next;

	/* lghica co-resident */
#if 0
	if (s->sec && (PIC32_IS_DATA_ATTR(s->sec) || PIC32_IS_BSS_ATTR(s->sec))
            && !PIC32_IS_ABSOLUTE_ATTR(s->sec) && (s->sec->linked == 1))
	{
	    update_section_info(s->sec->lma, s, regn);
	}
	else 
#endif
        {
	    if (s->sec && (s->attributes & mask) &&
		((s->attributes & block) == 0)) {
		bfd_vma len = s->sec->rawsize? s->sec->rawsize: s->sec->size;
		
		if (pic32_debug) {
		    char *attr = pic32_section_attr_string(s->sec);
		    
		    fprintf(stderr,"  b \"%s\", len = %lx, %s\n",
			   s->sec->name, (unsigned long)len, attr ? attr : "");
		    if (attr) free(attr);
		}
		
		if (PIC32_IS_RAMFUNC_ATTR(s->sec)) {
		    if (ramfunc_section_count == 0) {
			ramfunc_section_count++;
			result |= locate_group_section(s, regn);
			if (s->sec->vma == 0)
			{
			    report_allocation_error(s);
			}
		    }
		}
		else {
		    result |= locate_single_section(s, regn);
		}
	    }
	}
    }
    return result;
} /* locate_sections() */


/*
 * next_aligned_address()
 */
static bfd_vma
next_aligned_address( bfd_vma start, unsigned int align_power) {

  bfd_vma mask = ~((~0) << align_power);
  return (bfd_vma) ((start + 1 + mask) & (~mask));
}


/*
 * prev_aligned_address()
 */
static bfd_vma
prev_aligned_address( bfd_vma start, unsigned int align_power) {

  bfd_vma mask = ~((~0) << align_power);
  if (start < mask)
    return (bfd_vma) 0;
  else
    return (bfd_vma) ((start - 1 - mask) & (~mask));
}


/*
 * select_free_block()
 *
 * This function attempts to select a suitable
 * free block for a specified section. For each
 * free block, the section is positioned two ways:
 *  (1) as close to the starting address as possible,
 *  (2) as close to the ending address as possible.
 *
 * The position that leaves the largest unused
 * remainder space is selected. If a suitable free
 * block is found, the block offset is written into
 * the free block structure and the structure is returned.
 *
 * The selection algorithm can be fine-tuned by previous
 * calls to set_locate_options().
 *
 * Called by: locate_sections()
 *            locate_group_sections()
 *
 * Calls:     pic32_static_assign_memory()
 *            next_aligned_address()
 *            prev_aligned_address()
 *
 * Returns:   suitable free block, or 0
 */

#if 0
static void
confirm_dma_range_defined() {
  struct bfd_link_hash_entry *h;
  static bfd_boolean base_err_reported = FALSE;
  static bfd_boolean end_err_reported = FALSE;

  if ((!dma_base_defined) || (!dma_end_defined)) {

    h = bfd_pic32_is_defined_global_symbol(str3);
    if (h) {
      dma_base = h->u.def.value;
      dma_base_defined = TRUE;
    }
    else {
      if (!base_err_reported) {
        einfo(_(str1), str3);
        base_err_reported = TRUE;
      }
    }

    h = bfd_pic32_is_defined_global_symbol(str4);
    if (h) {
      dma_end = h->u.def.value;
      dma_end_defined = TRUE;
    }
    else {
      if (!end_err_reported) {
        einfo(_(str1), str4);
        end_err_reported = TRUE;
      }
    }
  }
}
#endif


static struct pic32_memory *
select_free_block(struct pic32_region *regn, struct pic32_section *s, unsigned int len) {

  unsigned int align_power = s->sec->alignment_power;

  const char *err_str1 = "Link Error: Could not allocate section";

  struct pic32_memory *b = 0;
  bfd_vma option1, limit1;
  bfd_vma option2, limit2;
  bfd_boolean option1_valid, option2_valid;
/* lghica co-resident */
    unsigned int    opb = bfd_octets_per_byte (link_info.output_bfd);
    bfd_vma         old_option1, new_option1;
    bfd_vma         old_option2, new_option2;
    
    
  /*
   * If the section is absolute, call the static function
   */
  if (PIC32_IS_ABSOLUTE_ATTR(s->sec))
  {
    /* lghica co-resident */
#if 0
      if (PIC32_IS_SHARED_ATTR(s->sec) && PIC32_IS_ABSOLUTE_ATTR(s->sec))
      {
          b = pic32_static_assign_memory(shared_data_memory_blocks,len,s->sec->lma);
      }
      else
#endif
          b = pic32_static_assign_memory(regn->free_blocks, len, s->sec->lma);
    if (!b) {
        if ((s->sec->flags & SEC_NEVER_LOAD) ||
            (command_line.check_section_addresses == FALSE))
          /* OK, don't report as error */;
        else
        einfo(_("%X %s \'%s\' at 0x%v\n"),
                err_str1, s->sec->name, s->sec->lma);
        return (struct pic32_memory *) NULL;
    } else
      return b;
  }

#if 0
  /*
   * If DMA attribute is set, make sure
   * we have a definition for __DMA_BASE and __DMA_END
   * Usually this appears in the linker script.
   */
  if (PIC32_IS_DMA_ATTR(s->sec)) {
    confirm_dma_range_defined();
  }
#endif

  /*
   * Loop through the free blocks list
   */
  for (b = regn->free_blocks; b != NULL; b = b->next) {

    if ((b->addr == 0) && (b->size == 0))
      continue;

    if (pic32_debug)
      fprintf(stderr,"    consider block at %lx, len = %lx\n", b->addr, b->size);

    /*
     * Qualify the block first, so we don't waste
     *  time scanning for alignment.
     */
    if (b->size < len)
      continue;


    /* qualify NEAR with leftmost position in free block */
    if (PIC32_IS_NEAR_ATTR(s->sec) && !VALID_NEAR(b->addr, len))
      continue;

    /* qualify LOW_ADDR with leftmost position in free block */
    if (IS_LOCATE_OPTION(EXCLUDE_HIGH_ADDR) &&
        !VALID_LOW_ADDR(b->addr, len))
      continue;

    /* qualify HIGH_ADDR with rightmost position in free block */
    if (IS_LOCATE_OPTION(EXCLUDE_LOW_ADDR) &&
        !VALID_HIGH_ADDR((b->addr + b->size - len), len))
      continue;

    /* fall through if block passes pre-qual tests */

    /* Compute option1: scan from the start of the block */
    option1 = b->addr - 1 ;
    limit1  = b->addr + (b->size - len);
    option1_valid = TRUE; /* set default */
    if (pic32_debug)
      fprintf(stderr,"    option1 [advancing from %lx]\n", option1 + 2);
    while (1) {

      if (IS_LOCATE_OPTION(FAVOR_HIGH_ADDR)) {
        option1_valid = FALSE;
        break;
      }

      /* find the next aligned address */
      if (PIC32_IS_REVERSE_ATTR(s->sec)) {
        option1 = next_aligned_address(option1 + len, align_power);
        option1 -= len;
      }
      else
        option1 = next_aligned_address(option1, align_power);

      if (option1 > limit1) {
        option1_valid = FALSE;
        break;
      }


      if (pic32_debug)
        fprintf(stderr,"    option1 aligned at %lx\n", option1);

      /* aligned address is valid, check other attributes */


        if (PIC32_IS_NEAR_ATTR(s->sec)  && !VALID_NEAR(option1, len)) {
        option1_valid = FALSE;  /* scanning forward won't help */
        break;
      }
        
      /* lghica co-resident */
#if 1
        /* this check is necessary for Co-resident applications */
        /* to make sure persistent data is not over written at any time */
        
        if (PIC32_IS_PERSIST_ATTR(s->sec) || PIC32_IS_SHARED_ATTR(s->sec))
        {
            old_option1 = option1;
            new_option1 = option1;
            struct pic32_section *ls;
            for (ls = alloc_section_list; ls != NULL; ls = ls->next)
            {
                if (ls->sec && (ls->sec->linked == 1))
                {
                    if (((ls->sec->lma <= old_option1)
                         && (old_option1 < (ls->sec->lma + (ls->sec->rawsize / opb)))))
                    {
                        new_option1 = ls->sec->lma + (ls->sec->rawsize / opb);
                        break;
                    }
                }
            }
            if (new_option1 != old_option1)
                continue;
            else
                option1= new_option1;
        }
#endif
        
      if (IS_LOCATE_OPTION(EXCLUDE_LOW_ADDR) &&
          !VALID_HIGH_ADDR(option1, len)) {
        option1 = exclude_addr - 1;  /* skip ahead */
        if (pic32_debug)
          fprintf(stderr,"    approaching EXCLUDE boundary from %lx\n", option1);
        continue;
      }

      /* if we get here, the option is valid */
      break;
    } /* while (!done) */

    /* Compute option2: scan from the end of the block       */
    /*                  unless the section is BOOT or SECURE */
    option2 = limit1 + 1;
    limit2  = b->addr;
      option2_valid = TRUE; /* set default */
      if (pic32_debug)
        fprintf(stderr,"    option2 [descending from %lx]\n", option2 - 2);
      while (1) {

        if (IS_LOCATE_OPTION(FAVOR_LOW_ADDR)) {
          option2_valid = FALSE;
          break;
        }

        /* find the next aligned address */
        if (PIC32_IS_REVERSE_ATTR(s->sec)) {
          option2 = prev_aligned_address(option2 + len, align_power);
          option2 -= len;
        }
        else
          option2 = prev_aligned_address(option2, align_power);

        if (option2 < limit2) {
          option2_valid = FALSE;
          break;
        }

        if (pic32_debug)
          fprintf(stderr,"    option2 aligned at %lx\n", option2);

          /* lghica co-resident */
#if 1
          /* this check is necessary for Co-resident applications */
          /* to make sure persistent data is not over written at any time */
          if (PIC32_IS_PERSIST_ATTR(s->sec) || PIC32_IS_SHARED_ATTR(s->sec)) {
              old_option2 = option2;
              new_option2 = option2;
              struct pic32_section *ls;
              for (ls = alloc_section_list; ls != NULL; ls = ls->next)
              {
                  if (ls->sec && (ls->sec->linked == 1))
                  {
                      if (((ls->sec->lma <= old_option2)
                           && (old_option2 < (ls->sec->lma + (ls->sec->rawsize / opb)))))
                      {
                          new_option2 = ls->sec->lma - len;
                          break;
                      }
                  }
              }
              if (new_option2 != old_option2) 
                  continue;
              else
                  option2= new_option2;
          }
#endif
        if  (PIC32_IS_NEAR_ATTR(s->sec)    && !VALID_NEAR(option2, len)) {
          option2 = NEAR_BOUNDARY - len + 1;  /* skip back */
          if (pic32_debug)
            fprintf(stderr,"    approaching Near boundary from %lx\n", option2);
          continue;
        }


        if (IS_LOCATE_OPTION(EXCLUDE_HIGH_ADDR) &&
            !VALID_LOW_ADDR(option2, len)) {
          option2 = exclude_addr - len + 1;  /* skip back */
          if (pic32_debug)
            fprintf(stderr,"    approaching EXCLUDE boundary from %lx\n", option2);
          continue;
        }

        /* if we get here, the option is valid */
        break;
      } /* while (!done) */

    if ((option1_valid == FALSE) && (option2_valid == FALSE))
      continue;  /* try the next free block */

    if (pic32_debug) {
      fprintf(stderr,"    ");
      if (option1_valid)
        fprintf(stderr,"option1 = %lx, ", option1);
      if (option2_valid)
        fprintf(stderr,"option2 = %lx", option2);
      fprintf(stderr,"\n");
    }

    /* we have 1 or 2 valid options */
    if (option1_valid == FALSE)
      b->offset = option2 - b->addr;
    else if (option2_valid == FALSE)
      b->offset = option1 - b->addr;
    else {
      /* compare the two "outer" remainders */
      bfd_vma rem1 = option1 - b->addr;
      bfd_vma rem2 = (b->addr + b->size) - option2 - len;

      b->offset = (rem1 > rem2) ?
                  (option2 - b->addr) : (option1 - b->addr);
    }

    return b;
  } 

  /* If we get here, a suitable block could not be found */
  if (locate_options != NO_LOCATE_OPTION)
    return (struct pic32_memory *) NULL; /* exit quietly */
  else
    report_allocation_error(s);

#if 0
  {
#define PREFIX "/tmp"
    char *colon = "";
    char *filename = "";
    char *secname = "*";
    char *s1 = pic32_section_size_string(s->sec);
    char *s2 = pic32_section_attr_string(s->sec);

    /* report the file name, unless its a temp file */
    if (strncmp(s->sec->owner->filename, PREFIX, strlen(PREFIX)) != 0) {
      colon = ":";
      filename = (char *) s->sec->owner->filename;
    }

    /* report the section name, unless its based on a temp file */
    if (strncmp(s->sec->name, PREFIX, strlen(PREFIX)) != 0)
      secname = (char *) s->sec->name;

    einfo(_("%X%s%s Link Error: Could not allocate section %s, %s, %s\n"),
          filename, colon, secname, s1 ? s1 : "", s2 ? s2 : "");

    if (s1) free(s1);  /* free the malloc'ed strings */
    if (s2) free(s2);

    if (pic32_debug)
      fprintf(stderr,"\n    Error: Could not allocate section %s\n", s->sec->name);
  }
#endif

  return (struct pic32_memory *) NULL;
} /* select_free_block() */

static char *
unique_section_name(const char *s) {
  static int auto_sec_cnt = 0;
  char *name;
  int len;

  len = strlen(s) + 1 + 5;
  name = xmalloc (len + 1);
  snprintf(name, len, "%s%s%d", s, "%", ++auto_sec_cnt);
  return name;
}


static char *
unique_zero_length_section_name(const char *s) {
  static int auto_zero_sec_cnt = 0;
  char *name;
  int len;

  len = strlen(s) + 2 + 5;
  name = xmalloc (len + 1);
  snprintf(name, len, "%s%s%d", s, "%z", ++auto_zero_sec_cnt);
  return name;
}

/*
 * finish_section_info()
 *
 * called by:
 *  update_section_info()
 *  update_grouped_section_info()
 *
 * Note that we don't copy in the section size,
 * because lang_process() will calculate the
 * output section size after the emulation
 * returns control.
 */
static void
finish_section_info(struct pic32_section *s, lang_output_section_statement_type *os)
{
  etree_type *addr_tree;

  if (os->bfd_section == NULL) /* --gc-sections discarded this, so done */
    return;
  os->bfd_section->vma = s->sec->vma;
  os->bfd_section->lma = s->sec->lma;
  addr_tree = xmalloc(sizeof(etree_type));
  addr_tree->value.type.node_class = etree_value;
  addr_tree->value.value = s->sec->vma;
  os->addr_tree = addr_tree;

} /* finish_section_info() */


#define update_section_addr(s,addr)                     \
  {                                                     \
  s->lma = addr;                                        \
  s->vma = addr;                                        \
  }

/*
 * update_section_info()
 *
 * Update section info after allocation
 *
 * If section was a heap or stack, set some globals.
 *
 */
static void
update_section_info(bfd_vma alloc_addr,
                    struct pic32_section *s,
                    struct pic32_region *regn) {

  lang_output_section_statement_type *os;
  char *name;

  update_section_addr(s->sec, alloc_addr);

  if (pic32_debug) {
    fprintf(stderr,"    updating section info:"
           "  vma = %lx, lma = %lx\n", s->sec->vma, s->sec->lma);
  }

  /* create a unique name for the output section */
  name = (s->sec->size > 0) ?
  unique_section_name(s->sec->name) :
  unique_zero_length_section_name(s->sec->name);

  /* create an output section (statement) */
  os = lang_output_section_statement_lookup (name, 0, TRUE);

  if (pic32_debug)
    fprintf(stderr,"    creating output section statement \"%s\"\n\n", os->name);

  /* lang_add_section() will call init_os() if needed */
  lang_add_section (&os->children, s->sec, NULL, os);

  finish_section_info(s, os);

  /* hook section up to target region */
  s->region = regn;

#if 0
    ///\coresident - make sections absolute sections
   if (pic32_coresident_app)
   {
      PIC32_SET_ABSOLUTE_ATTR(s->sec->output_section);
   }
#endif
} /* update_section_info() */

/*
 * update_group_section_info()
 *
 * Update group section info after allocation
 *
 * Note: argument region is currently unused.
 */
static void
update_group_section_info(bfd_vma alloc_addr,
                    struct pic32_section *g,
                    struct pic32_region *region) {

  struct pic32_section *s, *next;
  asection *sec;
  char *name;
  bfd_vma addr = alloc_addr;
  lang_output_section_statement_type *os;

  /* create a unique name for the output section, if necessary */
     sec = bfd_get_section_by_name(link_info.output_bfd, g->sec->name);
     if (sec) {
       name = ( g->sec->size > 0) ?
               unique_section_name(g->sec->name) :
               unique_zero_length_section_name(g->sec->name);
     }
     else {
       name = (char *) g->sec->name;
       if (PIC32_IS_RAMFUNC_ATTR(g->sec))
         strcpy(name, ".RAMFUNC$");
     }
  /* create an output section (statement) */
  os = lang_output_section_statement_lookup (name, 0, TRUE);
  if (pic32_debug)
    fprintf(stderr,"    creating output section statement \"%s\"\n", os->name);

  /* loop through the input sections in this group */
  for (s = g; s != NULL; s = next) {
    next = s->next;
    if (s->sec && (PIC32_IS_RAMFUNC_ATTR(s->sec))) {
      update_section_addr(s->sec, addr);
      addr += (s->sec->size);

      /* lang_add_section() will call init_os() if needed */
      lang_add_section (&os->children, s->sec, NULL, os);

      if (pic32_debug)
        fprintf(stderr,"    updating grouped section info:"
               "  vma = %lx, lma = %lx\n", s->sec->vma, s->sec->lma);
    }
  }
  finish_section_info(g, os);

  /* hook section up to target region */
  g->region = region;
} /* update_group_section_info() */


/*
 * create_remainder_blocks()
 *
 * Create [0,1,2] remainder blocks based on
 * block->offset and section->size.
 */
static void
create_remainder_blocks(struct pic32_memory *lst,
                        struct pic32_memory *b, unsigned int len) {

  bfd_vma remainder = b->size - (len + b->offset);

  if (pic32_debug)
    fprintf(stderr,"    creating remainder blocks: %lx, %lx\n",
           b->offset, remainder);

  if (b->offset > 0)  /* gap at beginning */
    pic32_add_to_memory_list(lst, b->addr, b->offset);

  if (remainder > 0)  /* gap at end */
    pic32_add_to_memory_list(lst,
                             b->addr + b->offset + len, remainder);
} /* create_remainder_blocks() */


/*
 * insert_alloc_section()
 *
 * Insert a pic32_section into the list
 *  in order of decreasing size or alignment power.
 *
 * Note: a new copy of the pic32_section is made
 *
 */
static void
insert_alloc_section (struct pic32_section *lst,
                      struct pic32_section *as)
{
  struct pic32_section *new, *prev, *s;
  int done = 0;

  /* create a new element */
  new = ((struct pic32_section *)
         xmalloc(sizeof(struct pic32_section)));
  new->sec  = as->sec;
  new->attributes = as->attributes;
  new->file = as->file;
  new->use_vma = as->use_vma;
  new->region = as->region;

  /* insert it at the right spot */
  prev = lst;
  for (s = prev; s != NULL; s = s->next)
    {
      if (s->sec && ((new->sec->size > s->sec->size) ||
          ((bfd_vma)(1 << new->sec->alignment_power) > s->sec->size)) ) {
        prev->next = new;
        new->next = s;
        done++;
        break;
      }
      prev = s;
    }

  if (!done)
    {
      prev->next = new;
      new->next = NULL;
    }
} /* insert_alloc_section() */


/*
 * build_alloc_section_list()
 *
 * Build alloc_section_list from the
 * list of unassigned_sections, based on
 * an attribute mask.
 *
 * Sort the list in order of decreasing S,
 * where S is the greater of size and
 * alignment requirement.
 *
 * Note: If a section has length zero,
 * we allocate it anyway, although it
 * is stripped from the output file.
 * The linker needs to have an output
 * section statement created for it.
 *
 * Future enhancement:
 *   If --re-order-sections is set, also
 *   add explicity mapped output sections
 *   unless they have an absolute address.
 */
static void
build_alloc_section_list(unsigned int mask) {
  struct pic32_section *s, *prev, *next;

  if (alloc_section_list)
    pic32_free_section_list(&alloc_section_list);


  pic32_init_section_list(&alloc_section_list);


  /* insert the unassigned sections */
  prev = unassigned_sections;
  for (s = prev; s != NULL; s = next) {
    next = s->next;
    /* Don't add gc-sections to the alloc list */
    if (s->sec && (s->sec->flags & SEC_EXCLUDE))
      continue;    

    if (s->attributes & mask) {
      if (pic32_debug) {
        fprintf(stderr,"  input section \"%s\", len = %lx, flags = %x, attr = %x\n",
                s->sec->name, s->sec->rawsize? s->sec->rawsize :
                s->sec->size, s->sec->flags, s->attributes);
      }
      insert_alloc_section(alloc_section_list, s);
      prev->next = next; /* unlink it from unassigned_sections */        
    } else
      prev = s;
  }
} /* build_alloc_section_list() */


/*
 * build_free_block_list()
 *
 * Build a list of free memory blocks
 * in a specified region.
 *
 * Sort the list in order of increasing size.
 *
 * Future enhancement:
 *   If --re-order-sections is set, add free blocks
 *   for explicity mapped output sections unless
 *   they have an absolute address.
 */
static void
build_free_block_list(struct pic32_region *regn,
                      unsigned int mask ATTRIBUTE_UNUSED) {
  int cnt = 0;
  bfd_vma len, dot, limit, ma;
  struct pic32_section *s;
  struct memory_region_struct *region = regn->region;

  /* DZ - having free blocks associated with each memory region
     means we *dont* want to free - already allocated means already
     computed for the region. */
  if (regn->free_blocks)
    return; 

  pic32_init_memory_list(&regn->free_blocks);

  if (pic32_debug)
    fprintf(stderr,"\nBuilding free block list for region \"%s\"\n"
           "  origin = %lx, length = %lx, current = %lx\n",
           region->name_list.name, region->origin, region->length, region->current );


  /* find any gaps left by sequential allocator */
  dot = region->origin;
  limit = dot + region->length;
  for (s = pic32_section_list; s != NULL; s = s->next) {

      if (pic32_debug && s->sec)
        fprintf(stderr,"    Checking (section \"%s\", lma = %lx, vma = %lx, len = %lx, attr = %x)\n",
               s->sec->name, s->sec->lma, s->sec->vma,
               s->sec->rawsize? s->sec->rawsize : s->sec->size,
               s->attributes);

    if (s->sec && (s->use_vma==0) &&
        (s->sec->lma >= region->origin)
        && (s->sec->lma <= (region->origin + region->length))
        && (s->sec->lma!=0)
        )
        ma = s->sec->lma;
    else if (s->sec && (s->use_vma) &&
        (s->sec->vma >= region->origin) &&
           (s->sec->vma <= (region->origin + region->length)))
        ma = s->sec->vma;
    else
        ma = 0;

    if ((s->sec) && ((s->sec->rawsize | s->sec->size) > 0) &&
        (ma || ( (s->sec->lma == 0x0) && (region->origin == 0x0)))
        ) {
      if (pic32_debug)
        fprintf(stderr,"    (section \"%s\", addr = %lx, len = %lx, attr = %x)\n",
               s->sec->name, ma,
               s->sec->rawsize? s->sec->rawsize : s->sec->size,
               s->attributes);
      if (ma == dot)                   /* no gap, just advance dot */
        dot += s->sec->rawsize? s->sec->rawsize : s->sec->size;
      else if (ma >= limit)   /* section belongs to another region */
        continue;
      else if (ma > dot) {               /* a gap preceeds section */
        len = ma - dot;                          /* compute length */
        if (pic32_debug)
          fprintf(stderr,"  block %d, addr = %lx, len = %lx\n",
                 ++cnt, dot, len);
        pic32_add_to_memory_list(regn->free_blocks, dot, len);  /* add free block */
        dot += len + (s->sec->rawsize? s->sec->rawsize : s->sec->size);           /* advance dot    */
      }
    }
  }

  /* add a block for any free space remaining in this region */
  /* .. use dot, because region->current may have been set for CodeGuard */
  len = region->length - (dot - region->origin);
  if (len > 0) {
    if (pic32_debug)
      fprintf(stderr,"  block %d, addr = %lx, len = %lx\n",
             ++cnt, dot, len);
    pic32_add_to_memory_list(regn->free_blocks, dot, len);
  }
} /* build_free_block_list() */


/*
 * remove_free_block()
 *
regn-> * Remove an item from the free memory blocks list.
 *
 */
static void
remove_free_block(struct pic32_memory *lst, struct pic32_memory *b) {
  pic32_remove_from_memory_list(lst, b);
}


/*
 * align_free_blocks(unsigned int)
 *
 * go through the free block list and trim away any partial cache lines ...
 * this is necessary on some parts where code and data can both be allocated to the same space but
 * are not permitted to share cache lines.
 *
 * specifically we are doing this for Jubilee tcm allocation
 */
static void
align_free_blocks(struct pic32_memory *lst, unsigned int alignment_power) {

    bfd_vma bump_amount;
    bfd_vma alignment_amount = 1 << alignment_power;
    struct pic32_memory     *block;
    
    for(block = lst; block != NULL ; block = block->next){
	// for each free block in the list trim away partial cache lines at either end.

	bump_amount = block->addr % alignment_amount;
	if (bump_amount > 0) {
	    bump_amount = alignment_amount - bump_amount;
	    block->addr += bump_amount;
	    block->size -= bump_amount;
	}
	bump_amount =  (block->size % alignment_amount);
	if (bump_amount > 0) {
	    block->size -= bump_amount;            
	}	
    }   
}





/*
 * allocate_default_stack()
 *
 * If a stack section was not explicitly defined,
 * and the stack init symbols were not defined,
 * then this function is called to allocate the
 * largest stack possible from the given region.
 *
 * Called by: bfd_pic32_finish()
 *
 * Calls:     pic32_add_to_memory_list()
 *            pic32_remove_from_memory_list()
 *
 * Returns:   sets global vars stack_base, stack_limit
 *
 * Notes: The free blocks list is sorted in order
 *        of increasing size
 *
 *        The calling function will confirm that
 *        the resulting stack satisfies the minimum
 *        size requirements.
 *
 *
 */

static void
allocate_default_stack(struct pic32_region *regn) {
  struct pic32_memory *big_block = 0;
  struct pic32_memory *b, *next;
  bfd_vma under,over;
  bfd_vma max_addr;

  if (!pic32_valid_region(regn)) {
      if (pic32_debug)
          fprintf(stderr, "allocate_default_stack failed - invalid pic32 region\n");
      abort();
  }

  max_addr = regn->region->origin + regn->region->length - 1;

  if (regn->free_blocks == NULL) {
      einfo("%P%F: Error: Unable to allocate stack. No blocks available in %s. \n",
            regn->region->name_list.name);
      return;
  }
    
    
  /* if a free block straddles the upper limit, divide it */
  for (b = regn->free_blocks; b != NULL; b = next) {
    next = b->next;
    if ((b->addr < max_addr) && ((b->addr + b->size) > max_addr)) {

      if (pic32_debug)
        fprintf(stderr,"  free block at %lx crosses upper stack limit (%lx)\n",
               b->addr, max_addr);

      under = max_addr - b->addr;
      over  = (b->addr + b->size) - max_addr;

      pic32_add_to_memory_list(regn->free_blocks, b->addr, under);
      pic32_add_to_memory_list(regn->free_blocks, max_addr, over);
      pic32_remove_from_memory_list(regn->free_blocks, b);
      break;
    }
  }

  /* find the largest block that qualifies */
  for (b = regn->free_blocks; b != NULL; b = next) {
    if (b->addr < max_addr)
      big_block = b;
    next = b->next;
  }

  if ((unsigned int)(pic32c_heap_size) >
      (unsigned int)big_block->size) {
    heap_base = (big_block->addr + 1);
    heap_limit = (heap_base + (big_block->size) - 0x80u);
    einfo("%P%X Error: Not enough memory for heap"
          " (%u bytes needed, %u bytes available)\n",
            (unsigned int)(pic32c_heap_size),
            (unsigned )(heap_limit - heap_base));
  } else {
    heap_base = (big_block->addr + 1);
    heap_limit = (heap_base + pic32c_heap_size);
  }
  while (heap_base % 8 != 0)
    heap_base++;
  while (heap_limit % 8 != 0)
    heap_limit++;

  stack_base  = (heap_limit + 1 + pic32c_stackguard_size);
  stack_limit = (heap_base + big_block->size
                 - pic32c_stackguard_size + 1);
  /* Stack must be aligned by 8 */
  while (stack_base % 8 != 0)
    stack_base++;
  while (stack_limit % 8 != 0)
    stack_limit--;

  if (pic32_debug) {
    fprintf(stderr,"  selecting block at %lx\n", big_block->addr);
    fprintf(stderr,"  heap base = %x, heap limit = %x\n", heap_base, heap_limit);
    fprintf(stderr,"  stack base = %x, stack limit = %x\n", stack_base, stack_limit);
  }

  /* remove the block that we just used */
  pic32_remove_from_memory_list(regn->free_blocks, big_block);
}

static void 
allocate_stack_in_dtcm (void)
{
    struct pic32_memory     *block = NULL;
    struct pic32_memory     *iter;
    unsigned int            max_size = 0;
  
    if (!pic32_valid_region(region_info.data_tcm)) {
        einfo(_("%X Link error: requested stack in dtcm but dtcm region not present\n"));
        return;
    }

    lang_memory_region_type *dtcm_region = region_info.data_tcm->region;
    struct pic32_memory *free_blocks = region_info.data_tcm->free_blocks;

    if (free_blocks == NULL) {
        einfo("%P%F: Error: Unable to allocate stack. No blocks available in %s. \n",
              dtcm_region->name_list.name);
        return;
    }
    
    ///\ stack gets the largest, and preferably the highest address
    ///\ lookup for the free block with this property(ies)
    for (iter = free_blocks; iter != NULL; iter = iter->next) {
	if (iter->size > max_size)
        {
            block = iter;
            max_size = iter->size;
        }
    }
    
    ///\ check that addresses from data_memory_free_blocks belong to dtcm

    if (block == NULL || (block->addr > (unsigned long)(dtcm_region->origin + dtcm_region->length+1))
        || (block->addr < (unsigned long)dtcm_region->origin))
        einfo(_("%X Link Error: Data memory free blocks built for incorrect region\n"));
    
    
    dtcm_stack_limit = block->addr + block->size - 1 ;
    
    while (dtcm_stack_limit % 8 != 0)
        dtcm_stack_limit--;
    
    dtcm_stack_base = (block->addr%8)? (block->addr)
    : (block->addr + (8 - block->addr%8)) ;
    
    pic32_remove_from_memory_list(free_blocks, block);
}

#ifdef ENABLE_USER_MEMORY

static int
compare_memory_region_origin (const void *a, const void *b)
{
  const asection *sec1 = *(const asection **) a;
  const asection *sec2 = *(const asection **) b;

 
  /* If there's no section for some reason, compare equal.  */
  if (!sec1 || !sec2)
    return 0;
  
  if (sec1->vma < sec2->vma)
    return -1;
  else if (sec1->vma > sec2->vma)
    return 1;
  
  return 0;
}

static void
check_overlapping_memory_regions (asection **asec_list, unsigned int asec_count)
{
  unsigned int i ;
  asection* a = NULL ;
  asection* b = NULL ;
  const char *asec_name = NULL ;
  const char *bsec_name = NULL ;
  
  for (i=0; i<(asec_count-1); i++)
  {
    a = asec_list[i];
    b = asec_list[i+1];
    
    if ( (a->vma + a->lma) >= b->vma )
    {
        asec_name = a->name + strlen(memory_region_prefix) + 1;
        bsec_name = b->name + strlen(memory_region_prefix) + 1;

        einfo(_(" Link Error: user defined memory region \'%s\' crosses"
                " the boundary of region '%s'.\n"),
                asec_name, bsec_name);
    }
  }
}


static void
validate_memory_region_list()
{
  /* Build a sorted list of memory regions, then use that to validate */
  struct pic32_section *r, *rnext;
  unsigned int list_size = 10;
  unsigned int sec_count = 0;

  asection **sec_list = (asection **)
    xmalloc (list_size * sizeof (asection *));

  for (r = memory_region_list; r != NULL; r = rnext) 
  {
    rnext = r->next;
    asection *asec = r->sec;
    if (asec) 
    {
      if (sec_count == list_size) 
      {
        list_size *= 2;
        sec_list = (asection **) 
        xrealloc (sec_list, list_size * sizeof (asection *));
      }

      sec_list[sec_count++] = asec;
    }
  }

  qsort (sec_list, sec_count, sizeof (asection *), &compare_memory_region_origin);
  check_overlapping_memory_regions(sec_list, sec_count);
  free (sec_list);

  return ;
}



/*
 * allocate_user_memory()
 *
 * This function attempts to allocate sections
 * in user-defined memory regions.
 *
 * Called by: allocate_memory()
 *
 * Calls:     build_section_list()
 *            build_free_block_list()
 *            locate_sections() 
 *
 * Returns:   status code
 *            (0 = success)
 */
static int
allocate_user_memory() {
  struct memory_region_struct temp_region;
  struct pic32_section *r, *rnext, *s, *snext, *ss, *ssnext, *ssprev;
  const char *region_name;
  int result = 0;

  /* Validate that the memory regions aren't overlapping*/
  validate_memory_region_list();

  /* loop through any user-defined regions */
  for (r = memory_region_list; r != NULL; r = rnext) {
  
    rnext = r->next;
    if (r->sec == 0) continue;
    
    region_name = r->sec->name + strlen(memory_region_prefix);
    if (pic32_debug)
      fprintf(stderr,"\nBuilding allocation list for user-defined region \"%s\""
             " origin = %lx, length = %lx\n",
             region_name, r->sec->vma, r->sec->lma);

    /* the assembler will not accept a memory region
       definition with length 0... if we see one here,
       it indicates that a section has referenced a
       region that was never defined */
    if (r->sec->lma == 0)
      einfo(_("%s: Link Error: Memory region \'%s\' has been referenced"
              " but not defined\n"),
              r->sec->owner->filename, region_name);

    /* loop through the list of memory-type sections..
       note that only two fields of this list are
       used (file, sec) and they are both (char *) */
    for (s = user_memory_sections; s != NULL; s = snext) {

      snext = s->next;
      if (!s->file || !s->sec) continue;
      
#if 0
      fprintf(stderr,"s->file =%s, s->sec = %s \n", (char *) s->file, (char *) s->sec);
#endif
      if (strcmp((char *)s->file, region_name) == 0) {
        /* if this section has been assigned to the current region,
           add it to the allocation list */
        ssprev = unassigned_sections;
        for (ss = ssprev; ss != NULL; ss = ssnext) {
          ssnext = ss->next;
          if (ss->sec && strcmp((char *) s->sec, ss->sec->name) == 0) {
            if (pic32_debug)
              fprintf(stderr,"  input section \"%s\", len = %lx, flags = %x, attr = %x\n",
                     ss->sec->name, ss->sec->size,
                     ss->sec->flags, ss->attributes);

            insert_alloc_section(alloc_section_list, ss);
            ssprev->next = ssnext; /* unlink it from unassigned_sections */
          } else
            ssprev = ss;
        }
      }
    }
    
    if (pic32_section_list_length(alloc_section_list) == 0) {
      if (pic32_debug)
        fprintf(stderr,"\n  (none)\n");
      continue;
    }
      
//    temp_region.name = (char *) region_name;
    temp_region.next = &temp_region;
    temp_region.origin = r->sec->vma;
    temp_region.length = r->sec->lma;

    /* don't expect to need these.. */
    temp_region.current = r->sec->vma;
//    temp_region.old_length = 0;
    temp_region.flags = 0;
    temp_region.not_flags = 0;
    temp_region.had_full_message = FALSE;

    if (free_blocks)
      pic32_free_memory_list(&free_blocks);
    pic32_init_memory_list(&free_blocks);
  
    /* start with entire region as a free block */
    pic32_add_to_memory_list(free_blocks, temp_region.origin,
                             temp_region.length);

    if (pic32_debug) {
      pic32_print_section_list(alloc_section_list, "allocation");
    }
  
    result |= locate_sections(address, 0, &temp_region);   /* most restrictive  */
    result |= locate_sections(all_attr, 0, &temp_region);  /* least restrictive */
  }

  return result;
} /* allocate_user_memory() */
#endif /* ENABLE_USER_MEMORY */
